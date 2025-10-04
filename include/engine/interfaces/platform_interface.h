#ifndef GLCE_ENGINE_INTERFACES_PLATFORM_INTERFACE_H
#define GLCE_ENGINE_INTERFACES_PLATFORM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct platform_state platform_state_t;

// core/platform_utilsへ移動
typedef enum {
    PLATFORM_SUCCESS,
    PLATFORM_INVALID_ARGUMENT,
    PLATFORM_GL_ERROR,
} platform_error_t;

// core/platform_utilsへ移動
typedef enum {
    PLATFORM_USE_GLFW,
} platform_type_t;

typedef struct platform_vtable {
    platform_error_t (*platform_window_create)(const char* window_label_, int window_width_, int window_height_);
} platform_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
