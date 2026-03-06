/**
 * @ingroup renderer_backend_context
 *
 * @file context_vao.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するVAO機能の窓口を上位層に提供する
 *
 * @version 0.1
 * @date 2026-02-23
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_VAO_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_VAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

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
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - vertex_array_ == NULL
 * - *vertex_array_ != NULL
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_vertex_array_create(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_);

/**
 * @brief VAO内部状態管理構造体インスタンスを破棄する
 *
 * @details
 * - VAOのGPU側リソースの解放も行う
 * - 本関数実行後、vertex_array_ == NULLになる
 * - 既に解放済みのvertex_array_に対しては何もしない
 * - backend_context_ == NULLの場合は何もしない
 *
 * @param backend_context_ リソース破棄用vtable保有構造体インスタンスへのポインタ
 * @param vertex_array_ 破棄対象インスタンスへのダブルポインタ
 */
void renderer_backend_vertex_array_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_);

/**
 * @brief VAOをbindする
 *
 * @details
 * - 処理に成功した場合、backend_context_が保持する現在bind中のVAO値が更新される
 * - 既にbind済みのvertex_array_が渡された場合は何もしない
 *
 * @param backend_context_ bind用vtable保有構造体インスタンスへのポインタ
 * @param vertex_array_ bind対象VAOハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vertex_array_ == NULL
 * @retval RENDERER_BAD_OPERATION backend_context_->vao_vtable == NULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_vertex_array_bind(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_);

/**
 * @brief VAOをunbindする
 *
 * @param backend_context_ unbind用vtable保有構造体インスタンスへのポインタ
 * @param vertex_array_ unbind対象VAOハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vertex_array_ == NULL
 * @retval RENDERER_BAD_OPERATION backend_context_が未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_vertex_array_unbind(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_);

 /**
 * @brief 頂点情報のレイアウトをGPUに通知する
 *
 * @note 本関数内でVAOがbindされるため、事前のbindは不要
 *
 * @param backend_context_ アトリビュート設定用vtable保有構造体インスタンスへのポインタ
 * @param vertex_array_ VAOハンドル(OpenGL3.3では使用しない)
 * @param layout_ シェーダープログラム内のどのバッファ変数の設定値かを指定
 * @param size_ 頂点情報に含まれるデータの数([x, y, z]の3次元座標のみであれば3)
 * @param type_ バッファに格納されているデータの型 @ref renderer_type_t
 * @param normalized_ 与えられた頂点データを正規化するかどうかを指定
 * @param stride_ 頂点情報1つあたりのサイズを指定(GLfloat型の[x, y, z]であれば、sizeof(GLfloat) x 3を指定)
 * @param offset_ 「この頂点属性の先頭が、現在GL_ARRAY_BUFFERにバインドされているバッファの先頭から何バイト目にあるか」を指定
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
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vertex_array_ == NULL
 * @retval RENDERER_BAD_OPERATION backend_context_が未初期化
 * @retval RENDERER_RUNTIME_ERROR type_の値が既定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_vertex_array_attribute_set(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

#ifdef __cplusplus
}
#endif
#endif
