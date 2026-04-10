/** @ingroup renderer_core
 *
 * @file renderer_err_utils.c
 * @author chocolate-pie24
 * @brief レンダラーレイヤー全体で使用されるエラー処理関連APIの実装
 *
 * @version 0.1
 * @date 2025-12-19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/renderer/renderer_core/renderer_types.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"

#include "engine/core/memory/linear_allocator.h"
#include "engine/core/memory/choco_memory.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <string.h>

#include "test_controller.h"
#include "engine/base/choco_macros.h"

#include "engine/renderer/renderer_core/test_renderer_err_utils.h"

// renderer_err_utils用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_renderer_rslt_convert_linear_alloc;    /**< renderer_rslt_convert_linear_alloc()テスト設定 */
static test_call_control_t s_test_config_renderer_rslt_convert_choco_memory;    /**< renderer_rslt_convert_choco_memory()テスト設定 */
static test_call_control_t s_test_config_renderer_rslt_convert_choco_string;    /**< renderer_rslt_convert_choco_string()テスト設定 */
static test_call_control_t s_test_config_renderer_rslt_convert_fs_utils;        /**< renderer_rslt_convert_fs_utils()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_renderer_rslt_to_str(void);
static void test_renderer_rslt_convert_linear_alloc(void);
static void test_renderer_rslt_convert_choco_memory(void);
static void test_renderer_rslt_convert_choco_string(void);
static void test_renderer_rslt_convert_fs_utils(void);
#endif

static const char* s_rslt_str_success = "SUCCESS";                            /**< 実行結果コードRENDERER_SUCCESSの文字列 */
static const char* s_rslt_str_invalid_argument = "INVALID_ARGUMENT";          /**< 実行結果コードRENDERER_INVALID_ARGUMENTの文字列 */
static const char* s_rslt_str_runtime_error = "RUNTIME_ERROR";                /**< 実行結果コードRENDERER_RUNTIME_ERRORの文字列 */
static const char* s_rslt_str_no_memory = "NO_MEMORY";                        /**< 実行結果コードRENDERER_NO_MEMORYの文字列 */
static const char* s_rslt_str_shader_compile_error = "SHADER_COMPILE_ERROR";  /**< 実行結果コードRENDERER_SHADER_COMPILE_ERRORの文字列 */
static const char* s_rslt_str_shader_link_error = "SHADER_LINK_ERROR";        /**< 実行結果コードRENDERER_SHADER_LINK_ERRORの文字列 */
static const char* s_rslt_str_undefined_error = "UNDEFINED_ERROR";            /**< 実行結果コードRENDERER_UNDEFINED_ERRORの文字列 */
static const char* s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";              /**< 実行結果コードRENDERER_LIMIT_EXCEEDEDの文字列 */
static const char* s_rslt_str_bad_operation = "BAD_OPERATION";                /**< 実行結果コードRENDERER_BAD_OPERATIONの文字列 */
static const char* s_rslt_str_data_corrupted = "DATA_CORRUPTED";              /**< 実行結果コードRENDERER_DATA_CORRUPTEDの文字列 */

const char* renderer_rslt_to_str(renderer_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_SUCCESS:
        return s_rslt_str_success;
    case RENDERER_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RENDERER_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RENDERER_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RENDERER_SHADER_COMPILE_ERROR:
        return s_rslt_str_shader_compile_error;
    case RENDERER_SHADER_LINK_ERROR:
        return s_rslt_str_shader_link_error;
    case RENDERER_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RENDERER_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RENDERER_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RENDERER_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

renderer_result_t renderer_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_renderer_rslt_convert_linear_alloc.call_count++;
    if(s_test_config_renderer_rslt_convert_linear_alloc.fail_on_call != 0) {
        if(s_test_config_renderer_rslt_convert_linear_alloc.call_count == s_test_config_renderer_rslt_convert_linear_alloc.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_rslt_convert_linear_alloc.forced_result;
        }
    }
#endif
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return RENDERER_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return RENDERER_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return RENDERER_INVALID_ARGUMENT;
    default:
        return RENDERER_UNDEFINED_ERROR;
    }
}

renderer_result_t renderer_rslt_convert_choco_memory(memory_system_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_renderer_rslt_convert_choco_memory.call_count++;
    if(s_test_config_renderer_rslt_convert_choco_memory.fail_on_call != 0) {
        if(s_test_config_renderer_rslt_convert_choco_memory.call_count == s_test_config_renderer_rslt_convert_choco_memory.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_rslt_convert_choco_memory.forced_result;
        }
    }
#endif
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return RENDERER_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return RENDERER_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return RENDERER_RUNTIME_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return RENDERER_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return RENDERER_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return RENDERER_NO_MEMORY;
    default:
        return RENDERER_UNDEFINED_ERROR;
    }
}

