/** @ingroup resource
 *
 * @file resource_err_utils.c
 * @author chocolate-pie24
 * @brief Resourceレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能の実装
 *
 * @version 0.1
 * @date 2026-05-05
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/filesystem/filesystem.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <string.h>

#include "test_controller.h"
#include "engine/base/choco_macros.h"
#include "engine/resource/resource_core/test_resource_err_utils.h"

// resource_err_utils用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_resource_rslt_convert_choco_memory;    /**< resource_rslt_convert_choco_memory()テスト設定 */
static test_call_control_t s_test_config_resource_rslt_convert_filesystem;      /**< resource_rslt_convert_filesystem()テスト設定 */
static test_call_control_t s_test_config_resource_rslt_convert_fs_utils;        /**< resource_rslt_convert_fs_utils()テスト設定 */
static test_call_control_t s_test_config_resource_rslt_convert_choco_string;    /**< resource_rslt_convert_choco_string()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_resource_rslt_to_str(void);
static void test_resource_rslt_convert_choco_memory(void);
static void test_resource_rslt_convert_filesystem(void);
static void test_resource_rslt_convert_fs_utils(void);
static void test_resource_rslt_convert_choco_string(void);
#endif

static const char* const s_rslt_str_success = "SUCCESS";                      /**< 実行結果コード文字列: 正常終了 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                  /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";          /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";    /**< 実行結果コード文字列: 引数異常 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";        /**< 実行結果コード文字列: メモリ破壊 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";          /**< 実行結果コード文字列: API誤用、未初期化 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                    /**< 実行結果コード文字列: 計算過程でオーバーフロー発生 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";        /**< 実行結果コード文字列: システム使用可能範囲上限超過 */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";      /**< 実行結果コード文字列: ファイルオープンエラー */
static const char* const s_rslt_str_file_read_error = "FILE_READ_ERROR";      /**< 実行結果コード文字列: ファイル読み込みエラー */
static const char* const s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";    /**< 実行結果コード文字列: ファイルクローズエラー */
static const char* const s_rslt_str_unsupported_file = "UNSUPPORTED_FILE";    /**< 実行結果コード文字列: 未対応のファイル形式エラー */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";      /**< 実行結果コード文字列: 未定義エラー */

