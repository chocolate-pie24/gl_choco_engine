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
resource_result_t lit_mesh_geometry_create(lit_mesh_geometry_t** geometry_);

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
 * @note lit_mesh_geometry_tの内部リソースはlit_mesh_geometryが所有するため、一度初期化したあと、destroyをせずに再初期化するのは禁止する。これを行った場合、RESOURCE_BAD_OPERATIONを返す
 * @note geometry_にvertices_をdeep copyする。vertices_の所有権は呼び出し側にある
 * @note 失敗時にはgeometry_の内部状態は不変
 * 
 * @param[in] name_ ジオメトリ名称文字列
 * @param[in] vertex_count_ 頂点配列の配列要素数
 * @param[in] vertices_ 頂点配列
 * @param[in,out] geometry_ lit_mesh_geometry_t構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - name_ == NULL
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
resource_result_t lit_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t* geometry_);

/**
 * @brief STLファイル等のジオメトリデータ格納ファイルをロードし、lit_mesh_geometry_tを初期化する
 *
 * @note lit_mesh_geometry_tの内部リソースはlit_mesh_geometryが所有するため、一度初期化したあと、destroyをせずに再初期化するのは禁止する。これを行った場合、RESOURCE_BAD_OPERATIONを返す
 * @note 失敗時にはgeometry_の内部状態は不変
 * 
 * @param[in] path_ データファイルパス(末尾に'/'を付加すること)
 * @param[in] name_ データファイル名(拡張子は含まない)
 * @param[in] extension_ データファイル拡張子(先頭に'.'を付加すること)
 * @param[in,out] geometry_ lit_mesh_geometry_t構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - path_ == NULL
 * - name_ == NULL
 * - extension_ == NULL
 * - geometry_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - geometry_がすでに初期化済みで内部状態が0, NULL以外
 * - メモリシステムが未初期化
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_UNSUPPORTED_FILE lit_mesh_geometryがサポート対象外のファイル(現状はASCII形式のstlのみをサポート)
 * @retval RESOURCE_FILE_OPEN_ERROR ファイルオープン失敗
 * @retval RESOURCE_UNDEFINED_ERROR ファイル読み込み時に不明なエラーが発生
 * @retval RESOURCE_DATA_CORRUPTED 以下のいずれか
 * - stl_loader_t内部データ破損
 * - STLデータ不整合
 * - 頂点情報、法線情報のパース失敗
 * - 法線情報が[-1.0, 1.0]の範囲外、またはNaN、Infが含まれる
 * - 頂点情報にNaN、Infが含まれる
 * @retval RESOURCE_OVERFLOW 以下のいずれか
 * - ファイルフルパス文字列が長すぎる
 * - STLデータに格納されている頂点の数または法線の数がSIZE_MAXを超過
 * - 頂点配列確保サイズの計算でoverflowが発生
 * @retval RESOURCE_RUNTIME_ERROR 以下のいずれか
 * - STLファイルが読み込み中に変更された可能性がある
 * - 頂点数カウント結果と実際の読み込み結果が一致しない
 * - ファイル読み込み中にエラーが発生
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t lit_mesh_geometry_initialize_from_file(const char* path_, const char* name_, const char* extension_, lit_mesh_geometry_t* geometry_);

/**
 * @brief lit_mesh_geometry_tが保有するジオメトリ名称文字列を取得する
 *
 * @note geometry_またはgeometry_が保有する文字列がNULLの場合はNULLを返す
 * @note 戻り値はlit_mesh_geometry_t内部文字列への参照であり、呼び出し側で解放してはならない
 * @note 戻り値の有効期間はgeometry_が破棄または再初期化されるまで
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
 * @note 参照の有効期間はgeometry_が破棄されるまで
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
