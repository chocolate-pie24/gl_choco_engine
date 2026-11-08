// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_shader_vtable.h
 * @author chocolate-pie24
 * @brief シェーダープログラム/シェーダーオブジェクトの操作関数をまとめたvtableを定義する
 *
 * @note
 * - シェーダープログラム: シェーダーオブジェクトをリンクしたプログラム
 * - シェーダーオブジェクト: コンパイルされた各シェーダーステージごとのオブジェクト
 *
 * @date 2026-02-23
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_SHADER_VTABLE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_SHADER_VTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef renderer_backend_result_t (*pfn_renderer_shader_create)(renderer_backend_shader_t** out_shader_handle_);    /**< renderer_shader_vtableが保持するrenderer_shader_createの前方宣言 */
typedef void (*pfn_renderer_shader_destroy)(renderer_backend_shader_t** shader_handle_);    /**< renderer_shader_vtableが保持するrenderer_shader_destroyの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_shader_compile)(shader_stage_t shader_stage_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);    /**< renderer_shader_vtableが保持するrenderer_shader_compileの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_shader_link)(renderer_backend_shader_t* shader_handle_);   /**< renderer_shader_vtableが保持するrenderer_shader_linkの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_shader_use)(const renderer_backend_shader_t* shader_handle_);   /**< renderer_shader_vtableが保持するrenderer_shader_useの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_shader_uniform_location_get)(const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_);  /**< renderer_shader_vtableが保持するrenderer_shader_uniform_location_getの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_shader_mat4f_uniform_set)(int32_t location_, bool should_transpose_, const float* data_);  /**< renderer_shader_vtableが保持するrenderer_shader_mat4f_uniform_setの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_shader_vec3f_uniform_set)(int32_t location_, const float* data_);
typedef renderer_backend_result_t (*pfn_renderer_shader_float_uniform_set)(int32_t location_, float data_);
typedef renderer_backend_result_t (*pfn_renderer_shader_vec4u8_uniform_set)(int32_t location_, const uint8_t* data_);   /**< renderer_shader_vtableが保持するrenderer_shader_vec4u8_uniform_setの前方宣言 */
typedef bool (*pfn_renderer_shader_is_valid)(const renderer_backend_shader_t* shader_);

/**
 * @brief シェーダー機能仮想関数テーブル
 *
 */
