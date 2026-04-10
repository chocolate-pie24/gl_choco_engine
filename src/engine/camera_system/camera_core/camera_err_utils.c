/** @ingroup camera_system
 *
 * @file camera_err_utils.c
 * @author chocolate-pie24
 * @brief カメラシステムレイヤー内でのエラー処理仕様を統一する実行結果コード変換機能の実装
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/camera_system/camera_core/camera_err_utils.h"

#include "engine/camera_system/camera_core/camera_types.h"

#include "engine/containers/choco_string.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include "test_controller.h"
#include "engine/base/choco_macros.h"
#include "engine/camera_system/camera_core/test_camera_err_utils.h"

// camera_err_utils用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_camera_rslt_convert_choco_memory;  /**< camera_rslt_convert_choco_memory()テスト設定 */
static test_call_control_t s_test_config_camera_rslt_convert_choco_string;  /**< camera_rslt_convert_choco_string()テスト設定 */
static test_call_control_t s_test_config_camera_rslt_convert_linear_alloc;  /**< camera_rslt_convert_linear_alloc()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_camera_rslt_to_str(void);
static void test_camera_rslt_convert_choco_memory(void);
static void test_camera_rslt_convert_choco_string(void);
static void test_camera_rslt_convert_linear_alloc(void);
#endif

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コード(成功)文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(無効な引数)文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用範囲上限超過)文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(内部データ破損)文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コード(未定義エラー)文字列 */

