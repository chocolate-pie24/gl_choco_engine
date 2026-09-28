// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/core/filesystem/filesystem.h"

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/file_io/fs_types.h"

/*
 * Module Internal Contract
 *
 * Stable state:
 * - initializedなfilesystem_tは、1つのopened file streamを保持する。
 * - file_handle != NULLである。
 * - modeはvalidなfs_open_mode_tである。
 * - file_handleはmodeに対応するopen modeで取得されたfile streamを指す。
 *
 * Ownership:
 * - filesystem_t自身のstorageはGeneral Allocatorから取得し、Filesystem moduleが所有する。
 * - file_handleが指すfile streamはFilesystem moduleが所有する。
 * - destroy時はfile streamのcloseを試行した後、filesystem_t自身のstorageを解放する。
 *
 * File stream state:
 * - filesystem_tが直接保持するstructural stateはfile_handleとmodeである。
 * - FILE*内部のfile position、EOF state、error state等はexternal file stream stateとして扱い、
 *   filesystem_t自身のstructural stateとは区別する。
 * - Filesystem moduleはFILE*内部のstateを独自のcanonical invariantとして定義しない。
 * - file I/O operationによってexternal file stream stateは変化し得るが、
 *   filesystem_t自身のfile_handleおよびmodeは変更しない。
 *
 * State Transition:
 * - Public APIのentry / exitではfilesystem_t自身のStable stateを維持する。
 * - createではfilesystem_t storageの取得からfile open、mode設定、
 *   canonical validation完了までpartial construction stateを許容する。
 * - create途中のpartial stateはcallerへ公開しない。
 * - byte readではfile streamに対するI/Oによってexternal file stream stateが変化し得るが、
 *   filesystem_t自身のstructural stateは変更しない。
 * - destroyではfile streamのcloseを試行した後、filesystem_t自身のlifetimeを終了する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

/*
 * Module Validation Policy
 *
 * - ValidationはModule Internal Contractを基準として行う。
 *
 * - canonical validatorはinitializedなfilesystem_tのStable stateについて、
 *   Filesystem moduleが所有するwrapper stateのcanonical validityを検証する。
 * - canonical validatorは最初にshallow validatorを実行する。
 * - 現在のfilesystem_tには追加のowned internal structureが存在しないため、
 *   canonical validatorとshallow validatorの検査対象は実質的に同一である。
 *
 * - shallow validatorはfilesystem_t rootについて、
 *   operationを開始するために必要なlocal structural invariantを検証する。
 * - 現在のshallow validatorは、modeがvalidなfs_open_mode_tであること、
 *   およびfile_handle != NULLであることを確認する。
 *
 * - FILE*内部のfile position、EOF state、error stateその他の
 *   external file stream stateはFilesystem moduleのcanonical invariantとして扱わない。
 * - Filesystem moduleはFILE*内部構造を独自にvalidationせず、
 *   external file I/O operationの結果は対応するC standard I/O APIの結果を基準として処理する。
 *
 * - public APIでは、caller-controlledなpointer、size、output contract、
 *   およびoperation固有のstateを全BUILDで検証する。
 * - fs_open_mode_t固有のsemantic validationはfs_typesへ委譲し、
 *   Filesystem側で同じsemantic ruleを再定義しない。
 * - General Allocator固有のallocation / free validationはGeneral Allocatorへ委譲する。
 *
 * - DATA_CORRUPTEDが確定した後は通常cleanupおよび追加resource operationを行わない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
/**
 * @brief ファイルシステムモジュール内部状態管理構造体
 *
 */
struct filesystem {
    FILE* file_handle;              /**< ファイルハンドル */
    fs_open_mode_t mode;    /**< ファイルオープンモード */
};

