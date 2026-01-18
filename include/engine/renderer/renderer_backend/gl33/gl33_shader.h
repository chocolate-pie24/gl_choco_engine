/** @ingroup gl33
 *
 * @file gl33_shader.h
 * @author chocolate-pie24
 * @brief OpenGL3.3用のシェーダープログラムのコンパイル、リンク、使用開始APIを提供する
 *
 * @version 0.1
 * @date 2026-01-03
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_GL33_GL33_SHADER_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_GL33_GL33_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_core/renderer_types.h"

typedef struct gl33_shader gl33_shader_t; /**< OpenGL3.3用シェーダーモジュール内部状態管理構造体前方宣言 */

/**
 * @brief OpenGL3.3用のシェーダーモジュールのメモリを確保し初期化する
 *
 * @code{.c}
 * gl33_shader_t* shader_handle = NULL;
 * renderer_result_t ret = gl33_shader_create(&shader_handle);
 * // エラー処理
 * @endcode
 *
 * @param shader_handle_ 内部状態管理構造体へのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - *shader_handle_ != NULL
 * - メモリシステム未初期化(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_UNDEFINED_ERROR 想定していない実行結果コードを処理過程で受け取った(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_SUCCESS メモリ確保および初期化に成功し、正常終了
 */
renderer_result_t gl33_shader_create(gl33_shader_t** shader_handle_);

/**
 * @brief shader_handle_が保持するシェーダーハンドルを削除、シェーダープログラムを削除し、shader_handle_のメモリを解放する
 *
 * @note 本APIは2重デストロイを許可する
 *
 * @code{.c}
 * gl33_shader_t* shader_handle = NULL;
 * renderer_result_t ret = gl33_shader_create(&shader_handle);
 * // エラー処理
 * gl33_shader_destroy(&shader_handle); // shader_handle == NULLになる
 * gl33_shader_destroy(&shader_handle); // 2重デストロイ許可
 * @endcode
 *
 * @param shader_handle_ 破棄対象シェーダーハンドル構造体インスタンスへのダブルポインタ
 */
void gl33_shader_destroy(gl33_shader_t** shader_handle_);

/**
 * @brief シェーダープログラムをコンパイルする
 *
 * @code{.c}
 * gl33_shader_t* shader_handle = NULL;
 * renderer_result_t ret = gl33_shader_create(&shader_handle);
 * // エラー処理
 *
 * char* shader_source = NULL;
 * // シェーダーソース取得処理
 *
 * ret = gl33_shader_compile(SHADER_TYPE_VERTEX, shader_source, shader_handle);
 * // エラー処理
 * @endcode
 *
 * @param shader_type_ シェーダー種別 @ref shader_type_t
 * @param shader_source_ シェーダプログラムソースコード文字列
 * @param shader_handle_ シェーダーハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_source_ == NULL
 * - shader_handle_ == NULL
 * - shader_type_が規定値外
 * - メモリシステム未初期化(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - 指定したシェーダー種別のシェーダープログラムが既にコンパイル済み
 * - 指定したシェーダーハンドルが保持するシェーダープログラムが既にリンク済み
 * @retval RENDERER_SHADER_COMPILE_ERROR シェーダープログラムコンパイルエラー
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_UNDEFINED_ERROR 想定していない実行結果コードを処理過程で受け取った(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_SUCCESS シェーダープログラムのコンパイルに成功し、正常終了
 */
renderer_result_t gl33_shader_compile(shader_type_t shader_type_, const char* shader_source_, gl33_shader_t* shader_handle_);

/**
 * @brief コンパイル済みの各シェーダープログラムをリンクしOpenGLプログラムを生成する
 *
 * @note OpenGLシェーダープログラムのリンク後は、各シェーダープログラムへのハンドルを破棄してもOpenGLの仕様上は問題ないが、
 * 本モジュール仕様では、データ破損判定に用いるため、破棄を行わない。シェーダプログラムへのハンドルの破棄は @ref gl33_shader_destroy で行われる。
 *
 * @code{.c}
 * gl33_shader_t* shader_handle = NULL;
 * renderer_result_t ret = gl33_shader_create(&shader_handle);
 * // エラー処理
 *
 * char* vertex_shader_source = NULL;
 * // バーテックスシェーダーソース取得処理
 * ret = gl33_shader_compile(SHADER_TYPE_VERTEX, vertex_shader_source, shader_handle); // バーテックスシェーダーコンパイル
 * // エラー処理
 *
 * char* fragment_shader_source = NULL;
 * // フラグメントシェーダーソース取得処理
 * ret = gl33_shader_compile(SHADER_TYPE_FRAGMENT, fragment_shader_source, shader_handle); // フラグメントシェーダーコンパイル
 * // エラー処理
 *
 * ret = gl33_shader_link(shader_handle);
 * // エラー処理
 * @endcode
 *
 * @param shader_handle_ シェーダーハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - メモリシステム未初期化(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_BAD_OPERATION 以下のいずれか
 * - シェーダープログラムがリンク済み
 * - バーテックスシェーダーが未コンパイル
 * - フラグメントシェーダーが未コンパイル
 * @retval RENDERER_SHADER_LINK_ERROR シェーダープログラムのリンクに失敗
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_UNDEFINED_ERROR 想定していない実行結果コードを処理過程で受け取った(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_SUCCESS シェーダープログラムのリンクに成功し、正常終了
 */
renderer_result_t gl33_shader_link(gl33_shader_t* shader_handle_);

/**
 * @brief OpenGLコンテキストにshader_handle_が保持するOpenGLシェーダープログラムの使用開始を伝える
 *
 * @code{.c}
 * gl33_shader_t* shader_handle = NULL;
 * renderer_result_t ret = gl33_shader_create(&shader_handle);
 * // エラー処理
 *
 * // OpenGLシェーダーのコンパイル、リンク処理
 *
 * ret = gl33_shader_use(shader_handle);
 * // エラー処理
 * @endcode
 *
 * @param shader_handle_ リンク済みOpenGLシェーダープログラムを保持するシェーダーハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT shader_handle_ == NULL
 * @retval RENDERER_BAD_OPERATION OpenGLシェーダープログラムが未リンク
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - シェーダープログラムがリンク済みであるにも関わらず、バーテックスシェーダーが未コンパイル
 * - シェーダープログラムがリンク済みであるにも関わらず、フラグメントシェーダーが未コンパイル
 * @retval RENDERER_SUCCESS OpenGLシェーダプログラムの使用開始に成功し、正常終了
 */
renderer_result_t gl33_shader_use(gl33_shader_t* shader_handle_);

#ifdef TEST_BUILD
// 引数で与えた実行結果コードを強制的に出力させる
void gl33_shader_fail_enable(renderer_result_t result_code_);
void gl33_shader_fail_disable(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
