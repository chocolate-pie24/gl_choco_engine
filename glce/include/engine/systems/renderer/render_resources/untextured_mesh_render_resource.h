// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_UNTEXTURED_MESH_RENDER_RESOURCE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_UNTEXTURED_MESH_RENDER_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"

typedef struct untextured_mesh_render_resource untextured_mesh_render_resource_t;

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct untextured_mesh_shader_config untextured_mesh_shader_config_t;
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct mat4x4f mat4x4f_t;
typedef struct aabb_3d aabb_3d_t;

render_resource_result_t untextured_mesh_render_resource_create(const untextured_mesh_shader_config_t* config_, size_t max_geometry_count_, size_t max_material_count_, renderer_backend_context_t* backend_context_, subsystem_allocator_t* allocator_, const char* executable_directory_, const char* shader_dir_, untextured_mesh_render_resource_t** out_render_resource_);

void untextured_mesh_render_resource_deinitialize(untextured_mesh_render_resource_t* render_resource_);

// Geometry resource operation
render_resource_result_t untextured_mesh_render_resource_geometry_import_from_file(untextured_mesh_render_resource_t* render_resource_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

render_resource_result_t untextured_mesh_render_resource_geometry_release(untextured_mesh_render_resource_t* render_resource_, uint16_t geometry_id_);

// Material resource operation
render_resource_result_t untextured_mesh_render_resource_material_import_from_file(untextured_mesh_render_resource_t* render_resource_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_);

render_resource_result_t untextured_mesh_render_resource_material_release(untextured_mesh_render_resource_t* render_resource_, uint16_t material_id_);

// Frame-global state
render_resource_result_t untextured_mesh_render_resource_view_matrix_set(untextured_mesh_render_resource_t* render_resource_, const mat4x4f_t* view_matrix_);

render_resource_result_t untextured_mesh_render_resource_projection_matrix_set(untextured_mesh_render_resource_t* render_resource_, const mat4x4f_t* projection_matrix_);

// direction_の座標系はview座標系
render_resource_result_t untextured_mesh_render_resource_directional_light_direction_set(untextured_mesh_render_resource_t* render_resource_, vec3f_t direction_);

render_resource_result_t untextured_mesh_render_resource_directional_light_color_intensity_set(untextured_mesh_render_resource_t* render_resource_, vec3f_t color_, float intensity_);

render_resource_result_t untextured_mesh_render_resource_ambient_light_color_intensity_set(untextured_mesh_render_resource_t* render_resource_, vec3f_t color_, float intensity_);

// Draw
render_resource_result_t untextured_mesh_render_resource_draw(untextured_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, uint16_t material_id_, const mat4x4f_t* model_matrix_);

// Utility
render_resource_result_t untextured_mesh_render_resource_geometry_convert_to_aabb_3d(untextured_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, aabb_3d_t* out_aabb_3d_);

// Canonical validator
bool untextured_mesh_render_resource_is_valid(const untextured_mesh_render_resource_t* render_resource_);

#ifdef __cplusplus
}
#endif
#endif
