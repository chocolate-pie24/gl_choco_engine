// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file lit_mesh_geometry_pipeline.h
 * @author chocolate-pie24
 *
 * @brief 単色ライティング描画用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIを提供する
 *
 * @note 本pipelineはCPU側ジオメトリリソース生成、shader resourceへの頂点転送、geometry registryへの登録を一連の手順として実行する
 * @note GPU頂点バッファ自体はshader resourceが所有し、本pipelineは所有しない
 *
 * @date 2026-06-30
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_LIT_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_LIT_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;     /**< Renderer Backend Contextのopaque型 */
typedef struct lit_mesh_shader lit_mesh_shader_t;                       /**< 単色ライティング描画用シェーダーリソースのopaque型 */
typedef struct lit_mesh_geometry_registry lit_mesh_geometry_registry_t; /**< 単色ライティング描画用ジオメトリレジストリのopaque型 */

/**
 * @brief 単色ライティング描画用ジオメトリアセットファイルをロードし、ジオメトリの生成、GPU頂点バッファへの転送、描画範囲のレジストリ登録を行う
 *
 * @note 現状はASCII形式のstlファイルのみをサポート
 *
 * @param[in] backend_context_ Renderer Backend Context構造体インスタンスへのポインタ
 * @param[in,out] shader_ 単色ライティング描画用シェーダーリソース構造体インスタンスへのポインタ
 * @param[in,out] geometry_registry_ 単色ライティング描画用ジオメトリレジストリ構造体インスタンスへのポインタ
 * @param[in] path_ アセットファイルのパス(空文字列許可, 空文字列ではない場合, 文字列の末尾に'/'を付加すること)
 * @param[in] name_ アセットファイル名称(拡張子、パスは含まない)
 * @param[in] extension_ アセットファイル拡張子(空文字列は禁止)
 * @param[out] out_geometry_id_ ジオメトリレジストリ内でのジオメトリ識別子
 *
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - shader_ == NULL
 * - geometry_registry_ == NULL
 * - path_ == NULL
 * - name_ == NULL
 * - name_[0] == '\0'
 * - extension_ == NULL
 * - extension_[0] == '\0'
 * - out_geometry_id_ == NULL
 * @retval RESOURCE_PIPELINE_UNSUPPORTED_FILE extension_が".stl"ではない
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED 以下のいずれか
 * - メモリシステム使用可能範囲上限超過
 * - 転送後にバーテックスバッファサイズを超過
 * - geometry_registry_に空きスロットが見つからない
 * @retval RESOURCE_PIPELINE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_PIPELINE_BAD_OPERATION 以下のいずれか
 * - メモリシステム未初期化
 * - name_のジオメトリがすでにgeometry_registry_に登録済み
 * - バーテックスバッファ未初期化
 * - Renderer Backend Contextが未初期化
 * @retval RESOURCE_PIPELINE_DATA_CORRUPTED 以下のいずれか
 * - ロードしたgeometryの頂点数が0
 * - アセットファイル内のデータが不正
 * - geometry_registry_または生成したgeometryデータに不整合が発生
 * @retval RESOURCE_PIPELINE_OVERFLOW 処理過程でオーバーフローが発生
 * @retval RESOURCE_PIPELINE_FILE_OPEN_ERROR アセットファイルのオープン失敗
 * @retval RESOURCE_PIPELINE_UNDEFINED_ERROR ファイル読み込み時に不明なエラーが発生
 * @retval RESOURCE_PIPELINE_RUNTIME_ERROR ファイル読み込み時エラーが発生
 * @retval RESOURCE_PIPELINE_SUCCESS 処理に成功し、正常終了
 */
resource_pipeline_result_t lit_mesh_geometry_pipeline_import_from_file(const renderer_backend_context_t* backend_context_, lit_mesh_shader_t* shader_, lit_mesh_geometry_registry_t* geometry_registry_, const char* path_, const char* name_, const char* extension_, int16_t* out_geometry_id_);

// TODO: 実装後にdoxygenコメントを追加する
// resource_pipeline_result_t lit_mesh_geometry_pipeline_import_from_vertices();

// TODO: 実装後にdoxygenコメントを追加する
resource_pipeline_result_t lit_mesh_geometry_pipeline_release(lit_mesh_shader_t* shader_, lit_mesh_geometry_registry_t* geometry_registry_, int16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