const char* resource_rslt_to_str(resource_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_SUCCESS:
        return s_rslt_str_success;
    case RESOURCE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RESOURCE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RESOURCE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RESOURCE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RESOURCE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RESOURCE_OVERFLOW:
        return s_rslt_str_overflow;
    case RESOURCE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RESOURCE_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case RESOURCE_FILE_READ_ERROR:
        return s_rslt_str_file_read_error;
    case RESOURCE_FILE_CLOSE_ERROR:
        return s_rslt_str_file_close_error;
    case RESOURCE_UNSUPPORTED_FILE:
        return s_rslt_str_unsupported_file;
    case RESOURCE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

resource_result_t resource_rslt_convert_choco_memory(memory_system_result_t result_) {
#ifdef TEST_BUILD
    s_test_config_resource_rslt_convert_choco_memory.call_count++;
    if(s_test_config_resource_rslt_convert_choco_memory.fail_on_call != 0) {
        if(s_test_config_resource_rslt_convert_choco_memory.call_count == s_test_config_resource_rslt_convert_choco_memory.fail_on_call) {
            return (resource_result_t)s_test_config_resource_rslt_convert_choco_memory.forced_result;
        }
    }
#endif
    switch(result_) {
    case MEMORY_SYSTEM_SUCCESS:
        return RESOURCE_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return RESOURCE_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return RESOURCE_RUNTIME_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return RESOURCE_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return RESOURCE_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return RESOURCE_NO_MEMORY;
    default:
        return RESOURCE_UNDEFINED_ERROR;
    }
}

resource_result_t resource_rslt_convert_filesystem(filesystem_result_t result_) {
#ifdef TEST_BUILD
    s_test_config_resource_rslt_convert_filesystem.call_count++;
    if(s_test_config_resource_rslt_convert_filesystem.fail_on_call != 0) {
        if(s_test_config_resource_rslt_convert_filesystem.call_count == s_test_config_resource_rslt_convert_filesystem.fail_on_call) {
            return (resource_result_t)s_test_config_resource_rslt_convert_filesystem.forced_result;
        }
    }
#endif
    switch(result_) {
    case FILESYSTEM_SUCCESS:
        return RESOURCE_SUCCESS;
    case FILESYSTEM_INVALID_ARGUMENT:
        return RESOURCE_INVALID_ARGUMENT;
    case FILESYSTEM_RUNTIME_ERROR:
        return RESOURCE_RUNTIME_ERROR;
    case FILESYSTEM_NO_MEMORY:
        return RESOURCE_NO_MEMORY;
    case FILESYSTEM_FILE_OPEN_ERROR:
        return RESOURCE_FILE_OPEN_ERROR;
    case FILESYSTEM_FILE_CLOSE_ERROR:
        return RESOURCE_FILE_CLOSE_ERROR;
    case FILESYSTEM_UNDEFINED_ERROR:
        return RESOURCE_UNDEFINED_ERROR;
    case FILESYSTEM_LIMIT_EXCEEDED:
        return RESOURCE_LIMIT_EXCEEDED;
    case FILESYSTEM_BAD_OPERATION:
        return RESOURCE_BAD_OPERATION;
    case FILESYSTEM_EOF:
        return RESOURCE_FILE_READ_ERROR;
    default:
        return RESOURCE_UNDEFINED_ERROR;
    }
}

resource_result_t resource_rslt_convert_fs_utils(fs_utils_result_t result_) {
#ifdef TEST_BUILD
    s_test_config_resource_rslt_convert_fs_utils.call_count++;
    if(s_test_config_resource_rslt_convert_fs_utils.fail_on_call != 0) {
        if(s_test_config_resource_rslt_convert_fs_utils.call_count == s_test_config_resource_rslt_convert_fs_utils.fail_on_call) {
            return (resource_result_t)s_test_config_resource_rslt_convert_fs_utils.forced_result;
        }
    }
#endif
    switch(result_) {
    case FS_UTILS_SUCCESS:
        return RESOURCE_SUCCESS;
    case FS_UTILS_INVALID_ARGUMENT:
        return RESOURCE_INVALID_ARGUMENT;
    case FS_UTILS_BAD_OPERATION:
        return RESOURCE_BAD_OPERATION;
    case FS_UTILS_DATA_CORRUPTED:
        return RESOURCE_DATA_CORRUPTED;
    case FS_UTILS_NO_MEMORY:
        return RESOURCE_NO_MEMORY;
    case FS_UTILS_LIMIT_EXCEEDED:
        return RESOURCE_LIMIT_EXCEEDED;
    case FS_UTILS_OVERFLOW:
        return RESOURCE_OVERFLOW;
    case FS_UTILS_FILE_OPEN_ERROR:
        return RESOURCE_FILE_OPEN_ERROR;
    case FS_UTILS_RUNTIME_ERROR:
        return RESOURCE_RUNTIME_ERROR;
    case FS_UTILS_UNDEFINED_ERROR:
        return RESOURCE_UNDEFINED_ERROR;
    default:
        return RESOURCE_UNDEFINED_ERROR;
    }
}

resource_result_t resource_rslt_convert_choco_string(choco_string_result_t result_) {
#ifdef TEST_BUILD
    s_test_config_resource_rslt_convert_choco_string.call_count++;
    if(s_test_config_resource_rslt_convert_choco_string.fail_on_call != 0) {
        if(s_test_config_resource_rslt_convert_choco_string.call_count == s_test_config_resource_rslt_convert_choco_string.fail_on_call) {
            return (resource_result_t)s_test_config_resource_rslt_convert_choco_string.forced_result;
        }
    }
#endif
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return RESOURCE_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return RESOURCE_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return RESOURCE_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return RESOURCE_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return RESOURCE_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return RESOURCE_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return RESOURCE_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return RESOURCE_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return RESOURCE_LIMIT_EXCEEDED;
    default:
        return RESOURCE_UNDEFINED_ERROR;
    }
}

#ifdef TEST_BUILD
void NO_COVERAGE test_resource_rslt_convert_choco_memory_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_resource_rslt_convert_choco_memory.fail_on_call = config_->fail_on_call;
    s_test_config_resource_rslt_convert_choco_memory.forced_result = config_->forced_result;
}

void NO_COVERAGE test_resource_rslt_convert_filesystem_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_resource_rslt_convert_filesystem.fail_on_call = config_->fail_on_call;
    s_test_config_resource_rslt_convert_filesystem.forced_result = config_->forced_result;
}

void NO_COVERAGE test_resource_rslt_convert_fs_utils_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_resource_rslt_convert_fs_utils.fail_on_call = config_->fail_on_call;
    s_test_config_resource_rslt_convert_fs_utils.forced_result = config_->forced_result;
}

void NO_COVERAGE test_resource_rslt_convert_choco_string_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_resource_rslt_convert_choco_string.fail_on_call = config_->fail_on_call;
    s_test_config_resource_rslt_convert_choco_string.forced_result = config_->forced_result;
}

void NO_COVERAGE test_resource_err_utils_config_reset(void) {
    test_call_control_reset(&s_test_config_resource_rslt_convert_choco_memory);
    test_call_control_reset(&s_test_config_resource_rslt_convert_filesystem);
    test_call_control_reset(&s_test_config_resource_rslt_convert_fs_utils);
    test_call_control_reset(&s_test_config_resource_rslt_convert_choco_string);
}

