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
#include "engine/systems/platform_system/platform_concretes/glfw/platform_glfw.h"

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

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/platform_system/core/platform_system_types.h"
#include "engine/systems/platform_system/core/platform_system_err_utils.h"
#include "engine/systems/platform_system/core/platform_event_view.h"
#include "engine/systems/platform_system/config/platform_system_config.h"
#include "engine/systems/platform_system/vtables/platform_backend_vtable.h"

/**
 * @brief 入力状態格納構造体
 *
 */
typedef struct platform_glfw_snapshot {
    double cursor_x;    /**< マウス座標x */
    double cursor_y;    /**< マウス座標y */

    int window_width;   /**< ウィンドウ幅 */
    int window_height;  /**< ウィンドウ高さ */

    int framebuffer_width;  /**< フレームバッファサイズ(幅) */
    int framebuffer_height; /**< フレームバッファサイズ(高さ) */

    bool window_should_close;   /**< ウィンドウクローズイベント発生 */

    bool keycode_state[KEY_CODE_MAX];   /**< 各キーコード（keycode_t）ごとの押下状態(true: 押下中 / false: 非押下) */

    bool left_button_pressed;   /**< マウス左ボタン押下フラグ */
    bool right_button_pressed;  /**< マウス右ボタン押下フラグ */
} platform_glfw_snapshot_t;

typedef struct window_event_storage {
    size_t max_event_count;
    size_t current_event_count;
    window_event_t* event_storage;
} window_event_storage_t;

typedef struct keyboard_event_storage {
    size_t max_event_count;
    size_t current_event_count;
    keyboard_event_t* event_storage;
} keyboard_event_storage_t;

typedef struct mouse_event_storage {
    size_t max_event_count;
    size_t current_event_count;
    mouse_event_t* event_storage;
} mouse_event_storage_t;

/**
 * @brief GLFWプラットフォーム内部状態管理構造体
 *
 */
struct platform_backend {
    choco_string_t* window_label;   /**< ウィンドウラベル */
    GLFWwindow* window;             /**< GLFWウィンドウ構造体インスタンス */

    platform_glfw_snapshot_t current;
    platform_glfw_snapshot_t prev;

    window_event_storage_t window_event_storage;
    keyboard_event_storage_t keyboard_event_storage;
    mouse_event_storage_t mouse_event_storage;

    platform_event_view_t event_view;
};

// Backend entry points / vtable implementation
static platform_system_result_t platform_glfw_create(const platform_system_config_t* config_, linear_alloc_t* linear_alloc_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_backend_t** out_platform_backend_);
static void platform_glfw_deinitialize(platform_backend_t* platform_backend_);
static platform_system_result_t platform_glfw_update(platform_backend_t* platform_backend_, const platform_event_view_t** out_event_view_);
static platform_system_result_t platform_glfw_swap_buffers(platform_backend_t* platform_backend_);
static bool platform_glfw_is_valid(const platform_backend_t* platform_backend_);

// Initialization / lifecycle helpers
static platform_system_result_t glfw_runtime_initialize(void);
static platform_system_result_t glfw_window_create(const char* window_label_, int window_width_, int window_height_, int* out_framebuffer_width_, int* out_framebuffer_height_, GLFWwindow** out_window_);
static platform_system_result_t event_storage_initialize(const platform_system_config_t* config_, linear_alloc_t* linear_alloc_, window_event_storage_t* window_event_storage_, keyboard_event_storage_t* keyboard_event_storage_, mouse_event_storage_t* mouse_event_storage_);
static void snapshot_reset(platform_glfw_snapshot_t* snapshot_);

// Per-frame update helpers
static platform_system_result_t snapshot_collect(platform_backend_t* platform_backend_);
static void event_storage_counts_reset(window_event_storage_t* window_event_storage_, keyboard_event_storage_t* keyboard_event_storage_, mouse_event_storage_t* mouse_event_storage_);
static platform_system_result_t event_count(const platform_glfw_snapshot_t* prev_, const platform_glfw_snapshot_t* current_, size_t* out_window_event_count_, size_t* out_keyboard_event_count_, size_t* out_mouse_event_count_);
static platform_system_result_t window_events_generate(const platform_glfw_snapshot_t* prev_, const platform_glfw_snapshot_t* current_, window_event_storage_t* event_storage_);
static platform_system_result_t keyboard_events_generate(const platform_glfw_snapshot_t* prev_, const platform_glfw_snapshot_t* current_, keyboard_event_storage_t* event_storage_);
static platform_system_result_t mouse_events_generate(const platform_glfw_snapshot_t* prev_, const platform_glfw_snapshot_t* current_, mouse_event_storage_t* event_storage_);
static platform_system_result_t event_view_refresh(bool window_close_requested_, const window_event_storage_t* window_event_storage_, const keyboard_event_storage_t* keyboard_event_storage_, const mouse_event_storage_t* mouse_event_storage_, platform_event_view_t* event_view_);

