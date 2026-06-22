/** @ingroup resource
 *
 * @file lit_mesh_geometry.h
 * @author chocolate-pie24
 * @brief lit_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの定義
 * 
 * @note lit_mesh_shader: 光源・法線・材質色などを使って、陰影付きでmeshを描画するためのシェーダー
 *
 * @version 0.1
 * @date 2026-06-04
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_LIT_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_LIT_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct lit_mesh_geometry lit_mesh_geometry_t;   /**< lit_mesh_geometryモジュール内部状態管理構造体 */

/**
 * @brief lit_mesh_geometry_t構造体インスタンスのメモリを確保し、構造体フィールドを初期化する
 *
 * @note 失敗時には*geometry_は変更しない
 *
 * @param[out] geometry_ lit_mesh_geometry_t構造体インスタンスへのダブルポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - geometry_ == NULL
 * - *geometry_ != NULL
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t lit_mesh_geometry_default_create(lit_mesh_geometry_t** geometry_);

/**
 * @brief lit_mesh_geometry_t構造体インスタンスのメモリを確保し、引数で与えた頂点情報を使用して構造体フィールドを初期化する
 *
 * @note 失敗時には*geometry_は変更しない
 *
 * @param[in] name_ ジオメトリ名称文字列
 * @param[in] vertex_count_ 頂点配列の配列要素数で、三角形の頂点の数を三角形ごとに指定する(三角形の数 = vertex_count_ / 3となる)
 * @param[in] vertices_ 頂点配列
 * @param[out] geometry_ lit_mesh_geometry_t構造体インスタンスへのダブルポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - geometry_ == NULL
 * - *geometry_ != NULL
 * - name_ == NULL
 * - name_が空文字列
 * - vertex_count_が3の倍数ではない
 * - vertex_count_ == 0
 * - vertices_ == NULL
 * - vertex_count_が3の倍数ではない
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_OVERFLOW 以下のいずれか
 * - ジオメトリ名称文字列処理でoverflowが発生
 * - 頂点配列確保サイズの計算でoverflowが発生
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t lit_mesh_geometry_create(const char* name_, size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t** geometry_);

/**
 * @brief lit_mesh_geometry_tが保有するリソースと自身のメモリを解放する
 *
 * @warning 内部データの不整合が発生していた場合はlit_mesh_geometry_tが保有する頂点配列のメモリは解放されず、リーク状態となる。この場合、geometry_自身のメモリは解放し、エラーメッセージを出力する
 * @note geometry_ == NULL または *geometry_ == NULL の場合は何も行わない
 * @note 本API実行後、*geometry_はNULLとなる
 *
 * @param[in,out] geometry_ lit_mesh_geometry_t構造体インスタンスへのダブルポインタ
 */
void lit_mesh_geometry_destroy(lit_mesh_geometry_t** geometry_);

/**
 * @brief 引数で頂点配列を与えてlit_mesh_geometry_t構造体インスタンスを初期化する
 *
 * @note lit_mesh_geometry_tの内部リソースはlit_mesh_geometryが所有するため、一度初期化したあと、destroyまたはdeinitializeをせずに再初期化するのは禁止する。これを行った場合、RESOURCE_BAD_OPERATIONを返す
 * @note geometry_にvertices_をdeep copyする。vertices_の所有権は呼び出し側にある
 * @note 失敗時にはgeometry_の内部状態は不変
 *
 * @param[in] name_ ジオメトリ名称文字列
 * @param[in] vertex_count_ 頂点配列の配列要素数で、三角形の頂点の数を三角形ごとに指定する(三角形の数 = vertex_count_ / 3となる)
 * @param[in] vertices_ 頂点配列
 * @param[in,out] geometry_ lit_mesh_geometry_t構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - name_ == NULL
 * - name_が空文字列
 * - vertex_count_ == 0
 * - vertices_ == NULL
 * - geometry_ == NULL
 * - vertex_count_が3の倍数ではない
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - geometry_がすでに初期化済みで内部状態が0, NULL以外
 * - メモリシステムが未初期化
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_OVERFLOW 以下のいずれか
 * - ジオメトリ名称文字列処理でoverflowが発生
 * - 頂点配列確保サイズの計算でoverflowが発生
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t lit_mesh_geometry_initialize(const char* name_, size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t* geometry_);

/**
 * @brief lit_mesh_geometry_t構造体インスタンスが保持するリソースを解放し、初期化する
 *
 * @warning 内部データの不整合が発生していた場合はlit_mesh_geometry_tが保有する頂点配列のメモリは解放されず、エラーメッセージを出力し、リーク状態となる
 * @note geometry_ == NULLの場合は何もしない
 *
 * @param[in,out] geometry_ 初期化対象lit_mesh_geometry_t構造体インスタンスへのポインタ
 */
