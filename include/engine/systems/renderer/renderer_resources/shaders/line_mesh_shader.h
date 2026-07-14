/** @ingroup renderer
 *
 * @file line_mesh_shader.h
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
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_LINE_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_LINE_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

typedef struct line_mesh_shader line_mesh_shader_t;                 /**< 線分描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

renderer_result_t line_mesh_shader_create(line_mesh_shader_t** out_line_mesh_shader_);

/**
 * @brief 線分描画用シェーダーリソースインスタンスが保持するリソースと、自身のメモリを解放する
 *
 * @note
 * - 有効なbackend_context_と有効なline_mesh_shader_が渡された場合、破棄後に*line_mesh_shader_はNULLに設定される
 * - 引数が無効の場合にはWARNINGメッセージを出力し、何もしない
 * - 2重デストロイ許可
 * - GPU側のシェーダープログラムリソースも破棄される
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] line_mesh_shader_ 破棄対象線分描画用シェーダーリソースインスタンスへのダブルポインタ
 */
void line_mesh_shader_destroy(renderer_backend_context_t* backend_context_, line_mesh_shader_t** line_mesh_shader_);

renderer_result_t line_mesh_shader_program_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, const char* file_path_, const char* name_);

renderer_result_t line_mesh_shader_vbo_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, buffer_usage_t buffer_usage_, size_t buffer_size_, size_t max_free_node_count_);

renderer_result_t line_mesh_shader_vao_initialize(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_);

/**
 * @brief 線分描画用シェーダーが保持するVAO / VBOを破棄する
 *
 * @note 有効なbackend_context_と有効なline_mesh_shader_が渡された場合、line_mesh_shader_tの内部状態は以下の状態に初期化される
 * - line_vbo = NULL
 * - line_vao = NULL
 * - current_buffer_offset = 0
 * - vertex_buffer_size = 0
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] line_mesh_shader_ VAO, VBOリソースを保持する線分描画用シェーダー構造体インスタンスへのポインタ
 */
void line_mesh_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_);

/**
 * @brief 線分描画用シェーダーが保持するVBOに頂点情報を転送する(バーテックスバッファへのappend)
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] line_mesh_shader_ 転送先VBOを保持する線分描画用シェーダー構造体インスタンスへのポインタ
 * @param[in] size_ 転送データサイズ
 * @param[in] write_data_ 転送データ
 * @param[out] out_vertex_offset_ 転送前にバーテックスバッファに転送されている頂点の数
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - line_mesh_shader_ == NULL
 * - write_data_ == NULL
 * - size_ == 0
 * - out_vertex_offset_ == NULL
 * - size_がsizeof(line_vertex_t) x 2の倍数ではない
 * @retval RENDERER_LIMIT_EXCEEDED 転送後にバーテックスバッファサイズを超過
 * @retval RENDERER_OVERFLOW 転送サイズ後のcurrent_buffer_offsetがSIZE_MAXを超過
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - VBO未初期化
 * - backend_context_が未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_mesh_shader_vertex_buffer_append(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* line_mesh_shader_, size_t size_, const line_vertex_t* write_data_, size_t* out_vertex_offset_);

/**
 * @brief 線分描画用シェーダーが保持するVAOをbindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] line_mesh_shader_ VAOを保持する線分描画用シェーダー構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - line_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION VAOが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_mesh_shader_vertex_array_bind(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_);

/**
 * @brief 線分描画用シェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @note 処理に成功した場合、現在使用中のプログラム識別子が線分描画用シェーダープログラムに切り替わる
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] line_mesh_shader_ 線分描画用シェーダーリソースインスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - line_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク
 * - line_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - バーテックスシェーダーオブジェクトが未コンパイル
 * - フラグメントシェーダーオブジェクトが未コンパイル
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_mesh_shader_use(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_);

/**
 * @brief GPUにモデル行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] line_mesh_shader_ 線分描画用シェーダーリソースへのポインタ
 * @param[in] model_matrix_ 送信するモデル行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - model_matrix_ == NULL
 * - backend_context_ == NULL
 * - line_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - line_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

/**
 * @brief GPUにビュー行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] line_mesh_shader_ 線分描画用シェーダーリソースへのポインタ
 * @param[in] view_matrix_ 送信するビュー行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - view_matrix_ == NULL
 * - backend_context_ == NULL
 * - line_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - line_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

/**
 * @brief GPUにプロジェクション行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] line_mesh_shader_ 線分描画用シェーダーリソースへのポインタ
 * @param[in] projection_matrix_ 送信するプロジェクション行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - projection_matrix_ == NULL
 * - backend_context_ == NULL
 * - line_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - line_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

/**
 * @brief GPUに色情報を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] line_mesh_shader_ 線分描画用シェーダーリソースへのポインタ
 * @param[in] color_ 送信する色情報配列(格納順: RGB(4byte目はpadding) / 各要素の値: 0...255 / backend側でshader uniform vec4用に0.0〜1.0に正規化される)
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - color_ == NULL
 * - backend_context_ == NULL
 * - line_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - line_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t line_mesh_shader_color_set(const renderer_backend_context_t* backend_context_, const line_mesh_shader_t* line_mesh_shader_, const uint8_t color_[4]);

#ifdef __cplusplus
}
#endif
#endif
