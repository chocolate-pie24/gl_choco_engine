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

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "engine/platform_concretes/platform_glfw.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/platform/platform_utils.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/interfaces/platform_interface.h"

#include "engine/containers/choco_string.h"

// TODO:
// - [] 一つづつapiを差し替えていく
// - [] テスト関数
// - [] doxygenコメント
/**
 * @brief GLFW系のAPIの実行結果についてテスト可能にするため、モック化可能にする
 *
 */
typedef struct glfw_apis {
    void (*glfw_get_cursor_pos)(GLFWwindow* window_, double* cursor_x_, double* cusor_y_);
} glfw_apis_t;

typedef struct input_snapshot {
    double cursor_x;
    double cursor_y;

    int window_width;
    int window_height;

    bool window_should_close;
    bool escape_pressed;

    bool keycode_state[KEY_CODE_MAX];

    bool left_button_pressed;
    bool right_button_pressed;
} input_snapshot_t;

/**
 * @brief GLFWプラットフォーム内部状態管理オブジェクト
 *
 */
struct platform_backend {
    glfw_apis_t apis;
    choco_string_t* window_label;   /**< ウィンドウラベル */
    GLFWwindow* window;             /**< GLFWウィンドウオブジェクト */
    bool initialized_glfw;          /**< GLFW初期済みフラグ */
    input_snapshot_t current;       /**< 入力情報のスナップショット(最新値) */
    input_snapshot_t prev;          /**< 入力情報のスナップショット(前回値) */
};

/**
 * @brief 外部からの返り値制御用構造体
 *
 */
typedef struct test_controller {
    platform_result_t ret;  /**< 強制的にこのエラーコードを返すようにする */
    bool test_enable;       /**< テスト有効 */
} test_contoller_t;

static test_contoller_t s_test_controller;

static void platform_glfw_preinit(size_t* memory_requirement_, size_t* alignment_requirement_);
static platform_result_t platform_glfw_init(platform_backend_t* platform_backend_);
static void platform_glfw_destroy(platform_backend_t* platform_backend_);
static platform_result_t platform_glfw_window_create(platform_backend_t* platform_backend_, const char* window_label_, int window_width_, int window_height_);
static platform_result_t platform_snapshot_collect(platform_backend_t* platform_backend_);
static platform_result_t platform_snapshot_process(platform_backend_t* platform_backend_, void (*window_event_callback)(const window_event_t* event_), void (*keyboard_event_callback)(const keyboard_event_t* event_), void (*mouse_event_callback)(const mouse_event_t* event_));
static platform_result_t platform_pump_messages(platform_backend_t* platform_backend_, void (*window_event_callback)(const window_event_t* event_), void (*keyboard_event_callback)(const keyboard_event_t* event_), void (*mouse_event_callback)(const mouse_event_t* event_));

static int keycode_to_glfw_keycode(keycode_t keycode_);

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

static void NO_COVERAGE test_rslt_to_str(void);
static void NO_COVERAGE test_keycode_to_glfw_keycode(void);
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
    platform_backend_->prev.window_height = 0;
    platform_backend_->prev.window_width = 0;
    platform_backend_->current.window_height = 0;
    platform_backend_->current.window_width = 0;
    platform_backend_->window_label = NULL;

    platform_backend_->initialized_glfw = true;

    for(size_t i = 0; i != KEY_CODE_MAX; ++i) {
        platform_backend_->prev.keycode_state[i] = false;
        platform_backend_->current.keycode_state[i] = false;
    }

    platform_backend_->prev.left_button_pressed = false;
    platform_backend_->prev.right_button_pressed = false;
    platform_backend_->current.left_button_pressed = false;
    platform_backend_->current.right_button_pressed = false;

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
    platform_backend_->prev.window_height = 0;
    platform_backend_->prev.window_width = 0;
    platform_backend_->current.window_height = 0;
    platform_backend_->current.window_width = 0;
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

    platform_backend_->window = glfwCreateWindow(window_width_, window_height_, choco_string_c_str(platform_backend_->window_label), 0, 0);   // 第四引数でフルスクリーン化, 第五引数で他のウィンドウとリソース共有
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
    platform_backend_->prev.window_height = window_height_;
    platform_backend_->prev.window_width = window_width_;
    platform_backend_->current.window_height = window_height_;
    platform_backend_->current.window_width = window_width_;
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

