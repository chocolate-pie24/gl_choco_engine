// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UI_TEXT_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UI_TEXT_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct ui_text_mesh_shader ui_text_mesh_shader_t;

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */
typedef struct renderer_backend_vbo renderer_backend_vbo_t;
typedef struct mat4x4f mat4x4f_t;
typedef struct vec4u8 vec4u8_t;

shader_result_t ui_text_mesh_shader_create(const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_, renderer_backend_context_t* backend_context_, const renderer_backend_vbo_t* vbo_, ui_text_mesh_shader_t** out_shader_);

void ui_text_mesh_shader_destroy(renderer_backend_context_t* backend_context_, ui_text_mesh_shader_t** shader_);

shader_result_t ui_text_mesh_shader_vao_bind(const renderer_backend_context_t* backend_context_, const ui_text_mesh_shader_t* shader_);

shader_result_t ui_text_mesh_shader_use(const renderer_backend_context_t* backend_context_, const ui_text_mesh_shader_t* shader_);

shader_result_t ui_text_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const ui_text_mesh_shader_t* shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

shader_result_t ui_text_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const ui_text_mesh_shader_t* shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

shader_result_t ui_text_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const ui_text_mesh_shader_t* shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

shader_result_t ui_text_mesh_shader_text_color_set(const renderer_backend_context_t* backend_context_, const ui_text_mesh_shader_t* shader_, const vec4u8_t* text_color_);

bool ui_text_mesh_shader_is_valid(const renderer_backend_context_t* backend_context_, const ui_text_mesh_shader_t* shader_);

#ifdef __cplusplus
}
#endif
#endif
