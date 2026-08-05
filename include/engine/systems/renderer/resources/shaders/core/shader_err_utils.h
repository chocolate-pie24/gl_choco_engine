#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_CORE_SHADER_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_CORE_SHADER_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_utils.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/buffer_managers/buffer_manager_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

const char* shader_rslt_to_str(shader_result_t rslt_);

shader_result_t shader_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

shader_result_t shader_rslt_convert_choco_memory(memory_system_result_t rslt_);

shader_result_t shader_rslt_convert_choco_string(choco_string_result_t rslt_);

shader_result_t shader_rslt_convert_fs_utils(fs_utils_result_t rslt_);

shader_result_t shader_rslt_convert_renderer_backend(renderer_backend_result_t rslt_);

shader_result_t shader_rslt_convert_buffer_manager(buffer_manager_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
