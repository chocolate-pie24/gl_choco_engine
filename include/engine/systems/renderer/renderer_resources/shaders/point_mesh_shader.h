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

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

typedef struct point_mesh_shader point_mesh_shader_t;   /**< ポイント描画シェーダーリソース構造体前方宣言 */

/**
 * @brief ポイント描画用シェーダーリソースインスタンスのメモリを確保し初期化する
 *
 * @details 以下の処理を行う
 * - out_point_mesh_shader_自身のリソース確保
 * - シェーダーソースのコンパイル
 * - シェーダーモジュールのリンク
 * - ポイント描画用シェーダーが扱うモデル行列のLocation取得
 * - ポイント描画用シェーダーが扱うビュー行列のLocation取得
 * - ポイント描画用シェーダーが扱うプロジェクション行列のLocation取得
 *
 * @param[in] file_path_ シェーダーソース格納ファイルパス(文字列の最後を'/'にすること)
 * @param[in] name_ シェーダーソースファイル名称(拡張子は含まない)
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[out] out_point_mesh_shader_ リソース確保対象ポイント描画用シェーダーリソースへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - file_path_ == NULL
 * - name_ == NULL
 * - backend_context_ == NULL
 * - out_point_mesh_shader_ == NULL
 * - *out_point_mesh_shader_ != NULL
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
renderer_result_t point_mesh_shader_create(const char* file_path_, const char* name_, renderer_backend_context_t* backend_context_, point_mesh_shader_t** out_point_mesh_shader_);

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

/**
 * @brief ポイント描画用シェーダー用のバーテックスバッファを生成する
 *
 * @note バーテックスバッファ構成
 * - layout 0: 頂点座標。float x 3。
 * - layout 1: 色情報。uint8_t x 4 のRGBA, OpenGL側で0.0〜1.0に正規化され、shaderではvec4として扱われる。
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in,out] point_mesh_shader_ バーテックスバッファ生成対象ポイント描画用シェーダーリソースインスタンスへのポインタ
 * @param[in] point_buffer_usage_ 頂点情報バッファ使用用途(DYNAMIC / STATIC)
 * @param[in] color_buffer_usage_ 色情報バッファ使用用途(DYNAMIC / STATIC)
 * @param[in] point_buffer_size_ 頂点情報バーテックスバッファサイズ(byte)
 * @param[in] color_buffer_size_ 色情報バーテックスバッファサイズ(byte)
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - point_mesh_shader_ == NULL
 * - point_buffer_size_ == 0
 * - color_buffer_size_ == 0
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - backend_context_が未初期化
 * - point_mesh_shader_->point_vao != NULL
 * - point_mesh_shader_->point_vbo != NULL
 * - point_mesh_shader_->color_vbo != NULL
 * - point_mesh_shader_->point_current_buffer_offset != 0
 * - point_mesh_shader_->color_current_buffer_offset != 0
 * - point_mesh_shader_->current_vertex_count != 0
 * - メモリシステム未初期化
 * @retval RENDERER_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_RUNTIME_ERROR buffer_usage_またはbuffer_size_が規定値外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
renderer_result_t point_mesh_shader_vertex_buffer_create(renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_, buffer_usage_t point_buffer_usage_, buffer_usage_t color_buffer_usage_, size_t point_buffer_size_, size_t color_buffer_size_);

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
void point_mesh_shader_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, point_mesh_shader_t* point_mesh_shader_);

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
 * @retval RENDERER_LIMIT_EXCEEDED 転送サイズ後のcurrent_buffer_offsetがSIZE_MAXを超過
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - VBO未初期化
 * - 転送後にバーテックスバッファサイズを超過
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
