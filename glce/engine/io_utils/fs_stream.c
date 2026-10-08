// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/io_utils/fs_stream.h"

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/filesystem/filesystem.h"
#include "engine/core/file_io/fs_types.h"

#include "engine/containers/choco_string.h"

/*
 * Module Internal Contract
 *
 * Canonical state:
 * - filesystemはNULLではない。
 * - filesystemはGeneral Allocatorから取得されたliveなfilesystem_t objectを指す。
 * - filesystemが指すfilesystem_tはFilesystem moduleのcanonical stateを満たす。
 *
 * Representation / Ownership:
 * - fs_stream_tはfilesystemが指すfilesystem_t objectをexclusive ownershipし、
 *   そのlifetimeを管理する。
 * - filesystem_t内部のfile handle、open modeおよびexternal file streamに関する
 *   representationとownershipはFilesystem moduleのContractに従う。
 * - FS Stream moduleはFilesystem内部のresourceを直接所有または管理しない。
 *
 * Stable state:
 * - publicに存在するfs_stream_tは、利用可能なopen済みfile sessionを表す。
 * - publicなclosed stateまたはpartial initialization stateは持たない。
 * - file position、EOF state、error state等のexternal file stream stateは
 *   read operationによって変化し得るが、fs_stream_t自身のCanonical stateには含めない。
 *
 * State Transition:
 * - createではfs_stream_tおよびowned filesystem_tの構築中に
 *   partial construction stateを許容する。
 * - construction中のpartial stateはcallerへ公開しない。
 * - canonical state成立後にのみ、完成したfs_stream_tのownershipをcallerへcommitする。
 * - read operationはowned filesystem_tを通してexternal file stream stateを変更し得るが、
 *   fs_stream_t自身のownership relationは変更しない。
 * - destroyではowned filesystem_tのlifetimeを終了した後、
 *   fs_stream_t自身のlifetimeを終了する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

/*
 * Module Validation Policy
 *
 * - ValidationはModule Internal Contractで定義したCanonical stateを基準として行う。
 *
 * - 現在のfs_stream_tはowned filesystemのみを保持する単純なrepresentationであり、
 *   canonical validationとは別のvalidation depthとして利用する必要がないため、
 *   private shallow validatorは設けない。
 *
 * - canonical validatorはfilesystem != NULLであることを確認する。
 * - owned filesystemをdereferenceまたはFilesystem validatorへ渡す前に、
 *   general_allocator_ptr_is_allocated()によってfilesystemがGeneral Allocator上の
 *   current allocationであることを確認する。
 * - allocation validity確認後、filesystem_is_valid()へvalidationを委譲し、
 *   owned filesystem_tのcanonical validityを確認する。
 *
 * - canonical validatorは、引数として渡されたfs_stream_t自身のallocation validityを検証しない。
 *   fs_stream_tをowned pointerとして保持するownerが存在する場合、そのstorage validityは
 *   owner側のownership closureとして扱う。
 *
 * - filesystem_t内部のfile handle、open modeその他のrepresentation validationは
 *   Filesystem moduleへ委譲し、FS Stream moduleでは重複して検証しない。
 * - file position、EOF state、error state等のexternal file stream stateは
 *   FS Stream moduleが所有するcanonical invariantではないため、
 *   FS Stream canonical validatorの検証対象としない。
 *
 * - public boundaryでcanonical stateが確認済みのoperationから呼ばれるprivate helperは、
 *   その成立済みcanonical contractを信頼してdeep validationを繰り返さない。
 * - private helper自身が安全にargumentを扱うために必要なpointer existence等の
 *   memory-safety preconditionは、成立済みsemantic contractとは分離して扱う。
 *
 * - canonical validationによってDATA_CORRUPTEDが確定した場合、
 *   suspectなownership graphを辿るcleanup、resource releaseまたは追加operationは行わない。
 *
 * - explicit canonical validatorのvalidation semanticsはBUILD_MODEによって変更しない。
 * - validatorは対象stateおよびexternal file stream stateを変更せず、
 *   validation failure時はfalseを返す。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
struct fs_stream {
    filesystem_t* filesystem;
};

// ============================================================
// Private Constants
// ============================================================
#define FS_STREAM_READ_UNIT_SIZE 512  /**< ファイル読み込みの際に一度に読み込むバイト数(メモリの動的確保回数を減らすため,固定値にした) */
#define FS_STREAM_TEXT_FILE_LINE_BUFFER_SIZE 1024    /**< 改行コードを除く1行本文の最大長は1023bytes(改行コードがCRLFの場合は1022bytesとなる) */

