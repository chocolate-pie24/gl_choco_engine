/** @ingroup renderer
 *
 * @file lit_mesh_shader.h
 * @author chocolate-pie24
 * @brief 光源・法線・材質色などを使って、陰影付きでmeshを描画するためのシェーダーであるlit_meshシェーダーリソースの生成・破棄、VAO/VBO管理、uniform送信APIを提供する
 *
 * @note ライティング等がまだ未実装なので、当面は単色描画となる
 * @todo ライティング等の実装後、以下をメンテナンスする
 * - docs/layer.md
 * - docs/architecture/systems/renderer_system/renderer_system_ja(en).md
 *
 * @version 0.1
 * @date 2026-06-03
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_LIT_MESH_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_RESOURCES_SHADERS_LIT_MESH_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/renderer_resources/shaders/core/shader_types.h"
#include "engine/systems/renderer/renderer_resources/buffer_managers/vbo_manager.h"

typedef struct lit_mesh_shader lit_mesh_shader_t;                   /**< 単色ライティング描画用シェーダーリソースのopaque型 */

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

renderer_result_t lit_mesh_shader_create(lit_mesh_shader_t** out_lit_mesh_shader_);

/**
 * @brief lit_meshシェーダーリソースインスタンスが保持するリソースと、自身のメモリを解放する
 *
 * @note
 * - 有効なbackend_context_と有効なlit_mesh_shader_が渡された場合、破棄後に*lit_mesh_shader_はNULLに設定される
 * - 引数が無効の場合はWARNINGを出力し、何もしない
 * - 2重デストロイ許可
 * - GPU側のシェーダープログラムリソースも破棄される
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] lit_mesh_shader_ 破棄対象lit_meshシェーダーリソースインスタンスへのダブルポインタ
 */
void lit_mesh_shader_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t** lit_mesh_shader_);

renderer_result_t lit_mesh_shader_program_initialize(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, const char* file_path_, const char* name_);

renderer_result_t lit_mesh_shader_vbo_initialize(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, const vbo_manager_config_t* vbo_config_);

renderer_result_t lit_mesh_shader_vao_initialize(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_);

/**
 * @brief lit_meshシェーダーが保持するVAO / VBOを破棄する
 *
 * @note 有効なbackend_context_と有効なlit_mesh_shader_が渡された場合、lit_mesh_shader_tの内部状態は以下の状態に初期化される
 * - lit_mesh_vbo = NULL
 * - lit_mesh_vao = NULL
 * - current_buffer_offset = 0
 * - vertex_buffer_size = 0
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] lit_mesh_shader_ VAO, VBOリソースを保持するlit_meshシェーダー構造体インスタンスへのポインタ
 */
void lit_mesh_shader_vao_vbo_destroy(renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_);

renderer_result_t lit_mesh_shader_vbo_write(const renderer_backend_context_t* backend_context_, lit_mesh_shader_t* lit_mesh_shader_, size_t vertex_count_, const point_normal_vertex_t* vertices_, vertex_buffer_range_t* out_buffer_range_);

renderer_result_t lit_mesh_shader_vbo_free(lit_mesh_shader_t* lit_mesh_shader_, const vertex_buffer_range_t* buffer_range_);

/**
 * @brief lit_meshシェーダーが保持するVAOをbindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] lit_mesh_shader_ VAOを保持するlit_meshシェーダー構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - lit_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION VAOが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t lit_mesh_shader_vertex_array_bind(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_);

/**
 * @brief lit_meshシェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @note 処理に成功した場合、現在使用中のプログラム識別子がlit_meshシェーダープログラムに切り替わる
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] lit_mesh_shader_ lit_meshシェーダーリソースインスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - lit_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムが未リンク
 * - lit_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - バーテックスシェーダーオブジェクトが未コンパイル
 * - フラグメントシェーダーオブジェクトが未コンパイル
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t lit_mesh_shader_use(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_);

/**
 * @brief GPUにモデル行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] lit_mesh_shader_ lit_mesh描画用シェーダーリソースへのポインタ
 * @param[in] model_matrix_ 送信するモデル行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - model_matrix_ == NULL
 * - backend_context_ == NULL
 * - lit_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - lit_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t lit_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_, const mat4x4f_t* model_matrix_, bool should_transpose_);

/**
 * @brief GPUにビュー行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] lit_mesh_shader_ lit_mesh描画用シェーダーリソースへのポインタ
 * @param[in] view_matrix_ 送信するビュー行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - view_matrix_ == NULL
 * - backend_context_ == NULL
 * - lit_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - lit_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t lit_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_, const mat4x4f_t* view_matrix_, bool should_transpose_);

/**
 * @brief GPUにプロジェクション行列を送信する
 *
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] lit_mesh_shader_ lit_mesh描画用シェーダーリソースへのポインタ
 * @param[in] projection_matrix_ 送信するプロジェクション行列のポインタ
 * @param[in] should_transpose_ true: 送信時に行列を転置する, false: 送信時に行列を転置しない
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - projection_matrix_ == NULL
 * - backend_context_ == NULL
 * - lit_mesh_shader_ == NULL
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化でshader_vtableがNULL
 * - lit_mesh_shader_が保持するシェーダーハンドルが未初期化
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t lit_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const lit_mesh_shader_t* lit_mesh_shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_);

#ifdef __cplusplus
}
#endif
#endif