void NO_COVERAGE test_resource_err_utils(void) {
    test_resource_rslt_to_str();
    test_resource_rslt_convert_choco_memory();
    test_resource_rslt_convert_filesystem();
    test_resource_rslt_convert_fs_utils();
    test_resource_rslt_convert_choco_string();
}

// Generated by ChatGPT
static void NO_COVERAGE test_resource_rslt_to_str(void) {
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_SUCCESS), "SUCCESS"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_NO_MEMORY), "NO_MEMORY"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_RUNTIME_ERROR), "RUNTIME_ERROR"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "INVALID_ARGUMENT"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "DATA_CORRUPTED"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_BAD_OPERATION), "BAD_OPERATION"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_OVERFLOW), "OVERFLOW"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_LIMIT_EXCEEDED), "LIMIT_EXCEEDED"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_FILE_OPEN_ERROR), "FILE_OPEN_ERROR"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_FILE_READ_ERROR), "FILE_READ_ERROR"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_FILE_CLOSE_ERROR), "FILE_CLOSE_ERROR"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_UNSUPPORTED_FILE), "UNSUPPORTED_FILE"));
    assert(0 == strcmp(resource_rslt_to_str(RESOURCE_UNDEFINED_ERROR), "UNDEFINED_ERROR"));

    assert(0 == strcmp(resource_rslt_to_str((resource_result_t)-1), "UNDEFINED_ERROR"));
}

