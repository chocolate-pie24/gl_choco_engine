#include <stdbool.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "engine/platform/platform_glfw.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/interfaces/platform_interface.h"

static GLFWwindow* s_window = NULL; // TODO: remove this!!

static platform_error_t platform_glfw_window_create(const char* window_label_, int window_width_, int window_height);

static const platform_vtable_t s_glfw_vtable = {
    .platform_window_create = platform_glfw_window_create,
};

const platform_vtable_t* platform_glfw_vtable_get(void) {
    return &s_glfw_vtable;
}

// TODO: 引数エラーチェック
// TODO: 返り値をエラーコード
static platform_error_t platform_glfw_window_create(const char* window_label_, int window_width_, int window_height) {
    if (GL_FALSE == glfwInit()) {
        ERROR_MESSAGE("platform_glfw_window_create - Failed to initialize glfw.");
        return false;
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

    s_window = glfwCreateWindow(window_width_, window_height, window_label_, 0, 0);   // 第四引数でフルスクリーン化, 第五引数で他のウィンドウとリソース共有
    if (NULL == s_window) {
        ERROR_MESSAGE("platform_glfw_window_create - Failed to create window.");
        return false;
    }

    // 引数windowに指定したハンドルのウィンドウのレンダリングコンテキストをカレント(処理対象)にする。
    // レンダリングコンテキストは描画に用いられる情報で、ウィンドウごとに保持される。
    // 図形の描画はこれをカレントに設定したウィンドウに対して行われる。
    glfwMakeContextCurrent(s_window);
    glewExperimental = true;
    if (GLEW_OK != glewInit()) {
        ERROR_MESSAGE("platform_glfw_window_create - Failed to initialize GLEW.");
        return false;
    }

    // https://www.glfw.org/docs/latest/group__input.html#gaa92336e173da9c8834558b54ee80563b
    glfwSetInputMode(s_window, GLFW_STICKY_KEYS, GL_TRUE);  // これでエスケープキーが押されるのを捉えるのを保証する

    return true;
}
