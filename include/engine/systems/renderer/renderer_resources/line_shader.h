/** @ingroup renderer
 *
 * @file line_shader.h
 * @author chocolate-pie24
 * @brief 線分描画用シェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIを提供する
 *
 * @version 0.1
 * @date 2026-05-28
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_LINE_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_LINE_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

typedef struct line_shader line_shader_t;   /**< 線分描画シェーダーリソース構造体前方宣言 */

/**
 * @brief 線分描画用シェーダーリソースインスタンスのメモリを確保し初期化する
 *
 * @details 以下の処理を行う
 * - out_line_shader_自身のリソース確保
 * - シェーダーソースのコンパイル
 * - シェーダーモジュールのリンク
 * - 線分描画用シェーダーが扱うモデル行列のLocation取得
 * - 線分描画用シェーダーが扱うビュー行列のLocation取得
 * - 線分描画用シェーダーが扱うプロジェクション行列のLocation取得
 * - 線分描画用シェーダーが扱う色情報のLocation取得
 *
 * @param[in] file_path_ シェーダーソース格納ファイルパス(文字列の最後を'/'にすること)
 * @param[in] name_ シェーダーソースファイル名称(拡張子は含まない)
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[out] out_line_shader_ リソース確保対象線分描画用シェーダーリソースへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - file_path_ == NULL
 * - name_ == NULL
 * - backend_context_ == NULL
 * - out_line_shader_ == NULL
 * - *out_line_shader_ != NULL
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
 * - メモリシステム未初期化
 * @retval RENDERER_SHADER_COMPILE_ERROR 以下のいずれか
 * - シェーダーモジュールのGPU側リソース確保に失敗
 * - シェーダーソースのコンパイルに失敗
 * @retval RENDERER_SHADER_LINK_ERROR シェーダーモジュールのリンクに失敗
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, line_shader_t** out_line_shader_);

/**
 * @brief 線分描画用シェーダーリソースインスタンスが保持するリソースと、自身のメモリを解放する
 *
 * @note
 * - 有効なbackend_context_と有効なline_shader_が渡された場合、破棄後に*line_shader_はNULLに設定される
 * - 引数が無効の場合にはWARNINGメッセージを出力し、何もしない
 * - 2重デストロイ許可
 * - GPU側のシェーダープログラムリソースも破棄される
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] line_shader_ 破棄対象線分描画用シェーダーリソースインスタンスへのダブルポインタ
 */
void line_shader_destroy(renderer_backend_context_t* backend_context_, line_shader_t** line_shader_);

/**
 * @brief 線分描画用シェーダー用のバーテックスバッファを生成する
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] line_shader_ バーテックスバッファ生成対象線分描画用シェーダーリソースインスタンスへのポインタ
 * @param[in] buffer_usage_ バッファ使用用途(DYNAMIC / STATIC)
 * @param[in] buffer_size_ バーテックスバッファサイズ(byte)
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - line_shader_ == NULL
 * - buffer_size_ == 0
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化
 * - line_shader_->line_vao != NULL
 * - line_shader_->line_vbo != NULL
 * - line_shader_->current_buffer_offset != 0
 * - メモリシステム未初期化
 * @retval RENDERER_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_RUNTIME_ERROR buffer_usage_またはbuffer_size_が規定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_, buffer_usage_t buffer_usage_, size_t buffer_size_);

/**
 * @brief 線分描画用シェーダーが保持するVAO / VBOを破棄する
 *
 * @note 有効なbackend_context_と有効なline_shader_が渡された場合、line_shader_tの内部状態は以下の状態に初期化される
 * - line_vbo = NULL
 * - line_vao = NULL
 * - current_buffer_offset = 0
 * - vertex_buffer_size = 0
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] line_shader_ VAO, VBOリソースを保持する線分描画用シェーダー構造体インスタンスへのポインタ
 */
void line_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_);

/**
 * @brief 線分描画用シェーダーが保持するVBOに頂点情報を転送する(バーテックスバッファへのappend)
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] line_shader_ 転送先VBOを保持する線分描画用シェーダー構造体インスタンスへのポインタ
 * @param[in] size_ 転送データサイズ
 * @param[in] write_data_ 転送データ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - line_shader_ == NULL
 * - write_data_ == NULL
 * - size_ == 0
 * @retval RENDERER_LIMIT_EXCEEDED 転送サイズ後のcurrent_buffer_offsetがSIZE_MAXを超過
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - VBO未初期化
 * - 転送後にバーテックスバッファサイズを超過
 * - backend_context_が未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 *
 * @todo TODO: void* -> line_vertex_t*, size_のチェック(line_vertex_tのサイズ x 2 x n)
 * @todo TODO: バッファの途中だけを書き換えるAPI追加した後で他のシェーダーリソースも含めてwrite -> appendに変更する
 */