static void NO_COVERAGE test_platform_pump_messages(void) {
    // TODO: ここでウィンドウを生成する、最後にウィンドウをdestroyする
    {
        DEBUG_MESSAGE("Test rubbish code.");
        // NOTE: WINDOW_RESIZEのところにブレークを貼り、画面の幅だけを変えてテスト
    }
    {
        DEBUG_MESSAGE("Test rubbish code.");
        // NOTE: WINDOW_RESIZEのところにブレークを貼り、画面の高さだけを変えてテスト
    }
    {
        DEBUG_MESSAGE("Test rubbish code.");
        // NOTE: WINDOW_RESIZEのところにブレークを貼り、画面の幅と高さ同時に変えてテスト
    }
    {
        DEBUG_MESSAGE("Test rubbish code.");
        // NOTE: マウス右クリックでイベント発生テスト
    }
    {
        DEBUG_MESSAGE("Test rubbish code.");
        // NOTE: 上の後でリリースイベント発生テスト
    }
    {
        DEBUG_MESSAGE("Test rubbish code.");
        // NOTE: マウス左クリックでイベント発生テスト
    }
    {
        DEBUG_MESSAGE("Test rubbish code.");
        // NOTE: 上の後でリリースイベント発生テスト
    }
    {
        DEBUG_MESSAGE("Test rubbish code.");
        // NOTE: キーボードをしてイベント発生テスト
    }
    {
        DEBUG_MESSAGE("Test rubbish code.");
        // NOTE: 上の後でリリースイベント発生テスト
    }
}

