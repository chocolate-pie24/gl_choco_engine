// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file line_mesh_shader.h
 * @author chocolate-pie24
 * @brief 線分描画用シェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIを提供する
 *
 * @date 2026-05-28
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_LINE_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_LINE_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct line_mesh_shader line_mesh_shader_t;                 /**< 線分描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

typedef struct mat4x4f mat4x4f_t;
typedef struct line_vertex line_vertex_t;
typedef struct line_mesh_shader_config line_mesh_shader_config_t;

shader_result_t line_mesh_shader_create(line_mesh_shader_t** out_line_mesh_shader_);

void line_mesh_shader_destroy(renderer_backend_context_t* backend_context_, line_mesh_shader_t** line_mesh_shader_);

shader_result_t line_mesh_shader_program_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, const char* file_path_, const char* name_);

shader_result_t line_mesh_shader_vbo_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, const line_mesh_shader_config_t* config_);

shader_result_t line_mesh_shader_vao_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_);

void line_mesh_shader_vao_vbo_destroy(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_);

shader_result_t line_mesh_shader_vbo_write(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, size_t vertex_count_, const line_vertex_t* vertices_, vbo_range_t* out_buffer_range_);

shader_result_t line_mesh_shader_vbo_free(line_mesh_shader_t* line_mesh_shader_, const vbo_range_t* buffer_range_);

shader_result_t line_mesh_shader_vao_bind(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_);

shader_result_t line_mesh_shader_use(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_);

shader_result_t line_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

shader_result_t line_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

shader_result_t line_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

shader_result_t line_mesh_shader_color_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const uint8_t color_[4]);

#ifdef __cplusplus
}
#endif
#endif