renderer_result_t line_shader_vertex_buffer_write(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_, size_t size_, const void* write_data_);

/**
 * @brief 線分描画用シェーダーが保持するVAOをbindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] line_shader_ VAOを保持する線分描画用シェーダー構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - line_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION VAOが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_shader_vertex_array_bind(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_);

/**
 * @brief 線分描画用シェーダーが保持するVAOをunbindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] line_shader_ VAOを保持する線分描画用シェーダー構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - line_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION VAOが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_shader_vertex_array_unbind(renderer_backend_context_t* backend_context_, line_shader_t* line_shader_);

/**
 * @brief 線分描画用シェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @note 処理に成功した場合、現在使用中のプログラム識別子が線分描画用シェーダープログラムに切り替わる
 *
 * @param[in] line_shader_ 線分描画用シェーダーリソースインスタンスへのポインタ
 * @param[in,out] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - line_shader_ == NULL
 * - 線分描画用シェーダーリソースが保持するシェーダープログラムハンドルインスタンスがNULL
 * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - shader_handle_が保持するバーテックスシェーダーオブジェクトが未コンパイル
 * - shader_handle_が保持するフラグメントシェーダーオブジェクトが未コンパイル
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_shader_use(const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

/**
 * @brief GPUにモデル行列を送信する
 *
 * @note 本API実行後、backend_context_が保持する現在使用中のプログラムIDが切り替わる
 *
 * @param[in] model_matrix_ 送信するモデル行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 * @param[in] line_shader_ 線分描画用シェーダーリソースへのポインタ
 * @param[in,out] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - model_matrix_ == NULL
 * - backend_context_ == NULL
 * - line_shader_ == NULL
 * - line_shader_が保持するシェーダープログラムハンドルインスタンスがNULL
 * @retval RENDERER_DATA_CORRUPTED line_shader_が保持するシェーダープログラムハンドルインスタンスの内部データが破損
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク状態
 * - backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_shader_model_matrix_set(const mat4x4f_t* model_matrix_, bool should_transpose_, const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

/**
 * @brief GPUにビュー行列を送信する
 *
 * @note 本API実行後、backend_context_が保持する現在使用中のプログラムIDが切り替わる
 *
 * @param[in] view_matrix_ 送信するビュー行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 * @param[in] line_shader_ 線分描画用シェーダーリソースへのポインタ
 * @param[in,out] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - view_matrix_ == NULL
 * - backend_context_ == NULL
 * - line_shader_ == NULL
 * - line_shader_が保持するシェーダープログラムハンドルインスタンスがNULL
 * @retval RENDERER_DATA_CORRUPTED line_shader_が保持するシェーダープログラムハンドルインスタンスの内部データが破損
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク状態
 * - backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_shader_view_matrix_set(const mat4x4f_t* view_matrix_, bool should_transpose_, const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

/**
 * @brief GPUにプロジェクション行列を送信する
 *
 * @note 本API実行後、backend_context_が保持する現在使用中のプログラムIDが切り替わる
 *
 * @param[in] projection_matrix_ 送信するプロジェクション行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 * @param[in] line_shader_ 線分描画用シェーダーリソースへのポインタ
 * @param[in,out] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - projection_matrix_ == NULL
 * - backend_context_ == NULL
 * - line_shader_ == NULL
 * - line_shader_が保持するシェーダープログラムハンドルインスタンスがNULL
 * @retval RENDERER_DATA_CORRUPTED line_shader_が保持するシェーダープログラムハンドルインスタンスの内部データが破損
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク状態
 * - backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_shader_projection_matrix_set(const mat4x4f_t* projection_matrix_, bool should_transpose_, const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

/**
 * @brief GPUに色情報を送信する
 *
 * @note 本API実行後、backend_context_が保持する現在使用中のプログラムIDが切り替わる
 *
 * @param[in] color_ 送信する色情報配列(格納順: RGBA / 各要素の値: 0...255 / backend側でshader uniform vec4用に0.0〜1.0に正規化される)
 * @param[in] line_shader_ 線分描画用シェーダーリソースへのポインタ
 * @param[in,out] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - color_ == NULL
 * - backend_context_ == NULL
 * - line_shader_ == NULL
 * - line_shader_が保持するシェーダープログラムハンドルインスタンスがNULL
 * @retval RENDERER_DATA_CORRUPTED line_shader_が保持するシェーダープログラムハンドルインスタンスの内部データが破損
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク状態
 * - backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_shader_color_set(const uint8_t color_[4], const line_shader_t* line_shader_, renderer_backend_context_t* backend_context_);

#ifdef __cplusplus
}
#endif
#endif