static const char* const s_result_str_success = "SUCCESS";                      /**< 実行結果コード文字列: 正常終了 */
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";    /**< 実行結果コード文字列: 無効な引数 */
static const char* const s_result_str_bad_operation = "BAD_OPERATION";          /**< 実行結果コード文字列: API誤用 */
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";        /**< 実行結果コード文字列: 内部データ破損or未初期化 */
static const char* const s_result_str_no_memory = "NO_MEMORY";                  /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";        /**< 実行結果コード文字列: システム使用可能範囲超過 */
static const char* const s_result_str_overflow = "OVERFLOW";                    /**< 実行結果コード文字列: 計算オーバーフロー */
static const char* const s_result_str_file_open_error = "FILE_OPEN_ERROR";      /**< 実行結果コード文字列: ファイルオープンエラー */
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";          /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";      /**< 実行結果コード文字列: 想定していないエラーが発生 */
static const char* const s_result_str_eof = "EOF";                              /**< 実行結果コード文字列: EOF */

// ============================================================
// Private Function Declarations
// ============================================================
// file stream helpers
static fs_stream_result_t byte_read(fs_stream_t* stream_, size_t read_bytes_, size_t* out_read_bytes_, char* out_buffer_);

// Utilities
static const char* result_to_str(fs_stream_result_t result_);
static fs_stream_result_t result_convert_filesystem(filesystem_result_t result_);
static fs_stream_result_t result_convert_general_allocator(general_allocator_result_t result_);
static fs_stream_result_t result_convert_choco_string(choco_string_result_t result_);

