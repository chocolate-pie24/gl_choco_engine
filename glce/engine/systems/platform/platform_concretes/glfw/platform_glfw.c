// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup platform
 *
 * @file platform_glfw.c
 * @author chocolate-pie24
 * @brief GLFW APIで実装されたプラットフォームシステムAPIの実装
 *
 * @todo glfwSetErrorCallback
 * @todo glfwSwapInterval
 *
 * @date 2025-10-14
 *
 */
#include "engine/systems/platform/platform_concretes/glfw/platform_glfw.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"
#include "engine/containers/ring_queue.h"

#include "engine/systems/platform/core/platform_types.h"
#include "engine/systems/platform/core/platform_err_utils.h"
#include "engine/systems/platform/config/platform_config.h"
#include "engine/systems/platform/vtables/platform_vtable.h"

/**
 * @brief 入力状態格納構造体
 *
 */
typedef struct input_snapshot {
    double cursor_x;    /**< マウス座標x */
    double cursor_y;    /**< マウス座標y */

    int window_width;   /**< ウィンドウ幅 */
    int window_height;  /**< ウィンドウ高さ */

    int framebuffer_width;  /**< フレームバッファサイズ(幅) */
    int framebuffer_height; /**< フレームバッファサイズ(高さ) */

    bool window_should_close;   /**< ウィンドウクローズイベント発生 */
    bool escape_pressed;        /**< エスケープキー押下イベント発生 */

    bool keycode_state[KEY_CODE_MAX];   /**< 各キーコード（keycode_t）ごとの押下状態(true: 押下中 / false: 非押下) */

    bool left_button_pressed;   /**< マウス左ボタン押下フラグ */
    bool right_button_pressed;  /**< マウス右ボタン押下フラグ */
} input_snapshot_t;

/**
 * @brief GLFWプラットフォーム内部状態管理構造体
 *
 */
struct platform_backend {
    choco_string_t* window_label;   /**< ウィンドウラベル */
    GLFWwindow* window;             /**< GLFWウィンドウ構造体インスタンス */

    input_snapshot_t current;       /**< 入力状態のスナップショット(現在値) */
    input_snapshot_t prev;          /**< 入力状態のスナップショット(前回値) */

    ring_queue_t* keyboard_event_queue;
    ring_queue_t* mouse_event_queue;
    ring_queue_t* window_event_queue;
};

static platform_result_t platform_glfw_initialize(const platform_config_t* config_, linear_alloc_t* linear_alloc_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_backend_t** out_platform_backend_);
static void platform_glfw_deinitialize(platform_backend_t* platform_backend_);
static platform_result_t platform_snapshot_collect(platform_backend_t* platform_backend_);
static platform_result_t platform_snapshot_process(platform_backend_t* platform_backend_, void (*window_event_callback)(const window_event_t* event_), void (*keyboard_event_callback)(const keyboard_event_t* event_), void (*mouse_event_callback)(const mouse_event_t* event_));
static platform_result_t platform_glfw_pump_messages(platform_backend_t* platform_backend_, void (*window_event_callback)(const window_event_t* event_), void (*keyboard_event_callback)(const keyboard_event_t* event_), void (*mouse_event_callback)(const mouse_event_t* event_));
static platform_result_t platform_glfw_swap_buffers(platform_backend_t* platform_backend_);
static bool platform_glfw_is_valid(const platform_backend_t* platform_backend_);

static platform_result_t window_create(const char* window_label_, int window_width_, int window_height_, int* out_framebuffer_width_, int* out_framebuffer_height_, GLFWwindow** out_window_);
static int keycode_to_glfw_keycode(keycode_t keycode_);

static void input_snapshot_initialize(input_snapshot_t* snapshot_);

static bool is_valid_shallow(const platform_backend_t* platform_backend_);

/**
 * @brief GLFW用仮想関数テーブル定義
 *
 */
