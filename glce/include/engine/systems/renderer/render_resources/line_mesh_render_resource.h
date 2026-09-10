// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_LINE_MESH_RENDER_RESOURCE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_LINE_MESH_RENDER_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"

typedef struct line_mesh_render_resource line_mesh_render_resource_t;
typedef struct linear_alloc linear_alloc_t;
typedef struct line_mesh_shader_config line_mesh_shader_config_t;
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct line_vertex line_vertex_t;
typedef struct aabb_3d aabb_3d_t;

render_resource_result_t line_mesh_render_resource_initialize(const line_mesh_shader_config_t* shader_config_, size_t max_geometry_count_, renderer_backend_context_t* renderer_backend_context_, linear_alloc_t* allocator_, const char* executable_directory_, const char* shader_dir_, line_mesh_render_resource_t** out_render_resource_);

void line_mesh_render_resource_deinitialize(line_mesh_render_resource_t* render_resource_);

// Geometry resource operation
render_resource_result_t line_mesh_render_resource_import_from_vertices(line_mesh_render_resource_t* render_resource_, const char* resource_name_, const line_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_);

render_resource_result_t line_mesh_render_resource_import_from_aabb(line_mesh_render_resource_t* render_resource_, const char* resource_name_, const aabb_3d_t* aabb_, uint16_t* out_geometry_id_);

render_resource_result_t line_mesh_render_resource_release(line_mesh_render_resource_t* render_resource_, uint16_t geometry_id_);

// Frame-global state

bool line_mesh_render_resource_is_valid(const line_mesh_render_resource_t* render_resource_);

#ifdef __cplusplus
}
#endif
#endif
