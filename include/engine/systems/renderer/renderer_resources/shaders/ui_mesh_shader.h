/** @ingroup renderer
 *
 * @file ui_mesh_shader.h
 * @author chocolate-pie24
 * @brief UIシェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIを提供する
 *
 * @version 0.1
 * @date 2026-03-11
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_UI_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_UI_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

typedef struct ui_mesh_shader ui_mesh_shader_t;                     /**< UI描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

/**
 * @brief UIシェーダーリソースインスタンスのメモリを確保し初期化する
 *
 * @details 以下の処理を行う
 * - out_ui_mesh_shader_自身のリソース確保
 * - シェーダーソースのコンパイル
 * - シェーダーモジュールのリンク
 * - UIシェーダーが扱うモデル行列のLocation取得
 * - UIシェーダーが扱うビュー行列のLocation取得
 * - UIシェーダーが扱うプロジェクション行列のLocation取得
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] file_path_ シェーダーソース格納ファイルパス(文字列の最後を'/'にすること)
 * @param[in] name_ シェーダーソースファイル名称(拡張子は含まない)
 * @param[out] out_ui_mesh_shader_ リソース確保対象UIシェーダーリソースへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - file_path_ == NULL
 * - name_ == NULL
 * - backend_context_ == NULL
 * - out_ui_mesh_shader_ == NULL
 * - *out_ui_mesh_shader_ != NULL
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
renderer_result_t ui_mesh_shader_create(renderer_backend_context_t* backend_context_, const char* file_path_, const char* name_, ui_mesh_shader_t** out_ui_mesh_shader_);

/**
 * @brief UIシェーダーリソースインスタンスが保持するリソースと、自身のメモリを解放する
 *
 * @note
 * - 有効なbackend_context_と有効なui_mesh_shader_が渡された場合、破棄後に*ui_mesh_shader_はNULLに設定される
 * - 引数が無効の場合はWARNINGを出力し、何もしない
 * - 2重デストロイ許可
 * - GPU側のシェーダープログラムリソースも破棄される
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] ui_mesh_shader_ 破棄対象UIシェーダーリソースインスタンスへのダブルポインタ
 */
void ui_mesh_shader_destroy(renderer_backend_context_t* backend_context_, ui_mesh_shader_t** ui_mesh_shader_);

/**
 * @brief UIシェーダー用のバーテックスバッファを生成する
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] ui_mesh_shader_ バーテックスバッファ生成対象UIシェーダーリソースインスタンスへのポインタ
 * @param[in] buffer_usage_ バッファ使用用途(DYNAMIC / STATIC)
 * @param[in] buffer_size_ バーテックスバッファサイズ(byte)
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - ui_mesh_shader_ == NULL
 * - buffer_size_ == 0
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化
 * - ui_mesh_shader_->ui_vao != NULL
 * - ui_mesh_shader_->ui_vbo != NULL
 * - ui_mesh_shader_->current_buffer_offset != 0
 * - ui_mesh_shader_->current_vertex_count != 0
 * - メモリシステム未初期化
 * @retval RENDERER_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_RUNTIME_ERROR buffer_usage_またはbuffer_size_が規定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_mesh_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, ui_mesh_shader_t* ui_mesh_shader_, buffer_usage_t buffer_usage_, size_t buffer_size_);

/**
 * @brief UIシェーダーが保持するVAO / VBOを破棄する
 *
 * @note 有効なbackend_context_と有効なui_mesh_shader_が渡された場合、ui_mesh_shader_tの内部状態は以下の状態に初期化される
 * - ui_vbo = NULL
 * - ui_vao = NULL
 * - current_buffer_offset = 0
 * - vertex_buffer_size = 0
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] ui_mesh_shader_ VAO, VBOリソースを保持するUIシェーダー構造体インスタンスへのポインタ
 */
void ui_mesh_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, ui_mesh_shader_t* ui_mesh_shader_);

/**
 * @brief UIシェーダーが保持するVBOに頂点情報を転送する(バーテックスバッファへのappend)
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] ui_mesh_shader_ 転送先VBOを保持するUIシェーダー構造体インスタンスへのポインタ
 * @param[in] size_ 転送データサイズ
 * @param[in] write_data_ 転送データ
 * @param[out] out_vertex_offset_ 転送前にバーテックスバッファに転送されている頂点の数
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - ui_mesh_shader_ == NULL
 * - write_data_ == NULL
 * - size_ == 0
 * - out_vertex_offset_ == NULL
 * - size_ != sizeof(ui_vertex_t) x 6
 * @retval RENDERER_LIMIT_EXCEEDED 転送後にバーテックスバッファサイズを超過
 * @retval RENDERER_OVERFLOW 転送サイズ後のcurrent_buffer_offsetがSIZE_MAXを超過
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - VBO未初期化
 * - backend_context_が未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_mesh_shader_vertex_buffer_append(const renderer_backend_context_t* backend_context_, ui_mesh_shader_t* ui_mesh_shader_, size_t size_, const ui_vertex_t* write_data_, size_t* out_vertex_offset_);

/**
 * @brief UIシェーダーが保持するVAOをbindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] ui_mesh_shader_ VAOを保持するUIシェーダー構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - ui_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION VAOが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_mesh_shader_vertex_array_bind(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_);

/**
 * @brief UIシェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @note 処理に成功した場合、現在使用中のプログラム識別子がUIシェーダープログラムに切り替わる
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] ui_mesh_shader_ UIシェーダーリソースインスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - ui_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク
 * - ui_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - バーテックスシェーダーオブジェクトが未コンパイル
 * - フラグメントシェーダーオブジェクトが未コンパイル
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_mesh_shader_use(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_);

/**
 * @brief GPUにモデル行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] ui_mesh_shader_ UI描画用シェーダーリソースへのポインタ
 * @param[in] model_matrix_ 送信するモデル行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - model_matrix_ == NULL
 * - backend_context_ == NULL
 * - ui_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - ui_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

/**
 * @brief GPUにビュー行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] ui_mesh_shader_ UI描画用シェーダーリソースへのポインタ
 * @param[in] view_matrix_ 送信するビュー行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - view_matrix_ == NULL
 * - backend_context_ == NULL
 * - ui_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - ui_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

/**
 * @brief GPUにプロジェクション行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] ui_mesh_shader_ UI描画用シェーダーリソースへのポインタ
 * @param[in] projection_matrix_ 送信するプロジェクション行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - projection_matrix_ == NULL
 * - backend_context_ == NULL
 * - ui_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - ui_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t ui_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const ui_mesh_shader_t* ui_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

#ifdef __cplusplus
}
#endif
#endif