static platform_result_t platform_snapshot_collect(platform_backend_t* platform_backend_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    int left_button_state = 0;
    int right_button_state = 0;

    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_collect", "platform_backend_")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_collect", "platform_backend_->window")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(platform_backend_->initialized_glfw, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_collect", "platform_backend_->initialized_glfw")

    platform_backend_->current.escape_pressed = (GLFW_PRESS == glfwGetKey(platform_backend_->window, GLFW_KEY_ESCAPE)) ? true : false;
    platform_backend_->current.window_should_close = (0 != glfwWindowShouldClose(platform_backend_->window)) ? true : false;

    glfwGetWindowSize(platform_backend_->window, &platform_backend_->current.window_width, &platform_backend_->current.window_height);

    // keyboard events.
    for(int i = KEY_1; i != KEY_CODE_MAX; ++i) {
        const int glfw_key = keycode_to_glfw_keycode(i);
        const int action = glfwGetKey(platform_backend_->window, glfw_key);
        platform_backend_->current.keycode_state[i] = (GLFW_PRESS == action) ? true : false;
    }

    // mouse event.
    glfwGetCursorPos(platform_backend_->window, &platform_backend_->current.cursor_x, &platform_backend_->current.cursor_y);

    left_button_state = glfwGetMouseButton(platform_backend_->window, GLFW_MOUSE_BUTTON_LEFT);
    platform_backend_->current.left_button_pressed = (GLFW_PRESS == left_button_state) ? true : false;

    right_button_state = glfwGetMouseButton(platform_backend_->window, GLFW_MOUSE_BUTTON_RIGHT);
    platform_backend_->current.right_button_pressed = (GLFW_PRESS == right_button_state) ? true : false;

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

static platform_result_t platform_snapshot_process(
    platform_backend_t* platform_backend_,
    void (*window_event_callback)(const window_event_t* event_),
    void (*keyboard_event_callback)(const keyboard_event_t* event_),
    void (*mouse_event_callback)(const mouse_event_t* event_)) {

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "window_event_callback")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "platform_backend_->window")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(platform_backend_->initialized_glfw, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "platform_backend_->initialized_glfw")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "window_event_callback")
    CHECK_ARG_NULL_GOTO_CLEANUP(keyboard_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "keyboard_event_callback")
    CHECK_ARG_NULL_GOTO_CLEANUP(mouse_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "mouse_event_callback")

    if(platform_backend_->current.escape_pressed) {
        ret = PLATFORM_WINDOW_CLOSE;
        platform_backend_->prev = platform_backend_->current;
        goto cleanup;
    }
    if(platform_backend_->current.window_should_close) {
        ret = PLATFORM_WINDOW_CLOSE;
        platform_backend_->prev = platform_backend_->current;
        goto cleanup;
    }

    // window event
    if(platform_backend_->current.window_width != platform_backend_->prev.window_width || platform_backend_->current.window_height != platform_backend_->prev.window_height) {
        window_event_t window_event;
        window_event.event_code = WINDOW_EVENT_RESIZE;
        window_event.window_height = platform_backend_->current.window_height;
        window_event.window_width = platform_backend_->current.window_width;

        window_event_callback(&window_event);
    }

    // keyboard events.
    for(int i = KEY_1; i != KEY_CODE_MAX; ++i) {
        if(platform_backend_->prev.keycode_state[i] != platform_backend_->current.keycode_state[i]) {
            keyboard_event_t key_event;
            key_event.key = (keycode_t)i;
            key_event.pressed = platform_backend_->current.keycode_state[i];
            keyboard_event_callback(&key_event);
        }
    }

    // mouse events.
    if(platform_backend_->prev.left_button_pressed != platform_backend_->current.left_button_pressed) {
        mouse_event_t mouse_event;
        mouse_event.button = MOUSE_BUTTON_LEFT;
        mouse_event.pressed = platform_backend_->current.left_button_pressed;
        mouse_event.x = (int)platform_backend_->current.cursor_x;
        mouse_event.y = (int)platform_backend_->current.cursor_y;
        mouse_event_callback(&mouse_event);
    }
    if(platform_backend_->prev.right_button_pressed != platform_backend_->current.right_button_pressed) {
        mouse_event_t mouse_event;
        mouse_event.button = MOUSE_BUTTON_RIGHT;
        mouse_event.pressed = platform_backend_->current.right_button_pressed;
        mouse_event.x = (int)platform_backend_->current.cursor_x;
        mouse_event.y = (int)platform_backend_->current.cursor_y;
        mouse_event_callback(&mouse_event);
    }

    platform_backend_->prev = platform_backend_->current;
    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

static platform_result_t platform_pump_messages(
    platform_backend_t* platform_backend_,
    void (*window_event_callback)(const window_event_t* event_),
    void (*keyboard_event_callback)(const keyboard_event_t* event_),
    void (*mouse_event_callback)(const mouse_event_t* event_)) {

#ifdef TEST_BUILD
    if(s_test_controller.test_enable) {
        return s_test_controller.ret;
    }
#endif

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "platform_backend_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(platform_backend_->initialized_glfw, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "platform_backend_->initialized_glfw")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "window_event_callback")
    CHECK_ARG_NULL_GOTO_CLEANUP(keyboard_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "keyboard_event_callback")
    CHECK_ARG_NULL_GOTO_CLEANUP(mouse_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "mouse_event_callback")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "platform_backend_->window")

    // イベントの取得
    glfwPollEvents();

    ret = platform_snapshot_collect(platform_backend_);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_snapshot_collect(%s) - Failed to correct snapshot.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = platform_snapshot_process(platform_backend_, window_event_callback, keyboard_event_callback, mouse_event_callback);
    if(PLATFORM_WINDOW_CLOSE == ret) {
        goto cleanup;
    }
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_snapshot_collect(%s) - Failed to process snapshot.", rslt_to_str(ret));
        goto cleanup;
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
    case KEY_LEFT_SHIFT:
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

