/**
 * @ingroup renderer_backend_context
 *
 * @file context_shader.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するシェーダー機能の窓口を上位層に提供する
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
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_SHADER_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_CONTEXT_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

/**
 * @brief シェーダー構造体インスタンスのメモリを確保し0で初期化する
 *
 * @note
 * - メモリ確保、初期化処理はrenderer_backend_context_が保持する仮想関数テーブルを使用する
 * - 確保したリソースの解放は @ref renderer_backend_shader_destroy を使用して解放する
 *
 * @param[in] renderer_backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[out] shader_handle_ リソース確保対象シェーダーハンドル構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - renderer_backend_context_ == NULL
 * - shader_handle_ == NULL
 * - *shader_handle_ != NULL
 * @retval RENDERER_BAD_OPERATION renderer_backend_context_が未初期化
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_SUCCESS メモリ確保および初期化に成功し、正常終了
 */
renderer_result_t renderer_backend_shader_create(renderer_backend_context_t* renderer_backend_context_, renderer_backend_shader_t** shader_handle_);

/**
 * @brief シェーダー構造体インスタンスを破棄する
 *
 * @note
 * - 本関数実行後はshader_handle_はNULLになり再利用不可となる
 * - 2重解放を許可する(shader_handle_ == NULLまたは*shader_handle_ == NULLの場合は何もしない)
 *
 * @details 以下の処理を行う
 * - シェーダープログラムのGPU側リソースの破棄
 * - シェーダー構造体インスタンスが保持するリソースの破棄
 * - シェーダー構造体インスタンス自身のリソースの破棄
 *
 * @param[in] renderer_backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] shader_handle_ 破棄対象構造体インスタンスへのダブルポインタ
 */
void renderer_backend_shader_destroy(renderer_backend_context_t* renderer_backend_context_, renderer_backend_shader_t** shader_handle_);

/**
 * @brief シェーダープログラムをコンパイルする
 *
 * @note
 * - shader_source_のリソース解放は呼び出し側で行うこと
 * - シェーダープログラムのGPUリソース確保に成功後、コンパイルに失敗した場合はGPUリソースは破棄される
 *
 * @details 以下の処理を行う
 * - シェーダープログラムのGPU側リソース確保
 * - シェーダープログラムのコンパイル
 *
 * @param[in] shader_type_ シェーダー種別 @ref shader_type_t
 * @param[in] shader_source_ シェーダーソース文字列
 * @param[in] renderer_backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] shader_handle_ コンパイルしたシェーダープログラムへのハンドルを格納する
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_source_ == NULL
 * - shader_handle_ == NULL
 * - shader_type_が無効
 * @retval RENDERER_BAD_OPERATION shader_handle_が保持するシェーダープログラムがコンパイル済み、またはリンク済み
 * @retval RENDERER_SHADER_COMPILE_ERROR シェーダープログラムのGPU側リソース確保に失敗、またはコンパイル失敗
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_SUCCESS シェーダープログラムのコンパイルに成功し、正常終了
 */
renderer_result_t renderer_backend_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_);

/**
 * @brief コンパイル済みのシェーダープログラムをリンクする
 *
 * @note 現状ではバーテックスシェーダーとフラグメントシェーダーのみを対象にリンクする
 *
 * @details 以下の処理を行う
 * - リンクして生成されるプログラムのGPU側リソースを確保する
 * - シェーダープログラムのリンク
 *
 * @param[in] renderer_backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] shader_handle_ リンクしたプログラム識別子を格納する
 *
 * @retval RENDERER_INVALID_ARGUMENT shader_handle_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - プログラムが既にリンク済み
 * - バーテックスシェーダーが未コンパイル
 * - フラグメントシェーダーが未コンパイル
 * @retval RENDERER_SHADER_LINK_ERROR プログラムのGPU側リソース確保に失敗、またはシェーダーリンクエラー
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_SUCCESS プログラムのリンクに成功し、正常終了
 */
renderer_result_t renderer_backend_shader_link(renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_);

/**
 * @brief シェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @note 処理に成功した場合、現在使用中のプログラム識別子がbackend_context_が保持するフィールドに記憶される
 *
 * @param[in] renderer_backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] shader_handle_ シェーダープログラムハンドル格納構造体インスタンス
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - out_program_id_ == NULL
 * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - shader_handle_が保持するバーテックスシェーダープログラムが未コンパイル
 * - shader_handle_が保持するフラグメントシェーダープログラムが未コンパイル
 */
renderer_result_t renderer_backend_shader_use(renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_);

#ifdef __cplusplus
}
#endif
#endif
