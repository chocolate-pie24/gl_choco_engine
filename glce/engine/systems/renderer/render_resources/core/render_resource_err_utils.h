// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_CORE_RENDER_RESOURCE_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_CORE_RENDER_RESOURCE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/io_utils/fs_path.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/textures/core/texture_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"

const char* render_resource_result_to_str(render_resource_result_t result_);

render_resource_result_t render_resource_result_convert_subsystem_allocator(subsystem_allocator_result_t result_);

render_resource_result_t render_resource_result_convert_fs_path(fs_path_result_t result_);

render_resource_result_t render_resource_result_convert_shader(shader_result_t result_);

render_resource_result_t render_resource_result_convert_resource_registry(resource_registry_result_t result_);

render_resource_result_t render_resource_result_convert_resource_pipeline(resource_pipeline_result_t result_);

render_resource_result_t render_resource_result_convert_geometry(geometry_result_t result_);

render_resource_result_t render_resource_result_convert_texture(texture_result_t result_);

render_resource_result_t render_resource_result_convert_buffer_manager(buffer_manager_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
