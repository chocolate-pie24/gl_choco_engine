/** @ingroup resource
 *
 * @file line_mesh_geometry.h
 * @author chocolate-pie24
 * @brief line_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの定義
 * 
 * @note line_mesh_shader: 複数の線分を描画する。色情報はuniform変数で扱い、RGBで指定する。このため、全ての線分が指定した色で描画される
 * @note line_mesh_geometryは線分の幾何情報のみを保持し、色情報はline_mesh_geometryを保持する親構造体で扱う
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
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_LINE_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_LINE_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/aabb_3d.h"

typedef struct line_mesh_geometry line_mesh_geometry_t;   /**< line_mesh_geometryモジュール内部状態管理構造体 */

/**
 * @brief line_mesh_geometry_t構造体インスタンスのメモリを確保し、構造体フィールドを初期化する
 *
 * @note 失敗時には*geometry_は変更しない
 * 
 * @param[out] geometry_ line_mesh_geometry_t構造体インスタンスへのダブルポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - geometry_ == NULL
 * - *geometry_ != NULL
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t line_mesh_geometry_create(line_mesh_geometry_t** geometry_);

/**
 * @brief line_mesh_geometry_tが保有するリソースと自身のメモリを解放する
 *
 * @warning 内部データの不整合が発生していた場合はline_mesh_geometry_tが保有する頂点配列のメモリは解放されず、リーク状態となる。この場合、geometry_自身のメモリは解放し、エラーメッセージを出力する
 * @note geometry_ == NULL または *geometry_ == NULL の場合は何も行わない
 * @note 本API実行後、*geometry_はNULLとなる
 * 
 * @param[in,out] geometry_ line_mesh_geometry_t構造体インスタンスへのダブルポインタ
 */
void line_mesh_geometry_destroy(line_mesh_geometry_t** geometry_);

/**
 * @brief 引数で頂点配列を与えてline_mesh_geometry_t構造体インスタンスを初期化する
 *
 * @note line_mesh_geometry_tの内部リソースはline_mesh_geometryが所有するため、一度初期化したあと、destroyをせずに再初期化するのは禁止する。これを行った場合、RESOURCE_BAD_OPERATIONを返す
 * @note geometry_にvertices_をdeep copyする。vertices_の所有権は呼び出し側にある
 * @note 失敗時にはgeometry_の内部状態は不変
 * 
 * @param[in] name_ ジオメトリ名称文字列
 * @param[in] vertex_count_ 頂点配列の配列要素数で、線分の端点の数を線分ごとに指定する(線分の数 = vertex_count_ / 2となる)
 * @param[in] vertices_ 頂点配列
 * @param[in,out] geometry_ line_mesh_geometry_t構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - name_ == NULL
 * - vertex_count_ == 0
 * - vertices_ == NULL
 * - geometry_ == NULL
 * - vertex_count_が2の倍数ではない
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - geometry_ が初期状態ではない
 * - メモリシステムが未初期化
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_OVERFLOW 以下のいずれか
 * - ジオメトリ名称文字列処理でoverflowが発生
 * - 頂点配列確保サイズの計算でoverflowが発生
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t line_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const line_vertex_t* vertices_, line_mesh_geometry_t* geometry_);

/**
 * @brief 引数でaabb_3d_t配列を与えてline_mesh_geometry_t構造体インスタンスを初期化する
 *
 * @note line_mesh_geometry_tの内部リソースはline_mesh_geometryが所有するため、一度初期化したあと、destroyをせずに再初期化するのは禁止する。これを行った場合、RESOURCE_BAD_OPERATIONを返す
 * @note geometry_にaabb_3d_tから頂点情報を生成し、geometry_に格納する。aabbs_の所有権は呼び出し側にある
 * @note 失敗時にはgeometry_の内部状態は不変
 * 
 * @param[in] name_ ジオメトリ名称文字列
 * @param[in] aabb_count_ aabbs_に含まれるaabb_3d_t構造体インスタンスの数
 * @param[in] aabbs_ aabb_3d_t構造体インスタンス配列
 * @param[in,out] geometry_ line_mesh_geometry_t構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - name_ == NULL
 * - aabb_count_ == 0
 * - aabbs_ == NULL
 * - geometry_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - geometry_ が初期状態ではない
 * - メモリシステムが未初期化
 * - aabbs_に不正なAABBが含まれる
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_OVERFLOW 以下のいずれか
 * - ジオメトリ名称文字列処理でoverflowが発生
 * - 頂点配列確保サイズの計算でoverflowが発生
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t line_mesh_geometry_initialize_from_aabbs(const char* name_, size_t aabb_count_, const aabb_3d_t* aabbs_, line_mesh_geometry_t* geometry_);

/**
 * @brief line_mesh_geometry_tが保有するジオメトリ名称文字列を取得する
 *
 * @note geometry_またはgeometry_が保有する文字列がNULLの場合はNULLを返す
 * @note 戻り値はline_mesh_geometry_t内部文字列への参照であり、呼び出し側で解放してはならない
 * @note 戻り値の有効期間はgeometry_が破棄または再初期化されるまで
 * 
 * @param[in] geometry_ line_mesh_geometry_t構造体インスタンスへのポインタ
 *
 * @return const char* ジオメトリ名称文字列
 */
const char* line_mesh_geometry_name_get(const line_mesh_geometry_t* geometry_);

/**
 * @brief geometry_が保有する頂点情報配列への参照を取得する
 *
 * @note 委譲ではないため、リソースの所有権はline_mesh_geometry_tが保持する
 * @note 失敗時には*out_vertices_は変更しない
 * @note 取得した頂点配列参照は読み取り専用であり、呼び出し側で書き換え・解放してはならない
 * @note 参照の有効期間はgeometry_が破棄されるまで
 * 
 * @param[in] geometry_ line_mesh_geometry_t構造体インスタンスへのポインタ
 * @param[out] out_vertices_ 頂点情報配列への参照格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - geometry_ == NULL
 * - out_vertices_ == NULL
 * - *out_vertices_ != NULL
 * @retval RESOURCE_BAD_OPERATION geometry_が未初期化
 * @retval RESOURCE_DATA_CORRUPTED geometry_が保持する頂点数が2の倍数ではない
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t line_mesh_geometry_vertices_get(const line_mesh_geometry_t* geometry_, const line_vertex_t** out_vertices_);

/**
 * @brief geometry_が保有する頂点数を取得する
 *
 * @note 失敗時には*out_vertex_count_は変更しない
 * @note 頂点数は線分の端点ごとにカウントするため、線分の数の2倍となる
 * 
 * @param[in] geometry_ line_mesh_geometry_t構造体インスタンスへのポインタ
 * @param[out] out_vertex_count_ 頂点数格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - geometry_ == NULL
 * - out_vertex_count_ == NULL
 * @retval RESOURCE_BAD_OPERATION geometry_が未初期化
 * @retval RESOURCE_DATA_CORRUPTED geometry_が保持する頂点数が2の倍数ではない
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t line_mesh_geometry_vertex_count_get(const line_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