renderer_result_t renderer_rslt_convert_choco_string(choco_string_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_renderer_rslt_convert_choco_string.call_count++;
    if(s_test_config_renderer_rslt_convert_choco_string.fail_on_call != 0) {
        if(s_test_config_renderer_rslt_convert_choco_string.call_count == s_test_config_renderer_rslt_convert_choco_string.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_rslt_convert_choco_string.forced_result;
        }
    }
#endif
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return RENDERER_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return RENDERER_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return RENDERER_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return RENDERER_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return RENDERER_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return RENDERER_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return RENDERER_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW: // 文字列長さオーバーフローはRUNTIME_ERRORに変換
        return RENDERER_RUNTIME_ERROR;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return RENDERER_LIMIT_EXCEEDED;
    default:
        return RENDERER_UNDEFINED_ERROR;
    }
}

renderer_result_t renderer_rslt_convert_fs_utils(fs_utils_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_renderer_rslt_convert_fs_utils.call_count++;
    if(s_test_config_renderer_rslt_convert_fs_utils.fail_on_call != 0) {
        if(s_test_config_renderer_rslt_convert_fs_utils.call_count == s_test_config_renderer_rslt_convert_fs_utils.fail_on_call) {
            return (renderer_result_t)s_test_config_renderer_rslt_convert_fs_utils.forced_result;
        }
    }
#endif
    switch(rslt_) {
    case FS_UTILS_SUCCESS:
        return RENDERER_SUCCESS;
    case FS_UTILS_INVALID_ARGUMENT:
        return RENDERER_INVALID_ARGUMENT;
    case FS_UTILS_BAD_OPERATION:
        return RENDERER_BAD_OPERATION;
    case FS_UTILS_DATA_CORRUPTED:
        return RENDERER_DATA_CORRUPTED;
    case FS_UTILS_NO_MEMORY:
        return RENDERER_NO_MEMORY;
    case FS_UTILS_LIMIT_EXCEEDED:
        return RENDERER_LIMIT_EXCEEDED;
    case FS_UTILS_OVERFLOW: // オーバーフローはRUNTIME_ERRORに変換
        return RENDERER_RUNTIME_ERROR;
    case FS_UTILS_FILE_OPEN_ERROR:
        return RENDERER_RUNTIME_ERROR;
    case FS_UTILS_RUNTIME_ERROR:
        return RENDERER_RUNTIME_ERROR;
    case FS_UTILS_UNDEFINED_ERROR:
        return RENDERER_UNDEFINED_ERROR;
    default:
        return RENDERER_UNDEFINED_ERROR;
    }
}

#ifdef TEST_BUILD
void NO_COVERAGE test_renderer_rslt_convert_linear_alloc_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_rslt_convert_linear_alloc.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_rslt_convert_linear_alloc.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_rslt_convert_choco_memory_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_rslt_convert_choco_memory.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_rslt_convert_choco_memory.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_rslt_convert_choco_string_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_rslt_convert_choco_string.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_rslt_convert_choco_string.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_rslt_convert_fs_utils_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_renderer_rslt_convert_fs_utils.fail_on_call = config_->fail_on_call;
    s_test_config_renderer_rslt_convert_fs_utils.forced_result = config_->forced_result;
}

void NO_COVERAGE test_renderer_err_utils_config_reset(void) {
    test_call_control_reset(&s_test_config_renderer_rslt_convert_linear_alloc);
    test_call_control_reset(&s_test_config_renderer_rslt_convert_choco_memory);
    test_call_control_reset(&s_test_config_renderer_rslt_convert_choco_string);
    test_call_control_reset(&s_test_config_renderer_rslt_convert_fs_utils);
}

