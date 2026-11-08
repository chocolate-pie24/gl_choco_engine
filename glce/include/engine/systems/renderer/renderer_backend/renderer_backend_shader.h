// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_shader.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するシェーダーオブジェクト/シェーダープログラム操作機能の窓口を上位層に提供する
 *
 * @date 2026-02-23
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

/**
 * @brief Shader管理Objectを生成する
 *
 * @details
 * Backend Contextが保持するShader vtableを介して、
 * Concrete側のShader管理Object生成処理を呼び出す。
 *
 * 現在のGL33 ConcreteではGeneral AllocatorでStorageを確保し、
 * Program / Vertex Shader / Fragment ShaderのHandleを0に初期化する。
 *
 * 本APIの成功時点ではShaderのコンパイルおよびリンクは行われない。
 *
 * 生成した管理ObjectはCallerが所有し、
 * renderer_backend_shader_destroy()で破棄する。
 *
 * @param[in] backend_context_
 * Shader vtableを保持するRenderer Backend Context
 *
 * @param[out] out_shader_handle_
 * 生成した管理Objectの格納先。
 * 呼び出し前に*out_shader_handle_はNULLでなければならない。
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * 管理Objectの生成に成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * backend_context_またはout_shader_handle_がNULL、
 * *out_shader_handle_がNULLではない、
 * または下位Allocatorが引数不正を返した
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * Shader vtableがNULL、または下位Allocatorの操作条件違反
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * Storageの確保に失敗した
 *
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
 * 下位Allocatorの管理上限を超えた
 *
 * @retval RENDERER_BACKEND_OVERFLOW
 * 下位Allocatorで算術Overflowを検出した
 *
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * 下位Allocatorで内部状態の破損を検出した
 *
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR
 * 下位Allocatorから未定義のResultを受け取った
 */
renderer_backend_result_t renderer_backend_shader_create(renderer_backend_context_t* backend_context_, renderer_backend_shader_t** out_shader_handle_);

/**
 * @brief Shader管理Objectと所有するGPU Resourceを破棄する
 *
 * @details
 * Backend ContextのShader vtableを介して、
 * Concrete側のShader破棄処理を実行する。
 *
 * 現在のGL33 Concreteでは、Handleが0ではない
 * Vertex Shader、Fragment ShaderおよびShader Programについて、
 * OpenGLへ削除を要求する。
 *
 * その後、General Allocatorで管理Objectの解放を要求する。
 *
 * backend_context_またはShader vtableがNULLの場合は、
 * 破棄処理を実行せず終了する。
 *
 * shader_handle_または*shader_handle_がNULLの場合も、
 * GL33 Concreteでは何も行わない。
 *
 * General Allocatorのfreeが成功した場合、
 * *shader_handle_はNULLになる。
 *
 * 現在の実装ではOpenGL側の削除結果と
 * General Allocatorのfree結果をCallerへ通知しない。
 *
 * @param[in] backend_context_
 * Shader破棄用vtableを保持するRenderer Backend Context
 *
 * @param[in,out] shader_handle_
 * 破棄対象のShader管理Objectを保持するPointerの格納先
 */
void renderer_backend_shader_destroy(renderer_backend_context_t* backend_context_, renderer_backend_shader_t** shader_handle_);

