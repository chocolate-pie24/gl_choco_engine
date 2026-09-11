// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

// NOTE: このモジュールは将来的にRenderer Frontendに移す
// TODO: meshが完成し、render_packetが完成したらbegin_frame, draw_frame, end_frameを追加する、その後、shader_useやvao_bind、vao_unbindは削除する
#ifndef GLCE_APPLICATION_RENDERER_APPLICATION_RENDERER_H
#define GLCE_APPLICATION_RENDERER_APPLICATION_RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "application/core/application_types.h"

typedef struct linear_alloc linear_alloc_t;
typedef struct renderer_config renderer_config_t;
typedef struct application_renderer application_renderer_t;
typedef struct mat4x4f mat4x4f_t;
typedef struct line_vertex line_vertex_t;
typedef struct aabb_3d aabb_3d_t;
typedef struct point_vertex point_vertex_t;

application_result_t application_renderer_initialize(const renderer_config_t* renderer_config_, target_graphics_api_t target_api_, linear_alloc_t* allocator_, const char* executable_directory_, const char* shader_dir_, application_renderer_t** out_application_renderer_);

void application_renderer_deinitialize(application_renderer_t* application_renderer_);

application_result_t application_renderer_update(application_renderer_t* application_renderer_, bool view_dirty_, bool projection_dirty_, const mat4x4f_t* view_matrix_, const mat4x4f_t* projection_matrix_);

// Geometry Import
application_result_t application_renderer_line_mesh_geometry_import_from_vertices(application_renderer_t* application_renderer_, const char* resource_name_, const line_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_);

application_result_t application_renderer_line_mesh_geometry_import_from_aabb(application_renderer_t* application_renderer_, const char* resource_name_, const aabb_3d_t* aabb_, uint16_t* out_geometry_id_);

application_result_t application_renderer_lit_mesh_geometry_import_from_file(application_renderer_t* application_renderer_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

application_result_t application_renderer_point_mesh_geometry_import_from_vertices(application_renderer_t* application_renderer_, const char* resource_name_, const point_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_);

application_result_t application_renderer_ui_mesh_geometry_import_from_file(application_renderer_t* application_renderer_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

// Geometry Release
application_result_t application_renderer_line_mesh_release(application_renderer_t* application_renderer_, uint16_t geometry_id_);

application_result_t application_renderer_lit_mesh_release(application_renderer_t* application_renderer_, uint16_t geometry_id_);

application_result_t application_renderer_point_mesh_release(application_renderer_t* application_renderer_, uint16_t geometry_id_);

application_result_t application_renderer_ui_mesh_release(application_renderer_t* application_renderer_, uint16_t geometry_id_);

// Texture Inport
application_result_t application_renderer_ui_mesh_texture_import_from_bmp(application_renderer_t* application_renderer_, int32_t gpu_unit_num_, const char* resource_name_, const char* texture_fullpath_, uint16_t* out_texture_id_);

application_result_t application_renderer_ui_mesh_texture_import_from_solid_color(application_renderer_t* application_renderer_, int32_t gpu_unit_num_, const char* resource_name_, uint8_t red_, uint8_t green_, uint8_t blue_, uint16_t* out_texture_id_);

// Texture Release
application_result_t application_renderer_ui_mesh_texture_release(application_renderer_t* application_renderer_, uint16_t texture_id_);

// Mesh Draw
application_result_t application_renderer_line_mesh_draw(application_renderer_t* application_renderer_, uint16_t geometry_id_, const mat4x4f_t* model_matrix_, const uint8_t color_[4]);

application_result_t application_renderer_lit_mesh_draw(application_renderer_t* application_renderer_, uint16_t geometry_id_, const mat4x4f_t* model_matrix_);

application_result_t application_renderer_point_mesh_draw(application_renderer_t* application_renderer_, uint16_t geometry_id_, const mat4x4f_t* model_matrix_);

application_result_t application_renderer_ui_mesh_draw(application_renderer_t* application_renderer_, uint16_t geometry_id_, uint16_t texture_id_, const mat4x4f_t* model_matrix_);

// Utility(これらはそのうち適切な場所に移す)
application_result_t application_renderer_lit_mesh_geometry_to_aabb_3d(application_renderer_t* application_renderer_, uint16_t geometry_id_, aabb_3d_t* out_aabb_3d_);

bool application_renderer_is_valid(const application_renderer_t* application_renderer_);

#ifdef __cplusplus
}
#endif
#endif
