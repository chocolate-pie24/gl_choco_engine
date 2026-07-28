#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_BUFFER_MANAGERS_BUFFER_MANAGER_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_BUFFER_MANAGERS_BUFFER_MANAGER_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"
#include "engine/systems/renderer/renderer_core/allocators/range_allocator.h"

#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_types.h"

const char* buffer_manager_rslt_to_str(buffer_manager_result_t rslt_);

buffer_manager_result_t buffer_manager_rslt_convert_range_allocator(range_allocator_result_t rslt_);

buffer_manager_result_t buffer_manager_rslt_convert_renderer(renderer_result_t rslt_);

buffer_manager_result_t buffer_manager_rslt_convert_choco_memory(memory_system_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
