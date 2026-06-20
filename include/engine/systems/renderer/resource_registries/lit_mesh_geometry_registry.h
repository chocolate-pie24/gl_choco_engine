/** @ingroup renderer
 *
 * @file lit_mesh_geometry_registry.h
 * @author chocolate-pie24
 * @brief 光源・法線・材質色などを使って、陰影付きで描画するための、lit_mesh_geometry幾何情報のCPUリソースとGPUリソースをIDを用いて管理するシステムで、以下の機能を提供する
 * - リソースの検索機能
 * - リソースの登録、登録解除機能
 * - 描画のための頂点数、頂点オフセット情報の取得機能
 *
 * @version 0.1
 * @date 2026-06-20
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_LIT_MESH_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_LIT_MESH_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

#include "engine/resource/geometry/lit_mesh_geometry.h"

#include "engine/core/memory/linear_allocator.h"

typedef struct lit_mesh_geometry_registry lit_mesh_geometry_registry_t; /**< lit_mesh_geometry_registry_t前方宣言 */

/**
 * @brief lit_mesh_geometry_registry_t管理システムのリソースを確保し初期化する
 *
 * @note 失敗した場合out_registry_は不変
 *
 * @param[in] max_geometry_count_ 管理システムで管理可能なジオメトリ数
 * @param[in,out] allocator_ リニアアロケータ
 * @param[out] out_registry_ 管理システム構造体インスタンスへのダブルポインタ
 *
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 以下のいずれか
 * - max_geometry_count_ == 0
 * - max_geometry_count_がINT16_MAXを超過
 * - allocator_ == NULL
 * - out_registry_ == NULL
 * - *out_registry_ != NULL
 * @retval RESOURCE_REGISTRY_NO_MEMORY リニアアロケータによるメモリ確保失敗
 * @retval RESOURCE_REGISTRY_OVERFLOW メモリ割り当てサイズがオーバーフロー
 * @retval RESOURCE_REGISTRY_SUCCESS 処理に成功し、正常終了
 */
resource_registry_result_t lit_mesh_geometry_registry_initialize(size_t max_geometry_count_, linear_alloc_t* allocator_, lit_mesh_geometry_registry_t** out_registry_);

/**
 * @brief lit_mesh描画ジオメトリ管理システムを初期状態に戻す
 *
 * @note 初期状態とは、以下の状態を指す
 * - lit_mesh_geometry_registry_t自身のメモリと構造体フィールドのメモリが確保されている
 * - 構造体フィールドの全てNULLまたは0で初期化されている
 *
 * @note registry_が保持するリソースのメモリは解放しない
 *
 * @param[in,out] registry_ 初期化対象lit_mesh_geometry_registry_t構造体インスタンスへのポインタ
 */
void lit_mesh_geometry_registry_deinitialize(lit_mesh_geometry_registry_t* registry_);

/**
 * @brief lit_mesh_geometry管理システムにジオメトリ名称がname_のジオメトリが存在しているかを判定する
 *
 * @param[in] name_ 検索ジオメトリ名称文字列
 * @param[in] registry_ lit_mesh_geometry_registry_t構造体インスタンスへのポインタ
 *
 * @retval 管理システム内にname_という名称のジオメトリが存在する
 * @retval 以下のいずれか
 * - name_ == NULL
 * - registry_ == NULL
 * - registry_の内部データ不整合(この場合はエラーメッセージを出力する)
 * - registry_に名称name_のジオメトリが存在しない
 */
bool lit_mesh_geometry_registry_geometry_find(const char* name_, const lit_mesh_geometry_registry_t* registry_);

/**
 * @brief registry_が管理システムジオメトリから名称name_のジオメトリのidを返す
 *
 * @note idはregistry_が保持するジオメトリ配列のインデックスで0以上の値
 *
 * @note 失敗時にout_geometry_id_は不変
 *
 * @param[in] name_ id取得対象ジオメトリ名称文字列
 * @param[in] registry_ lit_mesh_geometry_registry_t構造体インスタンスへのポインタ
 * @param[out] out_geometry_id_ ジオメトリid格納先
 *
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 以下のいずれか
 * - name_ == NULL
 * - registry_ == NULL
 * - out_geometry_id_ == NULL
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED registry_の内部データ不整合が発生
 * @retval RESOURCE_REGISTRY_BAD_OPERATION registry_内に名称name_のジオメトリが存在しない
 * @retval RESOURCE_REGISTRY_SUCCESS 処理に成功し、正常終了
 */