/**
 * @brief 指定されたShader StageのSourceをコンパイルする
 *
 * @details
 * Shader vtableを介してConcrete側のコンパイル処理を実行する。
 *
 * 現在のGL33 ConcreteではVertex ShaderとFragment Shaderを
 * サポートする。
 *
 * 対象StageのHandleが既に設定されている場合、
 * またはShader Programがリンク済みの場合はコンパイルしない。
 *
 * glCreateShader()でHandleを生成し、
 * glShaderSource()、glCompileShader()を呼び出す。
 * GL_COMPILE_STATUSを取得してコンパイル結果を判定する。
 *
 * Info Logが存在する場合は内容を取得し、
 * エラーまたは警告として出力する。
 *
 * コンパイル成功時だけ、対応するStageのHandleを
 * Shader管理Objectへ格納する。
 *
 * 失敗時に生成途中のShader Handleが存在する場合は、
 * glDeleteShader()による削除を要求する。
 *
 * @pre
 * shader_source_はNUL終端された読み取り可能な文字列であること。
 *
 * @param[in] shader_stage_
 * コンパイルするShader Stage。
 * SHADER_STAGE_VERTEXまたはSHADER_STAGE_FRAGMENT。
 *
 * @param[in] shader_source_
 * コンパイル対象のShader Source文字列
 *
 * @param[in] backend_context_
 * Shader vtableを保持するRenderer Backend Context
 *
 * @param[in,out] shader_handle_
 * コンパイル結果のShader Handleを保持する管理Object
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * コンパイルに成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * 必須PointerがNULL、Shader Stageが未対応、
 * または下位Allocatorが引数不正を返した
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * Shader Programがリンク済み、
 * 対象Stageが既にコンパイル済み、
 * Shader vtableがNULL、
 * または下位Allocatorの操作条件違反
 *
 * @retval RENDERER_BACKEND_SHADER_COMPILE_ERROR
 * Shader Handleを生成できない、
 * またはShader Sourceのコンパイルに失敗した
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * Info Log用Storageを確保できなかった
 *
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
 * 下位Allocatorの管理上限を超えた
 *
 * @retval RENDERER_BACKEND_OVERFLOW
 * 下位Allocatorで算術Overflowを検出した
 *
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * 下位Allocatorで内部状態の破損を検出した
 *
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR
 * 下位Allocatorから未定義のResultを受け取った
 */
renderer_backend_result_t renderer_backend_shader_compile(shader_stage_t shader_stage_, const char* shader_source_, renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_);

/**
 * @brief コンパイル済みのVertex / Fragment Shaderをリンクする
 *
 * @details
 * Shader vtableを介してConcrete側のリンク処理を実行する。
 *
 * 現在のGL33 Concreteでは、Vertex ShaderとFragment Shaderの
 * 両方のHandleが設定され、Programが未リンクであることを要求する。
 *
 * glCreateProgram()で生成したProgramに両ShaderをAttachし、
 * glLinkProgram()を実行する。
 *
 * GL_LINK_STATUSを取得して結果を判定する。
 * リンク成功時だけProgram Handleを管理Objectへ保持する。
 *
 * リンク後も両Shader ObjectのHandleは保持され、
 * Shader管理Objectのdestroy時に破棄される。
 *
 * @param[in] backend_context_
 * Shader vtableを保持するRenderer Backend Context
 *
 * @param[in,out] shader_handle_
 * リンク対象のShader管理Object
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * リンクに成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * 必須PointerがNULL、または下位Allocatorが引数不正を返した
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * Shader vtableがNULL、Programがリンク済み、
 * Vertex / Fragment Shaderが未コンパイル、
 * または下位Allocatorの操作条件違反
 *
 * @retval RENDERER_BACKEND_SHADER_LINK_ERROR
 * Program Handleの生成、またはリンクに失敗した
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * Info Log用Storageの確保に失敗した
 *
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
 * 下位Allocatorの管理上限を超えた
 *
 * @retval RENDERER_BACKEND_OVERFLOW
 * 下位Allocatorで算術Overflowを検出した
 *
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * 下位Allocatorで内部状態の破損を検出した
 *
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR
 * 下位Allocatorから未定義のResultを受け取った
 */
renderer_backend_result_t renderer_backend_shader_link(renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_);