// ============================================================
// Public API
// ============================================================
// fs_stream_create Validation Policy
//
// - create前には有効なfs_stream_tが存在しないため、
//   PreconditionsではFS Stream validatorを使用しない。
// - out_stream_、*out_stream_、fullpath_およびmode_に関する
//   public API contractはRELEASE_BUILDを含む全BUILDで検証する。
// - out_stream_ == NULLはINVALID_ARGUMENTとして扱う。
// - *out_stream_ != NULLは既存pointerを上書きするAPI misuseであるためBAD_OPERATIONとして扱う。
// - fullpath_は空文字列ではなく、'/'から始まるabsolute pathであることを要求する。
// - mode_のsemantic validityはfs_open_mode_is_valid()によって検証する。
//
// - fs_stream_t storageのallocation validationはGeneral Allocatorへ委譲する。
// - owned filesystem_tのconstructionおよびFilesystem固有のvalidationは
//   filesystem_create()へ委譲する。
// - 下位moduleからDATA_CORRUPTEDを受け取った場合はFS_STREAM_DATA_CORRUPTEDへ変換し、
//   fail-stop ruleに従って通常cleanupを行わない。
//
// - DEBUG_BUILD / TEST_BUILDではtemporary objectのconstruction完了後、
//   callerへ公開する前のStable boundaryでfs_stream_is_valid()を実行し、
//   canonical Postcondition validationを行う。
// - Postcondition validationに成功した場合のみ、完成したfs_stream_tのownershipを
//   callerへcommitする。
// - RELEASE_BUILDではautomatic canonical Postcondition validationを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
fs_stream_result_t fs_stream_create(fs_stream_t** out_stream_, const char* fullpath_, fs_open_mode_t mode_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    filesystem_result_t ret_filesystem = FILESYSTEM_INVALID_ARGUMENT;

    fs_stream_t* tmp_stream = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_stream_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_create", "out_stream_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_stream_, ret, FS_STREAM_BAD_OPERATION, result_to_str(FS_STREAM_BAD_OPERATION), "fs_stream_create", "*out_stream_")
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_create", "fullpath_")
    if('\0' == fullpath_[0] || '/' != fullpath_[0]) {
        ret = FS_STREAM_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_stream_create(%s) - Provided fullpath_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(!fs_open_mode_is_valid(mode_)) {
        ret = FS_STREAM_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_stream_create(%s) - Provided mode_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(fs_stream_t), GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO, (void**)&tmp_stream);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("fs_stream_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_filesystem = filesystem_create(&tmp_stream->filesystem, fullpath_, mode_);
    if(FILESYSTEM_SUCCESS != ret_filesystem) {
        ret = result_convert_filesystem(ret_filesystem);
        ERROR_MESSAGE("fs_stream_create(%s) - filesystem_create failed.", result_to_str(ret));
        goto cleanup;
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_stream_is_valid(tmp_stream)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_create(%s) - Postcondition validation failed for 'tmp_stream'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_stream_ = tmp_stream;
    tmp_stream = NULL;

    ret = FS_STREAM_SUCCESS;

cleanup:
    if(FS_STREAM_DATA_CORRUPTED != ret) {
        if(NULL != tmp_stream) {
            filesystem_destroy(&tmp_stream->filesystem, NULL);
            general_allocator_free((void**)&tmp_stream, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);
        }
    }

    return ret;
}

// fs_stream_destroy Validation Policy
//
// - stream_ == NULLまたは*stream_ == NULLはno-opとして扱う。
// - out_close_succeeded_はoptional outputであり、NULLを許可する。
//
// - DEBUG_BUILD / TEST_BUILDではresource release開始前に
//   fs_stream_is_valid()を実行し、canonical Precondition validationを行う。
// - canonical validationに失敗した場合はsuspectなowned filesystemを辿らず、
//   filesystem destroyおよびfs_stream_t storageのfreeを行わない。
// - canonical validation failure時、out_close_succeeded_ != NULLであればfalseを格納する。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてdestroyを実行する。
//
// - owned filesystem_tのcloseおよびresource releaseに関するvalidationは
//   filesystem_destroy()へ委譲する。
// - fs_stream_t自身のstorage releaseに関するvalidationはGeneral Allocatorへ委譲する。
// - destroyによってobject lifetimeが終了するため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
void fs_stream_destroy(fs_stream_t** stream_, bool* out_close_succeeded_) {
    if(NULL == stream_) {
        return;
    }
    if(NULL == *stream_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_stream_is_valid(*stream_)) {
        ERROR_MESSAGE("fs_stream_destroy(%s) - Precondition validation failed for '*stream_'.", result_to_str(FS_STREAM_DATA_CORRUPTED));
        if(NULL != out_close_succeeded_) {
            *out_close_succeeded_ = false;
        }
        return;
    }
#endif

    filesystem_destroy(&(*stream_)->filesystem, out_close_succeeded_);
    general_allocator_free((void**)stream_, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);
}

// fs_stream_byte_read Validation Policy
//
// - stream_、out_read_bytes_およびout_buffer_のpointer existenceは
//   memory safetyとpublic API contractのため、RELEASE_BUILDを含む全BUILDで検証する。
// - read_bytes_ == 0はbyte read operationとして成立しないため、
//   checked preconditionとして全BUILDで検証する。
//
// - DEBUG_BUILD / TEST_BUILDではread operation開始前に
//   fs_stream_is_valid()を実行し、canonical Precondition validationを行う。
// - canonical validationに失敗した場合はowned filesystemへ進まず、
//   FS_STREAM_DATA_CORRUPTEDを返す。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提とする。
//
// - canonical validation成功後の実際のbyte readはprivate byte_read()へ委譲する。
// - private helperではFS Stream canonical validationを再実行せず、
//   public boundaryで成立済みのcanonical contractを信頼する。
// - Filesystem固有のreadability、file stream stateおよびexternal read resultの
//   validationはfilesystem_byte_read()へ委譲する。
//
// - 下位からDATA_CORRUPTEDを受け取った場合はFS_STREAM_DATA_CORRUPTEDへ変換し、
//   fail-stop ruleに従って追加のoutput normalizationを行わない。
// - 本operationはfs_stream_t自身のownership relationを変更しないため、
//   canonical Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
fs_stream_result_t fs_stream_byte_read(fs_stream_t* stream_, size_t read_bytes_, size_t* out_read_bytes_, char* out_buffer_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(stream_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_byte_read", "stream_")
    IF_ARG_NULL_GOTO_CLEANUP(out_read_bytes_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_byte_read", "out_read_bytes_")
    IF_ARG_NULL_GOTO_CLEANUP(out_buffer_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_byte_read", "out_buffer_")
    if(0 == read_bytes_) {
        ret = FS_STREAM_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_stream_byte_read(%s) - Provided read_bytes_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_stream_is_valid(stream_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_byte_read(%s) - Precondition validation failed for 'stream_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    return byte_read(stream_, read_bytes_, out_read_bytes_, out_buffer_);

cleanup:
    return ret;
}

// fs_stream_text_file_read Validation Policy
//
// - stream_およびout_string_のpointer existenceは
//   RELEASE_BUILDを含む全BUILDで検証する。
//
// - DEBUG_BUILD / TEST_BUILDではread operation開始前に
//   fs_stream_is_valid()を実行し、stream_にcanonical Precondition validationを行う。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提とする。
//
// - canonical validation成功後の反復byte readではprivate byte_read()を使用し、
//   FS Stream canonical validationを各read単位で繰り返さない。
// - external file streamのread stateおよびFilesystem固有のvalidationは
//   private byte_read()を通してFilesystem moduleへ委譲する。
//
// - out_string_が指すChoco Stringのstateおよびsemantic validationは
//   choco_string_concat_from_c_string()へ委譲し、FS Stream側では重複して検証しない。
// - 下位moduleからDATA_CORRUPTEDを受け取った場合はFS_STREAM_DATA_CORRUPTEDへ変換し、
//   fail-stop ruleに従って追加operationを行わずreturnする。
//
// - 本operationはfs_stream_t自身のownership relationを変更しないため、
//   canonical Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
fs_stream_result_t fs_stream_text_file_read(fs_stream_t* stream_, choco_string_t* out_string_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    bool complete = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(stream_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_text_file_read", "stream_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_text_file_read", "out_string_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_stream_is_valid(stream_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_text_file_read(%s) - Precondition validation failed for 'stream_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    while(!complete) {
        char tmp_buffer[FS_STREAM_READ_UNIT_SIZE + 1] = { 0 };
        size_t result = 0;
        ret = byte_read(stream_, FS_STREAM_READ_UNIT_SIZE, &result, tmp_buffer);
        if(FS_STREAM_EOF == ret) {
            complete = true;
        } else if(FS_STREAM_SUCCESS == ret) {
            ret_choco_string = choco_string_concat_from_c_string(tmp_buffer, out_string_);
            if(CHOCO_STRING_SUCCESS != ret_choco_string) {
                ret = result_convert_choco_string(ret_choco_string);
                ERROR_MESSAGE("fs_stream_text_file_read(%s) - Failed to append to output string.", result_to_str(ret));
                goto cleanup;
            }
            if(FS_STREAM_READ_UNIT_SIZE > result) {
                complete = true;
            }
        } else {
            ERROR_MESSAGE("fs_stream_text_file_read(%s) - fs_stream_byte_read failed.", result_to_str(ret));
            goto cleanup;
        }
    }

    ret = FS_STREAM_SUCCESS;

cleanup:
    return ret;
}

// fs_stream_text_file_line_read Validation Policy
//
// - stream_およびout_string_のpointer existenceは
//   RELEASE_BUILDを含む全BUILDで検証する。
//
// - DEBUG_BUILD / TEST_BUILDではread operation開始前に
//   fs_stream_is_valid()を実行し、stream_にcanonical Precondition validationを行う。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提とする。
//
// - canonical validation成功後の1byte単位の反復readではprivate byte_read()を使用し、
//   FS Stream canonical validationを各byteごとに繰り返さない。
// - external file streamのread stateおよびFilesystem固有のvalidationは
//   private byte_read()を通してFilesystem moduleへ委譲する。
//
// - line bufferのcapacity超過は本operationが所有するsemantic conditionとして
//   全BUILDで検出し、FS_STREAM_LIMIT_EXCEEDEDとして扱う。
// - out_string_が指すChoco Stringのstateおよびsemantic validationは
//   choco_string_copy_from_c_string()へ委譲し、FS Stream側では重複して検証しない。
// - 下位moduleからDATA_CORRUPTEDを受け取った場合はFS_STREAM_DATA_CORRUPTEDへ変換し、
//   fail-stop ruleに従って追加operationを行わずreturnする。
//
// - 本operationはfs_stream_t自身のownership relationを変更しないため、
//   canonical Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
fs_stream_result_t fs_stream_text_file_line_read(fs_stream_t* stream_, choco_string_t* out_string_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    bool complete = false;
    size_t read_byte = 0;
    char tmp_buffer[FS_STREAM_TEXT_FILE_LINE_BUFFER_SIZE] = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(stream_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_text_file_line_read", "stream_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_text_file_line_read", "out_string_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_stream_is_valid(stream_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_text_file_line_read(%s) - Precondition validation failed for 'stream_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    while(!complete) {
        char tmp = 0;   // 改行コードは文字数カウントに含めないため、一時的にtmpにコピーし、改行コード以外だったらtmp_bufferにコピー
        size_t result = 0;
        ret = byte_read(stream_, 1, &result, &tmp);
        if(FS_STREAM_EOF == ret) {
            complete = true;
            if(0 == read_byte) {
                ret = FS_STREAM_EOF;
                goto cleanup;
            }
        } else if(FS_STREAM_SUCCESS == ret) {
            if('\n' == tmp) {
                complete = true;
            } else {
                tmp_buffer[read_byte] = tmp;
                read_byte++;
            }
            if(read_byte == FS_STREAM_TEXT_FILE_LINE_BUFFER_SIZE) {
                ret = FS_STREAM_LIMIT_EXCEEDED;
                ERROR_MESSAGE("fs_stream_text_file_line_read(%s) - Line length exceeded. Max body length is 1023 bytes for LF/EOF lines, or 1022 bytes for CRLF lines.", result_to_str(ret));
                goto cleanup;
            }
        } else {
            ERROR_MESSAGE("fs_stream_text_file_line_read(%s) - Failed to read 1 byte while reading a text line.", result_to_str(ret));
            goto cleanup;
        }
    }

    if(read_byte != 0 && '\r' == tmp_buffer[read_byte - 1]) {
        tmp_buffer[read_byte - 1] = '\0';
    }

    // Output.
    ret_choco_string = choco_string_copy_from_c_string(tmp_buffer, out_string_);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("fs_stream_text_file_line_read(%s) - Failed to copy the read line to out_string_.", result_to_str(ret));
        goto cleanup;
    }
    ret = FS_STREAM_SUCCESS;

cleanup:
    return ret;
}

// fs_stream_is_valid Validation Policy
//
// - 本APIはfs_stream_tのpublic canonical validatorである。
// - stream_ == NULLの場合はfalseを返す。
// - Module Internal Contractで定義されたCanonical state全体を検証する。
//
// - filesystem == NULLの場合はfalseを返す。
// - owned filesystemをdereferenceまたはFilesystem validatorへ渡す前に、
//   general_allocator_ptr_is_allocated()によってfilesystemがGeneral Allocator上の
//   current allocationであることを確認する。
// - owned filesystemのactual allocation sizeとcapacityの整合性検証については、
//   allocation metadataの利用方法と合わせて将来検討する。
// - allocation validity確認後、filesystem_is_valid()へvalidationを委譲し、
//   owned filesystem_tのcanonical validityを確認する。
//
// - stream_自身のallocation validityは本validatorでは検証しない。
//   stream_をowned pointerとして保持するownerが存在する場合、そのstorage validityは
//   owner側のownership closureとして扱う。
// - filesystem_t内部のrepresentationおよびexternal file stream stateのvalidationは
//   Filesystem moduleへ委譲し、FS Stream側では重複して検証しない。
//
// - explicit validatorであるため、BUILD_MODEによってvalidation semanticsを変更しない。
// - validation中にfs_stream_t、owned filesystem_tまたはexternal file stream stateを変更しない。
// - validation failure時はfalseを返すのみとし、error messageは出力しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool fs_stream_is_valid(const fs_stream_t* stream_) {
    if(NULL == stream_) {
        return false;
    }
    if(NULL == stream_->filesystem) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated((const void*)stream_->filesystem)) {
        return false;
    }
    if(!filesystem_is_valid(stream_->filesystem)) {
        return false;
    }

    return true;
}

// ============================================================
// file stream helpers
// ============================================================
/*
 * public APIで何回もcanonical validatorを呼ばないためのprivate helper
 *
 * Contract:
 * - stream_はcanonical stateである。
 * - read_bytes_は0より大きい。
 *
 * 本helperは上記contractを再検証しない。
 */
static fs_stream_result_t byte_read(fs_stream_t* stream_, size_t read_bytes_, size_t* out_read_bytes_, char* out_buffer_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    filesystem_result_t ret_filesystem = FILESYSTEM_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(stream_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "byte_read", "stream_")
    IF_ARG_NULL_GOTO_CLEANUP(out_read_bytes_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "byte_read", "out_read_bytes_")
    IF_ARG_NULL_GOTO_CLEANUP(out_buffer_, ret, FS_STREAM_INVALID_ARGUMENT, result_to_str(FS_STREAM_INVALID_ARGUMENT), "byte_read", "out_buffer_")

    // Commmit.
    ret_filesystem = filesystem_byte_read(stream_->filesystem, read_bytes_, out_read_bytes_, out_buffer_);
    if(FILESYSTEM_SUCCESS != ret_filesystem) {
        ret = result_convert_filesystem(ret_filesystem);
        if(FS_STREAM_EOF != ret) {
            ERROR_MESSAGE("byte_read(%s) - filesystem_byte_read failed.", result_to_str(ret));
        }
        goto cleanup;
    }

    ret = FS_STREAM_SUCCESS;

cleanup:
    if(FS_STREAM_DATA_CORRUPTED != ret) {
        if(NULL != out_read_bytes_ && FS_STREAM_SUCCESS != ret) {
            *out_read_bytes_ = 0;
        }
    }

    return ret;
}

// ============================================================
// Utilities
// ============================================================
static const char* result_to_str(fs_stream_result_t result_) {
    switch(result_) {
    case FS_STREAM_SUCCESS:
        return s_result_str_success;
    case FS_STREAM_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case FS_STREAM_BAD_OPERATION:
        return s_result_str_bad_operation;
    case FS_STREAM_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case FS_STREAM_NO_MEMORY:
        return s_result_str_no_memory;
    case FS_STREAM_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case FS_STREAM_OVERFLOW:
        return s_result_str_overflow;
    case FS_STREAM_FILE_OPEN_ERROR:
        return s_result_str_file_open_error;
    case FS_STREAM_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case FS_STREAM_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    case FS_STREAM_EOF:
        return s_result_str_eof;
    default:
        return s_result_str_undefined_error;
    }
}

static fs_stream_result_t result_convert_filesystem(filesystem_result_t result_) {
    switch(result_) {
    case FILESYSTEM_SUCCESS:
        return FS_STREAM_SUCCESS;
    case FILESYSTEM_INVALID_ARGUMENT:
        return FS_STREAM_UNDEFINED_ERROR;
    case FILESYSTEM_RUNTIME_ERROR:
        return FS_STREAM_RUNTIME_ERROR;
    case FILESYSTEM_NO_MEMORY:
        return FS_STREAM_NO_MEMORY;
    case FILESYSTEM_FILE_OPEN_ERROR:
        return FS_STREAM_FILE_OPEN_ERROR;
    case FILESYSTEM_UNDEFINED_ERROR:
        return FS_STREAM_UNDEFINED_ERROR;
    case FILESYSTEM_LIMIT_EXCEEDED:
        return FS_STREAM_LIMIT_EXCEEDED;
    case FILESYSTEM_BAD_OPERATION:
        return FS_STREAM_BAD_OPERATION;
    case FILESYSTEM_EOF:
        return FS_STREAM_EOF;
    case FILESYSTEM_DATA_CORRUPTED:
        return FS_STREAM_DATA_CORRUPTED;
    default:
        return FS_STREAM_UNDEFINED_ERROR;
    }
}

static fs_stream_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return FS_STREAM_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return FS_STREAM_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return FS_STREAM_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return FS_STREAM_UNDEFINED_ERROR;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return FS_STREAM_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return FS_STREAM_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return FS_STREAM_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return FS_STREAM_UNDEFINED_ERROR;
    default:
        return FS_STREAM_UNDEFINED_ERROR;
    }
}

static fs_stream_result_t result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return FS_STREAM_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return FS_STREAM_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return FS_STREAM_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return FS_STREAM_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return FS_STREAM_UNDEFINED_ERROR;
    case CHOCO_STRING_RUNTIME_ERROR:
        return FS_STREAM_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return FS_STREAM_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return FS_STREAM_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return FS_STREAM_LIMIT_EXCEEDED;
    default:
        return FS_STREAM_UNDEFINED_ERROR;
    }
}
