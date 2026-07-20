/** @ingroup renderer
 *
 * @file point_mesh_shader.h
 * @author chocolate-pie24
 * @brief ポイント描画用シェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIを提供する
 *
 * @version 0.1
 * @date 2026-05-29
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_POINT_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_POINT_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/renderer_core/renderer_geometry_types.h"

#include "engine/systems/renderer/renderer_resources/buffer_managers/vbo_manager.h"

typedef struct point_mesh_shader point_mesh_shader_t;               /**< 点描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

renderer_result_t point_mesh_shader_create(point_mesh_shader_t** out_point_mesh_shader_);

/**
 * @brief ポイント描画用シェーダーリソースインスタンスが保持するリソースと、自身のメモリを解放する
 *
 * @note
 * - 有効なbackend_context_と有効なpoint_mesh_shader_が渡された場合、破棄後に*point_mesh_shader_はNULLに設定される
 * - 引数が無効の場合にはWARNINGメッセージを出力し、何もしない
 * - 2重デストロイ許可
 * - GPU側のシェーダープログラムリソースも破棄される
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] point_mesh_shader_ 破棄対象ポイント描画用シェーダーリソースインスタンスへのダブルポインタ
 */
void point_mesh_shader_destroy(renderer_backend_context_t* backend_context_, point_mesh_shader_t** point_mesh_shader_);

renderer_result_t point_mesh_shader_program_initialize(renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, const char* file_path_, const char* name_);

renderer_result_t point_mesh_shader_vbo_initialize(renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, const vbo_manager_config_t* point_vbo_config_, const vbo_manager_config_t* color_vbo_config_);

renderer_result_t point_mesh_shader_vao_initialize(renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_);

/**
 * @brief ポイント描画用シェーダーが保持するVAO / VBOを破棄する
 *
 * @note 有効なbackend_context_と有効なpoint_mesh_shader_が渡された場合、point_mesh_shader_tの内部状態は以下の状態に初期化される
 * - point_vao = NULL
 * - point_vbo = NULL
 * - color_vbo = NULL
 * - point_current_buffer_offset = 0
 * - point_vertex_buffer_size = 0
 * - color_current_buffer_offset = 0
 * - color_vertex_buffer_size = 0
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] point_mesh_shader_ VAO, VBOリソースを保持するポイント描画用シェーダー構造体インスタンスへのポインタ
 */
void point_mesh_shader_vao_vbo_destroy(renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_);

// colorはマテリアルとして扱うため、geometry_pipelineでは座標情報のみをGPUに転送する
renderer_result_t point_mesh_shader_vbo_point_write(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, size_t size_, const point_vertex_t* write_data_, vertex_buffer_range_t* out_buffer_range_);

renderer_result_t point_mesh_shader_vbo_color_write(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, size_t size_, const vec4u8_t* write_data_, vertex_buffer_range_t* out_buffer_range_);

renderer_result_t point_mesh_shader_vbo_point_free(point_mesh_shader_t* point_mesh_shader_, const vertex_buffer_range_t* buffer_range_);

renderer_result_t color_mesh_shader_vbo_color_free(point_mesh_shader_t* point_mesh_shader_, const vertex_buffer_range_t* buffer_range_);

/**
 * @brief ポイント描画用シェーダーが保持する頂点情報VBOに頂点情報を転送する(バーテックスバッファへのappend)
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] point_mesh_shader_ 転送先VBOを保持するポイント描画用シェーダー構造体インスタンスへのポインタ
 * @param[in] size_ 転送データサイズ
 * @param[in] write_data_ 転送データ
 * @param[out] out_vertex_offset_ 転送前にバーテックスバッファに転送されている頂点の数
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - point_mesh_shader_ == NULL
 * - write_data_ == NULL
 * - size_ == 0
 * - out_vertex_offset_ == NULL
 * - size_がsizeof(point_vertex_t)の倍数ではない
 * @retval RENDERER_LIMIT_EXCEEDED 転送サイズ後のcurrent_buffer_offsetがSIZE_MAXを超過
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - VBO未初期化
 * - 転送後にバーテックスバッファサイズを超過
 * - backend_context_が未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t point_mesh_shader_vertex_buffer_point_append(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, size_t size_, const point_vertex_t* write_data_, size_t* out_vertex_offset_);

/**
 * @brief ポイント描画用シェーダーが保持する色情報VBOに色情報を転送する(バーテックスバッファへのappend)
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] point_mesh_shader_ 転送先VBOを保持するポイント描画用シェーダー構造体インスタンスへのポインタ
 * @param[in] size_ 転送データサイズ
 * @param[in] write_data_ 転送データ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - point_mesh_shader_ == NULL
 * - write_data_ == NULL
 * - size_ == 0
 * @retval RENDERER_LIMIT_EXCEEDED 転送後にバーテックスバッファサイズを超過
 * @retval RENDERER_OVERFLOW 転送サイズ後のcurrent_buffer_offsetがSIZE_MAXを超過
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - VBO未初期化
 * - backend_context_が未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t point_mesh_shader_vertex_buffer_color_append(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, size_t size_, const vec4u8_t* write_data_);

/**
 * @brief ポイント描画用シェーダーが保持するVAOをbindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] point_mesh_shader_ VAOを保持するポイント描画用シェーダー構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - point_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION VAOが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t point_mesh_shader_vertex_array_bind(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_);

/**
 * @brief ポイント描画用シェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @note 処理に成功した場合、現在使用中のプログラム識別子がポイント描画用シェーダープログラムに切り替わる
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] point_mesh_shader_ ポイント描画用シェーダーリソースインスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - point_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク
 * - point_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - バーテックスシェーダーオブジェクトが未コンパイル
 * - フラグメントシェーダーオブジェクトが未コンパイル
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t point_mesh_shader_use(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_);

/**
 * @brief GPUにモデル行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] point_mesh_shader_ ポイント描画用シェーダーリソースへのポインタ
 * @param[in] model_matrix_ 送信するモデル行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - model_matrix_ == NULL
 * - backend_context_ == NULL
 * - point_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - point_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t point_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

/**
 * @brief GPUにビュー行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] point_mesh_shader_ ポイント描画用シェーダーリソースへのポインタ
 * @param[in] view_matrix_ 送信するビュー行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - view_matrix_ == NULL
 * - backend_context_ == NULL
 * - point_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - point_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t point_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

/**
 * @brief GPUにプロジェクション行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] point_mesh_shader_ ポイント描画用シェーダーリソースへのポインタ
 * @param[in] projection_matrix_ 送信するプロジェクション行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - projection_matrix_ == NULL
 * - backend_context_ == NULL
 * - point_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - point_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t point_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const point_mesh_shader_t* point_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

#ifdef __cplusplus
}
#endif
#endif