/**
 * @brief シェーダープログラムの使用開始をグラフィックスAPIに伝える
 *
 * @param[in,out] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] shader_handle_ シェーダープログラムハンドル格納構造体インスタンス
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - backend_context_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION 以下のいずれか
 * - Shader vtableがNULL
 * - シェーダープログラムのハンドルが0
 * @retval RENDERER_BACKEND_DATA_CORRUPTED 以下のいずれか
 * - program_idが非ゼロであるにもかかわらずvertex_shader_handleが0
 * - program_idが非ゼロであるにもかかわらずfragment_shader_handleが0
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_shader_use(const renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_);

/**
 * @brief シェーダープログラムのユニフォーム変数のLocationを取得する
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] shader_handle_ シェーダープログラムハンドル構造体インスタンスへのポインタ
 * @param[in] name_ ユニフォーム変数名称
 * @param[out] out_location_ Location格納先
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - shader_handle_ == NULL
 * - name_ == NULL
 * - out_location_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_が未初期化でbackend_context_->shader_vtableがNULL
 * @retval RENDERER_BACKEND_RUNTIME_ERROR ユニフォーム変数のLocation取得に失敗
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_shader_uniform_location_get(const renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);

/**
 * @brief シェーダープログラムにmat4f型のユニフォーム変数を送信する
 *
 * @note OpenGL 3.3実装
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] should_transpose_ true: 送信時に行列を転置する / false: 送信時に行列を転置しない
 * @param[in] data_ 送信データへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - data_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_shader_mat4f_uniform_set(const renderer_backend_context_t* backend_context_, int32_t location_, bool should_transpose_, const float* data_);

/**
 * @brief シェーダープログラムにvec3f型のユニフォーム変数を送信する
 *
 * @details
 * Renderer Backend Contextが保持するShader vtableを介して、
 * Concrete側のvec3f Uniform設定処理を実行する。
 *
 * 現在のOpenGL 3.3実装ではdata_が指す3個のfloat値を
 * glUniform3f()へ渡す。
 *
 * @note 現在のOpenGL 3.3実装ではOpenGL APIの実行結果を検証しない。
 * @warning 本APIを呼ぶ前に対象のシェーダープログラムをuse状態にすること
 * @pre data_は3個のfloat値を読み取り可能なメモリ領域を指していること
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] data_ 送信する3個のfloat値へのポインタ
 *
 * @retval RENDERER_BACKEND_SUCCESS Concrete側のユニフォーム設定処理に成功
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_がNULL
 * - data_がNULL
 * @retval RENDERER_BACKEND_BAD_OPERATION Shader vtableがNULL
 */
renderer_backend_result_t renderer_backend_shader_vec3f_uniform_set(const renderer_backend_context_t* backend_context_, int32_t location_, const float* data_);

/**
 * @brief シェーダープログラムにfloat型のユニフォーム変数を送信する
 *
 * @details
 * Renderer Backend Contextが保持するShader vtableを介して、
 * Concrete側のfloat Uniform設定処理を実行する。
 *
 * 現在のOpenGL 3.3実装ではdata_をglUniform1f()へ渡す。
 *
 * @note 現在のOpenGL 3.3実装では引数の値域および
 * OpenGL APIの実行結果を検証しない。
 * @warning 本APIを呼ぶ前に対象のシェーダープログラムをuse状態にすること
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] data_ 送信するfloat値
 *
 * @retval RENDERER_BACKEND_SUCCESS Concrete側のユニフォーム設定処理に成功
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT backend_context_がNULL
 * @retval RENDERER_BACKEND_BAD_OPERATION Shader vtableがNULL
 */
renderer_backend_result_t renderer_backend_shader_float_uniform_set(const renderer_backend_context_t* backend_context_, int32_t location_, float data_);

/**
 * @brief シェーダープログラムにvec4u8型のユニフォーム変数を送信する
 *
 * @note OpenGL 3.3実装
 * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
 *
 * @param[in] backend_context_ レンダラーバックエンドコンテキストへのポインタ
 * @param[in] location_ ユニフォーム変数のLocation
 * @param[in] data_ 送信データへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - data_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_が未初期化でshader_vtableがNULL
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_shader_vec4u8_uniform_set(const renderer_backend_context_t* backend_context_, int32_t location_, const uint8_t* data_);

/**
 * @brief Shader管理Objectの構造的妥当性を検証する
 *
 * @details
 * 次の条件を検証する。
 *
 * - shader_がNULLではない
 * - Renderer Backend Contextがvalidである
 * - Concrete側のShader Validatorがtrueを返す
 *
 * 現在のGL33 Concreteでは、program_idが0ではない場合に、
 * Vertex ShaderとFragment ShaderのHandleも
 * 0ではないことを要求する。
 *
 * program_idが0の場合は、コンパイル前の状態や
 * 片方のShaderだけがコンパイルされた状態もvalidと判定する。
 *
 * 本関数はOpenGL側のResource存在確認、
 * 実際のCompile / Link Status、
 * および管理Object自身のAllocation Livenessを検証しない。
 *
 * @pre
 * NULLではないshader_は、読み取り可能な有効期間中の
 * Shader管理Objectを指していること。
 *
 * @param[in] backend_context_
 * 検証対象のRenderer Backend Context
 *
 * @param[in] shader_
 * 検証対象のShader管理Object
 *
 * @retval true
 * 検査対象の構造的妥当性が成立している
 *
 * @retval false
 * 検査対象の構造的妥当性が成立していない
 *
 * @note
 * 本関数は対象Objectを変更しない。
 */
bool renderer_backend_shader_is_valid(const renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_);

#ifdef __cplusplus
}
#endif
#endif