typedef enum {
    // 要求したbyte数をすべて読み取った状態。
    // fread()の戻り値 == requested_bytesとなり、read operationは要求どおり完了している。
    FILESYSTEM_READ_STATE_COMPLETE = 0,

    // 要求byte数には届かなかったが、1byte以上を読み取った後にEOFへ到達した状態。
    // fread()の戻り値は0より大きくrequested_bytes未満で、EOF indicatorがsetされている。
    FILESYSTEM_READ_STATE_PARTIAL_EOF,

    // 1byteも読み取れずEOFへ到達した状態。
    // fread()の戻り値 == 0で、EOF indicatorがsetされている。
    FILESYSTEM_READ_STATE_EOF,

    // 要求byte数を読み取れず、stream errorが発生した状態。
    // fread()の戻り値はrequested_bytes未満で、error indicatorがsetされている。
    FILESYSTEM_READ_STATE_ERROR,

    // 要求byte数を読み取れなかったが、EOF indicatorとerror indicatorの
    // どちらからも原因を特定できない状態。
    FILESYSTEM_READ_STATE_UNDEFINED,
} filesystem_read_state_t;

// ============================================================
// Private Constants
// ============================================================
static const char* const s_result_str_success = "SUCCESS";                        /**< 実行結果コード文字列: 成功 */
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";      /**< 実行結果コード文字列: 無効な引数 */
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";            /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_result_str_no_memory = "NO_MEMORY";                    /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_result_str_file_open_error = "FILE_OPEN_ERROR";        /**< 実行結果コード文字列: ファイルオープン失敗 */
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";          /**< 実行結果コード文字列: システムリソースが使用可能範囲を超過 */
static const char* const s_result_str_bad_operation = "BAD_OPERATION";            /**< 実行結果コード文字列: API誤用 */
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";        /**< 実行結果コード文字列: 未定義エラー */
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";          /**< 実行結果コード文字列: 内部データ破損 */
static const char* const s_result_str_eof = "EOF";                                /**< 実行結果コード文字列: ファイル読み込みEOF */

// ============================================================
// Private Function Declarations
// ============================================================
// Mock functions
static FILE* mock_fopen(const char* fullpath_, const char* mode_);
static int mock_fclose(FILE* stream_);
static size_t mock_fread(void *ptr_, size_t size_, size_t nmemb_, FILE *stream_);
static int mock_ferror(FILE *stream_);
static int mock_feof(FILE *stream_);

// File I/O helpers
static filesystem_read_state_t read_state_get(FILE* file_handle_, size_t requested_bytes_, size_t read_bytes_);

// Utilities
static const char* result_to_str(filesystem_result_t result_);
static filesystem_result_t result_convert_general_allocator(general_allocator_result_t result_);

// Validators
static bool is_valid_shallow(const filesystem_t* filesystem_);

// ============================================================
// Public API
// ============================================================