#ifdef TEST_BUILD

void test_platform_glfw(void) {
    test_rslt_to_str();
    test_keycode_to_glfw_keycode();
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

static void NO_COVERAGE test_keycode_to_glfw_keycode(void) {
    int glfw_key = GLFW_KEY_1;

    glfw_key = keycode_to_glfw_keycode(KEY_1);
    assert(GLFW_KEY_1 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_2);
    assert(GLFW_KEY_2 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_3);
    assert(GLFW_KEY_3 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_4);
    assert(GLFW_KEY_4 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_5);
    assert(GLFW_KEY_5 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_6);
    assert(GLFW_KEY_6 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_7);
    assert(GLFW_KEY_7 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_8);
    assert(GLFW_KEY_8 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_9);
    assert(GLFW_KEY_9 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_0);
    assert(GLFW_KEY_0 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_A);
    assert(GLFW_KEY_A == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_B);
    assert(GLFW_KEY_B == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_C);
    assert(GLFW_KEY_C == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_D);
    assert(GLFW_KEY_D == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_E);
    assert(GLFW_KEY_E == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F);
    assert(GLFW_KEY_F == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_G);
    assert(GLFW_KEY_G == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_H);
    assert(GLFW_KEY_H == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_I);
    assert(GLFW_KEY_I == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_J);
    assert(GLFW_KEY_J == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_K);
    assert(GLFW_KEY_K == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_L);
    assert(GLFW_KEY_L == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_M);
    assert(GLFW_KEY_M == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_N);
    assert(GLFW_KEY_N == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_O);
    assert(GLFW_KEY_O == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_P);
    assert(GLFW_KEY_P == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_Q);
    assert(GLFW_KEY_Q == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_R);
    assert(GLFW_KEY_R == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_S);
    assert(GLFW_KEY_S == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_T);
    assert(GLFW_KEY_T == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_U);
    assert(GLFW_KEY_U == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_V);
    assert(GLFW_KEY_V == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_W);
    assert(GLFW_KEY_W == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_X);
    assert(GLFW_KEY_X == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_Y);
    assert(GLFW_KEY_Y == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_Z);
    assert(GLFW_KEY_Z == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_RIGHT);
    assert(GLFW_KEY_RIGHT == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_LEFT);
    assert(GLFW_KEY_LEFT == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_UP);
    assert(GLFW_KEY_UP == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_DOWN);
    assert(GLFW_KEY_DOWN == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_LEFT_SHIFT);
    assert(GLFW_KEY_LEFT_SHIFT == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_SPACE);
    assert(GLFW_KEY_SPACE == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_SEMICOLON);
    assert(GLFW_KEY_SEMICOLON == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_MINUS);
    assert(GLFW_KEY_MINUS == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F1);
    assert(GLFW_KEY_F1 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F2);
    assert(GLFW_KEY_F2 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F3);
    assert(GLFW_KEY_F3 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F4);
    assert(GLFW_KEY_F4 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F5);
    assert(GLFW_KEY_F5 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F6);
    assert(GLFW_KEY_F6 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F7);
    assert(GLFW_KEY_F7 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F8);
    assert(GLFW_KEY_F8 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F9);
    assert(GLFW_KEY_F9 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F10);
    assert(GLFW_KEY_F10 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F11);
    assert(GLFW_KEY_F11 == glfw_key);

    glfw_key = keycode_to_glfw_keycode(KEY_F12);
    assert(GLFW_KEY_F12 == glfw_key);

    // エラーメッセージ確認
    glfw_key = keycode_to_glfw_keycode(1000);
    assert(GLFW_KEY_0 == glfw_key);
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
