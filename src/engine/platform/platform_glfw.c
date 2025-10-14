/** @addtogroup platform
 * @{
 *
 * @file platform_glfw.c
 * @author chocolate-pie24
 * @brief GLFWを使用する際の仮想関数テーブルを取得する処理の実装
 *
 * @version 0.1
 * @date 2025-10-14
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 * @license MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "engine/platform/platform_glfw.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/platform/platform_utils.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/interfaces/platform_interface.h"

#include "engine/containers/choco_string.h"

/*
TODO: そのうちやる
 - [] glfwSetErrorCallback
 - [] glfwSwapInterval
*/

/**
 * @brief GLFWプラットフォーム内部状態管理オブジェクト
 *
 */
struct platform_state {
    int window_width;                   /**< ウィンドウ幅 */
    int window_height;                  /**< ウィンドウ高さ */
    choco_string_t* window_label;       /**< ウィンドウラベル */
    GLFWwindow* window;                 /**< GLFWウィンドウオブジェクト */
    bool initialized_glfw;              /**< GLFW初期済みフラグ */

    bool keycode_state[KEY_CODE_MAX];   /**< キーボード各キーコード押下状態 */

    bool left_button_state;             /**< マウス左ボタン押下状態 */
    bool right_button_state;            /**< マウス右ボタン押下状態 */
};

static void platform_glfw_preinit(size_t* memory_requirement_, size_t* alignment_requirement_);
static platform_result_t platform_glfw_init(platform_state_t* platform_state_);
static void platform_glfw_destroy(platform_state_t* platform_state_);
static platform_result_t platform_glfw_window_create(platform_state_t* platform_state_, const char* window_label_, int window_width_, int window_height_);
static platform_result_t platform_pump_messages(platform_state_t* platform_state_, void (*window_event_callback)(const window_event_t* event_), void (*keyboard_event_callback)(const keyboard_event_t* event_), void (*mouse_event_callback)(const mouse_event_t* event_));

static int keycode_to_glfw_keycode(keycode_t keycode_);

static const char* const s_rslt_str_success = "SUCCESS";                    /**< プラットフォームAPI実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< プラットフォームAPI実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< プラットフォームAPI実行結果コード(実行時エラー)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< プラットフォームAPI実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< プラットフォームAPI実行結果コード(未定義エラー)に対応する文字列 */
static const char* const s_rslt_str_window_close = "WINDOW_CLOSE";          /**< プラットフォームAPI実行結果コード(ウィンドウクローズ)に対応する文字列 */

static const char* rslt_to_str(platform_result_t rslt_);
static platform_result_t rslt_convert_string(choco_string_result_t rslt_);

/**
 * @brief GLFW用仮想関数テーブル定義
 *
 */
static const platform_vtable_t s_glfw_vtable = {
    .platform_state_preinit = platform_glfw_preinit,
    .platform_state_init = platform_glfw_init,
    .platform_state_destroy = platform_glfw_destroy,
    .platform_window_create = platform_glfw_window_create,
    .platform_pump_messages = platform_pump_messages,
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
        WARN_MESSAGE("platform_state_preinit - No-op: 'memory_requirement_' or 'alignment_requirement_' is NULL.");
        goto cleanup;
    }
    *memory_requirement_ = sizeof(platform_state_t);
    *alignment_requirement_ = alignof(platform_state_t);
    goto cleanup;
cleanup:
    return;
}

