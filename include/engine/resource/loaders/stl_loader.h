/** @ingroup resource
 *
 * @file stl_loader.h
 * @author chocolate-pie24
 * @brief STLファイルのロード処理を行うAPIの定義
 *
 * @details 以下のSTLファイルをサポートする
 * - ASCII形式のSTL(BINARY形式は将来的にサポート予定)
 * - ファイルに含まれる法線情報は[-1.0...1.0]の範囲であり、NaN、Infを含まないこと
 *
 * @todo 以下を行う
 * - GLCEカスタムフォーマットでの出力機能
 * - カスタムフォーマットが存在する場合はそちらで読み込み、ない場合は通常STLを読み込みカスタムフォーマットファイルを出力
 * @todo 現状では法線情報が[-1.0...1.0]の範囲に収まっているかをチェックしているが、この範囲に収まっていても長さが非1.0で正規化されていない場合がある。チェックを厳密化する。
 *
 * @version 0.1
 * @date 2026-06-02
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_LOADERS_STL_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_STL_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct stl_loader stl_loader_t; /**< STLローダー内部状態管理構造体前方宣言 */

/**
 * @brief stl_loader_tのメモリを確保し、内部状態を初期化する
 *
 * @note 内部状態は以下の値で初期化される
 * - vertex_count = 0
 * - vertices = NULL
 * @note 失敗時にはstl_loader_の状態は不変
 *
 * @param[out] stl_loader_ stl_loader_t構造体インスタンスへのダブルポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - stl_loader_ == NULL
 * - *stl_loader_ != NULL
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t stl_loader_create(stl_loader_t** stl_loader_);

/**
 * @brief stl_loader_が保有するリソースを解放し、自身のメモリを解放する
 *
 * @note 本APIが成功した場合、stl_loader_はNULLとなり、再使用不可
 * @note 2重destroyを許可
 * @note NULL != (*stl_loader_)->verticesかつ0 == (*stl_loader_)->vertex_countのように壊れたデータの場合には、verticesのメモリ解放サイズが不明であるため解放せず、verticesはリーク状態となる(この場合はエラーメッセージを出力する)
 *
 * @param[out] stl_loader_ リソース解放対象stl_loader_t構造体インスタンスへのダブルポインタ
 */
void stl_loader_destroy(stl_loader_t** stl_loader_);

/**
 * @brief ASCII形式のSTLファイルを読み込む
 *
 * @note ロード可能なSTLデータは法線情報が-1.0...1.0に正規化されている必要がある(されていない場合はRESOURCE_DATA_CORRUPTEDを返す)
 * @note 失敗時にはstl_loader_の状態は不変
 *
 * @param[in] path_ STLファイルが格納されているパス(最後は'/'が入っていること)
 * @param[in] name_ STLファイル名(拡張子は含まない)
 * @param[in] extension_ STLファイル拡張子('.'で始まること)
 * @param[in,out] stl_loader_ stl_loader_t構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - path_ == NULL
 * - name_ == NULL
 * - extension_ == NULL
 * - stl_loader_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - 0 != stl_loader_->vertex_count
 * - stl_loader_->vertices != NULL
 * - メモリシステム未初期化
 * @retval RESOURCE_FILE_OPEN_ERROR STLファイルオープン失敗
 * @retval RESOURCE_UNDEFINED_ERROR ファイル読み込み時に不明なエラーが発生
 * @retval RESOURCE_DATA_CORRUPTED 以下のいずれか
 * - 内部データ破損
 * - STLデータ不整合
 * - 頂点情報、法線情報のパース失敗
 * - 法線情報が[-1.0...1.0]の範囲外、またはNaN、Infが含まれる
 * - 頂点情報にNaN、Infが含まれる
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_OVERFLOW 以下のいずれか
 * - ファイルフルパス文字列が長すぎる
 * - STLデータに格納されている頂点の数または法線の数がSIZE_MAXを超過
 * - 頂点配列確保サイズの計算でoverflowが発生
 * @retval RESOURCE_RUNTIME_ERROR 以下のいずれか
 * - 1回目の頂点数カウント結果と2回目の読み込み結果が一致しない(STLファイルが読み込み中に変更された可能性がある)
 * - ファイル読み込みでエラー発生
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t stl_loader_ascii_load(const char* path_, const char* name_, const char* extension_, stl_loader_t* stl_loader_);

/**
 * @brief stl_loader_が保有する頂点配列情報の所有権をAPI呼び出し側へ委譲する
 *
 * @note 本APIが成功すると、頂点配列の所有権は呼び出し側へ移り、stl_loader_ は未ロード状態に戻る
 *
 * @param[in,out] stl_loader_ 頂点情報、頂点数情報を保有するstl_loader_t構造体インスタンスへのポインタ
 * @param[out] out_vertices_ 頂点情報委譲先
 * @param[out] out_vertex_count_ 頂点数情報格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - stl_loader_ == NULL
 * - out_vertices_ == NULL
 * - *out_vertices_ != NULL
 * - out_vertex_count_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - stl_loader_->vertex_count == 0
 * - stl_loader_->vertices == NULL
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t stl_loader_vertices_move(stl_loader_t* stl_loader_, point_normal_vertex_t** out_vertices_, size_t* out_vertex_count_);

/**
 * @brief stl_loader_が保持する頂点数情報を取得する
 *
 * @warning stl_loader_vertices_move()実行後はstl_loader_が頂点配列を保持しない状態になるため、本APIはRESOURCE_BAD_OPERATIONを返す
 *
 * @param[in] stl_loader_ stl_loader_t構造体インスタンスへのポインタ
 * @param[out] out_vertex_count_ 頂点数情報格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - stl_loader_ == NULL
 * - out_vertex_count_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - stl_loader_->vertex_count == 0
 * - stl_loader_->vertices == NULL
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t stl_loader_vertices_count_get(const stl_loader_t* stl_loader_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