const char* camera_rslt_to_str(camera_result_t rslt_) {
    switch(rslt_) {
    case CAMERA_SUCCESS:
        return s_rslt_str_success;
    case CAMERA_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case CAMERA_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case CAMERA_NO_MEMORY:
        return s_rslt_str_no_memory;
    case CAMERA_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case CAMERA_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case CAMERA_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case CAMERA_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

camera_result_t camera_rslt_convert_choco_memory(memory_system_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_camera_rslt_convert_choco_memory.call_count++;
    if(s_test_config_camera_rslt_convert_choco_memory.fail_on_call != 0) {
        if(s_test_config_camera_rslt_convert_choco_memory.call_count == s_test_config_camera_rslt_convert_choco_memory.fail_on_call) {
            return (camera_result_t)s_test_config_camera_rslt_convert_choco_memory.forced_result;
        }
    }
#endif
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return CAMERA_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return CAMERA_RUNTIME_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return CAMERA_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return CAMERA_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
}

camera_result_t camera_rslt_convert_choco_string(choco_string_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_camera_rslt_convert_choco_string.call_count++;
    if(s_test_config_camera_rslt_convert_choco_string.fail_on_call != 0) {
        if(s_test_config_camera_rslt_convert_choco_string.call_count == s_test_config_camera_rslt_convert_choco_string.fail_on_call) {
            return (camera_result_t)s_test_config_camera_rslt_convert_choco_string.forced_result;
        }
    }
#endif
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return CAMERA_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return CAMERA_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return CAMERA_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return CAMERA_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return CAMERA_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return CAMERA_RUNTIME_ERROR;  // OVERFLOW -> RUNTIMEERRORに伝播
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return CAMERA_LIMIT_EXCEEDED;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
}

camera_result_t camera_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
#ifdef TEST_BUILD
    s_test_config_camera_rslt_convert_linear_alloc.call_count++;
    if(s_test_config_camera_rslt_convert_linear_alloc.fail_on_call != 0) {
        if(s_test_config_camera_rslt_convert_linear_alloc.call_count == s_test_config_camera_rslt_convert_linear_alloc.fail_on_call) {
            return (camera_result_t)s_test_config_camera_rslt_convert_linear_alloc.forced_result;
        }
    }
#endif
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return CAMERA_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
}

#ifdef TEST_BUILD
void NO_COVERAGE test_camera_rslt_convert_choco_memory_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_rslt_convert_choco_memory.fail_on_call = config_->fail_on_call;
    s_test_config_camera_rslt_convert_choco_memory.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_rslt_convert_choco_string_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_rslt_convert_choco_string.fail_on_call = config_->fail_on_call;
    s_test_config_camera_rslt_convert_choco_string.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_rslt_convert_linear_alloc_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_rslt_convert_linear_alloc.fail_on_call = config_->fail_on_call;
    s_test_config_camera_rslt_convert_linear_alloc.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_err_utils_config_reset(void) {
    test_call_control_reset(&s_test_config_camera_rslt_convert_choco_memory);
    test_call_control_reset(&s_test_config_camera_rslt_convert_choco_string);
    test_call_control_reset(&s_test_config_camera_rslt_convert_linear_alloc);
}

void NO_COVERAGE test_camera_err_utils(void) {
    test_camera_rslt_to_str();
    test_camera_rslt_convert_choco_memory();
    test_camera_rslt_convert_choco_string();
    test_camera_rslt_convert_linear_alloc();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_camera_rslt_to_str(void) {
    const char* actual = NULL;

    actual = camera_rslt_to_str(CAMERA_SUCCESS);
    assert(actual == s_rslt_str_success);

    actual = camera_rslt_to_str(CAMERA_INVALID_ARGUMENT);
    assert(actual == s_rslt_str_invalid_argument);

    actual = camera_rslt_to_str(CAMERA_RUNTIME_ERROR);
    assert(actual == s_rslt_str_runtime_error);

    actual = camera_rslt_to_str(CAMERA_NO_MEMORY);
    assert(actual == s_rslt_str_no_memory);

    actual = camera_rslt_to_str(CAMERA_LIMIT_EXCEEDED);
    assert(actual == s_rslt_str_limit_exceeded);

    actual = camera_rslt_to_str(CAMERA_BAD_OPERATION);
    assert(actual == s_rslt_str_bad_operation);

    actual = camera_rslt_to_str(CAMERA_DATA_CORRUPTED);
    assert(actual == s_rslt_str_data_corrupted);

    actual = camera_rslt_to_str(CAMERA_UNDEFINED_ERROR);
    assert(actual == s_rslt_str_undefined_error);

    /* 未定義の実行結果コードは UNDEFINED_ERROR にフォールバックすること */
    actual = camera_rslt_to_str((camera_result_t)(-1));
    assert(actual == s_rslt_str_undefined_error);

    actual = camera_rslt_to_str((camera_result_t)999);
    assert(actual == s_rslt_str_undefined_error);
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_camera_rslt_convert_choco_memory(void) {
    camera_result_t actual = CAMERA_UNDEFINED_ERROR;
    test_call_control_t config = { 0 };

    test_camera_err_utils_config_reset();

    /* 通常の変換結果確認 */
    actual = camera_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS);
    assert(actual == CAMERA_SUCCESS);

    actual = camera_rslt_convert_choco_memory(MEMORY_SYSTEM_INVALID_ARGUMENT);
    assert(actual == CAMERA_INVALID_ARGUMENT);

    actual = camera_rslt_convert_choco_memory(MEMORY_SYSTEM_RUNTIME_ERROR);
    assert(actual == CAMERA_RUNTIME_ERROR);

    actual = camera_rslt_convert_choco_memory(MEMORY_SYSTEM_LIMIT_EXCEEDED);
    assert(actual == CAMERA_LIMIT_EXCEEDED);

    actual = camera_rslt_convert_choco_memory(MEMORY_SYSTEM_NO_MEMORY);
    assert(actual == CAMERA_NO_MEMORY);

    /* 未定義の入力は UNDEFINED_ERROR にフォールバックすること */
    actual = camera_rslt_convert_choco_memory((memory_system_result_t)(-1));
    assert(actual == CAMERA_UNDEFINED_ERROR);

    actual = camera_rslt_convert_choco_memory((memory_system_result_t)999);
    assert(actual == CAMERA_UNDEFINED_ERROR);

    /* 1回目呼び出しでの失敗注入確認 */
    test_camera_err_utils_config_reset();
    test_call_control_reset(&config);

    config.fail_on_call = 1;
    config.forced_result = (int)CAMERA_BAD_OPERATION;
    test_camera_rslt_convert_choco_memory_config_set(&config);

    actual = camera_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS);
    assert(actual == CAMERA_BAD_OPERATION);

    /* 2回目呼び出しでの失敗注入確認 */
    test_camera_err_utils_config_reset();
    test_call_control_reset(&config);

    config.fail_on_call = 2;
    config.forced_result = (int)CAMERA_DATA_CORRUPTED;
    test_camera_rslt_convert_choco_memory_config_set(&config);

    actual = camera_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS);
    assert(actual == CAMERA_SUCCESS);

    actual = camera_rslt_convert_choco_memory(MEMORY_SYSTEM_SUCCESS);
    assert(actual == CAMERA_DATA_CORRUPTED);

    test_camera_err_utils_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_camera_rslt_convert_choco_string(void) {
    camera_result_t actual = CAMERA_UNDEFINED_ERROR;
    test_call_control_t config = { 0 };

    test_camera_err_utils_config_reset();

    /* 通常の変換結果確認 */
    actual = camera_rslt_convert_choco_string(CHOCO_STRING_SUCCESS);
    assert(actual == CAMERA_SUCCESS);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_DATA_CORRUPTED);
    assert(actual == CAMERA_DATA_CORRUPTED);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_BAD_OPERATION);
    assert(actual == CAMERA_BAD_OPERATION);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_NO_MEMORY);
    assert(actual == CAMERA_NO_MEMORY);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_INVALID_ARGUMENT);
    assert(actual == CAMERA_INVALID_ARGUMENT);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_RUNTIME_ERROR);
    assert(actual == CAMERA_RUNTIME_ERROR);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_UNDEFINED_ERROR);
    assert(actual == CAMERA_UNDEFINED_ERROR);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_OVERFLOW);
    assert(actual == CAMERA_RUNTIME_ERROR);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_LIMIT_EXCEEDED);
    assert(actual == CAMERA_LIMIT_EXCEEDED);

    /* 未定義の入力は UNDEFINED_ERROR にフォールバックすること */
    actual = camera_rslt_convert_choco_string((choco_string_result_t)(-1));
    assert(actual == CAMERA_UNDEFINED_ERROR);

    actual = camera_rslt_convert_choco_string((choco_string_result_t)999);
    assert(actual == CAMERA_UNDEFINED_ERROR);

    /* 1回目呼び出しでの失敗注入確認 */
    test_camera_err_utils_config_reset();
    test_call_control_reset(&config);

    config.fail_on_call = 1;
    config.forced_result = (int)CAMERA_BAD_OPERATION;
    test_camera_rslt_convert_choco_string_config_set(&config);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_SUCCESS);
    assert(actual == CAMERA_BAD_OPERATION);

    /* 2回目呼び出しでの失敗注入確認 */
    test_camera_err_utils_config_reset();
    test_call_control_reset(&config);

    config.fail_on_call = 2;
    config.forced_result = (int)CAMERA_DATA_CORRUPTED;
    test_camera_rslt_convert_choco_string_config_set(&config);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_SUCCESS);
    assert(actual == CAMERA_SUCCESS);

    actual = camera_rslt_convert_choco_string(CHOCO_STRING_SUCCESS);
    assert(actual == CAMERA_DATA_CORRUPTED);

    test_camera_err_utils_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_camera_rslt_convert_linear_alloc(void) {
    camera_result_t actual = CAMERA_UNDEFINED_ERROR;
    test_call_control_t config = { 0 };

    test_camera_err_utils_config_reset();

    /* 通常の変換結果確認 */
    actual = camera_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(actual == CAMERA_SUCCESS);

    actual = camera_rslt_convert_linear_alloc(LINEAR_ALLOC_NO_MEMORY);
    assert(actual == CAMERA_NO_MEMORY);

    actual = camera_rslt_convert_linear_alloc(LINEAR_ALLOC_INVALID_ARGUMENT);
    assert(actual == CAMERA_INVALID_ARGUMENT);

    /* 未定義の入力は UNDEFINED_ERROR にフォールバックすること */
    actual = camera_rslt_convert_linear_alloc((linear_allocator_result_t)(-1));
    assert(actual == CAMERA_UNDEFINED_ERROR);

    actual = camera_rslt_convert_linear_alloc((linear_allocator_result_t)999);
    assert(actual == CAMERA_UNDEFINED_ERROR);

    /* 1回目呼び出しでの失敗注入確認 */
    test_camera_err_utils_config_reset();
    test_call_control_reset(&config);

    config.fail_on_call = 1;
    config.forced_result = (int)CAMERA_BAD_OPERATION;
    test_camera_rslt_convert_linear_alloc_config_set(&config);

    actual = camera_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(actual == CAMERA_BAD_OPERATION);

    /* 2回目呼び出しでの失敗注入確認 */
    test_camera_err_utils_config_reset();
    test_call_control_reset(&config);

    config.fail_on_call = 2;
    config.forced_result = (int)CAMERA_DATA_CORRUPTED;
    test_camera_rslt_convert_linear_alloc_config_set(&config);

    actual = camera_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(actual == CAMERA_SUCCESS);

    actual = camera_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(actual == CAMERA_DATA_CORRUPTED);

    test_camera_err_utils_config_reset();
}
#endif
