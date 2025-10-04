#ifndef GLCE_ENGINE_CORE_PLATFORM_PLATFORM_UTILS_H
#define GLCE_ENGINE_CORE_PLATFORM_PLATFORM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLATFORM_SUCCESS,
    PLATFORM_INVALID_ARGUMENT,
    PLATFORM_RUNTIME_ERROR,
    PLATFORM_NO_MEMORY,
} platform_error_t;

typedef enum {
    PLATFORM_USE_GLFW,
} platform_type_t;

#ifdef __cplusplus
}
#endif
#endif
