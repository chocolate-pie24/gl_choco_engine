#ifndef GLCE_ENGINE_PLATFORM_PLATFORM_GLFW_H
#define GLCE_ENGINE_PLATFORM_PLATFORM_GLFW_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/interfaces/platform_interface.h"

const platform_vtable_t* platform_glfw_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
