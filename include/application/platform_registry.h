#ifndef GLCE_APP_PLATFORM_REGISTRY_H
#define GLCE_APP_PLATFORM_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/interfaces/platform_interface.h"

const platform_vtable_t* platform_registry_vtable_get(platform_type_t platform_type_);

#ifdef __cplusplus
}
#endif
#endif
