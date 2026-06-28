/** @ingroup renderer
 *
 * @file ui_mesh_geometry_registry.h
 * @author chocolate-pie24
 *
 * @brief UI描画用ジオメトリの複製と、対応するGPU頂点バッファ上の配置情報をIDで管理するレジストリAPIを提供する
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
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_UI_MESH_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_UI_MESH_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

typedef struct ui_mesh_geometry_registry ui_mesh_geometry_registry_t;   /**< UI描画用ジオメトリレジストリのopaque型 */

typedef struct linear_alloc linear_alloc_t;                             /**< リニアアロケータのopaque型 */
typedef struct ui_mesh_geometry ui_mesh_geometry_t;                     /**< UI描画用ジオメトリのopaque型 */

/**
 * @brief UI描画用ジオメトリレジストリ用のメモリを確保し、初期化する
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
resource_registry_result_t ui_mesh_geometry_registry_initialize(size_t max_geometry_count_, linear_alloc_t* allocator_, ui_mesh_geometry_registry_t** out_registry_);

/**
 * @brief registry_に登録された全ジオメトリを破棄し、登録内容を空に戻す
 *
 * @note registry_自身および内部配列用に確保されたメモリは解放しない
 * @note GPU頂点バッファ上のデータの消去および領域の解放は行わない
 *
 * @param[in,out] registry_ 登録内容を空に戻すレジストリ
 */
void ui_mesh_geometry_registry_deinitialize(ui_mesh_geometry_registry_t* registry_);

/**
 * @brief registry_にname_のジオメトリが登録されているか判定する
 *
 * @param[in] name_ 検索ジオメトリ名称文字列
 * @param[in] registry_ ui_mesh_geometry_registry_t構造体インスタンスへのポインタ
 *
 * @retval true registry_にname_という名称のジオメトリが存在する
 * @retval false 以下のいずれか
 * - name_ == NULL
 * - registry_ == NULL
 * - registry_の内部データ不整合(この場合はエラーメッセージを出力する)
 * - registry_に名称name_のジオメトリが存在しない
 */
bool ui_mesh_geometry_registry_geometry_find(const char* name_, const ui_mesh_geometry_registry_t* registry_);

/**
 * @brief registry_に登録されているname_のジオメトリidを取得する
 *
 * @note idはregistry_が保持するジオメトリ配列のインデックスで0以上の値
 *
 * @note 失敗時にout_geometry_id_は不変
 *
 * @param[in] name_ id取得対象ジオメトリ名称文字列
 * @param[in] registry_ ui_mesh_geometry_registry_t構造体インスタンスへのポインタ
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
resource_registry_result_t ui_mesh_geometry_registry_geometry_id_get(const char* name_, const ui_mesh_geometry_registry_t* registry_, int16_t* out_geometry_id_);

/**
 * @brief registry_に登録されているgeometry_id_のジオメトリの描画範囲を取得する
 *
 * @note 失敗時にout_vertex_offset_, out_vertex_count_は不変
 *
 * @param[in] geometry_id_ 取得対象ジオメトリのid
 * @param[in] registry_ ui_mesh_geometry_registry_t構造体インスタンスへのポインタ
 * @param[out] out_vertex_offset_ GPU頂点バッファ上の先頭頂点オフセット格納先
 * @param[out] out_vertex_count_ ジオメトリの頂点数格納先
 *
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 以下のいずれか
 * - registry_ == NULL
 * - out_vertex_offset_ == NULL
 * - out_vertex_count_ == NULL
 * - geometry_id_が不正
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 以下のいずれか
 * - registry_の内部データ不整合が発生している
 * - geometry_id_に相当するジオメトリの内部データ不整合が発生している
 * @retval RESOURCE_REGISTRY_BAD_OPERATION registry_にgeometry_id_のジオメトリが登録されていない
 * @retval RESOURCE_REGISTRY_SUCCESS 処理に成功し、正常終了
 */
resource_registry_result_t ui_mesh_geometry_registry_draw_range_get(int16_t geometry_id_, const ui_mesh_geometry_registry_t* registry_, size_t* out_vertex_offset_, size_t* out_vertex_count_);

/**
 * @brief geometry_の複製と対応する頂点オフセットをregistry_に登録し、ジオメトリidを取得する
 *
 * @note geometry_をregistry_へdeep copyする。geometry_の所有権は呼び出し側にある
 * @note vertex_offset_は頂点単位のオフセットであり、byte単位ではない
 * @note GPU頂点バッファへの書き込みおよび領域の確保は行わない
 * @note 失敗時にregistry_, out_geometry_id_は不変
 * @note 登録解除されたジオメトリのidは、後から登録される別のジオメトリに再利用される場合がある
 *
 * @param[in] geometry_ 登録するui_mesh_geometry_t構造体インスタンスへのポインタ
 * @param[in] vertex_offset_ geometry_に対応するGPU頂点バッファ上の先頭頂点オフセット
 * @param[in,out] registry_ ui_mesh_geometry_registry_t構造体インスタンスへのポインタ
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
resource_registry_result_t ui_mesh_geometry_registry_geometry_register(const ui_mesh_geometry_t* geometry_, size_t vertex_offset_, ui_mesh_geometry_registry_t* registry_, int16_t* out_geometry_id_);

/**
 * @brief registry_からgeometry_id_のジオメトリを登録解除する
 *
 * @note 登録時に作成したジオメトリの複製は破棄する
 * @note GPU頂点バッファ上のデータの消去および領域の解放は行わない
 * @note 失敗した場合、registry_は不変
 *
 * @param[in] geometry_id_ 削除対象ジオメトリid
 * @param[in,out] registry_ ui_mesh_geometry_registry_t構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 以下のいずれか
 * - registry_ == NULL
 * - geometry_id_の値が異常
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED registry_の内部データ不整合が発生している
 * @retval RESOURCE_REGISTRY_BAD_OPERATION registry_にgeometry_id_のジオメトリが見つからない
 * @retval RESOURCE_REGISTRY_SUCCESS 処理に成功し、正常終了
 */
resource_registry_result_t ui_mesh_geometry_registry_geometry_unregister(int16_t geometry_id_, ui_mesh_geometry_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
