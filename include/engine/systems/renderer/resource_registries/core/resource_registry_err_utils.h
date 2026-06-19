#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_CORE_RESOURCE_REGISTRY_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_CORE_RESOURCE_REGISTRY_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

#include "engine/core/memory/linear_allocator.h"
#include "engine/resource/resource_core/resource_types.h"

const char* resource_registry_rslt_to_str(resource_registry_result_t rslt_);

resource_registry_result_t resource_registry_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

resource_registry_result_t resource_registry_rslt_convert_resource(resource_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