// Generated by ChatGPT
static void NO_COVERAGE test_resource_rslt_convert_choco_memory(void) {
    test_call_control_t config = {0};

    test_resource_err_utils_config_reset();

    assert(RESOURCE_SUCCESS == resource_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS));
    assert(RESOURCE_INVALID_ARGUMENT == resource_rslt_convert_choco_memory(MEMORY_SYSTEM_INVALID_ARGUMENT));
    assert(RESOURCE_RUNTIME_ERROR == resource_rslt_convert_choco_memory(MEMORY_SYSTEM_RUNTIME_ERROR));
    assert(RESOURCE_LIMIT_EXCEEDED == resource_rslt_convert_choco_memory(MEMORY_SYSTEM_LIMIT_EXCEEDED));
    assert(RESOURCE_BAD_OPERATION == resource_rslt_convert_choco_memory(MEMORY_SYSTEM_BAD_OPERATION));
    assert(RESOURCE_NO_MEMORY == resource_rslt_convert_choco_memory(MEMORY_SYSTEM_NO_MEMORY));

    assert(RESOURCE_UNDEFINED_ERROR == resource_rslt_convert_choco_memory((memory_system_result_t)-1));

    test_resource_err_utils_config_reset();

    config.fail_on_call = 2U;
    config.forced_result = RESOURCE_NO_MEMORY;
    test_resource_rslt_convert_choco_memory_config_set(&config);

    assert(RESOURCE_SUCCESS == resource_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS));
    assert(RESOURCE_NO_MEMORY == resource_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS));

    test_resource_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_resource_rslt_convert_filesystem(void) {
    test_call_control_t config = {0};

    test_resource_err_utils_config_reset();

    assert(RESOURCE_SUCCESS == resource_rslt_convert_filesystem(FILESYSTEM_SUCCESS));
    assert(RESOURCE_INVALID_ARGUMENT == resource_rslt_convert_filesystem(FILESYSTEM_INVALID_ARGUMENT));
    assert(RESOURCE_RUNTIME_ERROR == resource_rslt_convert_filesystem(FILESYSTEM_RUNTIME_ERROR));
    assert(RESOURCE_NO_MEMORY == resource_rslt_convert_filesystem(FILESYSTEM_NO_MEMORY));
    assert(RESOURCE_FILE_OPEN_ERROR == resource_rslt_convert_filesystem(FILESYSTEM_FILE_OPEN_ERROR));
    assert(RESOURCE_FILE_CLOSE_ERROR == resource_rslt_convert_filesystem(FILESYSTEM_FILE_CLOSE_ERROR));
    assert(RESOURCE_UNDEFINED_ERROR == resource_rslt_convert_filesystem(FILESYSTEM_UNDEFINED_ERROR));
    assert(RESOURCE_LIMIT_EXCEEDED == resource_rslt_convert_filesystem(FILESYSTEM_LIMIT_EXCEEDED));
    assert(RESOURCE_BAD_OPERATION == resource_rslt_convert_filesystem(FILESYSTEM_BAD_OPERATION));
    assert(RESOURCE_FILE_READ_ERROR == resource_rslt_convert_filesystem(FILESYSTEM_EOF));

    assert(RESOURCE_UNDEFINED_ERROR == resource_rslt_convert_filesystem((filesystem_result_t)-1));

    test_resource_err_utils_config_reset();

    config.fail_on_call = 2U;
    config.forced_result = RESOURCE_NO_MEMORY;
    test_resource_rslt_convert_filesystem_config_set(&config);

    assert(RESOURCE_SUCCESS == resource_rslt_convert_filesystem(FILESYSTEM_SUCCESS));
    assert(RESOURCE_NO_MEMORY == resource_rslt_convert_filesystem(FILESYSTEM_SUCCESS));

    test_resource_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_resource_rslt_convert_fs_utils(void) {
    test_call_control_t config = {0};

    test_resource_err_utils_config_reset();

    assert(RESOURCE_SUCCESS == resource_rslt_convert_fs_utils(FS_UTILS_SUCCESS));
    assert(RESOURCE_INVALID_ARGUMENT == resource_rslt_convert_fs_utils(FS_UTILS_INVALID_ARGUMENT));
    assert(RESOURCE_BAD_OPERATION == resource_rslt_convert_fs_utils(FS_UTILS_BAD_OPERATION));
    assert(RESOURCE_DATA_CORRUPTED == resource_rslt_convert_fs_utils(FS_UTILS_DATA_CORRUPTED));
    assert(RESOURCE_NO_MEMORY == resource_rslt_convert_fs_utils(FS_UTILS_NO_MEMORY));
    assert(RESOURCE_LIMIT_EXCEEDED == resource_rslt_convert_fs_utils(FS_UTILS_LIMIT_EXCEEDED));
    assert(RESOURCE_OVERFLOW == resource_rslt_convert_fs_utils(FS_UTILS_OVERFLOW));
    assert(RESOURCE_FILE_OPEN_ERROR == resource_rslt_convert_fs_utils(FS_UTILS_FILE_OPEN_ERROR));
    assert(RESOURCE_RUNTIME_ERROR == resource_rslt_convert_fs_utils(FS_UTILS_RUNTIME_ERROR));
    assert(RESOURCE_UNDEFINED_ERROR == resource_rslt_convert_fs_utils(FS_UTILS_UNDEFINED_ERROR));

    assert(RESOURCE_UNDEFINED_ERROR == resource_rslt_convert_fs_utils((fs_utils_result_t)-1));

    test_resource_err_utils_config_reset();

    config.fail_on_call = 2U;
    config.forced_result = RESOURCE_NO_MEMORY;
    test_resource_rslt_convert_fs_utils_config_set(&config);

    assert(RESOURCE_SUCCESS == resource_rslt_convert_fs_utils(FS_UTILS_SUCCESS));
    assert(RESOURCE_NO_MEMORY == resource_rslt_convert_fs_utils(FS_UTILS_SUCCESS));

    test_resource_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_resource_rslt_convert_choco_string(void) {
    test_call_control_t config = {0};

    test_resource_err_utils_config_reset();

    assert(RESOURCE_SUCCESS == resource_rslt_convert_choco_string(CHOCO_STRING_SUCCESS));
    assert(RESOURCE_DATA_CORRUPTED == resource_rslt_convert_choco_string(CHOCO_STRING_DATA_CORRUPTED));
    assert(RESOURCE_BAD_OPERATION == resource_rslt_convert_choco_string(CHOCO_STRING_BAD_OPERATION));
    assert(RESOURCE_NO_MEMORY == resource_rslt_convert_choco_string(CHOCO_STRING_NO_MEMORY));
    assert(RESOURCE_INVALID_ARGUMENT == resource_rslt_convert_choco_string(CHOCO_STRING_INVALID_ARGUMENT));
    assert(RESOURCE_RUNTIME_ERROR == resource_rslt_convert_choco_string(CHOCO_STRING_RUNTIME_ERROR));
    assert(RESOURCE_UNDEFINED_ERROR == resource_rslt_convert_choco_string(CHOCO_STRING_UNDEFINED_ERROR));
    assert(RESOURCE_OVERFLOW == resource_rslt_convert_choco_string(CHOCO_STRING_OVERFLOW));
    assert(RESOURCE_LIMIT_EXCEEDED == resource_rslt_convert_choco_string(CHOCO_STRING_LIMIT_EXCEEDED));

    assert(RESOURCE_UNDEFINED_ERROR == resource_rslt_convert_choco_string((choco_string_result_t)-1));

    test_resource_err_utils_config_reset();

    config.fail_on_call = 2U;
    config.forced_result = RESOURCE_NO_MEMORY;
    test_resource_rslt_convert_choco_string_config_set(&config);

    assert(RESOURCE_SUCCESS == resource_rslt_convert_choco_string(CHOCO_STRING_SUCCESS));
    assert(RESOURCE_NO_MEMORY == resource_rslt_convert_choco_string(CHOCO_STRING_SUCCESS));

    test_resource_err_utils_config_reset();
}
#endif
