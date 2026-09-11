// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_CORE_RENDER_RESOURCE_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_CORE_RENDER_RESOURCE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/memory/linear_allocator.h"

#include "engine/io_utils/fs_path.h"

#include "engine/resource/core/resource_types.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"
#include "engine/systems/renderer/resources/texture/texture_gpu_resource_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

const char* render_resource_rslt_to_str(render_resource_result_t rslt_);

render_resource_result_t render_resource_rslt_convert_linear_allocator(linear_allocator_result_t rslt_);

render_resource_result_t render_resource_rslt_convert_fs_path(fs_path_result_t rslt_);

render_resource_result_t render_resource_rslt_convert_shader(shader_result_t rslt_);

render_resource_result_t render_resource_rslt_convert_resource_registry(resource_registry_result_t rslt_);

render_resource_result_t render_resource_rslt_convert_resource_pipeline(resource_pipeline_result_t rslt_);

render_resource_result_t render_resource_rslt_convert_resource(resource_result_t rslt_);

render_resource_result_t render_resource_rslt_convert_texture_gpu_resource(texture_gpu_resource_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