typedef struct renderer_shader_vtable {
    /**
     * @brief Shader管理Objectを確保し、Handleをゼロ初期化する
     *
     * @details
     * Concrete側でShader管理ObjectのStorageを確保し、
     * Shader Programおよび各StageのHandleをゼロ初期化する。
     *
     * 現在のGL33 Concreteでは、この段階でGPU Resourceを生成しない。
     *
     * @param[out] out_shader_handle_
     * 生成するShader管理Objectの格納先。
     * 呼び出し前に*out_shader_handle_はNULLであること。
     *
     * @retval RENDERER_BACKEND_SUCCESS
     * 管理Objectの生成に成功した
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT
     * Output Pointerの条件違反、または下位Allocatorの引数不正
     *
     * @retval RENDERER_BACKEND_BAD_OPERATION
     * 下位Allocatorの操作条件違反
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
     * @brief 指定されたシェーダーステージのソースコードをコンパイルする
     *
     * @details
     * Concrete側でシェーダーオブジェクトを生成し、
     * ソースコードのコンパイルを実行する。
     *
     * 現在のOpenGL 3.3実装ではVertex / Fragment Shaderに対応する。
     * コンパイル結果はGL_COMPILE_STATUSによって判定する。
     *
     * 成功時に生成したシェーダーハンドルをshader_handle_へ設定し、
     * 失敗時には生成途中のシェーダーオブジェクトの削除を要求する。
     *
     * @pre shader_source_はNULL終端された読み取り可能な文字列であること
     *
     * @param[in] shader_stage_ コンパイル対象のシェーダーステージ
     * @param[in] shader_source_ コンパイル対象のシェーダーソースコード
     * @param[in,out] shader_handle_ シェーダーGPUリソース内部状態管理構造体インスタンス
     *
     * @retval RENDERER_BACKEND_SUCCESS コンパイルに成功
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT 引数異常、未対応ステージ、または下位Allocatorの引数異常
     * @retval RENDERER_BACKEND_BAD_OPERATION 既にコンパイル済み、リンク済み、または下位Allocatorの操作条件違反
     * @retval RENDERER_BACKEND_SHADER_COMPILE_ERROR シェーダーオブジェクトの生成またはコンパイルに失敗
     * @retval RENDERER_BACKEND_NO_MEMORY 一時領域のメモリ確保に失敗
     * @retval RENDERER_BACKEND_LIMIT_EXCEEDED 下位Allocatorの管理上限を超過
     * @retval RENDERER_BACKEND_OVERFLOW 下位Allocatorでオーバーフローを検出
     * @retval RENDERER_BACKEND_DATA_CORRUPTED 下位Allocatorで内部状態の破損を検出
     * @retval RENDERER_BACKEND_UNDEFINED_ERROR 下位Allocatorから未定義のResultを受け取った
     */
    pfn_renderer_shader_compile renderer_shader_compile;

    /**
     * @brief コンパイル済みShader Objectをリンクする
     *
     * @details
     * 現在のGL33 Concreteでは、Vertex / Fragment Shaderの
     * 両方のHandleが設定されていることを要求する。
     *
     * Program Handleを生成し、両ShaderをAttachしてリンクする。
     * リンク結果を判定し、成功時にProgram Handleを保持する。
     *
     * Info Logの取得には必要に応じてGeneral Allocatorを使用する。
     *
     * @param[in,out] shader_handle_
     * リンク対象のShader管理Object
     *
     * @retval RENDERER_BACKEND_SUCCESS
     * リンクに成功した
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT
     * shader_handle_がNULL、または下位Allocatorの引数不正
     *
     * @retval RENDERER_BACKEND_BAD_OPERATION
     * Programがリンク済み、必要なShaderが未コンパイル、
     * または下位Allocatorの操作条件違反
     *
     * @retval RENDERER_BACKEND_SHADER_LINK_ERROR
     * Program Handle生成またはリンクに失敗した
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
    pfn_renderer_shader_link renderer_shader_link;

    /**
     * @brief シェーダープログラムを切り替える
     *
     * @param[in] shader_handle_ 切り替え先シェーダープログラムを管理する内部状態管理構造体インスタンスへのポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT shader_handle_がNULL
     * @retval RENDERER_BACKEND_BAD_OPERATION シェーダープログラムのハンドルが0
     * @retval RENDERER_BACKEND_DATA_CORRUPTED 以下のいずれか
     * - program_idが非ゼロであるにもかかわらずvertex_shader_handleが0
     * - program_idが非ゼロであるにもかかわらずfragment_shader_handleが0
     * @retval RENDERER_BACKEND_SUCCESS OpenGLへのプログラム使用要求が完了
     */
    pfn_renderer_shader_use renderer_shader_use;

    /**
     * @brief シェーダープログラムのユニフォーム変数のLocationを取得する
     *
     * @details
     * 現在のGL33 ConcreteではglGetUniformLocation()を使用し、
     * 戻り値が-1の場合はRUNTIME_ERRORを返す。
     *
     * Shader Programがリンク済みかどうかの事前検証は行わない。
     *
     * @param[in] shader_handle_ シェーダープログラムハンドルインスタンスへのポインタ
     * @param[in] name_ ユニフォーム変数名称
     * @param[out] out_location_ Location格納先
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
     * - shader_handle_ == NULL
     * - name_ == NULL
     * - out_location_ == NULL
     * @retval RENDERER_BACKEND_RUNTIME_ERROR ユニフォーム変数が存在しない、未使用として最適化された、またはLocation取得に失敗
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_uniform_location_get renderer_shader_uniform_location_get;

    /**
     * @brief シェーダープログラムにmat4f型のユニフォーム変数を送信する
     *
     * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
     *
     * @param[in] location_ ユニフォーム変数のLocation
     * @param[in] should_transpose_ true: 送信時に行列を転置する / false: 送信時に行列を転置しない
     * @param[in] data_ 送信データへのポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT data_ == NULL
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_mat4f_uniform_set renderer_shader_mat4f_uniform_set;

    /**
     * @brief シェーダープログラムにvec3f型のユニフォーム変数を送信する
     *
     * @details
     * 現在のOpenGL 3.3実装では、data_が指す3個のfloat値を
     * glUniform3f()へ渡す。
     *
     * @note OpenGL APIの実行結果は検証しない。
     * @warning 本APIを呼ぶ前に対象のシェーダープログラムをuse状態にすること
     * @pre data_は3個のfloat値を読み取り可能なメモリ領域を指していること
     *
     * @param[in] location_ ユニフォーム変数のLocation
     * @param[in] data_ 送信する3個のfloat値へのポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT data_がNULL
     * @retval RENDERER_BACKEND_SUCCESS OpenGLへのユニフォーム設定要求が完了
     */
    pfn_renderer_shader_vec3f_uniform_set renderer_shader_vec3f_uniform_set;

    /**
     * @brief シェーダープログラムにfloat型のユニフォーム変数を送信する
     *
     * @details
     * 現在のOpenGL 3.3実装では、指定されたfloat値を
     * glUniform1f()へ渡す。
     *
     * @note OpenGL側の実行結果は検証しない。
     * @warning 本APIを呼ぶ前に対象のシェーダープログラムをuse状態にすること
     *
     * @param[in] location_ ユニフォーム変数のLocation
     * @param[in] data_ 送信するfloat値
     *
     * @retval RENDERER_BACKEND_SUCCESS OpenGLへのユニフォーム設定要求が完了
     */
    pfn_renderer_shader_float_uniform_set renderer_shader_float_uniform_set;

    /**
     * @brief シェーダープログラムにvec4u8型のユニフォーム変数を送信する
     *
     * @details
     * 現在のGL33 Concreteでは、uint8_tの4成分を
     * それぞれ255.0fで除算し、0.0から1.0のfloat値へ変換する。
     *
     * 変換した4成分をglUniform4fv()で送信する。
     * OpenGL側の実行結果は検証しない。
     *
     * @warning 本APIを呼ぶ前に必ず対象のシェーダープログラムをuseしておくこと
     *
     * @param[in] location_ ユニフォーム変数のLocation
     * @param[in] data_ 送信データへのポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT data_ == NULL
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_shader_vec4u8_uniform_set renderer_shader_vec4u8_uniform_set;

    /**
     * @brief Concrete側のShader管理Objectを検証する
     *
     * @details
     * Concreteが所有するShader内部表現の妥当性を検証する。
     *
     * 現在のGL33 Concreteでは、Program Handleが設定されている場合、
     * Vertex / Fragment ShaderのHandleも設定されていることを確認する。
     *
     * 未リンク状態および部分コンパイル状態は許容する。
     *
     * Renderer Backend Contextの検証、
     * OpenGL側のResource存在確認および
     * 実際のCompile / Link Statusの問い合わせは行わない。
     *
     * @param[in] shader_
     * 検証対象のShader管理Object
     *
     * @retval true 検査対象の構造的妥当性が成立している
     * @retval false 検査対象の構造的妥当性が成立していない
     */
    pfn_renderer_shader_is_valid renderer_shader_is_valid;
} renderer_shader_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
