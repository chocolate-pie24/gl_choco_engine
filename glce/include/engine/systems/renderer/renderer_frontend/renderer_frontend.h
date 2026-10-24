// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_FRONTEND_RENDERER_FRONTEND_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_FRONTEND_RENDERER_FRONTEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

typedef struct renderer_frontend renderer_frontend_t;

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct directional_light directional_light_t;
typedef struct ambient_light ambient_light_t;
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct resource_manager resource_manager_t;

renderer_frontend_result_t renderer_frontend_create(subsystem_allocator_t* allocator_, const char* executable_directory_, const char* shader_dir_, renderer_backend_context_t* backend_context_, resource_manager_t* resource_manager_, renderer_frontend_t** out_renderer_frontend_);

void renderer_frontend_deinitialize(renderer_backend_context_t* backend_context_, renderer_frontend_t* renderer_frontend_);

//////////////////////////////////////////////////////////////////////////////////////////
// for Untexture Mesh
//////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 登録済みのUntextured Meshを描画する
 *
 * @details
 * Geometry IDからDraw Range、Material IDからMaterialを取得する。
 * ShaderにMaterialとModel Matrixを設定し、指定Geometryを描画する。
 *
 * model_matrix_は呼び出し中のみborrowする。
 * GeometryとMaterialの登録状態および所有権は変更しない。
 *
 * @param[in,out] renderer_frontend_ 描画に使用するFrontend
 * @param[in] geometry_id_ 登録済みGeometry ID
 * @param[in] material_id_ 登録済みMaterial ID
 * @param[in] model_matrix_ 描画対象のModel Matrix
 *
 * @retval RENDERER_FRONTEND_SUCCESS 描画成功
 * @retval RENDERER_FRONTEND_INVALID_ARGUMENT 引数または指定IDが不正
 * @retval RENDERER_FRONTEND_LIMIT_EXCEEDED Draw Rangeが描画APIの値域を超過
 * @retval RENDERER_FRONTEND_OVERFLOW Draw Rangeの算術計算でoverflowを検出
 * @return その他、Shader操作の失敗を変換したResult
 */
renderer_frontend_result_t renderer_frontend_untextured_mesh_draw(const resource_manager_t* resource_manager_, const renderer_backend_context_t* backend_context_, renderer_frontend_t* renderer_frontend_, uint16_t geometry_id_, uint16_t material_id_, const mat4x4f_t* model_matrix_);

/**
 * @brief Untextured Mesh描画用のView / Projection MatrixおよびLightを更新する
 *
 * @details
 * 指定された行列とLightの情報をShaderへ設定する。
 * Directional Lightの方向はView座標系へ変換して設定する。
 *
 * 入力された行列とLightは呼び出し中のみborrowし、所有権を取得しない。
 * 更新途中で失敗した場合、先行するShader Stateの変更は残り得る。
 *
 * @param[in,out] renderer_frontend_ 更新対象のFrontend
 * @param[in] view_matrix_ View Matrix
 * @param[in] projection_matrix_ Projection Matrix
 * @param[in] directional_light_ Directional Light
 * @param[in] ambient_light_ Ambient Light
 *
 * @retval RENDERER_FRONTEND_SUCCESS 更新成功
 * @retval RENDERER_FRONTEND_INVALID_ARGUMENT 引数不正
 * @return その他、Shader操作の失敗を変換したResult
 */
renderer_frontend_result_t renderer_frontend_untextured_mesh_render_update(const renderer_backend_context_t* backend_context_, renderer_frontend_t* renderer_frontend_, const mat4x4f_t* view_matrix_, const mat4x4f_t* projection_matrix_, const directional_light_t* directional_light_, const ambient_light_t* ambient_light_);

#ifdef __cplusplus
}
#endif
#endif
