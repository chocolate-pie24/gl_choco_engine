// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_CORE_SHADER_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_CORE_SHADER_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"
#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_stream.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

const char* shader_result_to_str(shader_result_t result_);

shader_result_t shader_result_convert_linear_allocator(linear_allocator_result_t result_);

shader_result_t shader_result_convert_choco_string(choco_string_result_t result_);

shader_result_t shader_result_convert_fs_stream(fs_stream_result_t result_);

shader_result_t shader_result_convert_renderer_backend(renderer_backend_result_t result_);

shader_result_t shader_result_convert_buffer_manager(buffer_manager_result_t result_);

shader_result_t shader_result_convert_general_allocator(general_allocator_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