// filesystem_create API Specification
//
// - 本APIはfilesystem_tのcomplete constructorである。
// - 成功時にはfilesystem_t自身のstorage取得、file open、mode設定を完了し、
//   即座に利用可能なfilesystem_tをout_filesystem_へ公開する。
// - create途中のpartial stateはcallerへ公開しない。
// - out_filesystem_はNULLでなく、*out_filesystem_ == NULLであることを要求する。
// - *out_filesystem_ != NULLはoutput slotの誤用としてFILESYSTEM_BAD_OPERATIONを返す。
// - fullpath_はNULLでなく、空文字列ではなく、'/'から始まるabsolute pathであることを要求する。
// - mode_はvalidなfs_open_mode_tであることを要求する。
// - fullpath_はfile openにのみ使用し、Filesystem moduleは文字列自体のownershipを取得しない。
// - file openに失敗した場合はFILESYSTEM_FILE_OPEN_ERRORを返す。
// - recoverable failureでは*out_filesystem_を変更せず、construction途中に取得したtemporary resourceを解放する。
// - 成功時のみfilesystem_tのownershipをcallerへ移転する。
//
// filesystem_create Validation Policy
//
// - create前には有効なfilesystem_tが存在しないため、
//   PreconditionsではFilesystem validatorを使用しない。
// - caller-controlledなpointer、output slot、fullpath_およびmode_のAPI contractは全BUILDで検証する。
// - fs_open_mode_tのsemantic validityはfs_open_mode_is_valid()によって検証する。
// - filesystem_t storageのallocation validationはGeneral Allocatorへ委譲する。
// - file openの成否はexternal operation resultとして全BUILDで処理する。
// - DEBUG_BUILD / TEST_BUILDでは、temporary objectのconstruction完了後、
//   callerへ公開する前のStable boundaryでfilesystem_is_valid()を実行し、
//   canonical Postcondition validationを行う。
// - RELEASE_BUILDではautomatic canonical Postcondition validationを行わない。
// - canonical Postcondition validationに失敗してFILESYSTEM_DATA_CORRUPTEDが確定した場合は、
//   fail-stop ruleに従いtemporary resourceの通常cleanupを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
filesystem_result_t filesystem_create(filesystem_t** out_filesystem_, const char* fullpath_, fs_open_mode_t mode_) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    filesystem_t* tmp_filesystem = NULL;
    const char* open_mode_str = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "out_filesystem_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_filesystem_, ret, FILESYSTEM_BAD_OPERATION, result_to_str(FILESYSTEM_BAD_OPERATION), "filesystem_create", "*out_filesystem_")
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "fullpath_")
    if('\0' == fullpath_[0] || '/' != fullpath_[0]) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_create(%s) - Provided fullpath_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(!fs_open_mode_is_valid(mode_)) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_create(%s) - Provided mode_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(filesystem_t), GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO, (void**)&tmp_filesystem);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("filesystem_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }
    open_mode_str = fs_open_mode_c_str(mode_);
    if(NULL == open_mode_str) { // fopenへNULLが渡るのを避けるため、validなmodeであっても一応チェックを入れておく
        ret = FILESYSTEM_UNDEFINED_ERROR;
        ERROR_MESSAGE("filesystem_create(%s) - fs_open_mode_c_str returned NULL for valid mode.", result_to_str(ret));
        goto cleanup;
    }
    tmp_filesystem->file_handle = mock_fopen(fullpath_, open_mode_str);
    if(NULL == tmp_filesystem->file_handle) {
        ret = FILESYSTEM_FILE_OPEN_ERROR;
        ERROR_MESSAGE("filesystem_create(%s) - Failed to open file: '%s'.", result_to_str(ret), fullpath_);
        goto cleanup;
    }
    tmp_filesystem->mode = mode_;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(tmp_filesystem)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_create(%s) - Postcondition validation failed for 'tmp_filesystem'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_filesystem_ = tmp_filesystem;
    tmp_filesystem = NULL;

    ret = FILESYSTEM_SUCCESS;

cleanup:
    if(NULL != tmp_filesystem && FILESYSTEM_DATA_CORRUPTED != ret) {
        general_allocator_free((void**)&tmp_filesystem, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);
    }
    return ret;
}

