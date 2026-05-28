/**
 * @ingroup renderer
 *
 * @file interface_shader.h
 * @author chocolate-pie24
 * @brief シェーダープログラム/シェーダーオブジェクトの操作関数をまとめたvtableを定義する
 *
 * @note
 * - シェーダープログラム: シェーダーオブジェクトをリンクしたプログラム
 * - シェーダーオブジェクト: コンパイルされた各シェーダーステージごとのオブジェクト
 *
 * @version 0.1
 * @date 2026-02-23
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_core/renderer_types.h"

typedef renderer_result_t (*pfn_renderer_shader_create)(renderer_backend_shader_t** shader_handle_);    /**< renderer_shader_vtableが保持するrenderer_shader_createの前方宣言 */
typedef void (*pfn_renderer_shader_destroy)(renderer_backend_shader_t** shader_handle_);    /**< renderer_shader_vtableが保持するrenderer_shader_destroyの前方宣言 */
typedef renderer_result_t (*pfn_renderer_shader_compile)(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);    /**< renderer_shader_vtableが保持するrenderer_shader_compileの前方宣言 */
typedef renderer_result_t (*pfn_renderer_shader_link)(renderer_backend_shader_t* shader_handle_);   /**< renderer_shader_vtableが保持するrenderer_shader_linkの前方宣言 */
typedef renderer_result_t (*pfn_renderer_shader_use)(const renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_);   /**< renderer_shader_vtableが保持するrenderer_shader_useの前方宣言 */
typedef renderer_result_t (*pfn_renderer_shader_uniform_location_get)(const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);  /**< renderer_shader_vtableが保持するrenderer_shader_uniform_location_getの前方宣言 */
typedef renderer_result_t (*pfn_renderer_shader_mat4f_uniform_set)(const renderer_backend_shader_t* shader_handle_, int32_t location_, bool should_transpose_, const float* data_, uint32_t* out_program_id_);  /**< renderer_shader_vtableが保持するrenderer_shader_mat4f_uniform_setの前方宣言 */
typedef renderer_result_t (*pfn_renderer_shader_vec4u8_uniform_set)(const renderer_backend_shader_t* shader_handle_, int32_t location_, const uint8_t* data_, uint32_t* out_program_id_);   /**< renderer_shader_vtableが保持するrenderer_shader_vec4u8_uniform_setの前方宣言 */

/**
 * @brief シェーダー機能仮想関数テーブル
 *
 */