resource_registry_result_t lit_mesh_geometry_registry_geometry_id_get(const char* name_, const lit_mesh_geometry_registry_t* registry_, int16_t* out_geometry_id_);

/**
 * @brief ジオメトリ管理システムからid = geometry_id_のジオメトリを描画するために必要な、頂点数とVBO頂点オフセットを取得する
 *
 * @note 失敗時にout_vertex_offset_, out_vertex_count_は不変
 *
 * @param[in] geometry_id_ 取得対象ジオメトリのid
 * @param[in] registry_ lit_mesh_geometry_registry_t構造体インスタンスへのポインタ
 * @param[out] out_vertex_offset_ 頂点VBOオフセット格納先
 * @param[out] out_vertex_count_ ジオメトリ頂点数格納先
 *
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 以下のいずれか
 * - registry_ == NULL
 * - out_vertex_offset_ == NULL
 * - out_vertex_count_ == NULL
 * - geometry_id_が不正
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 以下のいずれか
 * - registry_の内部データ不整合が発生している
 * - geometry_id_に相当するジオメトリの内部データ不整合が発生している
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 管理システムにgeometry_id_に相当するジオメトリが存在しない
 * @retval RESOURCE_REGISTRY_SUCCESS 処理に成功し、正常終了
 */
resource_registry_result_t lit_mesh_geometry_registry_draw_range_get(int16_t geometry_id_, const lit_mesh_geometry_registry_t* registry_, size_t* out_vertex_offset_, size_t* out_vertex_count_);

/**
 * @brief geometry_とgeometry_のGPU側リソース(vertex_offset_)をregistry_に登録し、geometry_idをout_geometry_id_に格納する
 *
 * @note geometry_をgeometry_registry_へdeep copyする。geometry_の所有権は呼び出し側にある
 * @note 失敗時にregistry_, out_geometry_id_は不変
 *
 * @param[in] geometry_ 登録するlit_mesh_geometry_t構造体インスタンスへのポインタ
 * @param[in] vertex_offset_ 登録ジオメトリのGPU側リソース(VBO頂点オフセット)
 * @param[in,out] registry_ lit_mesh_geometry_registry_t構造体インスタンスへのポインタ
 * @param[out] out_geometry_id_ ジオメトリid格納先
 *
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 以下のいずれか
 * - registry_ == NULL
 * - geometry_ == NULL
 * - out_geometry_id_ == NULL
 * - geometry_が未初期化でジオメトリ名称が取得できない
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 以下のいずれか
 * - registry_の内部データ不整合が発生している
 * - geometry_の内部データ不整合が発生している
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 以下のいずれか
 * - geometry_のジオメトリ名称が既にregistry_に登録されている
 * - メモリシステム未初期化
 * @retval RESOURCE_REGISTRY_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED 以下のいずれか
 * - メモリシステム使用可能範囲上限超過
 * - registry_に空きスロットが見つからない
 * @retval RESOURCE_REGISTRY_SUCCESS 処理に成功し、正常終了
 */
resource_registry_result_t lit_mesh_geometry_registry_geometry_register(const lit_mesh_geometry_t* geometry_, size_t vertex_offset_, lit_mesh_geometry_registry_t* registry_, int16_t* out_geometry_id_);

/**
 * @brief registry_からgeometry_id_のジオメトリを削除する
 *
 * @note 失敗した場合、registry_は不変
 *
 * @param[in] geometry_id_ 削除対象ジオメトリid
 * @param[in,out] registry_ lit_mesh_geometry_registry_t構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 以下のいずれか
 * - registry_ == NULL
 * - geometry_id_の値が異常
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED registry_の内部データ不整合が発生している
 * @retval RESOURCE_REGISTRY_BAD_OPERATION registry_にgeometry_id_のジオメトリが見つからない
 * @retval RESOURCE_REGISTRY_SUCCESS 処理に成功し、正常終了
 */
resource_registry_result_t lit_mesh_geometry_registry_geometry_unregister(int16_t geometry_id_, lit_mesh_geometry_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
