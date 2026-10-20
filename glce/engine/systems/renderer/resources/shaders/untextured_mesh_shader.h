// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file untextured_mesh_shader.h
 * @author chocolate-pie24
 * @brief 光源・法線・材質色などを使って、陰影付きでmeshを描画するためのシェーダーであるuntextured_meshシェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIを提供する
 *
 * @note ライティング等がまだ未実装なので、当面は単色描画となる
 * @todo ライティング等の実装後、以下をメンテナンスする
 * - docs/layer.md
 * - docs/architecture/systems/renderer_system/renderer_system_ja(en).md
 *
 * @date 2026-06-03
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UNTEXTURED_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UNTEXTURED_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct untextured_mesh_shader untextured_mesh_shader_t;                   /**< 単色ライティング描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */
typedef struct vbo_manager vbo_manager_t;
typedef struct mat4x4f mat4x4f_t;
typedef struct point_normal_vertex point_normal_vertex_t;
typedef struct untextured_material untextured_material_t;

shader_result_t untextured_mesh_shader_create(const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_, renderer_backend_context_t* backend_context_, vbo_manager_t* vbo_manager_, untextured_mesh_shader_t** out_shader_);

void untextured_mesh_shader_destroy(untextured_mesh_shader_t** shader_);

shader_result_t untextured_mesh_shader_vao_bind(const untextured_mesh_shader_t* shader_);

shader_result_t untextured_mesh_shader_use(const untextured_mesh_shader_t* shader_);

shader_result_t untextured_mesh_shader_model_matrix_set(const untextured_mesh_shader_t* shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

shader_result_t untextured_mesh_shader_view_matrix_set(const untextured_mesh_shader_t* shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

shader_result_t untextured_mesh_shader_projection_matrix_set(const untextured_mesh_shader_t* shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

// direction_の座標系はview座標系
shader_result_t untextured_mesh_shader_directional_light_direction_set(const untextured_mesh_shader_t* shader_, vec3f_t direction_);

shader_result_t untextured_mesh_shader_directional_light_color_intensity_set(const untextured_mesh_shader_t* shader_, vec3f_t color_, float intensity_);

shader_result_t untextured_mesh_shader_ambient_light_color_intensity_set(const untextured_mesh_shader_t* shader_, vec3f_t color_, float intensity_);

shader_result_t untextured_mesh_shader_material_set(const untextured_mesh_shader_t* shader_, const untextured_material_t* material_);

render_resource_result_t untextured_mesh_render_resource_draw(untextured_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, uint16_t material_id_, const mat4x4f_t* model_matrix_);

#ifdef __cplusplus
}
#endif
#endif