static const platform_vtable_t s_glfw_vtable = {
    .platform_backend_initialize = platform_glfw_initialize,
    .platform_backend_deinitialize = platform_glfw_deinitialize,
    .platform_backend_pump_messages = platform_glfw_pump_messages,
    .platform_backend_swap_buffers = platform_glfw_swap_buffers,
    .platform_backend_is_valid = platform_glfw_is_valid,
};

const platform_vtable_t* platform_glfw_vtable_get(void) {
    return &s_glfw_vtable;
}

static platform_result_t platform_glfw_initialize(const platform_config_t* config_, linear_alloc_t* linear_alloc_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_backend_t** out_platform_backend_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    platform_backend_t* tmp_platform_backend = NULL;
    ring_queue_t* tmp_window_event_queue = NULL;
    ring_queue_t* tmp_keyboard_event_queue = NULL;
    ring_queue_t* tmp_mouse_event_queue = NULL;
    choco_string_t* tmp_window_label = NULL;
    GLFWwindow* tmp_window = NULL;
    int tmp_framebuffer_width = 0;
    int tmp_framebuffer_height = 0;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_initialize", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(linear_alloc_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_initialize", "linear_alloc_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_width_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_initialize", "out_framebuffer_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_height_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_initialize", "out_framebuffer_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_platform_backend_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_initialize", "out_platform_backend_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_platform_backend_, ret, PLATFORM_BAD_OPERATION, platform_rslt_to_str(PLATFORM_BAD_OPERATION), "platform_glfw_initialize", "*out_platform_backend_")

    ret_ring_queue = ring_queue_create(config_->max_window_event_count, sizeof(window_event_t), alignof(window_event_t), &tmp_window_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = platform_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("platform_glfw_initialize(%s) - ring_queue_create failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    ret_ring_queue = ring_queue_create(config_->max_keyboard_event_count, sizeof(keyboard_event_t), alignof(keyboard_event_t), &tmp_keyboard_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = platform_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("platform_glfw_initialize(%s) - ring_queue_create failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    ret_ring_queue = ring_queue_create(config_->max_mouse_event_count, sizeof(mouse_event_t), alignof(mouse_event_t), &tmp_mouse_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = platform_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("platform_glfw_initialize(%s) - ring_queue_create failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    ret_string = choco_string_create_from_c_string(config_->window_label, &tmp_window_label);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = platform_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("platform_glfw_initialize(%s) - choco_string_create_from_c_string failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_allocate(linear_alloc_, sizeof(platform_backend_t), alignof(platform_backend_t), (void**)&tmp_platform_backend);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = platform_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("platform_glfw_initialize(%s) - linear_allocator_allocate failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_platform_backend, 0, sizeof(platform_backend_t));

    if(GL_FALSE == glfwInit()) {
        ret = PLATFORM_RUNTIME_ERROR;
        ERROR_MESSAGE("platform_glfw_initialize(%s) - Failed to initialize glfw.", platform_rslt_to_str(ret));
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

    ret = window_create(config_->window_label, config_->window_width, config_->window_height, &tmp_framebuffer_width, &tmp_framebuffer_height, &tmp_window);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_initialize(%s) - window_create failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    input_snapshot_initialize(&tmp_platform_backend->prev);

    tmp_platform_backend->keyboard_event_queue = tmp_keyboard_event_queue;
    tmp_platform_backend->mouse_event_queue = tmp_mouse_event_queue;
    tmp_platform_backend->window_event_queue = tmp_window_event_queue;
    tmp_keyboard_event_queue = NULL;
    tmp_mouse_event_queue = NULL;
    tmp_window_event_queue = NULL;

    tmp_platform_backend->window_label = tmp_window_label;
    tmp_window_label = NULL;

    tmp_platform_backend->window = tmp_window;
    tmp_window = NULL;

    tmp_platform_backend->prev.window_height = config_->window_height;
    tmp_platform_backend->prev.window_width = config_->window_width;
    tmp_platform_backend->prev.framebuffer_height = tmp_framebuffer_height;
    tmp_platform_backend->prev.framebuffer_width = tmp_framebuffer_width;

    tmp_platform_backend->current = tmp_platform_backend->prev;

    *out_platform_backend_ = tmp_platform_backend;
    *out_framebuffer_height_ = tmp_framebuffer_height;
    *out_framebuffer_width_ = tmp_framebuffer_width;

    tmp_platform_backend = NULL;

    ret = PLATFORM_SUCCESS;

cleanup:
    if(PLATFORM_DATA_CORRUPTED != ret) {
        if(NULL != tmp_platform_backend) {
            platform_glfw_deinitialize(tmp_platform_backend);
        }
        if(NULL != tmp_window_label) {
            choco_string_destroy(&tmp_window_label);
        }
        if(NULL != tmp_keyboard_event_queue) {
            ring_queue_destroy(&tmp_keyboard_event_queue);
        }
        if(NULL != tmp_mouse_event_queue) {
            ring_queue_destroy(&tmp_mouse_event_queue);
        }
        if(NULL != tmp_window_event_queue) {
            ring_queue_destroy(&tmp_window_event_queue);
        }

    }
    return ret;
}

static void platform_glfw_deinitialize(platform_backend_t* platform_backend_) {
    if(NULL == platform_backend_) {
        return;
    }

    if(NULL != platform_backend_->window) {
        glfwDestroyWindow(platform_backend_->window);
        platform_backend_->window = NULL;
    }

    glfwTerminate();

    choco_string_destroy(&platform_backend_->window_label);
    ring_queue_destroy(&platform_backend_->keyboard_event_queue);
    ring_queue_destroy(&platform_backend_->mouse_event_queue);
    ring_queue_destroy(&platform_backend_->window_event_queue);
    platform_backend_->window = NULL;

    input_snapshot_initialize(&platform_backend_->prev);
    input_snapshot_initialize(&platform_backend_->current);
}

static platform_result_t platform_snapshot_collect(platform_backend_t* platform_backend_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    int left_button_state = 0;
    int right_button_state = 0;

    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_snapshot_collect", "platform_backend_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_snapshot_collect", "platform_backend_->window")

    // window events.
    platform_backend_->current.window_should_close = (0 != glfwWindowShouldClose(platform_backend_->window)) ? true : false;

    glfwGetWindowSize(platform_backend_->window, &platform_backend_->current.window_width, &platform_backend_->current.window_height);
    glfwGetFramebufferSize(platform_backend_->window, &platform_backend_->current.framebuffer_width, &platform_backend_->current.framebuffer_height);

    // keyboard events.
    platform_backend_->current.escape_pressed = (GLFW_PRESS == glfwGetKey(platform_backend_->window, GLFW_KEY_ESCAPE)) ? true : false;
    for(int i = KEY_1; i != KEY_CODE_MAX; ++i) {
        const int glfw_key = keycode_to_glfw_keycode((keycode_t)(i));
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

    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_snapshot_process", "platform_backend_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_snapshot_process", "platform_backend_->window")
    IF_ARG_NULL_GOTO_CLEANUP(window_event_callback, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_snapshot_process", "window_event_callback")
    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_callback, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_snapshot_process", "keyboard_event_callback")
    IF_ARG_NULL_GOTO_CLEANUP(mouse_event_callback, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_snapshot_process", "mouse_event_callback")

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
        window_event.event_args.window_height = platform_backend_->current.window_height;
        window_event.event_args.window_width = platform_backend_->current.window_width;
        window_event.event_args.framebuffer_height = platform_backend_->current.framebuffer_height;
        window_event.event_args.framebuffer_width = platform_backend_->current.framebuffer_width;

        window_event_callback(&window_event);
    }

    // keyboard events.
    for(int i = KEY_1; i != KEY_CODE_MAX; ++i) {
        if(platform_backend_->prev.keycode_state[i] != platform_backend_->current.keycode_state[i]) {
            keyboard_event_t key_event;
            key_event.key = (keycode_t)i;
            key_event.event_args.pressed = platform_backend_->current.keycode_state[i];
            keyboard_event_callback(&key_event);
        }
    }

    // mouse events.
    if(platform_backend_->prev.left_button_pressed != platform_backend_->current.left_button_pressed) {
        mouse_event_t mouse_event;
        mouse_event.button = MOUSE_BUTTON_LEFT;
        mouse_event.event_args.pressed = platform_backend_->current.left_button_pressed;
        mouse_event.event_args.x = (int)platform_backend_->current.cursor_x;
        mouse_event.event_args.y = (int)platform_backend_->current.cursor_y;
        mouse_event_callback(&mouse_event);
    }
    if(platform_backend_->prev.right_button_pressed != platform_backend_->current.right_button_pressed) {
        mouse_event_t mouse_event;
        mouse_event.button = MOUSE_BUTTON_RIGHT;
        mouse_event.event_args.pressed = platform_backend_->current.right_button_pressed;
        mouse_event.event_args.x = (int)platform_backend_->current.cursor_x;
        mouse_event.event_args.y = (int)platform_backend_->current.cursor_y;
        mouse_event_callback(&mouse_event);
    }

    platform_backend_->prev = platform_backend_->current;

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

static platform_result_t platform_glfw_pump_messages(
    platform_backend_t* platform_backend_,
    void (*window_event_callback)(const window_event_t* event_),
    void (*keyboard_event_callback)(const keyboard_event_t* event_),
    void (*mouse_event_callback)(const mouse_event_t* event_)) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_pump_messages", "platform_backend_")
    IF_ARG_NULL_GOTO_CLEANUP(window_event_callback, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_pump_messages", "window_event_callback")
    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_callback, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_pump_messages", "keyboard_event_callback")
    IF_ARG_NULL_GOTO_CLEANUP(mouse_event_callback, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_pump_messages", "mouse_event_callback")
    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_pump_messages", "platform_backend_->window")

    // イベントの取得
    glfwPollEvents();

    ret = platform_snapshot_collect(platform_backend_);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_pump_messages(%s) - Failed to collect snapshot.", platform_rslt_to_str(ret));
        goto cleanup;
    }
    ret = platform_snapshot_process(platform_backend_, window_event_callback, keyboard_event_callback, mouse_event_callback);
    if(PLATFORM_WINDOW_CLOSE == ret) {
        goto cleanup;
    }
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_pump_messages(%s) - Failed to process snapshot.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

static platform_result_t platform_glfw_swap_buffers(platform_backend_t* platform_backend_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_glfw_swap_buffers", "platform_backend_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, ret, PLATFORM_BAD_OPERATION, platform_rslt_to_str(PLATFORM_BAD_OPERATION), "platform_glfw_swap_buffers", "platform_backend_->window")

    glfwSwapBuffers(platform_backend_->window);

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

static bool platform_glfw_is_valid(const platform_backend_t* platform_backend_) {
    if(NULL == platform_backend_) {
        return false;
    }
    if(!is_valid_shallow(platform_backend_)) {
        return false;
    }
    if(!ring_queue_is_valid(platform_backend_->keyboard_event_queue)) {
        return false;
    }
    if(!ring_queue_is_valid(platform_backend_->mouse_event_queue)) {
        return false;
    }
    if(!ring_queue_is_valid(platform_backend_->window_event_queue)) {
        return false;
    }
    if(!choco_string_is_valid(platform_backend_->window_label)) {
        return false;
    }
    return true;
}

static platform_result_t window_create(const char* window_label_, int window_width_, int window_height_, int* out_framebuffer_width_, int* out_framebuffer_height_, GLFWwindow** out_window_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    GLFWwindow* tmp_window = NULL;
    int tmp_framebuffer_width = 0;
    int tmp_framebuffer_height = 0;

    IF_ARG_NULL_GOTO_CLEANUP(window_label_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "window_create", "window_label_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_width_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "window_create", "out_framebuffer_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_height_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "window_create", "out_framebuffer_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_window_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "window_create", "out_window_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_window_, ret, PLATFORM_BAD_OPERATION, platform_rslt_to_str(PLATFORM_BAD_OPERATION), "window_create", "*out_window_")
    if(0 == window_width_) {
        ret = PLATFORM_INVALID_ARGUMENT;
        ERROR_MESSAGE("window_create(%s) - Provided window_width_ is not valid.", platform_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == window_height_) {
        ret = PLATFORM_INVALID_ARGUMENT;
        ERROR_MESSAGE("window_create(%s) - Provided window_height_ is not valid.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_window = glfwCreateWindow(window_width_, window_height_, window_label_, NULL, NULL);   // 第四引数でフルスクリーン化, 第五引数で他のウィンドウとリソース共有
    if(NULL == tmp_window) {
        ret = PLATFORM_RUNTIME_ERROR;
        ERROR_MESSAGE("window_create(%s) - glfwCreateWindow failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    // 引数windowに指定したハンドルのウィンドウのレンダリングコンテキストをカレント(処理対象)にする。
    // レンダリングコンテキストは描画に用いられる情報で、ウィンドウごとに保持される。
    // 図形の描画はこれをカレントに設定したウィンドウに対して行われる。
    glfwMakeContextCurrent(tmp_window);
    glewExperimental = true;
    if(GLEW_OK != glewInit()) {
        ret = PLATFORM_RUNTIME_ERROR;
        ERROR_MESSAGE("window_create(%s) - glewInit failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    // https://www.glfw.org/docs/latest/group__input.html#gaa92336e173da9c8834558b54ee80563b
    glfwSetInputMode(tmp_window, GLFW_STICKY_KEYS, GLFW_TRUE);  // これでエスケープキーが押されるのを捉えるのを保証する

    glfwGetFramebufferSize(tmp_window, &tmp_framebuffer_width, &tmp_framebuffer_height);

    *out_framebuffer_height_ = tmp_framebuffer_height;
    *out_framebuffer_width_ = tmp_framebuffer_width;
    *out_window_ = tmp_window;
    tmp_window = NULL;

    ret = PLATFORM_SUCCESS;

cleanup:
    if(PLATFORM_SUCCESS != ret) {
        if(NULL != tmp_window) {
            glfwDestroyWindow(tmp_window);
            tmp_window = NULL;
        }
    }
    return ret;
}

/**
 * @brief 全プラットフォーム共通で使用するキーコードを対応するGLFWキーコードに変換する
 *
 * @param[in] keycode_ 全プラットフォーム共通キーコード
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
    case KEY_CODE_MAX:
        return GLFW_KEY_LAST;
    default:
        ERROR_MESSAGE("keycode_to_glfw_keycode(%s) - Undefined key code. Returning key '0'", platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT));
        return GLFW_KEY_0;
    }
}

static void input_snapshot_initialize(input_snapshot_t* snapshot_) {
    if(NULL == snapshot_) {
        return;
    }
    snapshot_->left_button_pressed = false;
    snapshot_->right_button_pressed = false;
    snapshot_->cursor_x = 0.0;
    snapshot_->cursor_y = 0.0;
    snapshot_->window_width = 0;
    snapshot_->window_height = 0;
    snapshot_->framebuffer_width = 0;
    snapshot_->framebuffer_height = 0;
    snapshot_->window_should_close = false;
    snapshot_->escape_pressed = false;
    for(size_t i = 0; i != KEY_CODE_MAX; ++i) {
        snapshot_->keycode_state[i] = false;
    }
}

static bool is_valid_shallow(const platform_backend_t* platform_backend_) {
    if(NULL == platform_backend_) {
        return false;
    }
    if(NULL == platform_backend_->window_label) {
        return false;
    }
    if(NULL == platform_backend_->keyboard_event_queue) {
        return false;
    }
    if(NULL == platform_backend_->mouse_event_queue) {
        return false;
    }
    if(NULL == platform_backend_->window_event_queue) {
        return false;
    }
    if(NULL == platform_backend_->window) {
        return false;
    }
    return true;
}
