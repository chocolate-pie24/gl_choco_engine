// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_vbo.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するVBO機能の窓口を上位層に提供する
 *
 * @date 2026-02-23
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_VBO_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_VBO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

/**
 * @brief Renderer Backend VBOを生成する
 *
 * @details
 * backend_context_が保持するVBO vtableを使用し、
 * VBO管理Objectのメモリを確保する。
 *
 * GL33 Concreteでは、管理Objectの確保後に
 * glGenBuffers()を呼び出してVBO Handleの生成を要求する。
 *
 * 現在の実装では、glGenBuffers()の結果や
 * OpenGL側のError Stateを検証しない。
 *
 * したがって、本APIのSUCCESSは、
 * OpenGL側のResource Stateが正常であることまで
 * 保証するものではない。
 *
 * 生成したVBO管理Objectは、
 * renderer_backend_vbo_destroy()で破棄する。
 *
 * @param[in] backend_context_
 * VBO vtableを保持するRenderer Backend Context
 *
 * @param[out] out_vbo_
 * 生成されたVBO管理Objectの格納先。
 * 呼び出し前に*out_vbo_はNULLでなければならない。
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * VBO管理Objectの生成処理が成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * backend_context_またはout_vbo_がNULL、
 * または*out_vbo_がNULLではない。
 * 下位Allocator由来の引数不正も含む。
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * VBO vtableが設定されていない、
 * または下位Allocatorの操作条件を満たしていない
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * VBO管理Objectのメモリを確保できなかった
 *
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
 * 下位Allocatorの管理上限を超えた
 *
 * @retval RENDERER_BACKEND_OVERFLOW
 * 下位Allocatorで算術overflowが検出された
 *
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * 下位Allocatorで内部データ破損が検出された
 *
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR
 * 下位Allocatorから未定義または変換対象外の結果を受け取った
 */
renderer_backend_result_t renderer_backend_vbo_create(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** out_vbo_);

/**
 * @brief Renderer Backend VBOを破棄する
 *
 * @details
 * backend_context_が保持するVBO vtableを使用し、
 * Concrete側のVBO破棄処理を呼び出す。
 *
 * GL33 Concreteでは、OpenGLのVBO Handleの削除を要求し、
 * VBO管理Objectのメモリ解放を行う。
 *
 * backend_context_がNULL、またはVBO vtableがNULLの場合は、
 * 破棄処理を行わずに終了する。
 *
 * vbo_または*vbo_がNULLの場合も、何も行わない。
 *
 * 有効なBackend ContextとVBO管理Objectを指定した場合、
 * Concrete側の破棄処理を実行し、*vbo_をNULLへ設定する。
 *
 * 現在の実装では、OpenGL側の削除結果および
 * General Allocatorによる解放結果を
 * 呼び出し側へ通知しない。
 *
 * @param[in] backend_context_
 * VBO破棄用vtableを保持するRenderer Backend Context
 *
 * @param[in,out] vbo_
 * 破棄するVBO管理Objectを保持するpointerの格納先
 */
void renderer_backend_vbo_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vbo_);

/**
 * @brief VBOをbindする
 *
 * @param[in] backend_context_ bind用vtable保有構造体インスタンスへのポインタ
 * @param[in] vbo_ bind対象VBOハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vbo_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_->vbo_vtable == NULL
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_vbo_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_vbo_t* vbo_);

/**
 * @brief VBOをunbindする
 *
 * @note 特定のVBOを指定してunbindするAPIではなく、現在のGL_ARRAY_BUFFER bindingを解除するAPIである
 *
 * @param[in] backend_context_ unbind用vtable保有構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT backend_context_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_が未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_vbo_unbind(const renderer_backend_context_t* backend_context_);

/**
 * @brief Bind中のVBOにBuffer領域を確保し、必要に応じてデータを転送する
 *
 * @details
 * 現在bindされているGL_ARRAY_BUFFERに対して、
 * glBufferData()によるBuffer Storageの確保を要求する。
 *
 * load_data_がNULLでない場合は、
 * 指定されたデータを初期内容として渡す。
 *
 * load_data_がNULLの場合は、初期データを指定せずに
 * Buffer Storageの確保を要求する。
 *
 * 現在の実装では、OpenGL側の実行結果を検証しない。
 *
 * @warning
 * 本APIを呼び出す前に対象のVBOをbindしておくこと。
 * 本API自身はbind状態を検証しない。
 *
 * @param[in] backend_context_
 * VBO操作用vtableを保持するRenderer Backend Context
 *
 * @param[in] load_size_
 * 確保するBuffer Storageのサイズ(byte)。0は指定できない。
 *
 * @param[in] load_data_
 * 初期データへのpointer。NULLを許可する。
 *
 * @param[in] usage_
 * Buffer用途を表すbuffer_usage_t
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * OpenGLへのBuffer確保／転送要求を実行した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * backend_context_がNULL、またはload_size_が0
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * VBO vtableが設定されていない
 *
 * @retval RENDERER_BACKEND_RUNTIME_ERROR
 * usage_がサポート対象外の値である
 */
renderer_backend_result_t renderer_backend_vbo_vertex_load(const renderer_backend_context_t* backend_context_, size_t load_size_, const void* load_data_, buffer_usage_t usage_);

/**
 * @brief 生成済みのGPU側頂点情報格納領域に対し、転送位置を指定して頂点情報を転送する
 *
 * @warning 本APIを呼び出す前に対象のVBOをbindしておくこと
 *
 * @param[in] backend_context_ 転送用vtable保有構造体インスタンスへのポインタ
 * @param[in] offset_ 頂点情報格納領域の先頭から転送開始位置までのオフセット(byte)
 * @param[in] size_ 頂点情報転送サイズ(byte)
 * @param[in] load_data_ 転送する頂点情報配列へのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - load_data_ == NULL
 * - size_ == 0
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_->vbo_vtableがNULLで未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_vbo_vertex_subload(const renderer_backend_context_t* backend_context_, size_t offset_, size_t size_, const void* load_data_);

/**
 * @brief Renderer Backend VBOの構造的妥当性を検証する
 *
 * @details
 * 次の条件を検証する。
 *
 * - vbo_がNULLではない
 * - Renderer Backend Contextがvalidである
 * - Concrete側のVBO Validatorがtrueを返す
 *
 * 現在のGL33 Concreteでは、VBO Handleが0ではないことを
 * 構造的妥当性の条件とする。
 *
 * Renderer Backend ContextのValidationでは、
 * Shader、VAO、VBO、Textureの各vtableの
 * 整合性も検証する。
 *
 * 本関数はglIsBuffer()等による
 * OpenGL側のResource存在確認を行わない。
 *
 * @param[in] backend_context_
 * 検証対象のRenderer Backend Context
 *
 * @param[in] vbo_
 * 検証対象のVBO管理Object
 *
 * @retval true
 * 検査対象の構造的妥当性が成立している
 *
 * @retval false
 * 検査対象の構造的妥当性が成立していない
 *
 * @note
 * 本関数はRenderer Backend Contextおよび
 * VBO管理Objectを変更しない。
 */
bool renderer_backend_vbo_is_valid(const renderer_backend_context_t* backend_context_, const renderer_backend_vbo_t* vbo_);

#ifdef __cplusplus
}
#endif
#endif
