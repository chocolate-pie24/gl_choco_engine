// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file lit_mesh_shader.h
 * @author chocolate-pie24
 * @brief 光源・法線・材質色などを使って、陰影付きでmeshを描画するためのシェーダーであるlit_meshシェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIを提供する
 *
 * @note ライティング等がまだ未実装なので、当面は単色描画となる
 * @todo ライティング等の実装後、以下をメンテナンスする
 * - docs/layer.md
 * - docs/architecture/systems/renderer_system/renderer_system_ja(en).md
 *
 * @date 2026-06-03
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_LIT_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_LIT_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct lit_mesh_shader lit_mesh_shader_t;                   /**< 単色ライティング描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

typedef struct mat4x4f mat4x4f_t;
typedef struct point_normal_vertex point_normal_vertex_t;
typedef struct lit_mesh_shader_config lit_mesh_shader_config_t;

shader_result_t lit_mesh_shader_create(lit_mesh_shader_t** out_lit_mesh_shader_);

void lit_mesh_shader_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t** lit_mesh_shader_);

shader_result_t lit_mesh_shader_program_initialize(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_);

shader_result_t lit_mesh_shader_vbo_initialize(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, const lit_mesh_shader_config_t* config_);

shader_result_t lit_mesh_shader_vao_initialize(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_);

void lit_mesh_shader_vao_vbo_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_);

shader_result_t lit_mesh_shader_vbo_write(const renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, size_t vertex_count_, const point_normal_vertex_t* vertices_, vbo_range_t* out_buffer_range_);

shader_result_t lit_mesh_shader_vbo_free(lit_mesh_shader_t* lit_mesh_shader_, const vbo_range_t* buffer_range_);

shader_result_t lit_mesh_shader_vao_bind(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_);

shader_result_t lit_mesh_shader_use(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_);

shader_result_t lit_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

shader_result_t lit_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

shader_result_t lit_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

#ifdef __cplusplus
}
#endif
#endif
