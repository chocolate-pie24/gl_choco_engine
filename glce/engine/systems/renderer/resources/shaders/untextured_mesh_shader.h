// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file untextured_mesh_shader.h
 * @author chocolate-pie24
 * @brief 法線・光源・材質を使用するUntextured Mesh描画用Shader ResourceのAPIを提供する
 *
 * @details
 * Shader ProgramとVAOを所有し、Model / View / Projection Matrix、
 * LightおよびMaterialのUniform設定機能を提供する。
 *
 * Module Boundary Contract:
 * - Shader ResourceはShader ProgramとVAOを所有する。
 * - VAOは生成時に指定されたVBOとのBinding関係を保持するため、
 *   Shaderを使用した描画中は対応するVBOが有効である必要がある。
 * - Shaderの破棄はShader Program、VAOおよびShader Resource自身を解放する。
 * - Renderer Backend ContextはShader Resourceの破棄が完了するまで有効である必要がある。
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

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

typedef struct untextured_mesh_shader untextured_mesh_shader_t;                   /**< 単色ライティング描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */
typedef struct vbo_manager vbo_manager_t;
typedef struct mat4x4f mat4x4f_t;
typedef struct untextured_mesh_material untextured_mesh_material_t;

shader_result_t untextured_mesh_shader_create(const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_, renderer_backend_context_t* backend_context_, vbo_manager_t* vbo_manager_, untextured_mesh_shader_t** out_shader_);

void untextured_mesh_shader_destroy(renderer_backend_context_t* backend_context_, untextured_mesh_shader_t** shader_);

shader_result_t untextured_mesh_shader_vao_bind(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_);

shader_result_t untextured_mesh_shader_use(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_);

shader_result_t untextured_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

shader_result_t untextured_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

shader_result_t untextured_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

// direction_の座標系はview座標系
shader_result_t untextured_mesh_shader_directional_light_direction_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, vec3f_t direction_);

shader_result_t untextured_mesh_shader_directional_light_color_intensity_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, vec3f_t color_, float intensity_);

shader_result_t untextured_mesh_shader_ambient_light_color_intensity_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, vec3f_t color_, float intensity_);

shader_result_t untextured_mesh_shader_material_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const untextured_mesh_material_t* material_);

#ifdef __cplusplus
}
#endif
#endif
