/**
 * @ingroup renderer_backend_context
 *
 * @file context_shader.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するシェーダーオブジェクト/シェーダープログラム操作機能の窓口を上位層に提供する
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

#include <stdint.h>
#include <stdbool.h>

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

/**
 * @brief シェーダーハンドル構造体インスタンスのメモリを確保し0で初期化する
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
 * @brief シェーダーハンドル構造体インスタンスを破棄する
 *
 * @note
 * - 本関数実行後はshader_handle_はNULLになり再利用不可となる
 * - 2重解放を許可する(shader_handle_ == NULLまたは*shader_handle_ == NULLの場合は何もしない)
 *
 * @details 以下の処理を行う
 * - シェーダープログラムのGPU側リソースの破棄
 * - シェーダーオブジェクトのGPU側リソースの破棄
 * - シェーダーハンドル構造体インスタンスが保持するリソースの破棄
 * - シェーダーハンドル構造体インスタンス自身のリソースの破棄
 *
 * @param[in] renderer_backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] shader_handle_ 破棄対象構造体インスタンスへのダブルポインタ
 */
void renderer_backend_shader_destroy(renderer_backend_context_t* renderer_backend_context_, renderer_backend_shader_t** shader_handle_);

/**
 * @brief シェーダーソースをコンパイルし、シェーダーオブジェクトハンドルを初期化する
 *
 * @note
 * - shader_source_のリソース解放は呼び出し側で行うこと
 * - シェーダーオブジェクトのGPUリソース確保に成功後、コンパイルに失敗した場合はGPUリソースは破棄される
 *
 * @details 以下の処理を行う
 * - シェーダーオブジェクトのGPU側リソース確保
 * - シェーダーソースのコンパイル
 *
 * @param[in] shader_type_ シェーダー種別 @ref shader_type_t
 * @param[in] shader_source_ シェーダーソース文字列
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] shader_handle_ コンパイルしたシェーダーオブジェクトへのハンドルを格納する
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - shader_source_ == NULL
 * - shader_handle_ == NULL
 * - shader_type_が無効
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - shader_handle_が保持するシェーダーオブジェクトがコンパイル済み
 * - shader_handle_が保持するシェーダープログラムがリンク済み
 * @retval RENDERER_SHADER_COMPILE_ERROR シェーダーオブジェクトのGPU側リソース確保に失敗、またはシェーダーソースのコンパイル失敗
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_SUCCESS シェーダーオブジェクトのコンパイルに成功し、正常終了
 */
renderer_result_t renderer_backend_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_);

/**
 * @brief コンパイル済みのシェーダーオブジェクトをリンクする
 *
 * @note 現状ではバーテックスシェーダーとフラグメントシェーダーのみを対象にリンクする
 *
 * @details 以下の処理を行う
 * - リンクして生成されるプログラムのGPU側リソースを確保する
 * - シェーダーオブジェクトのリンク
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] shader_handle_ リンクしたプログラム識別子を格納する
 *
 * @retval RENDERER_INVALID_ARGUMENT
 * - shader_handle_ == NULL
 * - backend_context_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - プログラムが既にリンク済み
 * - バーテックスシェーダーオブジェクトが未コンパイル
 * - フラグメントシェーダーオブジェクトが未コンパイル
 * @retval RENDERER_SHADER_LINK_ERROR シェーダープログラムのGPU側リソース確保に失敗、またはシェーダーリンクエラー
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_SUCCESS シェーダープログラムのリンクに成功し、正常終了
 */
renderer_result_t renderer_backend_shader_link(renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_);

/**
 * @brief シェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @note 処理に成功した場合、現在使用中のプログラム識別子がbackend_context_が保持するフィールドに記憶される
 *
 * @param[in,out] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] shader_handle_ シェーダープログラムハンドル格納構造体インスタンス
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - backend_context_ == NULL
 * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - shader_handle_が保持するバーテックスシェーダーオブジェクトが未コンパイル
 * - shader_handle_が保持するフラグメントシェーダーオブジェクトが未コンパイル
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_shader_use(renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_);

/**
 * @brief シェーダープログラムのユニフォーム変数のLocationを取得する
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] shader_handle_ シェーダープログラムハンドル構造体インスタンスへのポインタ
 * @param[in] name_ ユニフォーム変数名称
 * @param[out] out_location_ Location格納先
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - shader_handle_ == NULL
 * - name_ == NULL
 * - out_location_ == NULL
 * @retval RENDERER_BAD_OPERATION backend_context_が未初期化でbackend_context_->shader_vtableがNULL
 * @retval RENDERER_RUNTIME_ERROR ユニフォーム変数のLocation取得に失敗
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_shader_uniform_location_get(const renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);

/**
 * @brief シェーダープログラムにmat4f型のユニフォーム変数を送信する
 *
 * @note
 * - OpenGL 3.3実装
 * - 現在使用中のシェーダープログラムと、送信対象シェーダープログラムが異なる場合は、使用中のプログラムが送信対象シェーダープログラムに切り替わる
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] shader_handle_ シェーダープログラムハンドルインスタンスへのポインタ
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] should_transpose_ true: 送信時に行列を転置する / false: 送信時に行列を転置しない
 * @param[in] data_ 送信データへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - shader_handle_ == NULL
 * - data_ == NULL
 * @retval RENDERER_DATA_CORRUPTED シェーダープログラムハンドルインスタンスの内部データが破損
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク状態
 * - backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t renderer_backend_shader_mat4f_uniform_set(renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_);

#ifdef __cplusplus
}
#endif
#endif
