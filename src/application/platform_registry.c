#include <stddef.h>

#include "application/platform_registry.h"

#include "engine/platform/platform_glfw.h"

const platform_vtable_t* platform_registry_vtable_get(platform_type_t platform_type_) {
    switch (platform_type_) {
    case PLATFORM_USE_GLFW:
        return platform_glfw_vtable_get();
    default:
        return NULL;
    }
}
