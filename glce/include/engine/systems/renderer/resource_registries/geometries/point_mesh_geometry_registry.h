/** @ingroup renderer
 *
 * @file point_mesh_geometry_registry.h
 * @author chocolate-pie24
 *
 * @brief 点描画用ジオメトリの複製と、対応するGPU頂点バッファ上の配置情報をIDで管理するレジストリAPIを提供する
 *
 * @note GPU頂点バッファ自体はshader resourceが所有し、本レジストリは所有しない
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
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_POINT_MESH_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_POINT_MESH_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

typedef struct point_mesh_geometry_registry point_mesh_geometry_registry_t; /**< 点描画用ジオメトリレジストリのopaque型 */

typedef struct linear_alloc linear_alloc_t;                                 /**< リニアアロケータのopaque型 */
typedef struct point_mesh_geometry point_mesh_geometry_t;                   /**< 点描画用ジオメトリのopaque型 */
typedef struct vbo_range vbo_range_t;

/**
 * @brief 点描画用ジオメトリレジストリ用のメモリを確保し、初期化する
 *
 * @note 失敗した場合out_registry_は不変
 *
 * @param[in] max_geometry_count_ レジストリに登録可能な最大ジオメトリ数
 * @param[in,out] allocator_ レジストリ用メモリの確保に使用するリニアアロケータ
 * @param[out] out_registry_ 初期化されたレジストリの格納先
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
resource_registry_result_t point_mesh_geometry_registry_initialize(size_t max_geometry_count_, linear_alloc_t* allocator_, point_mesh_geometry_registry_t** out_registry_);

/**
 * @brief registry_に登録された全ジオメトリを破棄し、登録内容を空に戻す
 *
 * @note registry_自身および内部配列用に確保されたメモリは解放しない
 * @note GPU頂点バッファ上のデータの消去および領域の解放は行わない
 *
 * @param[in,out] registry_ 登録内容を空に戻すレジストリ
 */
void point_mesh_geometry_registry_deinitialize(point_mesh_geometry_registry_t* registry_);

/**
 * @brief registry_にname_のジオメトリが登録されているか判定する
 *
 * @param[in] registry_ point_mesh_geometry_registry_t構造体インスタンスへのポインタ
 * @param[in] name_ 検索ジオメトリ名称文字列
 *
 * @retval true registry_にname_という名称のジオメトリが存在する
 * @retval false 以下のいずれか
 * - name_ == NULL
 * - registry_ == NULL
 * - registry_の内部データ不整合(この場合はエラーメッセージを出力する)
 * - registry_に名称name_のジオメトリが存在しない
 */
bool point_mesh_geometry_registry_find(const point_mesh_geometry_registry_t* registry_, const char* name_);

/**
 * @brief registry_からpoint_mesh_geometry_tへの参照を取得する
 *
 * @note 戻り値のジオメトリはregistry_が所有するため、呼び出し側で破棄してはならない
 * @note 戻り値の参照は該当ジオメトリのunregisterまたはregistryのdeinitialize後に無効となる
 * @note 以下の場合はNULLを返す
 * - geometry_id_が無効
 * - registry_ == NULL
 * - registry_内部データ異常
 *
 * @param[in] registry_ point_mesh_geometry_registry_t構造体インスタンスへのポインタ
 * @param[in] geometry_id_ 取得対象ジオメトリのid
 *
 * @return point_mesh_geometry_tへの参照
 */
const point_mesh_geometry_t* point_mesh_geometry_registry_geometry_get(const point_mesh_geometry_registry_t* registry_, int16_t geometry_id_);

/**
 * @brief registry_に登録されているname_のジオメトリidを取得する
 *
 * @note idはregistry_が保持するジオメトリ配列のインデックスで0以上の値
 *
 * @note 失敗時にout_geometry_id_は不変
 *
 * @param[in] registry_ point_mesh_geometry_registry_t構造体インスタンスへのポインタ
 * @param[in] name_ id取得対象ジオメトリ名称文字列
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
resource_registry_result_t point_mesh_geometry_registry_id_get(const point_mesh_geometry_registry_t* registry_, const char* name_, int16_t* out_geometry_id_);

resource_registry_result_t point_mesh_geometry_registry_vbo_range_get(const point_mesh_geometry_registry_t* registry_, int16_t geometry_id_, vbo_range_t* out_vbo_range_);

resource_registry_result_t point_mesh_geometry_registry_register(point_mesh_geometry_registry_t* registry_, const point_mesh_geometry_t* geometry_, const vbo_range_t* vbo_range_, int16_t* out_geometry_id_);

/**
 * @brief registry_からgeometry_id_のジオメトリを登録解除する
 *
 * @note 登録時に作成したジオメトリの複製は破棄する
 * @note GPU頂点バッファ上のデータの消去および領域の解放は行わない
 * @note 失敗した場合、registry_は不変
 *
 * @param[in,out] registry_ point_mesh_geometry_registry_t構造体インスタンスへのポインタ
 * @param[in] geometry_id_ 削除対象ジオメトリid
 *
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 以下のいずれか
 * - registry_ == NULL
 * - geometry_id_の値が異常
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED registry_の内部データ不整合が発生している
 * @retval RESOURCE_REGISTRY_BAD_OPERATION registry_にgeometry_id_のジオメトリが見つからない
 * @retval RESOURCE_REGISTRY_SUCCESS 処理に成功し、正常終了
 */
resource_registry_result_t point_mesh_geometry_registry_unregister(point_mesh_geometry_registry_t* registry_, int16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
