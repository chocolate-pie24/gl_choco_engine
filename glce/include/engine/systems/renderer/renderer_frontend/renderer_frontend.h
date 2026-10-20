// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_FRONTEND_RENDERER_FRONTEND_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_FRONTEND_RENDERER_FRONTEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

typedef struct renderer_frontend renderer_frontend_t;

typedef struct renderer_config renderer_config_t;
typedef struct vbo_manager_config vbo_manager_config_t;
typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct renderer_backend_context renderer_backend_context_t;

// TODO: 各種config validator
typedef struct renderer_frontend_vbo_config {
    size_t vbo_size;
    size_t max_allocation_count;
    size_t base_align;
    buffer_usage_t buffer_usage;
} renderer_frontend_vbo_config_t;

// TODO: xxx_meshのuse / not useを選択可能にするのを検討する
typedef struct renderer_frontend_untextured_mesh_config {
    renderer_frontend_vbo_config_t vbo;
    size_t max_geometry_count;
    size_t max_material_count;
} renderer_frontend_untextured_mesh_config_t;

typedef struct renderer_frontend_config {
    renderer_frontend_untextured_mesh_config_t untextured_mesh;
} renderer_frontend_config_t;

renderer_frontend_result_t renderer_frontend_create(
    const renderer_frontend_config_t* config_,
    subsystem_allocator_t* allocator_,
    const char* executable_directory_,
    const char* shader_dir_,
    renderer_frontend_t** out_renderer_frontend_);

void renderer_frontend_deinitialize(renderer_frontend_t* renderer_frontend_);

#ifdef __cplusplus
}
#endif
#endif