void lit_mesh_geometry_deinitialize(lit_mesh_geometry_t* geometry_);

/**
 * @brief src_のクローンを生成し、*out_geometry_に格納する
 *
 * @note lit_mesh_geometry_default_createで生成された空のsrc_が与えられた場合もクローンする
 * @note 処理に失敗した場合、*out_geometry_の内部状態は不変
 *
 * @param[in] src_ クローン生成元lit_mesh_geometry_t構造体インスタンスへのポインタ
 * @param[out] out_geometry_ lit_mesh_geometry_t構造体インスタンスへのダブルポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - src_ == NULL
 * - out_geometry_ == NULL
 * - *out_geometry_ != NULL
 * @retval RESOURCE_DATA_CORRUPTED src_の内部データ不整合
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t lit_mesh_geometry_clone(const lit_mesh_geometry_t* src_, lit_mesh_geometry_t** out_geometry_);

/**
 * @brief lit_mesh_geometry_tが保有するジオメトリ名称文字列を取得する
 *
 * @note geometry_またはgeometry_が保有する文字列がNULLの場合はNULLを返す
 * @note 戻り値はlit_mesh_geometry_t内部文字列への参照であり、呼び出し側で解放してはならない
 * @note 戻り値の有効期間はgeometry_が破棄またはdeinitializeされるまで
 *
 * @param[in] geometry_ lit_mesh_geometry_t構造体インスタンスへのポインタ
 *
 * @return const char* ジオメトリ名称文字列
 */
const char* lit_mesh_geometry_name_get(const lit_mesh_geometry_t* geometry_);

/**
 * @brief geometry_が保有する頂点情報配列への参照を取得する
 *
 * @note 委譲ではないため、リソースの所有権はlit_mesh_geometry_tが保持する
 * @note 失敗時には*out_vertices_は変更しない
 * @note 取得した頂点配列参照は読み取り専用であり、呼び出し側で書き換え・解放してはならない
 * @note 参照の有効期間はgeometry_が破棄またはdeinitializeされるまで
 *
 * @param[in] geometry_ lit_mesh_geometry_t構造体インスタンスへのポインタ
 * @param[out] out_vertices_ 頂点情報配列への参照格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - geometry_ == NULL
 * - out_vertices_ == NULL
 * - *out_vertices_ != NULL
 * @retval RESOURCE_BAD_OPERATION geometry_が未初期化
 * @retval RESOURCE_DATA_CORRUPTED geometry_が保持する頂点数が3の倍数ではない
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t lit_mesh_geometry_vertices_get(const lit_mesh_geometry_t* geometry_, const point_normal_vertex_t** out_vertices_);

/**
 * @brief geometry_が保有する頂点数を取得する
 *
 * @note 失敗時には*out_vertex_count_は変更しない
 * @note 頂点数は三角形の頂点ごとにカウントするため、三角形の数の3倍となる
 *
 * @param[in] geometry_ lit_mesh_geometry_t構造体インスタンスへのポインタ
 * @param[out] out_vertex_count_ 頂点数格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - geometry_ == NULL
 * - out_vertex_count_ == NULL
 * @retval RESOURCE_BAD_OPERATION geometry_が未初期化
 * @retval RESOURCE_DATA_CORRUPTED geometry_が保持する頂点数が3の倍数ではない
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t lit_mesh_geometry_vertex_count_get(const lit_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
