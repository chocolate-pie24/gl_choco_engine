/** @ingroup renderer_resources
 *
 * @file ui_shader.h
 * @author chocolate-pie24
 * @brief レンダラーリソースのうち、UIシェーダーリソース操作APIを提供する
 *
 * @version 0.1
 * @date 2026-03-11
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RESOURCES_UI_SHADER_H
#define GLCE_ENGINE_RENDERER_RESOURCES_UI_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_context/context.h"

typedef struct ui_shader ui_shader_t;   /**< UIシェーダーリソース構造体前方宣言 */

/**
 * @brief UIシェーダーリソースインスタンスのメモリを確保し初期化する
 *
 * @details 以下の処理を行う
 * - out_ui_shader_自身のリソース確保
 * - シェーダーソースのコンパイル
 * - シェーダーモジュールのリンク
 * - UIシェーダーが扱うプロジェクション行列のLocation取得
 * - UIシェーダーが扱うビュー行列のLocation取得
 * - UIシェーダーが扱うモデル行列のLocation取得
 *
 * @param file_path_ シェーダーソース格納ファイルパス(文字列の最後を'/'にすること)
 * @param name_ シェーダーソースファイル名称(拡張子は含まない)
 * @param backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param out_ui_shader_ リソース確保対象UIシェーダーリソースへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - file_path_ == NULL
 * - name_ == NULL
 * - backend_context_ == NULL
 * - out_ui_shader_ == NULL
 * - *out_ui_shader_ != NULL
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_LIMIT_EXCEEDED メモリシステムのメモリ使用量範囲上限超過
 * @retval RENDERER_RUNTIME_ERROR 以下のいずれか
 * - 計算過程でオーバーフロー発生(文字列長さ異常)
 * - ユニフォーム変数のLocation取得に失敗
 * @retval RENDERER_DATA_CORRUPTED 内部データ破損が発生
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - レンダラーバックエンドが未初期化
 * - シェーダーソースが既にコンパイル済み
 * - シェーダーモジュールが既にリンク済み
 * @retval RENDERER_SHADER_COMPILE_ERROR 以下のいずれか
 * - シェーダーモジュールのGPU側リソース確保に失敗
 * - シェーダーソースのコンパイルに失敗
 * @retval RENDERER_SHADER_LINK_ERROR シェーダーモジュールのリンクに失敗
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, ui_shader_t** out_ui_shader_);

/**
 * @brief UIシェーダーリソースインスタンスが保持するリソースと、自身のメモリを開放する
 *
 * @note
 * - 本APIを使用した後、ui_shader_ == NULLとなり再使用不可
 * - 2重デストロイ許可
 * - GPU側のシェーダープログラムリソースも破棄される
 *
 * @param backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param ui_shader_ 破棄対象UIシェーダーリソースインスタンスへのダブルポインタ
 */
void ui_shader_destroy(renderer_backend_context_t* backend_context_, ui_shader_t** ui_shader_);

/**
 * @brief UIシェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @note 処理に成功した場合、現在使用中のプログラム識別子がUIシェーダープログラムに切り替わる
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] ui_shader_ UIシェーダーリソースインスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - ui_shader_ == NULL
 * - UIシェーダーリソースが保持するシェーダープログラムハンドルインスタンスがNULL
 * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - shader_handle_が保持するバーテックスシェーダーオブジェクトが未コンパイル
 * - shader_handle_が保持するフラグメントシェーダーオブジェクトが未コンパイル
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_shader_use(renderer_backend_context_t* backend_context_, ui_shader_t* ui_shader_);

/**
 * @brief GPUにモデル行列を送信する
 *
 * @note 本API実行後、backend_context_が保持する現在使用中のプログラムIDが切り替わる
 *
 * @param[in] model_matrix_ 送信するモデル行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 * @param[in,out] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] ui_shader_ UI描画用シェーダーリソースへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - model_matrix_ == NULL
 * - backend_context_ == NULL
 * - ui_shader_ == NULL
 * - ui_shaderが保持するシェーダープログラムハンドルインスタンスがNULL
 * @retval RENDERER_DATA_CORRUPTED ui_shader_が保持するシェーダープログラムハンドルインスタンスの内部データが破損
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク状態
 * - backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_shader_model_matrix_set(const mat4x4f_t* model_matrix_, bool should_transpose_, renderer_backend_context_t* backend_context_, ui_shader_t* ui_shader_);

/**
 * @brief GPUにビュー行列を送信する
 *
 * @note 本API実行後、backend_context_が保持する現在使用中のプログラムIDが切り替わる
 *
 * @param[in] view_matrix_ 送信するビュー行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 * @param[in,out] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] ui_shader_ UI描画用シェーダーリソースへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - view_matrix_ == NULL
 * - backend_context_ == NULL
 * - ui_shader_ == NULL
 * - ui_shaderが保持するシェーダープログラムハンドルインスタンスがNULL
 * @retval RENDERER_DATA_CORRUPTED ui_shader_が保持するシェーダープログラムハンドルインスタンスの内部データが破損
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク状態
 * - backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_shader_view_matrix_set(const mat4x4f_t* view_matrix_, bool should_transpose_, renderer_backend_context_t* backend_context_, ui_shader_t* ui_shader_);

/**
 * @brief GPUにプロジェクション行列を送信する
 *
 * @note 本API実行後、backend_context_が保持する現在使用中のプログラムIDが切り替わる
 *
 * @param[in] projection_matrix_ 送信するプロジェクション行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 * @param[in,out] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] ui_shader_ UI描画用シェーダーリソースへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - projection_matrix_ == NULL
 * - backend_context_ == NULL
 * - ui_shader_ == NULL
 * - ui_shaderが保持するシェーダープログラムハンドルインスタンスがNULL
 * @retval RENDERER_DATA_CORRUPTED ui_shader_が保持するシェーダープログラムハンドルインスタンスの内部データが破損
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク状態
 * - backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_shader_projection_matrix_set(const mat4x4f_t* projection_matrix_, bool should_transpose_, renderer_backend_context_t* backend_context_, ui_shader_t* ui_shader_);

#ifdef __cplusplus
}
#endif
#endif
