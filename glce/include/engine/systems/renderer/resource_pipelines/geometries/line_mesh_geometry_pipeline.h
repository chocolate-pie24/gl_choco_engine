// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file line_mesh_geometry_pipeline.h
 * @author chocolate-pie24
 *
 * @brief line_mesh用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIを提供する
 *
 * @note 本pipelineはCPU側ジオメトリリソース生成、shader resourceへの頂点転送、geometry registryへの登録を一連の手順として実行する
 * @note GPU頂点バッファ自体はshader resourceが所有し、本pipelineは所有しない
 *
 * @date 2026-06-30
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_LINE_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_LINE_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;         /**< Renderer Backend Contextのopaque型 */
typedef struct line_mesh_shader line_mesh_shader_t;                         /**< 線分描画用シェーダーリソースのopaque型 */
typedef struct line_mesh_geometry_registry line_mesh_geometry_registry_t;   /**< 線分描画用ジオメトリレジストリのopaque型 */
typedef struct line_vertex line_vertex_t;
typedef struct aabb_3d aabb_3d_t;

/**
 * @brief 線分描画用頂点データを元にジオメトリの生成、GPU頂点バッファへの転送、描画範囲のレジストリ登録を行う
 *
 * @param[in] backend_context_ Renderer Backend Context構造体インスタンスへのポインタ
 * @param[in,out] shader_ 線分描画用シェーダーリソース構造体インスタンスへのポインタ
 * @param[in,out] geometry_registry_ 線分描画用ジオメトリレジストリ構造体インスタンスへのポインタ
 * @param[in] name_ ジオメトリ名称
 * @param[in] vertices_ ジオメトリ頂点配列
 * @param[in] vertex_count_ ジオメトリ頂点数(1線分あたり2頂点, 必ず2の倍数になる)
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
 * - vertex_count_が2の倍数ではない
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
 * - 生成したline_mesh_geometry_tの内部データ不整合が発生
 * @retval RESOURCE_PIPELINE_SUCCESS 処理に成功し、正常終了
 */
resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_vertices(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* name_, const line_vertex_t* vertices_, size_t vertex_count_, int16_t* out_geometry_id_);

/**
 * @brief AABB(3D)を使用して線分描画用ジオメトリの生成、GPU頂点バッファへの転送、描画範囲のレジストリ登録を行う
 *
 * @param[in] backend_context_ Renderer Backend Context構造体インスタンスへのポインタ
 * @param[in,out] shader_ 線分描画用シェーダーリソース構造体インスタンスへのポインタ
 * @param[in,out] geometry_registry_ 線分描画用ジオメトリレジストリ構造体インスタンスへのポインタ
 * @param[in] name_ ジオメトリ名称
 * @param[in] aabb_ 線分ジオメトリに変換するAABB(3D)
 * @param[out] out_geometry_id_ ジオメトリレジストリ内でのジオメトリ識別子
 *
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - shader_ == NULL
 * - geometry_registry_ == NULL
 * - name_ == NULL
 * - name_が空文字列
 * - out_geometry_id_ == NULL
 * - aabb_ == NULL
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED 以下のいずれか
 * - メモリシステム使用可能範囲上限超過
 * - バーテックスバッファサイズが規定範囲を超過
 * - geometry_registry_に空きスロットが見つからない
 * @retval RESOURCE_PIPELINE_BAD_OPERATION 以下のいずれか
 * - メモリシステム未初期化
 * - バーテックスバッファ未初期化
 * - aabb_が不正
 * - Renderer Backend Contextが未初期化
 * - name_のジオメトリがすでにgeometry_registry_に登録済み
 * - 何らかの理由で生成したgeometryが未初期化となり、後続APIに渡った
 * @retval RESOURCE_PIPELINE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_PIPELINE_OVERFLOW 処理過程でオーバーフローが発生
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED 以下のいずれか
 * - geometry_registry_の内部データ不整合が発生
 * - 生成したline_mesh_geometry_tの内部データ不整合が発生
 * @retval RESOURCE_PIPELINE_SUCCESS 処理に成功し、正常終了
 */
resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_aabb(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* name_, const aabb_3d_t* aabb_, int16_t* out_geometry_id_);

resource_pipeline_result_t line_mesh_geometry_pipeline_release(line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, int16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