typedef struct renderer_shader_vtable {
    /**
     * @brief シェーダーGPUリソース内部状態管理構造体インスタンスのメモリを確保し、フィールドを0で初期化する
     *
     * @param[out] shader_handle_ GPUリソース内部状態管理構造体インスタンスへのダブルポインタ
     *
     * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
     * - shader_handle_ == NULL
     * - *shader_handle_ != NULL
     * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システム使用可能範囲上限超過
     * @retval RENDERER_NO_MEMORY メモリ確保失敗
     * @retval RENDERER_BAD_OPERATION メモリシステム未初期化
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_create renderer_shader_create;

    /**
     * @brief シェーダーGPUリソース内部状態管理構造体が管理するリソースを破棄し、自身のメモリも解放する。メモリ解放後はNULLで初期化する
     *
     * @note 以下のGPUリソースを破棄する
     * - バーテックスシェーダー
     * - フラグメントシェーダー
     * - OpenGLシェーダープログラム
     * @note 2重destroyを許可する
     *
     * @param[in,out] shader_handle_ GPUリソース内部状態管理構造体インスタンスへのダブルポインタ
     */
    pfn_renderer_shader_destroy renderer_shader_destroy;

    /**
     * @brief シェーダーオブジェクトを生成し、シェーダーソースをコンパイルする
     *
     * @param[in] shader_type_ シェーダー種別指定値
     * @param[in] shader_source_ シェーダーソース
     * @param[in,out] shader_handle_ シェーダー関連リソース管理構造体インスタンスへのポインタ
     *
     * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
     * - shader_source_ == NULL
     * - shader_handle_ == NULL
     * - shader_type_が規定値外
     * @retval RENDERER_BAD_OPERATION 以下のいずれか
     * - 指定したシェーダー種別はすでにコンパイル済み
     * - シェーダープログラムがすでにリンク済み
     * - メモリシステム未初期化
     * @retval RENDERER_SHADER_COMPILE_ERROR シェーダーソースコンパイルエラー
     * @retval RENDERER_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
     * @retval RENDERER_NO_MEMORY メモリ確保失敗
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_compile renderer_shader_compile;

    /**
     * @brief コンパイル済みシェーダーオブジェクトをリンクし、シェーダープログラムを生成する
     *
     * @param[in,out] shader_handle_ シェーダー関連リソース管理構造体インスタンスへのポインタ
     *
     * @retval RENDERER_INVALID_ARGUMENT shader_handle_ == NULL
     * @retval RENDERER_BAD_OPERATION 以下のいずれか
     * - シェーダープログラムがすでにリンク済み
     * - バーテックスシェーダーが未コンパイル
     * - フラグメントシェーダーが未コンパイル
     * @retval RENDERER_SHADER_LINK_ERROR シェーダーリンクエラー
     * @retval RENDERER_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
     * @retval RENDERER_NO_MEMORY メモリ確保失敗
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_link renderer_shader_link;

    /**
     * @brief シェーダープログラムを切り替える
     *
     * @note 切り替え先シェーダープログラムがすでに使用中であれば切り替えは行わない
     *
     * @param[in] shader_handle_ 切り替え先シェーダープログラムを管理する内部状態管理構造体インスタンスへのポインタ
     * @param[in,out] out_program_id_ 現在使用中のシェーダープログラム識別子
     *
     * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
     * - shader_handle_ == NULL
     * - out_program_id_ == NULL
     * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク
     * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
     * - program_idが設定されているにもかかわらず、バーテックスシェーダーオブジェクトハンドルが未設定
     * - program_idが設定されているにもかかわらず、フラグメントシェーダーオブジェクトハンドルが未設定
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_use renderer_shader_use;

    /**
     * @brief シェーダープログラムのユニフォーム変数のLocationを取得する
     *
     * @param[in] shader_handle_ シェーダープログラムハンドルインスタンスへのポインタ
     * @param[in] name_ ユニフォーム変数名称
     * @param[out] out_location_ Location格納先
     *
     * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
     * - shader_handle_ == NULL
     * - name_ == NULL
     * - out_location_ == NULL
     * @retval RENDERER_RUNTIME_ERROR ユニフォーム変数が存在しない、未使用として最適化された、またはLocation取得に失敗
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_uniform_location_get renderer_shader_uniform_location_get;

    /**
     * @brief シェーダープログラムにmat4f型のユニフォーム変数を送信する
     *
     * @param[in] shader_handle_ シェーダープログラムハンドルインスタンスへのポインタ
     * @param[in] location_ ユニフォーム変数のLocation
     * @param[in] should_transpose_ true: 送信時に行列を転置する / false: 送信時に行列を転置しない
     * @param[in] data_ 送信データへのポインタ
     * @param[in,out] out_program_id_ 現在使用中のOpenGLプログラム識別子
     *
     * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
     * - shader_handle_ == NULL
     * - data_ == NULL
     * - out_program_id_ == NULL
     * @retval RENDERER_DATA_CORRUPTED シェーダープログラムハンドルインスタンスの内部データが破損
     * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク状態
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_mat4f_uniform_set renderer_shader_mat4f_uniform_set;

    /**
     * @brief シェーダープログラムにvec4u8型のユニフォーム変数を送信する
     *
     * @param[in] shader_handle_ シェーダープログラムハンドルインスタンスへのポインタ
     * @param[in] location_ ユニフォーム変数のLocation
     * @param[in] data_ 送信データへのポインタ
     * @param[in,out] out_program_id_ 現在使用中のOpenGLプログラム識別子
     *
     * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
     * - shader_handle_ == NULL
     * - data_ == NULL
     * - out_program_id_ == NULL
     * @retval RENDERER_DATA_CORRUPTED シェーダープログラムハンドルインスタンスの内部データが破損
     * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク状態
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_vec4u8_uniform_set renderer_shader_vec4u8_uniform_set;
} renderer_shader_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
