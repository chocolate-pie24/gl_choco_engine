// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/io_utils/fs_path.h"

#include <stddef.h>
#include <stdbool.h>
#include <string.h> // for strlen, strrchr
#include <stdint.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"

/*
 * Module Internal Contract
 *
 * Canonical state:
 * - fullpathはNULLではない。
 * - fullpathはGeneral Allocatorから取得されたliveなchoco_string_t objectを指す。
 * - fullpathが指すchoco_string_tはChoco String moduleのcanonical stateを満たす。
 * - fullpathが表す文字列は空ではない。
 *
 * Representation / Ownership:
 * - fs_path_tはfullpathが指すchoco_string_t objectを単独で所有し、
 *   そのlifetimeを管理する。
 * - fullpathが所有する文字列storageのownershipおよび内部representationは
 *   Choco String moduleのContractに従う。
 * - fs_path_tはChoco String内部の文字列storageを直接所有または管理しない。
 *
 * State Transition:
 * - Public APIへ公開されたfs_path_tはCanonical stateを維持する。
 * - createではfs_path_tおよびowned fullpathの構築中にpartial construction stateを許容する。
 * - create途中のpartial stateはcallerへ公開しない。
 * - canonical validation完了後にのみ、完成したfs_path_tのownershipをcallerへcommitする。
 * - destroyではowned fullpathを破棄した後、fs_path_t自身のlifetimeを終了する。
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
 * - 現在のfs_path_tはowned fullpathのみを保持し、
 *   module内部にshallow validation depthを必要とするcall siteは存在しないため、
 *   private shallow validatorは設けない。
 *
 * - canonical validatorはfullpath != NULLであることを確認する。
 * - owned fullpathをdereferenceする前に、
 *   general_allocator_ptr_is_allocated()でfullpathがGeneral Allocator上の
 *   current allocationであることを確認する。
 * - allocation validity確認後、choco_string_is_valid()へvalidationを委譲し、
 *   owned choco_string_tのcanonical validityを確認する。
 * - Choco Stringのcanonical validation成功後、
 *   FS Path固有のsemantic invariantとしてfullpathが空文字列ではないことを確認する。
 *
 * - canonical validatorは、引数path_自身のallocation validityを検証しない。
 *   path_をowned pointerとして保持するownerが、そのallocation validityを
 *   ownership closureの一部として検証する責務を持つ。
 *
 * - fs_path_tが所有するfullpath内部のbuffer representationおよび文字列semanticは
 *   Choco String moduleの責務であり、FS Path moduleでは重複して検証しない。
 *
 * - canonical validationによってDATA_CORRUPTEDが確定した場合、
 *   suspectなownership graphを辿るcleanupまたはresource releaseは行わない。
 *
 * - explicit validatorであるfs_path_is_valid()のvalidation semanticsは
 *   BUILD_MODEによって変更しない。
 * - validatorは対象stateを変更せず、validation failure時はfalseを返す。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
struct fs_path {
    choco_string_t* fullpath;
};

// ============================================================
// Private Constants
// ============================================================
#define FS_PATH_DEFAULT_BUFF_SIZE 128

#ifdef _WIN32
static const char s_path_separator = '\\';
#else
static const char s_path_separator = '/';
#endif

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

// ============================================================
// Private Function Declarations
// ============================================================
// Fullpath getters
static fs_path_result_t executable_fullpath_get(char** out_fullpath_, size_t* out_bufsize_);
#if defined(__APPLE__)
static fs_path_result_t executable_fullpath_get_apple(char** out_fullpath_, size_t* out_bufsize_);
#elif defined(__linux__)
static fs_path_result_t executable_fullpath_get_linux(char** out_fullpath_, size_t* out_bufsize_);
#elif defined(__FreeBSD__)
static fs_path_result_t executable_fullpath_get_freebsd(char** out_fullpath_, size_t* out_bufsize_);
#endif

// Utilities
static const char* result_to_str(fs_path_result_t result_);
static fs_path_result_t result_convert_general_allocator(general_allocator_result_t result_);
static fs_path_result_t result_convert_choco_string(choco_string_result_t result_);

// ============================================================
// Public API
// ============================================================

// fs_path_create Validation Policy
//
// - 対応platformでは、out_path_、base_path_、path_、name_のpointer contractを
//   operationを開始するために必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
// - *out_path_ == NULLであることは、新規objectを安全にcommitするために必要な
//   checked preconditionとして全BUILDで検証する。
// - *out_path_ != NULLは既存pointerを上書きするAPI misuseであるため、
//   FS_PATH_BAD_OPERATIONとして扱う。
//
// - base_path_、path_、name_は空文字列を許可しない。
// - path_はbase_path_からの相対pathとして扱うため、先頭が'/'である入力を許可しない。
// - extension_はNULLを許可する。
// - extension_ != NULLの場合は空文字列を許可せず、先頭が'.'である入力も許可しない。
// - これらはFS Path moduleが所有するoperation-specific semantic preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
//
// - base_path_、path_、name_、extension_として受け取るnon-NULLのC stringは、
//   Module Boundary Contractで定義された有効な終端NUL付きC stringとして扱う。
// - C string representation自体を別のvalidatorで再認証しない。
//
// - fullpath構築に使用するChoco String固有のstateおよびsemantic validationは、
//   対応するChoco String public APIへ委譲し、FS Path側では重複して検証しない。
//
// - DEBUG_BUILD / TEST_BUILDでは、fs_path_tとowned fullpathの構築完了後、
//   callerへのcommit前のstable boundaryでfs_path_is_valid()を実行し、
//   canonical Postcondition validationを行う。
// - Postcondition validationに成功した場合だけ、完成したfs_path_tのownershipを
//   *out_path_へcommitする。
// - RELEASE_BUILDではautomatic canonical Postcondition validationを行わず、
//   construction処理と下位moduleのContractによってcanonical stateが成立することを前提とする。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
fs_path_result_t fs_path_create(fs_path_t** out_path_, const char* base_path_, const char* path_, const char* name_, const char* extension_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    fs_path_t* tmp_path = NULL;
    choco_string_t* tmp_fullpath = NULL;
    size_t length = 0;

    // Preconditions
#ifdef _WIN32
    ret = FS_PATH_RUNTIME_ERROR;
    ERROR_MESSAGE("fs_path_create(%s) - Platform windows is not supported yet.", result_to_str(ret));
    goto cleanup;
#endif
    IF_ARG_NULL_GOTO_CLEANUP(out_path_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create", "out_path_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_path_, ret, FS_PATH_BAD_OPERATION, result_to_str(FS_PATH_BAD_OPERATION), "fs_path_create", "*out_path_")
    IF_ARG_NULL_GOTO_CLEANUP(base_path_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create", "base_path_")
    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create", "path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create", "name_")
    if('\0' == base_path_[0]) {
        ret = FS_PATH_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_path_create(%s) - Provided base_path_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if('\0' == path_[0] || '/' == path_[0]) {
        ret = FS_PATH_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_path_create(%s) - Provided path_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if('\0' == name_[0]) {
        ret = FS_PATH_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_path_create(%s) - Provided name_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(NULL != extension_ && ('\0' == extension_[0] || '.' == extension_[0])) {
        ret = FS_PATH_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_path_create(%s) - Provided extension_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    // fullpath生成
    ret_choco_string = choco_string_create_from_c_string(base_path_, &tmp_fullpath);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("fs_path_create(%s) - choco_string_create_from_c_string failed.", result_to_str(ret));
        goto cleanup;
    }
    length = strlen(base_path_);
    if('/' != base_path_[length - 1]) { // basepath_の末尾に'/'を付加
        ret_choco_string = choco_string_concat_from_c_string("/", tmp_fullpath);
        if(CHOCO_STRING_SUCCESS != ret_choco_string) {
            ret = result_convert_choco_string(ret_choco_string);
            ERROR_MESSAGE("fs_path_create(%s) - choco_string_concat_from_c_string failed.", result_to_str(ret));
            goto cleanup;
        }
    }
    ret_choco_string = choco_string_concat_from_c_string(path_, tmp_fullpath);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("fs_path_create(%s) - choco_string_concat_from_c_string failed.", result_to_str(ret));
        goto cleanup;
    }
    length = strlen(path_);
    if('/' != path_[length - 1]) {  // basepath_ + path_の末尾に'/'を付加
        ret_choco_string = choco_string_concat_from_c_string("/", tmp_fullpath);
        if(CHOCO_STRING_SUCCESS != ret_choco_string) {
            ret = result_convert_choco_string(ret_choco_string);
            ERROR_MESSAGE("fs_path_create(%s) - choco_string_concat_from_c_string failed.", result_to_str(ret));
            goto cleanup;
        }
    }
    ret_choco_string = choco_string_concat_from_c_string(name_, tmp_fullpath);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("fs_path_create(%s) - choco_string_concat_from_c_string failed.", result_to_str(ret));
        goto cleanup;
    }
    if(NULL != extension_) {
        ret_choco_string = choco_string_concat_from_c_string(".", tmp_fullpath);
        if(CHOCO_STRING_SUCCESS != ret_choco_string) {
            ret = result_convert_choco_string(ret_choco_string);
            ERROR_MESSAGE("fs_path_create(%s) - choco_string_concat_from_c_string failed.", result_to_str(ret));
            goto cleanup;
        }
        ret_choco_string = choco_string_concat_from_c_string(extension_, tmp_fullpath);
        if(CHOCO_STRING_SUCCESS != ret_choco_string) {
            ret = result_convert_choco_string(ret_choco_string);
            ERROR_MESSAGE("fs_path_create(%s) - choco_string_concat_from_c_string failed.", result_to_str(ret));
            goto cleanup;
        }
    }

    // fs_path_t生成
    ret_general_allocator = general_allocator_allocate(sizeof(fs_path_t), GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO, (void**)&tmp_path);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("fs_path_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    tmp_path->fullpath = tmp_fullpath;
    tmp_fullpath = NULL;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_path_is_valid(tmp_path)) {
        ret = FS_PATH_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_path_create(%s) - Postcondition validation failed for 'tmp_path'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_path_ = tmp_path;
    tmp_path = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    if(FS_PATH_DATA_CORRUPTED != ret) {
        if(NULL != tmp_fullpath) {
            choco_string_destroy(&tmp_fullpath);
        }
        if(NULL != tmp_path) {
            fs_path_destroy(&tmp_path);
        }
    }

    return ret;
}

// fs_path_create_from_executable_directory Validation Policy
//
// - out_path_のpointer contract、および*out_path_ == NULLであることは、
//   operationを開始し、新規objectを安全にcommitするために必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
// - *out_path_ != NULLは既存pointerを上書きするAPI misuseであるため、
//   FS_PATH_BAD_OPERATIONとして扱う。
//
// - executable fullpathの取得に必要なOS固有処理およびtemporary bufferの生成は
//   executable_fullpath_get()へ委譲する。
// - executable_fullpath_get()から正常に返されたC stringは、そのprivate helperの
//   Contractを満たしているものとして扱い、caller側で同じ条件を重複して検証しない。
//
// - executable fullpathからdirectory部分を抽出できない場合はruntime failureとして扱い、
//   fs_path_tのcanonical corruptionとは区別する。
//
// - fullpath生成に使用するChoco String固有のstateおよびsemantic validationは、
//   対応するChoco String public APIへ委譲し、FS Path側では重複して検証しない。
//
// - DEBUG_BUILD / TEST_BUILDでは、fs_path_tとowned fullpathの構築完了後、
//   callerへのcommit前のstable boundaryでfs_path_is_valid()を実行し、
//   canonical Postcondition validationを行う。
// - Postcondition validationに成功した場合だけ、完成したfs_path_tのownershipを
//   *out_path_へcommitする。
// - RELEASE_BUILDではautomatic canonical Postcondition validationを行わず、
//   construction処理と下位moduleのContractによってcanonical stateが成立することを前提とする。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
fs_path_result_t fs_path_create_from_executable_directory(fs_path_t** out_path_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    char* executable_path = NULL;
    char* separator_ptr = NULL;
    size_t executable_path_buf_size = 0;
    fs_path_t* tmp_path = NULL;
    choco_string_t* tmp_fullpath = NULL;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(out_path_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "fs_path_create_from_executable_directory", "out_path_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_path_, ret, FS_PATH_BAD_OPERATION, result_to_str(FS_PATH_BAD_OPERATION), "fs_path_create_from_executable_directory", "*out_path_")

    // Prepare.
    // fullpath生成
    ret = executable_fullpath_get(&executable_path, &executable_path_buf_size);
    if(FS_PATH_SUCCESS != ret) {
        ERROR_MESSAGE("fs_path_create_from_executable_directory(%s) - executable_fullpath_get failed.", result_to_str(ret));
        goto cleanup;
    }
    // 末尾の'/'を除去
    separator_ptr = strrchr(executable_path, s_path_separator);
    if(NULL == separator_ptr) {
        ret = FS_PATH_RUNTIME_ERROR;
        ERROR_MESSAGE("fs_path_create_from_executable_directory(%s) - Executable path does not contain a path separator.", result_to_str(ret));
        goto cleanup;
    }
    if(separator_ptr == executable_path) {
        separator_ptr[1] = '\0';
    } else {
        *separator_ptr = '\0';
    }
    ret_choco_string = choco_string_create_from_c_string(executable_path, &tmp_fullpath);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("fs_path_create_from_executable_directory(%s) - choco_string_create_from_c_string failed.", result_to_str(ret));
        goto cleanup;
    }
    // fs_path_t生成
    ret_general_allocator = general_allocator_allocate(sizeof(fs_path_t), GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO, (void**)&tmp_path);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("fs_path_create_from_executable_directory(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    tmp_path->fullpath = tmp_fullpath;
    tmp_fullpath = NULL;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_path_is_valid(tmp_path)) {
        ret = FS_PATH_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_path_create_from_executable_directory(%s) - Postcondition validation failed for 'tmp_path'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_path_ = tmp_path;
    tmp_path = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    if(FS_PATH_DATA_CORRUPTED != ret) {
        if(NULL != tmp_fullpath) {
            choco_string_destroy(&tmp_fullpath);
        }
        if(NULL != tmp_path) {
            fs_path_destroy(&tmp_path);
        }
        if(0 != executable_path_buf_size && NULL != executable_path) {
            general_allocator_free((void**)&executable_path, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);
        }
    }

    return ret;
}

// fs_path_destroy Validation Policy
//
// - path_ == NULLまたは*path_ == NULLの場合はno-opとして扱う。
//
// - DEBUG_BUILD / TEST_BUILDではowned resourceのreleaseを開始する前に
//   fs_path_is_valid()を実行し、canonical Precondition validationを行う。
// - canonical validationに失敗した場合は、suspectなowned fullpathを辿らず、
//   fullpathおよびfs_path_t自身のresource releaseを行わない。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてdestroyを実行する。
//
// - owned choco_string_tの破棄に必要なvalidationおよびresource releaseは
//   choco_string_destroy()へ委譲する。
// - fs_path_t自身のstorage releaseに関するvalidationはGeneral Allocatorへ委譲する。
//
// - destroyによってobject lifetimeが終了するため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
void fs_path_destroy(fs_path_t** path_) {
    if(NULL == path_) {
        return;
    }
    if(NULL == *path_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_path_is_valid(*path_)) {
        ERROR_MESSAGE("fs_path_destroy(%s) - Precondition validation failed for '*path_'.", result_to_str(FS_PATH_DATA_CORRUPTED));
        return;
    }
#endif

    choco_string_destroy(&(*path_)->fullpath);
    general_allocator_free((void**)path_, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);
}

// fs_path_fullpath_get Validation Policy
//
// - path_ == NULLの場合は、APIで定義されたfallback valueとしてNULLを返す。
//
// - 本operationはowned fullpathが保持するC stringへのborrowed pointerを
//   module外部へ公開するため、fullpathをcanonicalなChoco Stringとして
//   安全に参照できることを必要とする。
// - DEBUG_BUILD / TEST_BUILDではfs_path_is_valid()を実行し、
//   path_にcanonical Precondition validationを行う。
// - canonical validationに失敗した場合はowned fullpathを参照せずNULLを返す。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてfullpathを参照する。
//
// - canonicalなfs_path_tからC string pointerを取得する処理は
//   choco_string_c_str()へ委譲する。
// - 本operationはobject stateを変更しないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
const char* fs_path_fullpath_get(const fs_path_t* path_) {
    if(NULL == path_) {
        return NULL;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_path_is_valid(path_)) {
        ERROR_MESSAGE("fs_path_fullpath_get(%s) - Provided path_ is corrupted.", result_to_str(FS_PATH_DATA_CORRUPTED));
        return NULL;
    }
#endif
    return choco_string_c_str(path_->fullpath);
}

// fs_path_is_valid Validation Policy
//
// - 本APIはfs_path_tのpublic canonical validatorである。
// - path_ == NULLの場合はfalseを返す。
// - Module Internal Contractで定義されたCanonical state全体を検証する。
//
// - fullpath == NULLの場合はfalseを返す。
// - owned fullpathをdereferenceする前に、general_allocator_ptr_is_allocated()で
//   fullpathがGeneral Allocator上のcurrent allocationであることを確認する。
// - owned fullpathのactual allocation sizeとcapacityの整合性検証については、
//   allocation metadataの利用方法と合わせて将来検討する。
// - allocation validity確認後、choco_string_is_valid()を実行し、
//   owned choco_string_tのcanonical validityを確認する。
// - Choco Stringのcanonical validation成功後、FS Path固有のsemantic invariantとして
//   fullpathが空文字列ではないことを確認する。
//
// - path_自身のallocation validityは本validatorでは検証しない。
//   path_をowned pointerとして保持するowner側が、そのallocation validityを
//   ownership closureの一部として検証する。
//
// - fullpath内部のbuffer representationおよび文字列semanticの検証は
//   Choco String moduleへ委譲し、FS Path側では重複して検証しない。
//
// - explicit validatorであるため、BUILD_MODEによってvalidation semanticsを変更しない。
// - validation中に対象stateを変更しない。
// - validation failure時はfalseを返すのみとし、error messageは出力しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool fs_path_is_valid(const fs_path_t* path_) {
    if(NULL == path_) {
        return false;
    }
    if(NULL == path_->fullpath) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated((const void*)path_->fullpath)) {
        return false;
    }
    if(!choco_string_is_valid(path_->fullpath)) {
        return false;
    }
    if(0 == choco_string_length(path_->fullpath)) {
        return false;
    }
    return true;
}

// ============================================================
// Fullpath getters
// ============================================================
static fs_path_result_t executable_fullpath_get(char** out_fullpath_, size_t* out_bufsize_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_fullpath_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "executable_fullpath_get", "out_fullpath_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_fullpath_, ret, FS_PATH_BAD_OPERATION, result_to_str(FS_PATH_BAD_OPERATION), "executable_fullpath_get", "*out_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_bufsize_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "executable_fullpath_get", "out_bufsize_")

    // Output.
#if defined(__APPLE__)
    ret = executable_fullpath_get_apple(out_fullpath_, out_bufsize_);
    if(FS_PATH_SUCCESS != ret) {
        ERROR_MESSAGE("executable_fullpath_get(%s) - executable_fullpath_get_apple failed.", result_to_str(ret));
        goto cleanup;
    }
    ret = FS_PATH_SUCCESS;
#elif defined(__linux__)
    ret = executable_fullpath_get_linux(out_fullpath_, out_bufsize_);
    if(FS_PATH_SUCCESS != ret) {
        ERROR_MESSAGE("executable_fullpath_get(%s) - executable_fullpath_get_linux failed.", result_to_str(ret));
        goto cleanup;
    }
    ret = FS_PATH_SUCCESS;
#elif defined(__FreeBSD__)
    ret = executable_fullpath_get_freebsd(out_fullpath_, out_bufsize_);
    if(FS_PATH_SUCCESS != ret) {
        ERROR_MESSAGE("executable_fullpath_get(%s) - executable_fullpath_get_freebsd failed.", result_to_str(ret));
        goto cleanup;
    }
    ret = FS_PATH_SUCCESS;
#else
    ret = FS_PATH_RUNTIME_ERROR;
    ERROR_MESSAGE("executable_fullpath_get(%s) - Unsupported platform.", result_to_str(ret));
    goto cleanup;
#endif

cleanup:
    return ret;
}

#ifdef __APPLE__
static fs_path_result_t executable_fullpath_get_apple(char** out_fullpath_, size_t* out_bufsize_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    uint32_t bufsize = FS_PATH_DEFAULT_BUFF_SIZE;
    uint32_t allocated_size = 0;
    char* buf = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_fullpath_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "executable_fullpath_get_apple", "out_fullpath_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_fullpath_, ret, FS_PATH_BAD_OPERATION, result_to_str(FS_PATH_BAD_OPERATION), "executable_fullpath_get_apple", "*out_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_bufsize_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "executable_fullpath_get_apple", "out_bufsize_")

    // Prepare.
    ret_general_allocator = general_allocator_allocate(bufsize, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO, (void**)&buf);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("executable_fullpath_get_apple(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    allocated_size = bufsize;
    if(0 != _NSGetExecutablePath(buf, &bufsize)) {
        general_allocator_free((void**)&buf, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);

        ret_general_allocator = general_allocator_allocate(bufsize, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO, (void**)&buf);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("executable_fullpath_get_apple(%s) - general_allocator_allocate failed.", result_to_str(ret));
            goto cleanup;
        }

        allocated_size = bufsize;
        if(0 != _NSGetExecutablePath(buf, &bufsize)) {
            general_allocator_free((void**)&buf, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);
            ret = FS_PATH_UNDEFINED_ERROR;
            ERROR_MESSAGE("executable_fullpath_get_apple(%s) - _NSGetExecutablePath failed.", result_to_str(ret));
            goto cleanup;
        }
    }

    // Output.
    *out_fullpath_ = buf;
    *out_bufsize_ = (size_t)allocated_size;
    buf = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    return ret;
}
#endif

#ifdef __linux__
static fs_path_result_t executable_fullpath_get_linux(char** out_fullpath_, size_t* out_bufsize_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    bool success = false;
    ssize_t result = 0;
    size_t bufsize = FS_PATH_DEFAULT_BUFF_SIZE;
    size_t allocated_size = 0;
    char* buf = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_fullpath_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "executable_fullpath_get_linux", "out_fullpath_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_fullpath_, ret, FS_PATH_BAD_OPERATION, result_to_str(FS_PATH_BAD_OPERATION), "executable_fullpath_get_linux", "*out_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_bufsize_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "executable_fullpath_get_linux", "out_bufsize_")

    // Prepare.
    while(!success) {
        ret_general_allocator = general_allocator_allocate(bufsize, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO, (void**)&buf);
        if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
            ret = result_convert_general_allocator(ret_general_allocator);
            ERROR_MESSAGE("executable_fullpath_get_linux(%s) - general_allocator_allocate failed.", result_to_str(ret));
            goto cleanup;
        }

        allocated_size = bufsize;

        result = readlink("/proc/self/exe", buf, allocated_size);
        if (-1 == result) {
            general_allocator_free((void**)&buf, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);
            ret = FS_PATH_RUNTIME_ERROR;
            ERROR_MESSAGE("executable_fullpath_get_linux(%s) - readlink failed.", result_to_str(ret));
            goto cleanup;
        }

        if((size_t)result < allocated_size) {
            buf[result] = '\0';
            success = true;
        } else {    // 切り詰められている可能性があるためバッファを拡張し再取得
            general_allocator_free((void**)&buf, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);

            if((SIZE_MAX / 2) < bufsize) {
                ret = FS_PATH_OVERFLOW;
                ERROR_MESSAGE("executable_fullpath_get_linux(%s) - buffer size overflow.", result_to_str(ret));
                goto cleanup;
            }
            bufsize *= 2;
        }
    }

    // Output.
    *out_fullpath_ = buf;
    *out_bufsize_ = (size_t)allocated_size;
    buf = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    return ret;
}
#endif

#ifdef __FreeBSD__
static fs_path_result_t executable_fullpath_get_freebsd(char** out_fullpath_, size_t* out_bufsize_) {
    fs_path_result_t ret = FS_PATH_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    size_t allocated_size = 0;
    size_t required_size = 0;
    char* buf = NULL;
    int mib[4] = { 0 };

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = -1;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_fullpath_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "executable_fullpath_get_freebsd", "out_fullpath_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_fullpath_, ret, FS_PATH_BAD_OPERATION, result_to_str(FS_PATH_BAD_OPERATION), "executable_fullpath_get_freebsd", "*out_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_bufsize_, ret, FS_PATH_INVALID_ARGUMENT, result_to_str(FS_PATH_INVALID_ARGUMENT), "executable_fullpath_get_freebsd", "out_bufsize_")

    // Prepare.
    // バッファサイズ取得
    if(0 != sysctl(mib, 4, NULL, &required_size, NULL, 0)) {
        ret = FS_PATH_RUNTIME_ERROR;
        ERROR_MESSAGE("executable_fullpath_get_freebsd(%s) - sysctl failed.", result_to_str(ret));
        goto cleanup;
    }
    if(0 == required_size) {
        ret = FS_PATH_RUNTIME_ERROR;
        ERROR_MESSAGE("executable_fullpath_get_freebsd(%s) - sysctl failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(required_size, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO, (void**)&buf);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("executable_fullpath_get_freebsd(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }
    allocated_size = required_size;

    // パス文字列取得
    if(0 != sysctl(mib, 4, buf, &required_size, NULL, 0)) {
        general_allocator_free((void**)&buf, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);

        ret = FS_PATH_RUNTIME_ERROR;
        ERROR_MESSAGE("executable_fullpath_get_freebsd(%s) - sysctl failed.", result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_fullpath_ = buf;
    *out_bufsize_ = allocated_size;
    buf = NULL;

    ret = FS_PATH_SUCCESS;

cleanup:
    return ret;
}
#endif

// ============================================================
// Utilities
// ============================================================
static const char* result_to_str(fs_path_result_t result_) {
    switch(result_) {
    case FS_PATH_SUCCESS:
        return s_result_str_success;
    case FS_PATH_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case FS_PATH_BAD_OPERATION:
        return s_result_str_bad_operation;
    case FS_PATH_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case FS_PATH_NO_MEMORY:
        return s_result_str_no_memory;
    case FS_PATH_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case FS_PATH_OVERFLOW:
        return s_result_str_overflow;
    case FS_PATH_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case FS_PATH_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

static fs_path_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return FS_PATH_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return FS_PATH_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return FS_PATH_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return FS_PATH_UNDEFINED_ERROR;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return FS_PATH_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return FS_PATH_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return FS_PATH_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return FS_PATH_UNDEFINED_ERROR;
    default:
        return FS_PATH_UNDEFINED_ERROR;
    }
}

static fs_path_result_t result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return FS_PATH_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return FS_PATH_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return FS_PATH_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return FS_PATH_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return FS_PATH_UNDEFINED_ERROR;
    case CHOCO_STRING_RUNTIME_ERROR:
        return FS_PATH_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return FS_PATH_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return FS_PATH_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return FS_PATH_LIMIT_EXCEEDED;
    default:
        return FS_PATH_UNDEFINED_ERROR;
    }
}
