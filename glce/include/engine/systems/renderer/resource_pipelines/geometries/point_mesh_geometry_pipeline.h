/** @ingroup renderer
 *
 * @file point_mesh_geometry_pipeline.h
 * @author chocolate-pie24
 *
 * @brief 点描画用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIを提供する
 *
 * @note 当面は頂点情報を入力するが、将来的にはpcdファイルのロード機能を有するAPIを追加する
 * @note 本pipelineはCPU側ジオメトリリソース生成、shader resourceへの頂点転送、geometry registryへの登録を一連の手順として実行する
 * @note GPU頂点バッファ自体はshader resourceが所有し、本pipelineは所有しない
 *
 * @version 0.1
 * @date 2026-06-30
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_POINT_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_POINT_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;         /**< Renderer Backend Contextのopaque型 */
typedef struct point_mesh_shader point_mesh_shader_t;                       /**< 点描画用シェーダーリソースのopaque型 */
typedef struct point_mesh_geometry_registry point_mesh_geometry_registry_t; /**< 点描画用ジオメトリレジストリのopaque型 */
typedef struct point_vertex point_vertex_t;

/**
 * @brief 点描画用頂点データを元にジオメトリの生成、GPU頂点バッファへの転送、描画範囲のレジストリ登録を行う
 *
 * @note VBOへのappend成功後の後続処理で失敗した場合、shader_に追加された頂点データは巻き戻されない
 *
 * @param[in] backend_context_ Renderer Backend Context構造体インスタンスへのポインタ
 * @param[in,out] shader_ 点描画用シェーダーリソース構造体インスタンスへのポインタ
 * @param[in,out] geometry_registry_ 点描画用ジオメトリレジストリ構造体インスタンスへのポインタ
 * @param[in] name_ ジオメトリ名称
 * @param[in] vertices_ ジオメトリ頂点配列
 * @param[in] vertex_count_ ジオメトリ頂点数
 * @param[out] out_geometry_id_ ジオメトリレジストリ内でのジオメトリ識別子
 *
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - shader_ == NULL
 * - geometry_registry_ == NULL
 * - name_ == NULL
 * - name_が空文字列
 * - out_geometry_id_ == NULL
 * - vertices_ == NULL
 * - vertex_count_ == 0
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED 以下のいずれか
 * - メモリシステム使用可能範囲上限超過
 * - バーテックスバッファサイズが規定範囲を超過
 * - geometry_registry_に空きスロットが見つからない
 * @retval RESOURCE_PIPELINE_BAD_OPERATION 以下のいずれか
 * - メモリシステム未初期化
 * - バーテックスバッファ未初期化
 * - Renderer Backend Contextが未初期化
 * - name_のジオメトリがすでにgeometry_registry_に登録済み
 * @retval RESOURCE_PIPELINE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_PIPELINE_OVERFLOW 処理過程でオーバーフローが発生
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED 以下のいずれか
 * - geometry_registry_の内部データ不整合が発生
 * - 生成したpoint_mesh_geometry_tの内部データ不整合が発生
 * @retval RESOURCE_PIPELINE_SUCCESS 処理に成功し、正常終了
 */
resource_pipeline_result_t point_mesh_geometry_pipeline_import_from_vertices(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* shader_, point_mesh_geometry_registry_t* geometry_registry_, const char* name_, const point_vertex_t* vertices_, size_t vertex_count_, int16_t* out_geometry_id_);

resource_pipeline_result_t point_mesh_geometry_pipeline_release(point_mesh_shader_t* shader_, point_mesh_geometry_registry_t* geometry_registry_, int16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