static platform_result_t platform_glfw_init(platform_state_t* platform_state_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_state_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_init", "platform_state_")

    platform_state_->initialized_glfw = false;

    if (GL_FALSE == glfwInit()) {
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

    platform_state_->window = NULL;
    platform_state_->window_height = 0;
    platform_state_->window_width = 0;
    platform_state_->window_label = NULL;

    platform_state_->initialized_glfw = true;

    for(size_t i = 0; i != KEY_CODE_MAX; ++i) {
        platform_state_->keycode_state[i] = false;
    }

    platform_state_->left_button_state = false;
    platform_state_->right_button_state = false;

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

static void platform_glfw_destroy(platform_state_t* platform_state_) {
    if(NULL == platform_state_) {
        return;
    }
    if(NULL != platform_state_->window) {
        glfwDestroyWindow(platform_state_->window);
        platform_state_->window = NULL;
    }
    if(platform_state_->initialized_glfw) {
        glfwTerminate();
    }
    choco_string_destroy(&platform_state_->window_label);
    platform_state_->window = NULL;
    platform_state_->window_height = 0;
    platform_state_->window_width = 0;
    platform_state_->initialized_glfw = false;
}

static platform_result_t platform_glfw_window_create(platform_state_t* platform_state_, const char* window_label_, int window_width_, int window_height_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(platform_state_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "platform_state_")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_label_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "window_label_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != window_width_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "window_width_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != window_height_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "window_height_")
    if(!platform_state_->initialized_glfw) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - GLFW has not been initialized.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }
    if(NULL != platform_state_->window) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - GLFW window has already been created.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }

    ret_string = choco_string_create_from_char(&platform_state_->window_label, window_label_);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = rslt_convert_string(ret_string);
        ERROR_MESSAGE("platform_glfw_window_create(%s) - Failed to create window title string.", rslt_to_str(ret));
        goto cleanup;
    }

    platform_state_->window = glfwCreateWindow(window_width_, window_height_, choco_string_c_str(platform_state_->window_label), 0, 0);   // 第四引数でフルスクリーン化, 第五引数で他のウィンドウとリソース共有
    if (NULL == platform_state_->window) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - Failed to create window.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }

    // 引数windowに指定したハンドルのウィンドウのレンダリングコンテキストをカレント(処理対象)にする。
    // レンダリングコンテキストは描画に用いられる情報で、ウィンドウごとに保持される。
    // 図形の描画はこれをカレントに設定したウィンドウに対して行われる。
    glfwMakeContextCurrent(platform_state_->window);
    glewExperimental = true;
    if (GLEW_OK != glewInit()) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - Failed to initialize GLEW.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }

    // https://www.glfw.org/docs/latest/group__input.html#gaa92336e173da9c8834558b54ee80563b
    glfwSetInputMode(platform_state_->window, GLFW_STICKY_KEYS, GLFW_TRUE);  // これでエスケープキーが押されるのを捉えるのを保証する
    platform_state_->window_height = window_height_;
    platform_state_->window_width = window_width_;
    ret = PLATFORM_SUCCESS;

cleanup:
    if(PLATFORM_SUCCESS != ret) {
        if(NULL != platform_state_->window) {
            glfwDestroyWindow(platform_state_->window);
            platform_state_->window = NULL;
        }
        choco_string_destroy(&platform_state_->window_label);
    }
    return ret;
}

