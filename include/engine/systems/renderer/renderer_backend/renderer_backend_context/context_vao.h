/**
 * @ingroup renderer
 *
 * @file context_vao.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するVAO機能の窓口を上位層に提供する
 *
 * @version 0.1
 * @date 2026-02-23
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_VAO_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_VAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

/**
 * @brief VAO内部状態管理構造体インスタンスのメモリを確保する
 *
 * @note 確保されたリソースは @ref renderer_backend_vertex_array_destroy を使用して破棄する
 *
 * @details
 * - backend_context_が保有する仮想関数テーブルの関数を使用しメモリ確保を行う
 * - 構造体インスタンスのメモリ確保に成功した場合、VAOのGPU側リソースも確保される
 *
 * @param[in] backend_context_ VAOメモリ確保関数保有構造体インスタンスへのポインタ
 * @param[out] vertex_array_ メモリ確保対象VAO構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vertex_array_ == NULL
 * - *vertex_array_ != NULL
 * @retval RENDERER_BACKEND_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_BACKEND_BAD_OPERATION メモリシステム未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_vertex_array_create(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_);

/**
 * @brief VAO内部状態管理構造体インスタンスを破棄する
 *
 * @details
 * - VAOのGPU側リソースの解放も行う
 * - 本関数実行後、vertex_array_ == NULLになる
 * - 既に解放済みのvertex_array_に対しては何もしない
 * - backend_context_ == NULLの場合は何もしない
 *
 * @param[in] backend_context_ リソース破棄用vtable保有構造体インスタンスへのポインタ
 * @param[in] vertex_array_ 破棄対象インスタンスへのダブルポインタ
 */
void renderer_backend_vertex_array_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_);

/**
 * @brief VAOをbindする
 *
 * @param[in] backend_context_ bind用vtable保有構造体インスタンスへのポインタ
 * @param[in] vertex_array_ bind対象VAOハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vertex_array_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION 以下のいずれか
 * - backend_context_->vao_vtable == NULL
 * - VAOが未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_vertex_array_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_vao_t* vertex_array_);

/**
 * @brief VAOをunbindする
 *
 * @param[in] backend_context_ unbind用vtable保有構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT backend_context_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_が未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_vertex_array_unbind(const renderer_backend_context_t* backend_context_);

 /**
 * @brief 現在bind中のVAOに対し、頂点情報のレイアウトをGPUに通知する
 *
 * @warning 呼び出し側は、本APIを呼ぶ前に設定対象VAOと参照元VBOをbindしておく必要がある
 *
 * @param[in] backend_context_ アトリビュート設定用vtable保有構造体インスタンスへのポインタ
 * @param[in] layout_ シェーダープログラム内のどのバッファ変数の設定値かを指定
 * @param[in] size_ 頂点情報(layoutごと)に含まれるデータの数([x, y, z, u, v]のうち、3次元座標のみであれば3、テクスチャ座標であれば2)
 * @param[in] type_ バッファに格納されているデータの型 @ref renderer_type_t
 * @param[in] normalized_ 与えられた頂点データを正規化するかどうかを指定
 * @param[in] stride_ 頂点情報1つあたりのサイズを指定(GLfloat型の[x, y, z]であれば、sizeof(GLfloat) x 3を指定)
 * @param[in] offset_ 「この頂点属性の先頭が、現在GL_ARRAY_BUFFERにバインドされているバッファの先頭から何バイト目にあるか」を指定
 *
 * メモ:
 * @code{.c}
 * static const GLfloat vertex_buffer_data[] = {
 * -1.0f, -1.0f, 0.0f,
 * 1.0f, -1.0f, 0.0f,
 * 0.0f,  1.0f, 0.0f,
 * };
 *
 * glVertexAttribPointer(
 * 0,
 * 3,
 * GL_FLOAT,
 * GL_FALSE,
 * sizeof(GLfloat) * 3,
 * (void*)0
 * );
 * @endcode
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT backend_context_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_が未初期化
 * @retval RENDERER_BACKEND_RUNTIME_ERROR type_の値が既定値外
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_vertex_array_attribute_set(const renderer_backend_context_t* backend_context_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

#ifdef __cplusplus
}
#endif
#endif
