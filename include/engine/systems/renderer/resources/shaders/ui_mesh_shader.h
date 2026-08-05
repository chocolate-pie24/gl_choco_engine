/** @ingroup renderer
 *
 * @file ui_mesh_shader.h
 * @author chocolate-pie24
 * @brief UIシェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIを提供する
 *
 * @version 0.1
 * @date 2026-03-11
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UI_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_UI_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct ui_mesh_shader ui_mesh_shader_t;                     /**< UI描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

typedef struct mat4x4f mat4x4f_t;
typedef struct ui_vertex ui_vertex_t;

shader_result_t ui_mesh_shader_create(ui_mesh_shader_t** out_ui_mesh_shader_);

void ui_mesh_shader_destroy(renderer_backend_context_t* backend_context_, ui_mesh_shader_t** ui_mesh_shader_);

shader_result_t ui_mesh_shader_program_initialize(renderer_backend_context_t* backend_context_, ui_mesh_shader_t* ui_mesh_shader_, const char* file_path_, const char* name_);

shader_result_t ui_mesh_shader_vbo_initialize(renderer_backend_context_t* backend_context_, ui_mesh_shader_t* ui_mesh_shader_, const vbo_manager_config_t* vbo_config_);

shader_result_t ui_mesh_shader_vao_initialize(renderer_backend_context_t* backend_context_, ui_mesh_shader_t* ui_mesh_shader_);

void ui_mesh_shader_vao_vbo_destroy(renderer_backend_context_t* backend_context_, ui_mesh_shader_t* ui_mesh_shader_);

shader_result_t ui_mesh_shader_vbo_write(const renderer_backend_context_t* backend_context_, ui_mesh_shader_t* ui_mesh_shader_, size_t vertex_count_, const ui_vertex_t* vertices_, vertex_buffer_range_t* out_buffer_range_);

shader_result_t ui_mesh_shader_vbo_free(ui_mesh_shader_t* ui_mesh_shader_, const vertex_buffer_range_t* buffer_range_);

shader_result_t ui_mesh_shader_vertex_array_bind(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_);

shader_result_t ui_mesh_shader_use(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_);

shader_result_t ui_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

shader_result_t ui_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

shader_result_t ui_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

#ifdef __cplusplus
}
#endif
#endif