// Event storage primitive operations
static platform_system_result_t window_event_storage_push(window_event_storage_t* event_storage_, const window_event_t* event_);
static platform_system_result_t keyboard_event_storage_push(keyboard_event_storage_t* event_storage_, const keyboard_event_t* event_);
static platform_system_result_t mouse_event_storage_push(mouse_event_storage_t* event_storage_, const mouse_event_t* event_);

// GLFW conversion / utility
static int keycode_to_glfw_keycode(keycode_t keycode_);

// Validators
static bool window_event_storage_is_valid(const window_event_storage_t* event_storage_);
static bool keyboard_event_storage_is_valid(const keyboard_event_storage_t* event_storage_);
static bool mouse_event_storage_is_valid(const mouse_event_storage_t* event_storage_);
static bool is_valid_shallow(const platform_backend_t* platform_backend_);

/**
 * @brief GLFW用仮想関数テーブル定義
 *
 */
static const platform_backend_vtable_t s_glfw_vtable = {
    .platform_backend_create = platform_glfw_create,
    .platform_backend_deinitialize = platform_glfw_deinitialize,
    .platform_backend_update = platform_glfw_update,
    .platform_backend_swap_buffers = platform_glfw_swap_buffers,
    .platform_backend_is_valid = platform_glfw_is_valid,
};

const platform_backend_vtable_t* platform_glfw_vtable_get(void) {
    return &s_glfw_vtable;
}

