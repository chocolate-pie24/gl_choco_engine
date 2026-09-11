// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_POINT_MESH_RENDER_RESOURCE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_POINT_MESH_RENDER_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"

typedef struct point_mesh_render_resource point_mesh_render_resource_t;
typedef struct linear_alloc linear_alloc_t;
typedef struct point_mesh_shader_config point_mesh_shader_config_t;
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct point_vertex point_vertex_t;
typedef struct mat4x4f mat4x4f_t;

render_resource_result_t point_mesh_render_resource_initialize(const point_mesh_shader_config_t* shader_config_, size_t max_geometry_count_, renderer_backend_context_t* renderer_backend_context_, linear_alloc_t* allocator_, const char* executable_directory_, const char* shader_dir_, point_mesh_render_resource_t** out_render_resource_);

void point_mesh_render_resource_deinitialize(point_mesh_render_resource_t* render_resource_);

// Geometry resource operation
render_resource_result_t point_mesh_render_resource_geometry_import_from_vertices(point_mesh_render_resource_t* render_resource_, const char* resource_name_, const point_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_);

render_resource_result_t point_mesh_render_resource_geometry_release(point_mesh_render_resource_t* render_resource_, uint16_t geometry_id_);

// Frame-global state
render_resource_result_t point_mesh_render_resource_view_matrix_set(point_mesh_render_resource_t* render_resource_, const mat4x4f_t* view_matrix_);

render_resource_result_t point_mesh_render_resource_projection_matrix_set(point_mesh_render_resource_t* render_resource_, const mat4x4f_t* projection_matrix_);

// Draw
render_resource_result_t point_mesh_render_resource_draw(point_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, const mat4x4f_t* model_matrix_);

// Canonical validator
bool point_mesh_render_resource_is_valid(const point_mesh_render_resource_t* render_resource_);

#ifdef __cplusplus
}
#endif
#endif
