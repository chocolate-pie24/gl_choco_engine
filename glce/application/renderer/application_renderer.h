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

#include "application/core/application_types.h"

typedef struct linear_alloc linear_alloc_t;
typedef struct renderer_config renderer_config_t;
typedef struct application_renderer application_renderer_t;
typedef struct mat4x4f mat4x4f_t;

typedef enum {
    APPLICATION_RENDERER_SHADER_TYPE_LINE_MESH = 0,
    APPLICATION_RENDERER_SHADER_TYPE_LIT_MESH,
    APPLICATION_RENDERER_SHADER_TYPE_POINT_MESH,
    APPLICATION_RENDERER_SHADER_TYPE_UI_MESH,
} application_renderer_shader_type_t;

application_result_t application_renderer_initialize(const renderer_config_t* renderer_config_, target_graphics_api_t target_api_, linear_alloc_t* allocator_, const char* executable_directory_, const char* shader_dir_, application_renderer_t** out_application_renderer_);

void application_renderer_deinitialize(application_renderer_t* application_renderer_);

application_result_t application_renderer_update(application_renderer_t* application_renderer_, bool view_dirty_, bool projection_dirty_, const mat4x4f_t* view_matrix_, const mat4x4f_t* projection_matrix_, bool should_transpose_view_matrix_, bool should_transpose_projection_matrix_);

application_result_t application_renderer_shader_use(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_);

application_result_t application_renderer_vao_bind(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_);

application_result_t application_renderer_vao_unbind(application_renderer_t* application_renderer_);

application_result_t application_renderer_model_matrix_set(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_, const mat4x4f_t* model_matrix_, bool should_transpose_);

application_result_t application_renderer_view_matrix_set(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_, const mat4x4f_t* view_matrix_, bool should_transpose_);

application_result_t application_renderer_projection_matrix_set(application_renderer_t* application_renderer_, application_renderer_shader_type_t shader_type_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

application_result_t application_renderer_line_mesh_color_set(application_renderer_t* application_renderer_, const uint8_t color_[4]);

// begin temporary: TODO: REMOVE THIS!!
// shader + registryの上位モジュールができるまでの暫定API
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct line_mesh_shader line_mesh_shader_t;
typedef struct lit_mesh_shader lit_mesh_shader_t;
typedef struct point_mesh_shader point_mesh_shader_t;
typedef struct ui_mesh_shader ui_mesh_shader_t;

renderer_backend_context_t* application_renderer_renderer_backend_context_get(application_renderer_t* application_renderer_);
line_mesh_shader_t* application_renderer_line_mesh_shader_get(application_renderer_t* application_renderer_);
lit_mesh_shader_t* application_renderer_lit_mesh_shader_get(application_renderer_t* application_renderer_);
point_mesh_shader_t* application_renderer_point_mesh_shader_get(application_renderer_t* application_renderer_);
ui_mesh_shader_t* application_renderer_ui_mesh_shader_get(application_renderer_t* application_renderer_);
// end temporary

bool application_renderer_shader_type_is_valid(application_renderer_shader_type_t shader_type_);

bool application_renderer_is_valid(const application_renderer_t* application_renderer_);

#ifdef __cplusplus
}
#endif
#endif
