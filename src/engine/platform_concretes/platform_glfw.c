/**
 *
 * @file platform_glfw.c
 * @author chocolate-pie24
 * @brief GLFWを使用する際の仮想関数テーブルを取得する処理の実装
 *
 * @version 0.1
 * @date 2025-10-14
 *
 * @todo glfwSetErrorCallback
 * @todo glfwSwapInterval
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/platform_concretes/platform_glfw.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/platform/platform_utils.h"

#include "engine/interfaces/platform_interface.h"

/**
 * @brief GLFWプラットフォーム内部状態管理オブジェクト
 *
 */
struct platform_backend {
    bool initialized_glfw;          /**< GLFW初期済みフラグ */
};

static void platform_glfw_preinit(size_t* memory_requirement_, size_t* alignment_requirement_);
static platform_result_t platform_glfw_init(platform_backend_t* platform_backend_);
static void platform_glfw_destroy(platform_backend_t* platform_backend_);

static const char* const s_rslt_str_success = "SUCCESS";                    /**< プラットフォームAPI実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< プラットフォームAPI実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< プラットフォームAPI実行結果コード(実行時エラー)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< プラットフォームAPI実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< プラットフォームAPI実行結果コード(未定義エラー)に対応する文字列 */
static const char* const s_rslt_str_window_close = "WINDOW_CLOSE";          /**< プラットフォームAPI実行結果コード(ウィンドウクローズ)に対応する文字列 */

static const char* rslt_to_str(platform_result_t rslt_);
// #define TEST_BUILD
#ifdef TEST_BUILD
#include <assert.h>
#include <string.h>

typedef struct test_controller {
    platform_result_t ret;  /**< 強制的にこのエラーコードを返すようにする */
    bool test_enable;       /**< テスト有効 */
} test_contoller_t;

static test_contoller_t s_test_controller;

static void NO_COVERAGE test_rslt_to_str(void);
#endif

/**
 * @brief GLFW用仮想関数テーブル定義
 *
 */
static const platform_vtable_t s_glfw_vtable = {
    .platform_backend_preinit = platform_glfw_preinit,
    .platform_backend_init = platform_glfw_init,
    .platform_backend_destroy = platform_glfw_destroy,
};

const platform_vtable_t* platform_glfw_vtable_get(void) {
    return &s_glfw_vtable;
}

/**
 * @brief プラットフォーム2段階初期化の1段階目でメモリ要件とメモリアライメント要件を取得する
 *
 * @note
 * - memory_requirement_ == NULL または alignment_requirement_ == NULLの場合は何もしない
 *
 * @param[out] memory_requirement_ メモリ要件
 * @param[out] alignment_requirement_ メモリアライメント要件
 */
static void platform_glfw_preinit(size_t* memory_requirement_, size_t* alignment_requirement_) {
    if(NULL == memory_requirement_ || NULL == alignment_requirement_) {
        WARN_MESSAGE("platform_backend_preinit - No-op: 'memory_requirement_' or 'alignment_requirement_' is NULL.");
        goto cleanup;
    }
    *memory_requirement_ = sizeof(platform_backend_t);
    *alignment_requirement_ = alignof(platform_backend_t);
    goto cleanup;
cleanup:
    return;
}

static platform_result_t platform_glfw_init(platform_backend_t* platform_backend_) {
#ifdef TEST_BUILD
    if(s_test_controller.test_enable) {
        return s_test_controller.ret;
    }
#endif

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_init", "platform_backend_")

    platform_backend_->initialized_glfw = false;

    platform_backend_->initialized_glfw = true;

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

static void platform_glfw_destroy(platform_backend_t* platform_backend_) {
    if(NULL == platform_backend_) {
        return;
    }
    platform_backend_->initialized_glfw = false;
}

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param rslt_ 実行結果コード
 * @return const char* 実行結果コードを文字列に変換した値
 */
static const char* rslt_to_str(platform_result_t rslt_) {
    switch(rslt_) {
    case PLATFORM_SUCCESS:
        return s_rslt_str_success;
    case PLATFORM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case PLATFORM_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case PLATFORM_NO_MEMORY:
        return s_rslt_str_no_memory;
    case PLATFORM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case PLATFORM_WINDOW_CLOSE:
        return s_rslt_str_window_close;
    default:
        return s_rslt_str_undefined_error;
    }
}

#ifdef TEST_BUILD

void test_platform_glfw(void) {
    test_rslt_to_str();
}

static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* test = rslt_to_str(PLATFORM_SUCCESS);
        assert(0 == strcmp(test, s_rslt_str_success));
    }
    {
        const char* test = rslt_to_str(PLATFORM_INVALID_ARGUMENT);
        assert(0 == strcmp(test, s_rslt_str_invalid_argument));
    }
    {
        const char* test = rslt_to_str(PLATFORM_RUNTIME_ERROR);
        assert(0 == strcmp(test, s_rslt_str_runtime_error));
    }
    {
        const char* test = rslt_to_str(PLATFORM_NO_MEMORY);
        assert(0 == strcmp(test, s_rslt_str_no_memory));
    }
    {
        const char* test = rslt_to_str(PLATFORM_UNDEFINED_ERROR);
        assert(0 == strcmp(test, s_rslt_str_undefined_error));
    }
    {
        const char* test = rslt_to_str(PLATFORM_WINDOW_CLOSE);
        assert(0 == strcmp(test, s_rslt_str_window_close));
    }
    {
        const char* test = rslt_to_str(100);
        assert(0 == strcmp(test, s_rslt_str_undefined_error));
    }
}

void platform_glfw_result_controller_set(platform_result_t ret_) {
    s_test_controller.ret = ret_;
    s_test_controller.test_enable = true;
}

void platform_glfw_result_controller_reset(void) {
    s_test_controller.ret = PLATFORM_SUCCESS;
    s_test_controller.test_enable = false;
}

#endif
