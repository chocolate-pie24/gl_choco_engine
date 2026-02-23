/**
 * @ingroup renderer_backend_context
 *
 * @file context_vbo.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するVBO機能の窓口を上位層に提供する
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
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_VBO_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_VBO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;

/**
 * @brief VBO内部状態管理構造体インスタンスのメモリを確保する
 *
 * @note 確保されたリソースは @ref renderer_backend_vertex_buffer_destroy を使用して破棄する
 *
 * @details
 * - backend_context_が保有する仮想関数テーブルの関数を使用しメモリ確保を行う
 * - 構造体インスタンスのメモリ確保に成功した場合、VBOのGPU側リソースも確保される
 *
 * @param[in] backend_context_ VAOメモリ確保関数保有構造体インスタンスへのポインタ
 * @param[out] vertex_buffer_ メモリ確保対象VBO構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vertex_buffer_ == NULL
 * - *vertex_buffer_ != NULL
 * @retval RENDERER_BAD_OPERATION backend_context_が未初期化
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_vertex_buffer_create(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vertex_buffer_);

/**
 * @brief VBO内部状態管理構造体インスタンスを破棄する
 *
 * @details
 * - VBOのGPU側リソースの解放も行う
 * - 本関数実行後、vertex_buffer_ == NULLになる
 * - 既に解放済みのvertex_buffer_に対しては何もしない
 * - backend_context_ == NULLの場合は何もしない
 *
 * @param backend_context_ リソース破棄用vtable保有構造体インスタンスへのポインタ
 * @param vertex_buffer_ 破棄対象インスタンスへのダブルポインタ
 */
void renderer_backend_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vertex_buffer_);

/**
 * @brief VBOをbindする
 *
 * @details
 * - 処理に成功した場合、backend_context_が保持する現在bind中のVBO値が更新される
 * - 既にbind済みのvertex_buffer_が渡された場合は何もしない
 *
 * @param backend_context_ bind用vtable保有構造体インスタンスへのポインタ
 * @param vertex_buffer_ bind対象VBOハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vertex_buffer_ == NULL
 * @retval RENDERER_BAD_OPERATION backend_context_->vbo_vtable == NULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_vertex_buffer_bind(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_);

/**
 * @brief VBOをunbindする
 *
 * @param backend_context_ unbind用vtable保有構造体インスタンスへのポインタ
 * @param vertex_buffer_ unbind対象VBOハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vertex_buffer_ == NULL
 * @retval RENDERER_BAD_OPERATION backend_context_が未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_vertex_buffer_unbind(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_);

/**
 * @brief GPUの頂点情報格納バッファに頂点情報を転送する
 *
 * @note
 * - 転送の際には本関数内でVBOのbindが行われる
 *
 * @param backend_context_ 転送用vtable保有構造体インスタンスへのポインタ
 * @param vertex_buffer_ 転送対象VBOハンドルを保有する構造体インスタンスへのポインタ
 * @param load_size_ 転送サイズ(byte)
 * @param load_data_ 転送データ配列への先頭ポインタ
 * @param usage_ バッファ用途 @ref buffer_usage_t
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - load_size_ == 0
 * - backend_context_ == NULL
 * - vertex_buffer_ == NULL
 * @retval RENDERER_BAD_OPERATION backend_context_が未初期化
 * @retval RENDERER_RUNTIME_ERROR usage_の値が規定範囲外
 */
renderer_result_t renderer_backend_vertex_buffer_vertex_load(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_);

#ifdef __cplusplus
}
#endif
#endif
