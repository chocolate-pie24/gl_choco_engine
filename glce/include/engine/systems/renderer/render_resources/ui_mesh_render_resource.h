// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_UI_MESH_RENDER_RESOURCE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_UI_MESH_RENDER_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"

typedef struct ui_mesh_render_resource ui_mesh_render_resource_t;
typedef struct linear_alloc linear_alloc_t;
typedef struct ui_mesh_shader_config ui_mesh_shader_config_t;
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct mat4x4f mat4x4f_t;

render_resource_result_t ui_mesh_render_resource_initialize(const ui_mesh_shader_config_t* shader_config_, size_t max_geometry_count_, size_t max_texture_count_, renderer_backend_context_t* renderer_backend_context_, linear_alloc_t* allocator_, const char* executable_directory_, const char* shader_dir_, ui_mesh_render_resource_t** out_render_resource_);

void ui_mesh_render_resource_deinitialize(ui_mesh_render_resource_t* render_resource_);

// Geometry resource operation
render_resource_result_t ui_mesh_render_resource_geometry_import_from_file(ui_mesh_render_resource_t* render_resource_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

render_resource_result_t ui_mesh_render_resource_geometry_release(ui_mesh_render_resource_t* render_resource_, uint16_t geometry_id_);

// Texture resource operation
render_resource_result_t ui_mesh_render_resource_texture_import_from_bmp(ui_mesh_render_resource_t* render_resource_, int32_t gpu_unit_num_, const char* resource_name_, const char* texture_fullpath_, uint16_t* out_texture_id_);

render_resource_result_t ui_mesh_render_resource_texture_import_from_solid_color(ui_mesh_render_resource_t* render_resource_, int32_t gpu_unit_num_, const char* resource_name_, uint8_t red_, uint8_t green_, uint8_t blue_, uint16_t* out_texture_id_);

render_resource_result_t ui_mesh_render_resource_texture_release(ui_mesh_render_resource_t* render_resource_, uint16_t texture_id_);

// Frame-global state
render_resource_result_t ui_mesh_render_resource_view_matrix_set(ui_mesh_render_resource_t* render_resource_, const mat4x4f_t* view_matrix_);

render_resource_result_t ui_mesh_render_resource_projection_matrix_set(ui_mesh_render_resource_t* render_resource_, const mat4x4f_t* projection_matrix_);

// Draw
render_resource_result_t ui_mesh_render_resource_draw(ui_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, uint16_t texture_id_, const mat4x4f_t* model_matrix_);

// Canonical validator
bool ui_mesh_render_resource_is_valid(const ui_mesh_render_resource_t* render_resource_);

#ifdef __cplusplus
}
#endif
#endif