// filesystem_destroy API Specification
//
// - filesystem_ == NULLまたは*filesystem_ == NULLの場合は何も行わずreturnする。
// - out_close_succeeded_はoptionalであり、NULLを指定してよい。
// - 有効なfilesystem_tに対してfile streamのcloseを試行した後、
//   filesystem_t自身のstorageを解放してobject lifetimeを終了する。
// - out_close_succeeded_ != NULLの場合、file streamのclose成功時はtrue、失敗時はfalseを格納する。
// - file streamのcloseに失敗した場合でもfilesystem_t自身のlifetimeは終了する。
// - close失敗後に同じfilesystem_tを使用してcloseを再試行することはできない。
// - filesystem_ == NULLまたは*filesystem_ == NULLによるno-op時は、
//   out_close_succeeded_の事前値を変更しない。
//
// filesystem_destroy Validation Policy
//
// - filesystem_ == NULLまたは*filesystem_ == NULLはno-opとして扱う。
// - DEBUG_BUILD / TEST_BUILDではresource release開始前にfilesystem_is_valid()を実行し、
//   canonical Precondition validationを行う。
// - canonical validationに失敗した場合はsuspectなfile_handleを辿らず、
//   file closeおよびfilesystem_t storageの解放を行わない。
// - canonical validation failure時、out_close_succeeded_ != NULLであればfalseを格納する。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてdestroyを実行する。
// - file closeの成否はexternal operation resultとして処理し、
//   filesystem_tのcanonical validityとは区別する。
// - filesystem_t storageのfreeに関するvalidationはGeneral Allocatorへ委譲する。
// - destroyによってobject lifetimeが終了するため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
void filesystem_destroy(filesystem_t** filesystem_, bool* out_close_succeeded_) {
    // Preconditions.
    if(NULL == filesystem_) {
        return;
    }
    if(NULL == *filesystem_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(*filesystem_)) {
        ERROR_MESSAGE("filesystem_destroy(%s) - Precondition validation failed for '*filesystem_'.", result_to_str(FILESYSTEM_DATA_CORRUPTED));
        if(NULL != out_close_succeeded_) {
            *out_close_succeeded_ = false;
        }
        return;
    }
#endif

    // Commit.
    if(NULL != (*filesystem_)->file_handle) {
        if(EOF == mock_fclose((*filesystem_)->file_handle)) {
            ERROR_MESSAGE("filesystem_destroy - Failed to close file handle.");
            if(NULL != out_close_succeeded_) {
                *out_close_succeeded_ = false;
            }
        } else {
            if(NULL != out_close_succeeded_) {
                *out_close_succeeded_ = true;
            }
        }
    } else {
        if(NULL != out_close_succeeded_) {
            *out_close_succeeded_ = false;
        }
    }
    general_allocator_free((void**)filesystem_, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);
}

// filesystem_byte_read API Specification
//
// - filesystem_は有効なfilesystem_tでなければならない。
// - read_bytes_は0より大きくなければならない。
// - out_read_bytes_およびout_buffer_はNULLであってはならない。
// - callerはout_buffer_に少なくともread_bytes_ byteを書き込み可能な領域を用意する。
// - filesystem_はreadableなopen modeで作成されていなければならない。
//   readableでない場合はFILESYSTEM_BAD_OPERATIONを返す。
//
// - requested byte数をすべて読み取った場合はFILESYSTEM_SUCCESSを返し、
//   *out_read_bytes_にread_bytes_を格納する。
// - requested byte数には届かなかったが、1byte以上を読み取った後にEOFへ到達した場合は
//   partial readとしてFILESYSTEM_SUCCESSを返し、
//   *out_read_bytes_に実際に読み取ったbyte数を格納する。
// - 1byteも読み取らずEOFへ到達した場合はFILESYSTEM_EOFを返し、
//   *out_read_bytes_に0を格納する。
// - stream errorが発生した場合はFILESYSTEM_RUNTIME_ERRORを返す。
// - short readの原因をEOF stateまたはerror stateから特定できない場合は
//   FILESYSTEM_UNDEFINED_ERRORを返す。
//
// - FILESYSTEM_SUCCESSまたはFILESYSTEM_EOFの場合のみ*out_read_bytes_を更新する。
// - その他のfailureでは*out_read_bytes_の事前値を維持する。
// - file I/O operation開始前にfailureした場合、out_buffer_は変更しない。
// - file I/O operation開始後は、最終resultがfailureであっても、
//   failure確定前に読み取られたdataがout_buffer_へ書き込まれている可能性がある。
// - file I/O operation開始後は、operationの成否にかかわらず
//   file position、EOF state、error state等のfile stream内部状態が変化し得る。
// - out_buffer_およびfile stream内部状態に発生したこれらの変更はrollbackしない。
//
// filesystem_byte_read Validation Policy
//
// - caller-controlledなpointer、read_bytes_およびoperation stateに関するAPI contractは
//   RELEASE_BUILDを含む全BUILDで検証する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでis_valid_shallow()を実行し、
//   byte readを安全に開始するために必要なFilesystem rootのlocal structural invariantを検証する。
// - byte readではFilesystem ownership closureのdeep validationを必要としないため、
//   Preconditionsでcanonical validatorは使用しない。
// - fs_open_mode_is_readable()によるreadable stateの確認は、
//   filesystem_tのcanonical validityとは別のoperation-specific preconditionとして全BUILDで行う。
//
// - Commit後、read_state_get()によってread byte数、EOF stateおよびerror stateから
//   external read operationの結果をfilesystem_read_state_tへ分類する。
// - filesystem_read_state_tをFILESYSTEM_SUCCESS、FILESYSTEM_EOF、
//   FILESYSTEM_RUNTIME_ERRORまたはFILESYSTEM_UNDEFINED_ERRORへ変換する。
// - external file streamから発生し得るEOF、stream error等の結果判定は全BUILDで行う。
//
// - byte readによってfilesystem_t自身のfile_handleおよびmodeは変更されないため、
//   filesystem_tに対するcanonical Postcondition validationは行わない。
// - FILE*内部のfile position、EOF state、error state等は
//   Filesystem canonical validatorの対象としない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
filesystem_result_t filesystem_byte_read(filesystem_t* filesystem_, size_t read_bytes_, size_t* out_read_bytes_, char* out_buffer_) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    size_t result_n = 0;
    filesystem_read_state_t status = FILESYSTEM_READ_STATE_ERROR;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "filesystem_")
    IF_ARG_NULL_GOTO_CLEANUP(out_read_bytes_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "out_read_bytes_")
    IF_ARG_NULL_GOTO_CLEANUP(out_buffer_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "out_buffer_")
    if(0 == read_bytes_) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_byte_read(%s) - provided read_bytes_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(filesystem_)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_byte_read(%s) - Precondition validation failed for 'filesystem_'.", result_to_str(ret));
        goto cleanup;
    }