void NO_COVERAGE test_renderer_err_utils(void) {
    test_renderer_rslt_to_str();
    test_renderer_rslt_convert_linear_alloc();
    test_renderer_rslt_convert_choco_memory();
    test_renderer_rslt_convert_choco_string();
    test_renderer_rslt_convert_fs_utils();
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_rslt_to_str(void) {
    const char* actual = NULL;

    actual = renderer_rslt_to_str(RENDERER_SUCCESS);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "SUCCESS"));

    actual = renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "INVALID_ARGUMENT"));

    actual = renderer_rslt_to_str(RENDERER_RUNTIME_ERROR);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "RUNTIME_ERROR"));

    actual = renderer_rslt_to_str(RENDERER_NO_MEMORY);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "NO_MEMORY"));

    actual = renderer_rslt_to_str(RENDERER_SHADER_COMPILE_ERROR);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "SHADER_COMPILE_ERROR"));

    actual = renderer_rslt_to_str(RENDERER_SHADER_LINK_ERROR);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "SHADER_LINK_ERROR"));

    actual = renderer_rslt_to_str(RENDERER_LIMIT_EXCEEDED);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "LIMIT_EXCEEDED"));

    actual = renderer_rslt_to_str(RENDERER_BAD_OPERATION);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "BAD_OPERATION"));

    actual = renderer_rslt_to_str(RENDERER_DATA_CORRUPTED);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "DATA_CORRUPTED"));

    actual = renderer_rslt_to_str(RENDERER_UNDEFINED_ERROR);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "UNDEFINED_ERROR"));

    actual = renderer_rslt_to_str((renderer_result_t)99999);
    assert(NULL != actual);
    assert(0 == strcmp(actual, "UNDEFINED_ERROR"));
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_rslt_convert_linear_alloc(void) {
    renderer_result_t actual = RENDERER_UNDEFINED_ERROR;
    test_call_control_t config = { 0 };

    // 通常変換確認
    test_renderer_err_utils_config_reset();

    actual = renderer_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(RENDERER_SUCCESS == actual);

    actual = renderer_rslt_convert_linear_alloc(LINEAR_ALLOC_NO_MEMORY);
    assert(RENDERER_NO_MEMORY == actual);

    actual = renderer_rslt_convert_linear_alloc(LINEAR_ALLOC_INVALID_ARGUMENT);
    assert(RENDERER_INVALID_ARGUMENT == actual);

    actual = renderer_rslt_convert_linear_alloc((linear_allocator_result_t)99999);
    assert(RENDERER_UNDEFINED_ERROR == actual);

    // 失敗注入確認: 1回目の呼び出しで強制返却
    test_renderer_err_utils_config_reset();
    test_call_control_reset(&config);
    config.fail_on_call = 1;
    config.forced_result = (int)RENDERER_BAD_OPERATION;
    test_renderer_rslt_convert_linear_alloc_config_set(&config);

    actual = renderer_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(RENDERER_BAD_OPERATION == actual);

    // 失敗注入確認: 2回目の呼び出しで強制返却
    test_renderer_err_utils_config_reset();
    test_call_control_reset(&config);
    config.fail_on_call = 2;
    config.forced_result = (int)RENDERER_LIMIT_EXCEEDED;
    test_renderer_rslt_convert_linear_alloc_config_set(&config);

    actual = renderer_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(RENDERER_SUCCESS == actual);

    actual = renderer_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(RENDERER_LIMIT_EXCEEDED == actual);

    // 後続へ影響を残さないようにリセット
    test_renderer_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_rslt_convert_choco_memory(void) {
    renderer_result_t actual = RENDERER_UNDEFINED_ERROR;
    test_call_control_t config = { 0 };

    // 通常変換確認
    test_renderer_err_utils_config_reset();

    actual = renderer_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS);
    assert(RENDERER_SUCCESS == actual);

    actual = renderer_rslt_convert_choco_memory(MEMORY_SYSTEM_INVALID_ARGUMENT);
    assert(RENDERER_INVALID_ARGUMENT == actual);

    actual = renderer_rslt_convert_choco_memory(MEMORY_SYSTEM_RUNTIME_ERROR);
    assert(RENDERER_RUNTIME_ERROR == actual);

    actual = renderer_rslt_convert_choco_memory(MEMORY_SYSTEM_LIMIT_EXCEEDED);
    assert(RENDERER_LIMIT_EXCEEDED == actual);

    actual = renderer_rslt_convert_choco_memory(MEMORY_SYSTEM_BAD_OPERATION);
    assert(RENDERER_BAD_OPERATION == actual);

    actual = renderer_rslt_convert_choco_memory(MEMORY_SYSTEM_NO_MEMORY);
    assert(RENDERER_NO_MEMORY == actual);

    actual = renderer_rslt_convert_choco_memory((memory_system_result_t)99999);
    assert(RENDERER_UNDEFINED_ERROR == actual);

    // 失敗注入確認: 1回目の呼び出しで強制返却
    test_renderer_err_utils_config_reset();
    test_call_control_reset(&config);
    config.fail_on_call = 1;
    config.forced_result = (int)RENDERER_BAD_OPERATION;
    test_renderer_rslt_convert_choco_memory_config_set(&config);

    actual = renderer_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS);
    assert(RENDERER_BAD_OPERATION == actual);

    // 失敗注入確認: 2回目の呼び出しで強制返却
    test_renderer_err_utils_config_reset();
    test_call_control_reset(&config);
    config.fail_on_call = 2;
    config.forced_result = (int)RENDERER_DATA_CORRUPTED;
    test_renderer_rslt_convert_choco_memory_config_set(&config);

    actual = renderer_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS);
    assert(RENDERER_SUCCESS == actual);

    actual = renderer_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS);
    assert(RENDERER_DATA_CORRUPTED == actual);

    // 後続テストへの影響防止
    test_renderer_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_rslt_convert_choco_string(void) {
    renderer_result_t actual = RENDERER_UNDEFINED_ERROR;
    test_call_control_t config = { 0 };

    // 通常変換確認
    test_renderer_err_utils_config_reset();

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_SUCCESS);
    assert(RENDERER_SUCCESS == actual);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_DATA_CORRUPTED);
    assert(RENDERER_DATA_CORRUPTED == actual);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_BAD_OPERATION);
    assert(RENDERER_BAD_OPERATION == actual);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_NO_MEMORY);
    assert(RENDERER_NO_MEMORY == actual);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_INVALID_ARGUMENT);
    assert(RENDERER_INVALID_ARGUMENT == actual);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_RUNTIME_ERROR);
    assert(RENDERER_RUNTIME_ERROR == actual);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_UNDEFINED_ERROR);
    assert(RENDERER_UNDEFINED_ERROR == actual);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_OVERFLOW);
    assert(RENDERER_RUNTIME_ERROR == actual);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_LIMIT_EXCEEDED);
    assert(RENDERER_LIMIT_EXCEEDED == actual);

    actual = renderer_rslt_convert_choco_string((choco_string_result_t)99999);
    assert(RENDERER_UNDEFINED_ERROR == actual);

    // 失敗注入確認: 1回目の呼び出しで強制返却
    test_renderer_err_utils_config_reset();
    test_call_control_reset(&config);
    config.fail_on_call = 1;
    config.forced_result = (int)RENDERER_SHADER_COMPILE_ERROR;
    test_renderer_rslt_convert_choco_string_config_set(&config);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_SUCCESS);
    assert(RENDERER_SHADER_COMPILE_ERROR == actual);

    // 失敗注入確認: 2回目の呼び出しで強制返却
    test_renderer_err_utils_config_reset();
    test_call_control_reset(&config);
    config.fail_on_call = 2;
    config.forced_result = (int)RENDERER_SHADER_LINK_ERROR;
    test_renderer_rslt_convert_choco_string_config_set(&config);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_SUCCESS);
    assert(RENDERER_SUCCESS == actual);

    actual = renderer_rslt_convert_choco_string(CHOCO_STRING_SUCCESS);
    assert(RENDERER_SHADER_LINK_ERROR == actual);

    // 後続テストへの影響防止
    test_renderer_err_utils_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_renderer_rslt_convert_fs_utils(void) {
    renderer_result_t actual = RENDERER_UNDEFINED_ERROR;
    test_call_control_t config = { 0 };

    // 通常変換確認
    test_renderer_err_utils_config_reset();

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_SUCCESS);
    assert(RENDERER_SUCCESS == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_INVALID_ARGUMENT);
    assert(RENDERER_INVALID_ARGUMENT == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_BAD_OPERATION);
    assert(RENDERER_BAD_OPERATION == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_DATA_CORRUPTED);
    assert(RENDERER_DATA_CORRUPTED == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_NO_MEMORY);
    assert(RENDERER_NO_MEMORY == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_LIMIT_EXCEEDED);
    assert(RENDERER_LIMIT_EXCEEDED == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_OVERFLOW);
    assert(RENDERER_RUNTIME_ERROR == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_FILE_OPEN_ERROR);
    assert(RENDERER_RUNTIME_ERROR == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_RUNTIME_ERROR);
    assert(RENDERER_RUNTIME_ERROR == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_UNDEFINED_ERROR);
    assert(RENDERER_UNDEFINED_ERROR == actual);

    actual = renderer_rslt_convert_fs_utils((fs_utils_result_t)99999);
    assert(RENDERER_UNDEFINED_ERROR == actual);

    // 失敗注入確認: 1回目の呼び出しで強制返却
    test_renderer_err_utils_config_reset();
    test_call_control_reset(&config);
    config.fail_on_call = 1;
    config.forced_result = (int)RENDERER_SHADER_COMPILE_ERROR;
    test_renderer_rslt_convert_fs_utils_config_set(&config);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_SUCCESS);
    assert(RENDERER_SHADER_COMPILE_ERROR == actual);

    // 失敗注入確認: 2回目の呼び出しで強制返却
    test_renderer_err_utils_config_reset();
    test_call_control_reset(&config);
    config.fail_on_call = 2;
    config.forced_result = (int)RENDERER_SHADER_LINK_ERROR;
    test_renderer_rslt_convert_fs_utils_config_set(&config);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_SUCCESS);
    assert(RENDERER_SUCCESS == actual);

    actual = renderer_rslt_convert_fs_utils(FS_UTILS_SUCCESS);
    assert(RENDERER_SHADER_LINK_ERROR == actual);

    // 後続テストへの影響防止
    test_renderer_err_utils_config_reset();
}
#endif
