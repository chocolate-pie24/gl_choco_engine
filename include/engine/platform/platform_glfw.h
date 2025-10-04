#ifndef GLCE_ENGINE_PLATFORM_PLATFORM_GLFW_H
#define GLCE_ENGINE_PLATFORM_PLATFORM_GLFW_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool platform_glfw_window_create(const char* window_label_, int widow_width_, int window_height);

#ifdef __cplusplus
}
#endif
#endif
