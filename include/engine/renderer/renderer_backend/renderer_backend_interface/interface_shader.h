/**
 * @ingroup renderer_backend_interface
 *
 * @file interface_shader.h
 * @author chocolate-pie24
 * @brief shaderプログラム操作関数をまとめたvtableを定義する
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
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_SHADER_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_core/renderer_types.h"

/**
 * @brief シェーダー構造体インスタンスのメモリを確保し、renderer_backend_shader_tインスタンスのフィールドを全て0で初期化する
 *
 * @note shader_handle_のリソース管理は本モジュールで行うため、確保されたメモリは使用者が @ref renderer_shader_vtable_t が保有するdestroy関数を呼び出して解放する
 *
 * @param[out] shader_handle_ renderer_backend_shader_t構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - *shader_handle_ != NULL
 * - メモリシステム未初期化(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_UNDEFINED_ERROR 想定していない実行結果コードを処理過程で受け取った(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_SUCCESS メモリ確保および初期化に成功し、正常終了
 * @retval 上記以外 グラフィックスAPI実装依存
 */
typedef renderer_result_t (*pfn_renderer_shader_create)(renderer_backend_shader_t** shader_handle_);

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
 * @param[in,out] shader_handle_ 破棄対象構造体インスタンスへのダブルポインタ
 */
typedef void (*pfn_renderer_shader_destroy)(renderer_backend_shader_t** shader_handle_);

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
typedef renderer_result_t (*pfn_renderer_shader_compile)(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);

/**
 * @brief コンパイル済みのシェーダープログラムをリンクする
 *
 * @note 現状ではバーテックスシェーダーとフラグメントシェーダーのみを対象にリンクする
 *
 * @details 以下の処理を行う
 * - リンクして生成されるプログラムのGPU側リソースを確保する
 * - シェーダープログラムのリンク
 *
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
typedef renderer_result_t (*pfn_renderer_shader_link)(renderer_backend_shader_t* shader_handle_);

/**
 * @brief シェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @note 現在使用中のプログラム識別子(out_program_id_)がshader_handle_が保持するプログラム識別子と同一の場合は、既に使用中とし何もしない
 *
 * @param[in] shader_handle_ シェーダープログラムハンドル格納構造体インスタンス
 * @param[in,out] out_program_id_ 現在使用中のプログラム識別子で、本関数実行後に更新される
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - out_program_id_ == NULL
 * @retval RENDERER_BAD_OPERATION シェーダープログラムが未リンク
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - shader_handle_が保持するバーテックスシェーダープログラムが未コンパイル
 * - shader_handle_が保持するフラグメントシェーダープログラムが未コンパイル
 */
typedef renderer_result_t (*pfn_renderer_shader_use)(renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_);

/**
 * @brief シェーダー機能仮想関数テーブル
 *
 */
typedef struct renderer_shader_vtable {
    pfn_renderer_shader_create renderer_shader_create;      /**< 関数ポインタ @ref pfn_renderer_shader_create 参照 */
    pfn_renderer_shader_destroy renderer_shader_destroy;    /**< 関数ポインタ @ref pfn_renderer_shader_destroy 参照 */
    pfn_renderer_shader_compile renderer_shader_compile;    /**< 関数ポインタ @ref pfn_renderer_shader_compile 参照 */
    pfn_renderer_shader_link renderer_shader_link;          /**< 関数ポインタ @ref pfn_renderer_shader_link 参照 */
    pfn_renderer_shader_use renderer_shader_use;            /**< 関数ポインタ @ref pfn_renderer_shader_use 参照 */
} renderer_shader_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