static platform_result_t platform_pump_messages(
    platform_state_t* platform_state_,
    void (*window_event_callback)(const window_event_t* event_),
    void (*keyboard_event_callback)(const keyboard_event_t* event_),
    void (*mouse_event_callback)(const mouse_event_t* event_)) {

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    int width = 0;
    int height = 0;
    int button_state = 0;
    bool left_pressed = false;
    bool right_pressed = false;

    if(NULL == platform_state_ || !platform_state_->initialized_glfw) {
        ERROR_MESSAGE("platform_pump_messages(%s) - Argument 'platform_state_' is uninitialized.", s_rslt_str_invalid_argument);
        ret = PLATFORM_INVALID_ARGUMENT;
        goto cleanup;
    }

    // イベントの取得
    glfwPollEvents();

    // window events.
    // window events -> window close
    if(GLFW_PRESS == glfwGetKey(platform_state_->window, GLFW_KEY_ESCAPE) || 0 != glfwWindowShouldClose(platform_state_->window)) {
        ret = PLATFORM_WINDOW_CLOSE;
        goto cleanup;
    }
    // window events -> window resize
    glfwGetWindowSize(platform_state_->window, &width, &height);
    if (width != platform_state_->window_width || height != platform_state_->window_height) {
        platform_state_->window_height = height;
        platform_state_->window_width = width;

        window_event_t window_event;
        window_event.event_code = WINDOW_EVENT_RESIZE;
        window_event.window_height = height;
        window_event.window_width = width;

        window_event_callback(&window_event);
    }

    // keyboard events.
    for (int i = KEY_1; i != KEY_CODE_MAX; ++i) {
        const int glfw_key = keycode_to_glfw_keycode(i);
        const int action = glfwGetKey(platform_state_->window, glfw_key);
        if (GLFW_PRESS == action || GLFW_RELEASE == action) {
            keyboard_event_t key_event;
            key_event.key = (keycode_t)i;
            key_event.pressed = (GLFW_PRESS == action) ? true : false;
            if(platform_state_->keycode_state[i] != key_event.pressed) {
                keyboard_event_callback(&key_event);

                platform_state_->keycode_state[i] = key_event.pressed;
            }
        }
    }

    // mouse event.
    button_state = glfwGetMouseButton(platform_state_->window, GLFW_MOUSE_BUTTON_LEFT);
    left_pressed = (GLFW_PRESS == button_state) ? true : false;
    if(platform_state_->left_button_state != left_pressed) {
        double mouse_x = 0.0;
        double mouse_y = 0.0;
        glfwGetCursorPos(platform_state_->window, &mouse_x, &mouse_y);

        mouse_event_t mouse_event;
        mouse_event.button = MOUSE_BUTTON_LEFT;
        mouse_event.pressed = left_pressed;
        mouse_event.x = (int)mouse_x;
        mouse_event.y = (int)mouse_y;

        mouse_event_callback(&mouse_event);

        platform_state_->left_button_state = left_pressed;
    }

    button_state = glfwGetMouseButton(platform_state_->window, GLFW_MOUSE_BUTTON_RIGHT);
    right_pressed = (GLFW_PRESS == button_state) ? true : false;
    if(platform_state_->right_button_state != right_pressed) {
        double mouse_x = 0.0;
        double mouse_y = 0.0;
        glfwGetCursorPos(platform_state_->window, &mouse_x, &mouse_y);

        mouse_event_t mouse_event;
        mouse_event.button = MOUSE_BUTTON_RIGHT;
        mouse_event.pressed = right_pressed;
        mouse_event.x = (int)mouse_x;
        mouse_event.y = (int)mouse_y;

        mouse_event_callback(&mouse_event);

        platform_state_->right_button_state = right_pressed;
    }

    ret = PLATFORM_SUCCESS;

cleanup:
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
 * @brief 全プラットフォーム共通で使用するキーコードを対応するGLFWキーコードに変換する
 *
 * @param keycode_ 全プラットフォーム共通キーコード
 * @return int GLFWキーコード
 */
static int keycode_to_glfw_keycode(keycode_t keycode_) {
    switch(keycode_) {
    case KEY_1:
        return GLFW_KEY_1;
    case KEY_2:
        return GLFW_KEY_2;
    case KEY_3:
        return GLFW_KEY_3;
    case KEY_4:
        return GLFW_KEY_4;
    case KEY_5:
        return GLFW_KEY_5;
    case KEY_6:
        return GLFW_KEY_6;
    case KEY_7:
        return GLFW_KEY_7;
    case KEY_8:
        return GLFW_KEY_8;
    case KEY_9:
        return GLFW_KEY_9;
    case KEY_0:
        return GLFW_KEY_0;
    case KEY_A:
        return GLFW_KEY_A;
    case KEY_B:
        return GLFW_KEY_B;
    case KEY_C:
        return GLFW_KEY_C;
    case KEY_D:
        return GLFW_KEY_D;
    case KEY_E:
        return GLFW_KEY_E;
    case KEY_F:
        return GLFW_KEY_F;
    case KEY_G:
        return GLFW_KEY_G;
    case KEY_H:
        return GLFW_KEY_H;
    case KEY_I:
        return GLFW_KEY_I;
    case KEY_J:
        return GLFW_KEY_J;
    case KEY_K:
        return GLFW_KEY_K;
    case KEY_L:
        return GLFW_KEY_L;
    case KEY_M:
        return GLFW_KEY_M;
    case KEY_N:
        return GLFW_KEY_N;
    case KEY_O:
        return GLFW_KEY_O;
    case KEY_P:
        return GLFW_KEY_P;
    case KEY_Q:
        return GLFW_KEY_Q;
    case KEY_R:
        return GLFW_KEY_R;
    case KEY_S:
        return GLFW_KEY_S;
    case KEY_T:
        return GLFW_KEY_T;
    case KEY_U:
        return GLFW_KEY_U;
    case KEY_V:
        return GLFW_KEY_V;
    case KEY_W:
        return GLFW_KEY_W;
    case KEY_X:
        return GLFW_KEY_X;
    case KEY_Y:
        return GLFW_KEY_Y;
    case KEY_Z:
        return GLFW_KEY_Z;
    case KEY_RIGHT:
        return GLFW_KEY_RIGHT;
    case KEY_LEFT:
        return GLFW_KEY_LEFT;
    case KEY_UP:
        return GLFW_KEY_UP;
    case KEY_DOWN:
        return GLFW_KEY_DOWN;
    case KEY_SHIFT:
        return GLFW_KEY_LEFT_SHIFT;
    case KEY_SPACE:
        return GLFW_KEY_SPACE;
    case KEY_SEMICOLON:
        return GLFW_KEY_SEMICOLON;
    case KEY_MINUS:
        return GLFW_KEY_MINUS;
    case KEY_F1:
        return GLFW_KEY_F1;
    case KEY_F2:
        return GLFW_KEY_F2;
    case KEY_F3:
        return GLFW_KEY_F3;
    case KEY_F4:
        return GLFW_KEY_F4;
    case KEY_F5:
        return GLFW_KEY_F5;
    case KEY_F6:
        return GLFW_KEY_F6;
    case KEY_F7:
        return GLFW_KEY_F7;
    case KEY_F8:
        return GLFW_KEY_F8;
    case KEY_F9:
        return GLFW_KEY_F9;
    case KEY_F10:
        return GLFW_KEY_F10;
    case KEY_F11:
        return GLFW_KEY_F11;
    case KEY_F12:
        return GLFW_KEY_F12;
    default:
        ERROR_MESSAGE("keycode_to_glfw_keycode(%s) - Undefined key code. Returning key '0'", s_rslt_str_invalid_argument);
        return GLFW_KEY_0;
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
