// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_CORE_SHADER_PROGRAM_BUILDER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_CORE_SHADER_PROGRAM_BUILDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct renderer_backend_shader renderer_backend_shader_t;
typedef struct renderer_backend_context renderer_backend_context_t;

shader_result_t shader_program_builder_create_from_files(renderer_backend_context_t* backend_context_, const char* file_path_, const char* name_, renderer_backend_shader_t** out_shader_);

#ifdef __cplusplus
}
#endif
#endif
