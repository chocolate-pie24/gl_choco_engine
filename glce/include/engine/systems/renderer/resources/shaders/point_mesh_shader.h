// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file point_mesh_shader.h
 * @author chocolate-pie24
 * @brief ポイント描画用シェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIを提供する
 *
 * @date 2026-05-29
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_POINT_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_POINT_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct point_mesh_shader point_mesh_shader_t;               /**< 点描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

typedef struct mat4x4f mat4x4f_t;
typedef struct point_vertex point_vertex_t;
typedef struct point_mesh_shader_config point_mesh_shader_config_t;

shader_result_t point_mesh_shader_create(renderer_backend_context_t* backend_context_, const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_, const point_mesh_shader_config_t* config_, point_mesh_shader_t** out_point_mesh_shader_);

void point_mesh_shader_destroy(point_mesh_shader_t** point_mesh_shader_);

shader_result_t point_mesh_shader_vbo_write(point_mesh_shader_t* point_mesh_shader_, size_t vertex_count_, const point_vertex_t* vertices_, vbo_range_t* out_buffer_range_);

shader_result_t point_mesh_shader_vbo_free(point_mesh_shader_t* point_mesh_shader_, const vbo_range_t* buffer_range_);

shader_result_t point_mesh_shader_vao_bind(const point_mesh_shader_t* point_mesh_shader_);

shader_result_t point_mesh_shader_use(const point_mesh_shader_t* point_mesh_shader_);

shader_result_t point_mesh_shader_model_matrix_set(const point_mesh_shader_t* point_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

shader_result_t point_mesh_shader_view_matrix_set(const point_mesh_shader_t* point_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

shader_result_t point_mesh_shader_projection_matrix_set(const point_mesh_shader_t* point_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

#ifdef __cplusplus
}
#endif
#endif