#endif
    if(!fs_open_mode_is_readable(filesystem_->mode)) {
        ret = FILESYSTEM_BAD_OPERATION;
        ERROR_MESSAGE("filesystem_byte_read(%s) - File is not opened in a readable mode.", result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    result_n = mock_fread(out_buffer_, 1, read_bytes_, filesystem_->file_handle); // (1 x read_bytes_)を読み取り

    // Postconditions.
    status = read_state_get(filesystem_->file_handle, read_bytes_, result_n);
    switch(status) {
    case FILESYSTEM_READ_STATE_COMPLETE:
        ret = FILESYSTEM_SUCCESS;
        break;
    case FILESYSTEM_READ_STATE_PARTIAL_EOF:
        ret = FILESYSTEM_SUCCESS;
        break;
    case FILESYSTEM_READ_STATE_EOF:
        ret = FILESYSTEM_EOF;
        break;
    case FILESYSTEM_READ_STATE_ERROR:
        ret = FILESYSTEM_RUNTIME_ERROR;
        break;
    case FILESYSTEM_READ_STATE_UNDEFINED:
        ret = FILESYSTEM_UNDEFINED_ERROR;
        break;
    default:
        ret = FILESYSTEM_UNDEFINED_ERROR;
        break;
    }
    if(FILESYSTEM_SUCCESS != ret && FILESYSTEM_EOF != ret) {
        ERROR_MESSAGE("filesystem_byte_read(%s) - fileread failed.", result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_read_bytes_ = result_n;

cleanup:
    return ret;
}

// filesystem_is_valid API Specification
//
// - 本APIはfilesystem_tのpublic canonical validatorである。
// - filesystem_ == NULLの場合はfalseを返す。
// - initializedなfilesystem_tがModule Internal Contractで定義された
//   Stable stateを満たしている場合はtrueを返す。
// - validation中にfilesystem_tまたはfile streamのstateを変更しない。
// - validation failure時はfalseを返すのみとし、error messageは出力しない。
//
// filesystem_is_valid Validation Policy
//
// - explicit validatorであるためBUILD_MODEによってvalidation semanticsを変更しない。
// - canonical validationは最初にis_valid_shallow()を実行する。
// - shallow validationではfilesystem_t rootのlocal structural invariantとして、
//   modeがvalidなfs_open_mode_tであること、およびfile_handle != NULLであることを検証する。
// - 現在のfilesystem_tには追加のowned internal structureが存在しないため、
//   canonical validatorによる追加のdeep validationは行わない。
// - FILE*内部のfile position、EOF state、error stateその他のexternal file stream stateは
//   Filesystem moduleが所有するcanonical invariantではないため検証対象としない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool filesystem_is_valid(const filesystem_t* filesystem_) {
    if(NULL == filesystem_) {
        return false;
    }
    if(!is_valid_shallow(filesystem_)) {
        return false;
    }
    return true;
}

// ============================================================
// Mock functions
// ============================================================
static FILE* NO_COVERAGE mock_fopen(const char* fullpath_, const char* mode_) {
    return fopen(fullpath_, mode_);
}

static int NO_COVERAGE mock_fclose(FILE* stream_) {
    return fclose(stream_);
}

static size_t NO_COVERAGE mock_fread(void *ptr_, size_t size_, size_t nmemb_, FILE *stream_) {
    return fread(ptr_, size_, nmemb_, stream_);
}

static int NO_COVERAGE mock_ferror(FILE *stream_) {
    return ferror(stream_);
}

static int NO_COVERAGE mock_feof(FILE *stream_) {
    return feof(stream_);
}

// ============================================================
// File I/O helpers
// ============================================================
static filesystem_read_state_t read_state_get(FILE* file_handle_, size_t requested_bytes_, size_t read_bytes_) {
    filesystem_read_state_t ret = FILESYSTEM_READ_STATE_ERROR;

    if(NULL == file_handle_) {
        return FILESYSTEM_READ_STATE_ERROR;
    }

    if(requested_bytes_ == read_bytes_) {
        ret = FILESYSTEM_READ_STATE_COMPLETE;
    } else {
        if(mock_ferror(file_handle_)) {
            ret = FILESYSTEM_READ_STATE_ERROR;
        } else if(mock_feof(file_handle_)) {
            if(0 == read_bytes_) {
                ret = FILESYSTEM_READ_STATE_EOF;
            } else {
                ret = FILESYSTEM_READ_STATE_PARTIAL_EOF;
            }
        } else {
            ret = FILESYSTEM_READ_STATE_UNDEFINED;
        }
    }

    return ret;
}

// ============================================================
// Utilities
// ============================================================
/**
 * @brief filesystemモジュール実行結果コードを文字列に変換する
 *
 * @param[in] result_ 実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* result_to_str(filesystem_result_t result_) {
    switch(result_) {
    case FILESYSTEM_SUCCESS:
        return s_result_str_success;
    case FILESYSTEM_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case FILESYSTEM_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case FILESYSTEM_NO_MEMORY:
        return s_result_str_no_memory;
    case FILESYSTEM_FILE_OPEN_ERROR:
        return s_result_str_file_open_error;
    case FILESYSTEM_EOF:
        return s_result_str_eof;
    case FILESYSTEM_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case FILESYSTEM_BAD_OPERATION:
        return s_result_str_bad_operation;
    case FILESYSTEM_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case FILESYSTEM_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

static filesystem_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return FILESYSTEM_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return FILESYSTEM_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return FILESYSTEM_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return FILESYSTEM_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return FILESYSTEM_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return FILESYSTEM_UNDEFINED_ERROR;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return FILESYSTEM_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return FILESYSTEM_UNDEFINED_ERROR;
    default:
        return FILESYSTEM_UNDEFINED_ERROR;
    }
}

// ============================================================
// Validators
// ============================================================
static bool is_valid_shallow(const filesystem_t* filesystem_) {
    if(NULL == filesystem_) {
        return false;
    }
    if(!fs_open_mode_is_valid(filesystem_->mode)) {
        return false;
    }
    if(NULL == filesystem_->file_handle) {
        return false;
    }
    return true;
}
