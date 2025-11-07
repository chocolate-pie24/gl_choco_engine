/** @ingroup platform_glfw
 *
 * @file platform_glfw.c
 * @author chocolate-pie24
 * @brief GLFW APIで実装されたプラットフォームシステムAPIの実装
 *
 * @todo glfwSetErrorCallback
 * @todo glfwSwapInterval
 *
 * @version 0.1
 * @date 2025-10-14
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

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "engine/platform_concretes/platform_glfw.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/platform/platform_utils.h"

#include "engine/interfaces/platform_interface.h"

#include "engine/containers/choco_string.h"

/**
 * @brief GLFWプラットフォーム内部状態管理オブジェクト
 *
 */
struct platform_backend {
    choco_string_t* window_label;   /**< ウィンドウラベル */
    GLFWwindow* window;             /**< GLFWウィンドウオブジェクト */
    bool initialized_glfw;          /**< GLFW初期済みフラグ */
};

static void platform_glfw_preinit(size_t* memory_requirement_, size_t* alignment_requirement_);
static platform_result_t platform_glfw_init(platform_backend_t* platform_backend_);
static void platform_glfw_destroy(platform_backend_t* platform_backend_);
static platform_result_t platform_glfw_window_create(platform_backend_t* platform_backend_, const char* window_label_, int window_width_, int window_height_);

static const char* const s_rslt_str_success = "SUCCESS";                    /**< プラットフォームAPI実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< プラットフォームAPI実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< プラットフォームAPI実行結果コード(実行時エラー)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< プラットフォームAPI実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< プラットフォームAPI実行結果コード(未定義エラー)に対応する文字列 */
static const char* const s_rslt_str_window_close = "WINDOW_CLOSE";          /**< プラットフォームAPI実行結果コード(ウィンドウクローズ)に対応する文字列 */

static const char* rslt_to_str(platform_result_t rslt_);
static platform_result_t rslt_convert_string(choco_string_result_t rslt_);
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
static void NO_COVERAGE test_rslt_convert_string(void);
#endif

/**
 * @brief GLFW用仮想関数テーブル定義
 *
 */
static const platform_vtable_t s_glfw_vtable = {
    .platform_backend_preinit = platform_glfw_preinit,
    .platform_backend_init = platform_glfw_init,
    .platform_backend_destroy = platform_glfw_destroy,
    .platform_backend_window_create = platform_glfw_window_create,
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

    if(GL_FALSE == glfwInit()) {
        ERROR_MESSAGE("platform_glfw_init(%s) - Failed to initialize glfw.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }
    glfwWindowHint(GLFW_SAMPLES, 4);                // 4x アンチエイリアス
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef PLATFORM_MACOS
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);    // MacOS用
#endif
    // OpenGLプロファイル - 関数のパッケージ
    // - GLFW_OPENGL_CORE_PROFILE 最新の機能が全て含まれる
    // - COMPATIBILITY 最新の機能と古い機能の両方が含まれる
    // - 今回は最新の機能のみを使用することにする
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 古いOpenGLは使用しない

    platform_backend_->window = NULL;
    platform_backend_->window_label = NULL;
    platform_backend_->initialized_glfw = true;

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

static void platform_glfw_destroy(platform_backend_t* platform_backend_) {
    if(NULL == platform_backend_) {
        return;
    }
    if(NULL != platform_backend_->window) {
        glfwDestroyWindow(platform_backend_->window);
        platform_backend_->window = NULL;
    }
    if(platform_backend_->initialized_glfw) {
        glfwTerminate();
    }
    choco_string_destroy(&platform_backend_->window_label);
    platform_backend_->window = NULL;
    platform_backend_->initialized_glfw = false;
}

static platform_result_t platform_glfw_window_create(platform_backend_t* platform_backend_, const char* window_label_, int window_width_, int window_height_) {
#ifdef TEST_BUILD
    if(s_test_controller.test_enable) {
        return s_test_controller.ret;
    }
#endif

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "platform_backend_")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_label_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "window_label_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != window_width_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "window_width_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != window_height_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "window_height_")
    if(!platform_backend_->initialized_glfw) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - GLFW has not been initialized.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }
    if(NULL != platform_backend_->window) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - GLFW window has already been created.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }

    ret_string = choco_string_create_from_char(&platform_backend_->window_label, window_label_);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = rslt_convert_string(ret_string);
        ERROR_MESSAGE("platform_glfw_window_create(%s) - Failed to create window title string.", rslt_to_str(ret));
        goto cleanup;
    }

    platform_backend_->window = glfwCreateWindow(window_width_, window_height_, choco_string_c_str(platform_backend_->window_label), NULL, NULL);   // 第四引数でフルスクリーン化, 第五引数で他のウィンドウとリソース共有
    if(NULL == platform_backend_->window) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - Failed to create window.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }

    // 引数windowに指定したハンドルのウィンドウのレンダリングコンテキストをカレント(処理対象)にする。
    // レンダリングコンテキストは描画に用いられる情報で、ウィンドウごとに保持される。
    // 図形の描画はこれをカレントに設定したウィンドウに対して行われる。
    glfwMakeContextCurrent(platform_backend_->window);
    glewExperimental = true;
    if(GLEW_OK != glewInit()) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - Failed to initialize GLEW.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }

    // https://www.glfw.org/docs/latest/group__input.html#gaa92336e173da9c8834558b54ee80563b
    glfwSetInputMode(platform_backend_->window, GLFW_STICKY_KEYS, GLFW_TRUE);  // これでエスケープキーが押されるのを捉えるのを保証する
    ret = PLATFORM_SUCCESS;

cleanup:
    if(PLATFORM_SUCCESS != ret) {
        if(NULL != platform_backend_->window) {
            glfwDestroyWindow(platform_backend_->window);
            platform_backend_->window = NULL;
        }
        choco_string_destroy(&platform_backend_->window_label);
    }
    return ret;
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

/**
 * @brief エラー伝播のため、文字列コンテナオブジェクトの実行結果コードをプラットフォーム実行結果コードに変換する
 *
 * @param rslt_ 文字列コンテナオブジェクト実行結果コード
 * @return platform_result_t プラットフォーム実行結果コード
 */
static platform_result_t rslt_convert_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return PLATFORM_SUCCESS;
    case CHOCO_STRING_NO_MEMORY:
        return PLATFORM_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return PLATFORM_INVALID_ARGUMENT;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return PLATFORM_UNDEFINED_ERROR;
    default:
        return PLATFORM_UNDEFINED_ERROR;
    }
}

#ifdef TEST_BUILD

void test_platform_glfw(void) {
    test_rslt_to_str();
    test_rslt_convert_string();
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

static void NO_COVERAGE test_rslt_convert_string(void) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    ret = rslt_convert_string(CHOCO_STRING_SUCCESS);
    assert(PLATFORM_SUCCESS == ret);

    ret = rslt_convert_string(CHOCO_STRING_NO_MEMORY);
    assert(PLATFORM_NO_MEMORY == ret);

    ret = rslt_convert_string(CHOCO_STRING_INVALID_ARGUMENT);
    assert(PLATFORM_INVALID_ARGUMENT == ret);

    ret = rslt_convert_string(CHOCO_STRING_UNDEFINED_ERROR);
    assert(PLATFORM_UNDEFINED_ERROR == ret);

    ret = rslt_convert_string(100);
    assert(PLATFORM_UNDEFINED_ERROR == ret);
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
