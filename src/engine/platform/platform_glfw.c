#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>
#include <stdlib.h> // for free() TODO: remove this!!
#include <string.h> // for strdup(TODO: remove this!!)

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "engine/platform/platform_glfw.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/interfaces/platform_interface.h"

/*
TODO:
 - [x] platform_destroy
 - [x] s_window削除
 - [x] platform_glfw_window_createの引数にplatform_state追加
 - [x] platform_stateにwindow_label追加(deep copy)
 - [x] platform_glfw_window_createエラー処理
 - [] strdup廃止(choco_malloc経由でメモリを確保したい)
 - [x] docs step2TODOにchoco_string追加
 - [] free廃止
*/

struct platform_state {
    int window_width;
    int window_height;
    char* window_label; // TODO: choco_string
    GLFWwindow* window;
};

static void platform_glfw_preinit(size_t* memory_requirement_, size_t* alignment_requirement_);
static platform_error_t platform_glfw_init(platform_state_t* platform_state_);
static void platform_glfw_destroy(platform_state_t* platform_state_);
static platform_error_t platform_glfw_window_create(platform_state_t* platform_state_, const char* window_label_, int window_width_, int window_height_);

static const platform_vtable_t s_glfw_vtable = {
    .platform_state_preinit = platform_glfw_preinit,
    .platform_state_init = platform_glfw_init,
    .platform_state_destroy = platform_glfw_destroy,
    .platform_window_create = platform_glfw_window_create,
};

const platform_vtable_t* platform_glfw_vtable_get(void) {
    return &s_glfw_vtable;
}

static void platform_glfw_preinit(size_t* memory_requirement_, size_t* alignment_requirement_) {
    if(NULL == memory_requirement_ || NULL == alignment_requirement_) {
        WARN_MESSAGE("platform_state_preinit - No-op: memory_requirement_ or alignment_requirement_ is NULL.");
        goto cleanup;
    }
    *memory_requirement_ = sizeof(platform_state_t);
    *alignment_requirement_ = alignof(platform_state_t);
    goto cleanup;
cleanup:
    return;
}

static platform_error_t platform_glfw_init(platform_state_t* platform_state_) {
    platform_error_t ret = PLATFORM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_state_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_init", "platform_state_")

    platform_state_->window = NULL;
    platform_state_->window_height = 0;
    platform_state_->window_width = 0;

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

static void platform_glfw_destroy(platform_state_t* platform_state_) {
    if(NULL == platform_state_) {
        return;
    }
    if(NULL == platform_state_->window_label) {
        free(platform_state_->window_label);
        platform_state_->window_label = NULL;
    }
    platform_state_->window = NULL;
    platform_state_->window_height = 0;
    platform_state_->window_width = 0;
}

// TODO: 引数エラーチェック
// TODO: 返り値をエラーコード
static platform_error_t platform_glfw_window_create(platform_state_t* platform_state_, const char* window_label_, int window_width_, int window_height_) {
    platform_error_t ret = PLATFORM_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(platform_state_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "platform_state_")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_label_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "window_label_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != window_width_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "window_width_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != window_height_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_window_create", "window_height_")

    if (GL_FALSE == glfwInit()) {
        ERROR_MESSAGE("platform_glfw_window_create(RUNTIME_ERROR) - Failed to initialize glfw.");
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

    platform_state_->window_label = strdup(window_label_);  // TODO: choco_string
    if(NULL == platform_state_->window_label) {
        ERROR_MESSAGE("platform_glfw_window_create(NO_MEMORY) - Failed to duplicate window label.");
        ret = PLATFORM_NO_MEMORY;
        goto cleanup;
    }
    platform_state_->window = glfwCreateWindow(window_width_, window_height_, window_label_, 0, 0);   // 第四引数でフルスクリーン化, 第五引数で他のウィンドウとリソース共有
    if (NULL == platform_state_->window) {
        ERROR_MESSAGE("platform_glfw_window_create(RUNTIME_ERROR) - Failed to create window.");
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }

    // 引数windowに指定したハンドルのウィンドウのレンダリングコンテキストをカレント(処理対象)にする。
    // レンダリングコンテキストは描画に用いられる情報で、ウィンドウごとに保持される。
    // 図形の描画はこれをカレントに設定したウィンドウに対して行われる。
    glfwMakeContextCurrent(platform_state_->window);
    glewExperimental = true;
    if (GLEW_OK != glewInit()) {
        ERROR_MESSAGE("platform_glfw_window_create(RUNTIME_ERROR) - Failed to initialize GLEW.");
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }

    // https://www.glfw.org/docs/latest/group__input.html#gaa92336e173da9c8834558b54ee80563b
    glfwSetInputMode(platform_state_->window, GLFW_STICKY_KEYS, GL_TRUE);  // これでエスケープキーが押されるのを捉えるのを保証する
    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}
