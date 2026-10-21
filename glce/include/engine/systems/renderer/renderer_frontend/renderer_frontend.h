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

/**
 * @brief Untextured Mesh用VBO Managerの生成設定
 *
 * @details
 * - vbo_size: VBOの容量。
 * - max_allocation_count: 管理可能なAllocationの最大数。
 * - base_align: VBO Allocationの基準Alignment。
 * - buffer_usage: VBOのBuffer Usage。
 *
 * 各設定値の成立条件は、下位VBO Managerの生成契約に従う。
 * TODO: 各種config validator
 */
typedef struct renderer_frontend_vbo_config {
    size_t vbo_size;
    size_t max_allocation_count;
    size_t base_align;
    buffer_usage_t buffer_usage;
} renderer_frontend_vbo_config_t;

/**
 * @brief Untextured Mesh描画基盤の生成設定
 *
 * @details
 * VBO Managerの設定と、Geometry RegistryおよびMaterial Registryの
 * 最大登録数を指定する。
 * TODO: xxx_meshのuse / not useを選択可能にするのを検討する
 */
typedef struct renderer_frontend_untextured_mesh_config {
    renderer_frontend_vbo_config_t vbo;
    size_t max_geometry_count;
    size_t max_material_count;
} renderer_frontend_untextured_mesh_config_t;

/**
 * @brief Renderer Frontendの生成設定
 *
 * @details
 * 現在はUntextured Mesh描画基盤の設定のみを保持する。
 */
typedef struct renderer_frontend_config {
    renderer_frontend_untextured_mesh_config_t untextured_mesh;
} renderer_frontend_config_t;

renderer_frontend_result_t renderer_frontend_create(
    const renderer_frontend_config_t* config_,
    subsystem_allocator_t* allocator_,
    const char* executable_directory_,
    const char* shader_dir_,
    renderer_frontend_t** out_renderer_frontend_);

void renderer_frontend_deinitialize(renderer_frontend_t* renderer_frontend_);

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
renderer_frontend_result_t renderer_frontend_untextured_mesh_draw(renderer_frontend_t* renderer_frontend_, uint16_t geometry_id_, uint16_t material_id_, const mat4x4f_t* model_matrix_);

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
renderer_frontend_result_t renderer_frontend_untextured_mesh_render_update(renderer_frontend_t* renderer_frontend_, const mat4x4f_t* view_matrix_, const mat4x4f_t* projection_matrix_, const directional_light_t* directional_light_, const ambient_light_t* ambient_light_);

// Geometry Import
/**
 * @brief FileからUntextured Mesh Geometryを読み込み、登録する
 *
 * @details
 * Geometry Import Pipelineを使用してFileを読み込み、Geometryを生成する。
 * Vertex DataのVBOへの転送とGeometry Registryへの登録を行う。
 *
 * 成功時は登録されたGeometry IDを出力する。
 * 失敗時は出力先を変更しない。
 * 入力文字列は呼び出し中のみborrowする。
 *
 * @param[in,out] renderer_frontend_ Import先のFrontend
 * @param[in] resource_name_ Geometryの登録名
 * @param[in] resource_fullpath_ Geometry FileのFull Path
 * @param[out] out_geometry_id_ 登録されたGeometry IDの出力先
 *
 * @retval RENDERER_FRONTEND_SUCCESS Import成功
 * @retval RENDERER_FRONTEND_INVALID_ARGUMENT 引数不正
 * @return その他、Geometry Import Pipelineの失敗を変換したResult
 */
renderer_frontend_result_t renderer_frontend_untextured_mesh_geometry_import_from_file(renderer_frontend_t* renderer_frontend_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

// Geometry Release
/**
 * @brief 登録済みUntextured Mesh GeometryをReleaseする
 *
 * @details
 * Geometry Release Pipelineを使用して、指定IDに対応するGeometryの
 * Registry登録解除とVBO Allocationの解放を行う。
 *
 * 現在のReleaseはReference Countingに基づく参照解放ではなく、
 * 登録済みResourceの解除操作である。
 *
 * @param[in,out] renderer_frontend_ 対象Frontend
 * @param[in] geometry_id_ Release対象のGeometry ID
 *
 * @retval RENDERER_FRONTEND_SUCCESS Release成功
 * @retval RENDERER_FRONTEND_INVALID_ARGUMENT 引数不正
 * @return その他、Geometry Release Pipelineの失敗を変換したResult
 */
renderer_frontend_result_t renderer_frontend_untextured_mesh_geometry_release(renderer_frontend_t* renderer_frontend_, uint16_t geometry_id_);

// Material Import
/**
 * @brief FileからUntextured Materialを読み込み、登録する
 *
 * @details
 * Material Import Pipelineを使用してFileを読み込み、
 * Material Registryへ登録する。
 *
 * 成功時は登録されたMaterial IDを出力する。
 * 失敗時は出力先を変更しない。
 * 入力文字列は呼び出し中のみborrowする。
 *
 * @param[in,out] renderer_frontend_ Import先のFrontend
 * @param[in] resource_name_ Materialの登録名
 * @param[in] resource_fullpath_ Material FileのFull Path
 * @param[out] out_material_id_ 登録されたMaterial IDの出力先
 *
 * @retval RENDERER_FRONTEND_SUCCESS Import成功
 * @retval RENDERER_FRONTEND_INVALID_ARGUMENT 引数不正
 * @return その他、Material Import Pipelineの失敗を変換したResult
 */
renderer_frontend_result_t renderer_frontend_untextured_mesh_material_import_from_file(renderer_frontend_t* renderer_frontend_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_);

// Material Release
/**
 * @brief 登録済みUntextured MaterialをReleaseする
 *
 * @details
 * Material Release Pipelineを使用して、
 * 指定IDに対応するMaterialをRegistryから登録解除する。
 *
 * 現在のReleaseはReference Countingに基づく参照解放ではなく、
 * 登録済みResourceの解除操作である。
 *
 * @param[in,out] renderer_frontend_ 対象Frontend
 * @param[in] material_id_ Release対象のMaterial ID
 *
 * @retval RENDERER_FRONTEND_SUCCESS Release成功
 * @retval RENDERER_FRONTEND_INVALID_ARGUMENT 引数不正
 * @return その他、Material Release Pipelineの失敗を変換したResult
 */
renderer_frontend_result_t renderer_frontend_untextured_mesh_material_release(renderer_frontend_t* renderer_frontend_, uint16_t material_id_);

#ifdef __cplusplus
}
#endif
#endif
