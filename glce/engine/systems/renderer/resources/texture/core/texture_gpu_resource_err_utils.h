#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURE_CORE_TEXTURE_GPU_RESOURCE_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURE_CORE_TEXTURE_GPU_RESOURCE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/resources/texture/core/texture_gpu_resource_types.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

const char* texture_gpu_resource_rslt_to_str(texture_gpu_resource_result_t rslt_);

texture_gpu_resource_result_t texture_gpu_resource_rslt_convert_choco_memory(memory_system_result_t rslt_);

texture_gpu_resource_result_t texture_gpu_resource_rslt_convert_renderer_backend(renderer_backend_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