// ============================================================
// Backend entry points
// ============================================================
static platform_system_result_t platform_glfw_create(const platform_system_config_t* config_, linear_alloc_t* linear_alloc_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_backend_t** out_platform_backend_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    platform_backend_t* tmp_platform_backend = NULL;
    choco_string_t* tmp_window_label = NULL;

    GLFWwindow* tmp_window = NULL;
    int tmp_framebuffer_width = 0;
    int tmp_framebuffer_height = 0;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_glfw_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(linear_alloc_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_glfw_create", "linear_alloc_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_width_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_glfw_create", "out_framebuffer_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_height_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_glfw_create", "out_framebuffer_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_platform_backend_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_glfw_create", "out_platform_backend_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_platform_backend_, ret, PLATFORM_SYSTEM_BAD_OPERATION, platform_system_rslt_to_str(PLATFORM_SYSTEM_BAD_OPERATION), "platform_glfw_create", "*out_platform_backend_")

    // platform backend
    ret_linear_alloc = linear_allocator_allocate(linear_alloc_, sizeof(platform_backend_t), alignof(platform_backend_t), (void**)&tmp_platform_backend);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = platform_system_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("platform_glfw_create(%s) - linear_allocator_allocate failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_platform_backend, 0, sizeof(platform_backend_t));

    // event array, event count
    ret = event_storage_initialize(config_, linear_alloc_, &tmp_platform_backend->window_event_storage, &tmp_platform_backend->keyboard_event_storage, &tmp_platform_backend->mouse_event_storage);
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_create(%s) - event_storage_initialize failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    // event view
    ret = event_view_refresh(false, &tmp_platform_backend->window_event_storage, &tmp_platform_backend->keyboard_event_storage, &tmp_platform_backend->mouse_event_storage, &tmp_platform_backend->event_view);
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_create(%s) - event_view_refresh failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    // GLFW
    ret = glfw_runtime_initialize();
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_create(%s) - glfw_runtime_initialize failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    // window label
    ret_string = choco_string_create_from_c_string(config_->window_label, &tmp_window_label);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = platform_system_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("platform_glfw_create(%s) - choco_string_create_from_c_string failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_platform_backend->window_label = tmp_window_label;
    tmp_window_label = NULL;

    // window
    ret = glfw_window_create(config_->window_label, config_->window_width, config_->window_height, &tmp_framebuffer_width, &tmp_framebuffer_height, &tmp_window);
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_create(%s) - glfw_window_create failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    snapshot_reset(&tmp_platform_backend->prev);
    snapshot_reset(&tmp_platform_backend->current);
    tmp_platform_backend->prev.framebuffer_height = tmp_framebuffer_height;
    tmp_platform_backend->prev.framebuffer_width = tmp_framebuffer_width;
    tmp_platform_backend->window = tmp_window;
    tmp_window = NULL;

    tmp_platform_backend->prev.window_height = config_->window_height;
    tmp_platform_backend->prev.window_width = config_->window_width;

    *out_platform_backend_ = tmp_platform_backend;
    *out_framebuffer_height_ = tmp_framebuffer_height;
    *out_framebuffer_width_ = tmp_framebuffer_width;

    tmp_platform_backend = NULL;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    if(PLATFORM_SYSTEM_DATA_CORRUPTED != ret) {
        if(NULL != tmp_platform_backend) {
            platform_glfw_deinitialize(tmp_platform_backend);
        }
        if(NULL != tmp_window_label) {
            choco_string_destroy(&tmp_window_label);
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
    platform_backend_->window = NULL;

    event_storage_counts_reset(&platform_backend_->window_event_storage, &platform_backend_->keyboard_event_storage, &platform_backend_->mouse_event_storage);
    snapshot_reset(&platform_backend_->prev);
    snapshot_reset(&platform_backend_->current);
}

static platform_system_result_t platform_glfw_update(platform_backend_t* platform_backend_, const platform_event_view_t** out_event_view_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    bool tmp_window_close_requested = false;
    size_t window_event_count = 0;
    size_t keyboard_event_count = 0;
    size_t mouse_event_count = 0;

    // 毎ループcallされるAPIであるため、*out_event_view_ != NULLは許容する
    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_glfw_update", "platform_backend_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_glfw_update", "platform_backend_->window")
    IF_ARG_NULL_GOTO_CLEANUP(out_event_view_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_glfw_update", "out_event_view_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(platform_backend_)) {
        ret = PLATFORM_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("platform_glfw_update(%s) - Precondition validation failed for 'platform_backend_'.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    glfwPollEvents();

    ret = snapshot_collect(platform_backend_);
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_update(%s) - snapshot_collect failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    if(platform_backend_->current.window_should_close) {    // 後続処理でエラーが発生しても無視したいためウィンドウクローズは独立で処理する
        event_storage_counts_reset(&platform_backend_->window_event_storage, &platform_backend_->keyboard_event_storage, &platform_backend_->mouse_event_storage);
        tmp_window_close_requested = true;
    } else {
        ret = event_count(&platform_backend_->prev, &platform_backend_->current, &window_event_count, &keyboard_event_count, &mouse_event_count);
        if(PLATFORM_SYSTEM_SUCCESS != ret) {
            ret = PLATFORM_SYSTEM_UNDEFINED_ERROR; // ここではエラーは出ないはず
            ERROR_MESSAGE("platform_glfw_update(%s) - event_count failed.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }

        if(window_event_count > platform_backend_->window_event_storage.max_event_count) {
            ret = PLATFORM_SYSTEM_LIMIT_EXCEEDED;
            ERROR_MESSAGE("platform_glfw_update(%s) - window event count limit exceeded.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }

        if(keyboard_event_count > platform_backend_->keyboard_event_storage.max_event_count) {
            ret = PLATFORM_SYSTEM_LIMIT_EXCEEDED;
            ERROR_MESSAGE("platform_glfw_update(%s) - keyboard event count limit exceeded.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }

        if(mouse_event_count > platform_backend_->mouse_event_storage.max_event_count) {
            ret = PLATFORM_SYSTEM_LIMIT_EXCEEDED;
            ERROR_MESSAGE("platform_glfw_update(%s) - mouse event count limit exceeded.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }

        event_storage_counts_reset(&platform_backend_->window_event_storage, &platform_backend_->keyboard_event_storage, &platform_backend_->mouse_event_storage);

        ret = window_events_generate(&platform_backend_->prev, &platform_backend_->current, &platform_backend_->window_event_storage);
        if(PLATFORM_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("platform_glfw_update(%s) - window_events_generate failed.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }

        ret = keyboard_events_generate(&platform_backend_->prev, &platform_backend_->current, &platform_backend_->keyboard_event_storage);
        if(PLATFORM_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("platform_glfw_update(%s) - keyboard_events_generate failed.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }

        ret = mouse_events_generate(&platform_backend_->prev, &platform_backend_->current, &platform_backend_->mouse_event_storage);
        if(PLATFORM_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("platform_glfw_update(%s) - mouse_events_generate failed.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    platform_backend_->prev = platform_backend_->current;

    ret = event_view_refresh(tmp_window_close_requested, &platform_backend_->window_event_storage, &platform_backend_->keyboard_event_storage, &platform_backend_->mouse_event_storage, &platform_backend_->event_view);
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("platform_glfw_update(%s) - event_view_refresh failed.", platform_system_rslt_to_str(ret));
            goto cleanup;
    }

    *out_event_view_ = &platform_backend_->event_view;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static platform_system_result_t platform_glfw_swap_buffers(platform_backend_t* platform_backend_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_glfw_swap_buffers", "platform_backend_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, ret, PLATFORM_SYSTEM_BAD_OPERATION, platform_system_rslt_to_str(PLATFORM_SYSTEM_BAD_OPERATION), "platform_glfw_swap_buffers", "platform_backend_->window")

    glfwSwapBuffers(platform_backend_->window);

    ret = PLATFORM_SYSTEM_SUCCESS;

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
    if(!choco_string_is_valid(platform_backend_->window_label)) {
        return false;
    }
    if(!platform_event_view_is_valid(&platform_backend_->event_view)) {
        return false;
    }
    if(!window_event_storage_is_valid(&platform_backend_->window_event_storage)) {
        return false;
    }
    if(!keyboard_event_storage_is_valid(&platform_backend_->keyboard_event_storage)) {
        return false;
    }
    if(!mouse_event_storage_is_valid(&platform_backend_->mouse_event_storage)) {
        return false;
    }
    return true;
}

// ============================================================
// Initialization / lifecycle helpers
// ============================================================
static platform_system_result_t glfw_runtime_initialize(void) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    if(GL_FALSE == glfwInit()) {
        ret = PLATFORM_SYSTEM_RUNTIME_ERROR;
        ERROR_MESSAGE("glfw_runtime_initialize(%s) - Failed to initialize glfw.", platform_system_rslt_to_str(ret));
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

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static platform_system_result_t glfw_window_create(const char* window_label_, int window_width_, int window_height_, int* out_framebuffer_width_, int* out_framebuffer_height_, GLFWwindow** out_window_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    GLFWwindow* tmp_window = NULL;
    int tmp_framebuffer_width = 0;
    int tmp_framebuffer_height = 0;

    IF_ARG_NULL_GOTO_CLEANUP(window_label_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "glfw_window_create", "window_label_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_width_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "glfw_window_create", "out_framebuffer_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_height_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "glfw_window_create", "out_framebuffer_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_window_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "glfw_window_create", "out_window_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_window_, ret, PLATFORM_SYSTEM_BAD_OPERATION, platform_system_rslt_to_str(PLATFORM_SYSTEM_BAD_OPERATION), "glfw_window_create", "*out_window_")
    if(0 == window_width_) {
        ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("glfw_window_create(%s) - Provided window_width_ is not valid.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == window_height_) {
        ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("glfw_window_create(%s) - Provided window_height_ is not valid.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_window = glfwCreateWindow(window_width_, window_height_, window_label_, NULL, NULL);   // 第四引数でフルスクリーン化, 第五引数で他のウィンドウとリソース共有
    if(NULL == tmp_window) {
        ret = PLATFORM_SYSTEM_RUNTIME_ERROR;
        ERROR_MESSAGE("glfw_window_create(%s) - glfwCreateWindow failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    // 引数windowに指定したハンドルのウィンドウのレンダリングコンテキストをカレント(処理対象)にする。
    // レンダリングコンテキストは描画に用いられる情報で、ウィンドウごとに保持される。
    // 図形の描画はこれをカレントに設定したウィンドウに対して行われる。
    glfwMakeContextCurrent(tmp_window);
    glewExperimental = true;
    if(GLEW_OK != glewInit()) {
        ret = PLATFORM_SYSTEM_RUNTIME_ERROR;
        ERROR_MESSAGE("glfw_window_create(%s) - glewInit failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    // https://www.glfw.org/docs/latest/group__input.html#gaa92336e173da9c8834558b54ee80563b
    glfwSetInputMode(tmp_window, GLFW_STICKY_KEYS, GLFW_TRUE);  // これでエスケープキーが押されるのを捉えるのを保証する

    glfwGetFramebufferSize(tmp_window, &tmp_framebuffer_width, &tmp_framebuffer_height);

    *out_framebuffer_height_ = tmp_framebuffer_height;
    *out_framebuffer_width_ = tmp_framebuffer_width;
    *out_window_ = tmp_window;
    tmp_window = NULL;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
        if(NULL != tmp_window) {
            glfwDestroyWindow(tmp_window);
            tmp_window = NULL;
        }
    }
    return ret;
}

static platform_system_result_t event_storage_initialize(const platform_system_config_t* config_, linear_alloc_t* linear_alloc_, window_event_storage_t* window_event_storage_, keyboard_event_storage_t* keyboard_event_storage_, mouse_event_storage_t* mouse_event_storage_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    size_t allocation_size = 0;

    window_event_t* tmp_window_event_storage = NULL;
    keyboard_event_t* tmp_keyboard_event_storage = NULL;
    mouse_event_t* tmp_mouse_event_storage = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(linear_alloc_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "linear_alloc_")
    IF_ARG_NULL_GOTO_CLEANUP(window_event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "window_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "keyboard_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(mouse_event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "mouse_event_storage_")

    // window event
    if((SIZE_MAX / config_->max_window_event_count) < sizeof(window_event_t)) {
        ret = PLATFORM_SYSTEM_OVERFLOW;
        ERROR_MESSAGE("event_storage_initialize(%s) - window event array size overflow.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    allocation_size = sizeof(window_event_t) * config_->max_window_event_count;
    ret_linear_alloc = linear_allocator_allocate(linear_alloc_, allocation_size, alignof(window_event_t), (void**)&tmp_window_event_storage);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = platform_system_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("event_storage_initialize(%s) - linear_allocator_allocate failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_window_event_storage, 0, allocation_size);

    // keyboard event
    if((SIZE_MAX / config_->max_keyboard_event_count) < sizeof(keyboard_event_t)) {
        ret = PLATFORM_SYSTEM_OVERFLOW;
        ERROR_MESSAGE("event_storage_initialize(%s) - keyboard event array size overflow.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    allocation_size = sizeof(keyboard_event_t) * config_->max_keyboard_event_count;
    ret_linear_alloc = linear_allocator_allocate(linear_alloc_, allocation_size, alignof(keyboard_event_t), (void**)&tmp_keyboard_event_storage);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = platform_system_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("event_storage_initialize(%s) - linear_allocator_allocate failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_keyboard_event_storage, 0, allocation_size);

    // mouse event
    if((SIZE_MAX / config_->max_mouse_event_count) < sizeof(mouse_event_t)) {
        ret = PLATFORM_SYSTEM_OVERFLOW;
        ERROR_MESSAGE("event_storage_initialize(%s) - mouse event array size overflow.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    allocation_size = sizeof(mouse_event_t) * config_->max_mouse_event_count;
    ret_linear_alloc = linear_allocator_allocate(linear_alloc_, allocation_size, alignof(mouse_event_t), (void**)&tmp_mouse_event_storage);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = platform_system_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("event_storage_initialize(%s) - linear_allocator_allocate failed.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_mouse_event_storage, 0, allocation_size);

    window_event_storage_->current_event_count = 0;
    window_event_storage_->max_event_count = config_->max_window_event_count;
    window_event_storage_->event_storage = tmp_window_event_storage;
    tmp_window_event_storage = NULL;

    keyboard_event_storage_->current_event_count = 0;
    keyboard_event_storage_->max_event_count = config_->max_keyboard_event_count;
    keyboard_event_storage_->event_storage = tmp_keyboard_event_storage;
    tmp_keyboard_event_storage = NULL;

    mouse_event_storage_->current_event_count = 0;
    mouse_event_storage_->max_event_count = config_->max_mouse_event_count;
    mouse_event_storage_->event_storage = tmp_mouse_event_storage;
    tmp_mouse_event_storage = NULL;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static void snapshot_reset(platform_glfw_snapshot_t* snapshot_) {
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
    for(size_t i = 0; i != KEY_CODE_MAX; ++i) {
        snapshot_->keycode_state[i] = false;
    }
}

// ============================================================
// Per-frame update helpers
// ============================================================
static platform_system_result_t snapshot_collect(platform_backend_t* platform_backend_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    int left_button_state = 0;
    int right_button_state = 0;

    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "snapshot_collect", "platform_backend_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "snapshot_collect", "platform_backend_->window")

    // window events.
    platform_backend_->current.window_should_close = (0 != glfwWindowShouldClose(platform_backend_->window)) ? true : false;

    glfwGetWindowSize(platform_backend_->window, &platform_backend_->current.window_width, &platform_backend_->current.window_height);
    glfwGetFramebufferSize(platform_backend_->window, &platform_backend_->current.framebuffer_width, &platform_backend_->current.framebuffer_height);

    // keyboard events.
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

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static void event_storage_counts_reset(window_event_storage_t* window_event_storage_, keyboard_event_storage_t* keyboard_event_storage_, mouse_event_storage_t* mouse_event_storage_) {
    if(NULL == window_event_storage_ || NULL == keyboard_event_storage_ || NULL == mouse_event_storage_) {
        return;
    }

    window_event_storage_->current_event_count = 0;
    keyboard_event_storage_->current_event_count = 0;
    mouse_event_storage_->current_event_count = 0;
}

static platform_system_result_t event_count(const platform_glfw_snapshot_t* prev_, const platform_glfw_snapshot_t* current_, size_t* out_window_event_count_, size_t* out_keyboard_event_count_, size_t* out_mouse_event_count_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    size_t tmp_window_event_count = 0;
    size_t tmp_keyboard_event_count = 0;
    size_t tmp_mouse_event_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(prev_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_count", "prev_")
    IF_ARG_NULL_GOTO_CLEANUP(current_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_count", "current_")
    IF_ARG_NULL_GOTO_CLEANUP(out_window_event_count_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_count", "out_window_event_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_keyboard_event_count_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_count", "out_keyboard_event_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mouse_event_count_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_count", "out_mouse_event_count_")

    if((current_->window_width != prev_->window_width || current_->window_height != prev_->window_height) ||
       (current_->framebuffer_width != prev_->framebuffer_width || current_->framebuffer_height != prev_->framebuffer_height)) {
        tmp_window_event_count++;
    }

    for(int i = KEY_1; i != KEY_CODE_MAX; ++i) {
        if(prev_->keycode_state[i] != current_->keycode_state[i]) {
            tmp_keyboard_event_count++;
        }
    }

    // 左クリックイベント
    if(prev_->left_button_pressed != current_->left_button_pressed) {
        tmp_mouse_event_count++;
    }

    // 右クリックイベント
    if(prev_->right_button_pressed != current_->right_button_pressed) {
        tmp_mouse_event_count++;
    }

    *out_window_event_count_ = tmp_window_event_count;
    *out_keyboard_event_count_ = tmp_keyboard_event_count;
    *out_mouse_event_count_ = tmp_mouse_event_count;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static platform_system_result_t window_events_generate(const platform_glfw_snapshot_t* prev_, const platform_glfw_snapshot_t* current_, window_event_storage_t* event_storage_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(prev_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "window_events_generate", "prev_")
    IF_ARG_NULL_GOTO_CLEANUP(current_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "window_events_generate", "current_")
    IF_ARG_NULL_GOTO_CLEANUP(event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "window_events_generate", "event_storage_")

    if((current_->window_width != prev_->window_width || current_->window_height != prev_->window_height) ||
       (current_->framebuffer_width != prev_->framebuffer_width || current_->framebuffer_height != prev_->framebuffer_height)) {
        window_event_t tmp_event;

        tmp_event.event_code = WINDOW_EVENT_RESIZE;
        tmp_event.event_args.window_height = current_->window_height;
        tmp_event.event_args.window_width = current_->window_width;
        tmp_event.event_args.framebuffer_height = current_->framebuffer_height;
        tmp_event.event_args.framebuffer_width = current_->framebuffer_width;

        ret = window_event_storage_push(event_storage_, &tmp_event);
        if(PLATFORM_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("window_events_generate(%s) - window_event_storage_push failed.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static platform_system_result_t keyboard_events_generate(const platform_glfw_snapshot_t* prev_, const platform_glfw_snapshot_t* current_, keyboard_event_storage_t* event_storage_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(prev_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "keyboard_events_generate", "prev_")
    IF_ARG_NULL_GOTO_CLEANUP(current_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "keyboard_events_generate", "current_")
    IF_ARG_NULL_GOTO_CLEANUP(event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "keyboard_events_generate", "event_storage_")

    for(int i = KEY_1; i != KEY_CODE_MAX; ++i) {
        if(prev_->keycode_state[i] != current_->keycode_state[i]) {
            keyboard_event_t tmp_event;

            tmp_event.key = (keycode_t)i;
            tmp_event.event_args.pressed = current_->keycode_state[i];

            ret = keyboard_event_storage_push(event_storage_, &tmp_event);
            if(PLATFORM_SYSTEM_SUCCESS != ret) {
                ERROR_MESSAGE("keyboard_events_generate(%s) - keyboard_event_storage_push failed.", platform_system_rslt_to_str(ret));
                goto cleanup;
            }
        }
    }

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static platform_system_result_t mouse_events_generate(const platform_glfw_snapshot_t* prev_, const platform_glfw_snapshot_t* current_, mouse_event_storage_t* event_storage_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(prev_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "mouse_events_generate", "prev_")
    IF_ARG_NULL_GOTO_CLEANUP(current_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "mouse_events_generate", "current_")
    IF_ARG_NULL_GOTO_CLEANUP(event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "mouse_events_generate", "event_storage_")

    // 左クリックイベント
    if(prev_->left_button_pressed != current_->left_button_pressed) {
        mouse_event_t tmp_event;

        tmp_event.button = MOUSE_BUTTON_LEFT;
        tmp_event.event_args.pressed = current_->left_button_pressed;
        tmp_event.event_args.x = (int)current_->cursor_x;
        tmp_event.event_args.y = (int)current_->cursor_y;

        ret = mouse_event_storage_push(event_storage_, &tmp_event);
        if(PLATFORM_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("mouse_events_generate(%s) - mouse_event_storage_push failed.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    // 右クリックイベント
    if(prev_->right_button_pressed != current_->right_button_pressed) {
        mouse_event_t tmp_event;

        tmp_event.button = MOUSE_BUTTON_RIGHT;
        tmp_event.event_args.pressed = current_->right_button_pressed;
        tmp_event.event_args.x = (int)current_->cursor_x;
        tmp_event.event_args.y = (int)current_->cursor_y;

        ret = mouse_event_storage_push(event_storage_, &tmp_event);
        if(PLATFORM_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("mouse_events_generate(%s) - mouse_event_storage_push failed.", platform_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static platform_system_result_t event_view_refresh(bool window_close_requested_, const window_event_storage_t* window_event_storage_, const keyboard_event_storage_t* keyboard_event_storage_, const mouse_event_storage_t* mouse_event_storage_, platform_event_view_t* event_view_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(window_event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_view_refresh", "window_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_view_refresh", "keyboard_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(mouse_event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_view_refresh", "mouse_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(event_view_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "event_view_refresh", "event_view_")

    event_view_->keyboard_event_count = keyboard_event_storage_->current_event_count;
    event_view_->mouse_event_count = mouse_event_storage_->current_event_count;
    event_view_->window_event_count = window_event_storage_->current_event_count;

    event_view_->keyboard_events = keyboard_event_storage_->event_storage;
    event_view_->mouse_events = mouse_event_storage_->event_storage;
    event_view_->window_events = window_event_storage_->event_storage;

    event_view_->window_close_requested = window_close_requested_;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Event storage primitive operations
// ============================================================
static platform_system_result_t window_event_storage_push(window_event_storage_t* event_storage_, const window_event_t* event_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "window_event_storage_push", "event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(event_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "window_event_storage_push", "event_")
    if(event_storage_->current_event_count >= event_storage_->max_event_count) {
        ret = PLATFORM_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("window_event_storage_push(%s) - window event storage limit exceeded.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    event_storage_->event_storage[event_storage_->current_event_count] = *event_;
    event_storage_->current_event_count++;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static platform_system_result_t keyboard_event_storage_push(keyboard_event_storage_t* event_storage_, const keyboard_event_t* event_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "keyboard_event_storage_push", "event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(event_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "keyboard_event_storage_push", "event_")
    if(event_storage_->current_event_count >= event_storage_->max_event_count) {
        ret = PLATFORM_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("keyboard_event_storage_push(%s) - keyboard event storage limit exceeded.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    event_storage_->event_storage[event_storage_->current_event_count] = *event_;
    event_storage_->current_event_count++;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static platform_system_result_t mouse_event_storage_push(mouse_event_storage_t* event_storage_, const mouse_event_t* event_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(event_storage_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "mouse_event_storage_push", "event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(event_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "mouse_event_storage_push", "event_")
    if(event_storage_->current_event_count >= event_storage_->max_event_count) {
        ret = PLATFORM_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("mouse_event_storage_push(%s) - mouse event storage limit exceeded.", platform_system_rslt_to_str(ret));
        goto cleanup;
    }

    event_storage_->event_storage[event_storage_->current_event_count] = *event_;
    event_storage_->current_event_count++;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// GLFW conversion / utility
// ============================================================
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
        ERROR_MESSAGE("keycode_to_glfw_keycode(%s) - Undefined key code. Returning key '0'", platform_system_rslt_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT));
        return GLFW_KEY_0;
    }
}

// ============================================================
// Validators
// ============================================================
static bool window_event_storage_is_valid(const window_event_storage_t* event_storage_) {
    if(NULL == event_storage_) {
        return false;
    }
    if(0 == event_storage_->max_event_count) {
        return false;
    }
    if(event_storage_->current_event_count > event_storage_->max_event_count) {
        return false;
    }
    if(NULL == event_storage_->event_storage) {
        return false;
    }
    return true;
}

static bool keyboard_event_storage_is_valid(const keyboard_event_storage_t* event_storage_) {
    if(NULL == event_storage_) {
        return false;
    }
    if(0 == event_storage_->max_event_count) {
        return false;
    }
    if(event_storage_->current_event_count > event_storage_->max_event_count) {
        return false;
    }
    if(NULL == event_storage_->event_storage) {
        return false;
    }
    return true;
}

static bool mouse_event_storage_is_valid(const mouse_event_storage_t* event_storage_) {
    if(NULL == event_storage_) {
        return false;
    }
    if(0 == event_storage_->max_event_count) {
        return false;
    }
    if(event_storage_->current_event_count > event_storage_->max_event_count) {
        return false;
    }
    if(NULL == event_storage_->event_storage) {
        return false;
    }
    return true;
}

static bool is_valid_shallow(const platform_backend_t* platform_backend_) {
    if(NULL == platform_backend_) {
        return false;
    }
    if(NULL == platform_backend_->window_label) {
        return false;
    }
    if(NULL == platform_backend_->window) {
        return false;
    }
    if(NULL == platform_backend_->keyboard_event_storage.event_storage) {
        return false;
    }
    if(NULL == platform_backend_->mouse_event_storage.event_storage) {
        return false;
    }
    if(NULL == platform_backend_->window_event_storage.event_storage) {
        return false;
    }
    if(0 == platform_backend_->keyboard_event_storage.max_event_count) {
        return false;
    }
    if(0 == platform_backend_->mouse_event_storage.max_event_count) {
        return false;
    }
    if(0 == platform_backend_->window_event_storage.max_event_count) {
        return false;
    }
    if(platform_backend_->window_event_storage.current_event_count > platform_backend_->window_event_storage.max_event_count) {
        return false;
    }
    if(platform_backend_->keyboard_event_storage.current_event_count > platform_backend_->keyboard_event_storage.max_event_count) {
        return false;
    }
    if(platform_backend_->mouse_event_storage.current_event_count > platform_backend_->mouse_event_storage.max_event_count) {
        return false;
    }
    if(platform_backend_->keyboard_event_storage.event_storage != platform_backend_->event_view.keyboard_events) {
        return false;
    }
    if(platform_backend_->mouse_event_storage.event_storage != platform_backend_->event_view.mouse_events) {
        return false;
    }
    if(platform_backend_->window_event_storage.event_storage != platform_backend_->event_view.window_events) {
        return false;
    }
    if(platform_backend_->keyboard_event_storage.current_event_count != platform_backend_->event_view.keyboard_event_count) {
        return false;
    }
    if(platform_backend_->mouse_event_storage.current_event_count != platform_backend_->event_view.mouse_event_count) {
        return false;
    }
    if(platform_backend_->window_event_storage.current_event_count != platform_backend_->event_view.window_event_count) {
        return false;
    }
    return true;
}
