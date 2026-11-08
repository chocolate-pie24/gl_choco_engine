// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_CORE_RESOURCE_PIPELINE_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_CORE_RESOURCE_PIPELINE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_stream.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/resources/loaders/core/loader_types.h"
#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/textures/core/texture_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

const char* resource_pipeline_result_to_str(resource_pipeline_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_geometry(geometry_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_renderer_backend(renderer_backend_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_resource_registry(resource_registry_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_shader(shader_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_choco_string(choco_string_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_fs_stream(fs_stream_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_texture(texture_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_general_allocator(general_allocator_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_loader(loader_result_t result_);

resource_pipeline_result_t resource_pipeline_result_convert_buffer_manager(buffer_manager_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
