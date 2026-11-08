// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_vao.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するVAO機能の窓口を上位層に提供する
 *
 * @date 2026-02-23
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_VAO_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_VAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

/**
 * @brief Renderer Backend VAOを生成する
 *
 * @details
 * backend_context_が保持するVAO vtableを使用して、
 * Concrete側のVAO生成処理を呼び出す。
 *
 * 現在のGL33 Concreteでは、General Allocatorから
 * VAO管理Objectを確保した後、glGenVertexArrays()を呼び出す。
 *
 * GL33 ConcreteはOpenGL APIの実行結果や
 * 生成されたHandleの妥当性を検証しない。
 *
 * したがって、本APIのSUCCESSは、
 * OpenGL側のResource Stateの正常性まで保証するものではない。
 *
 * 生成したVAO管理Objectは、
 * renderer_backend_vao_destroy()で破棄する。
 *
 * @param[in] backend_context_
 * VAO vtableを保持するRenderer Backend Context
 *
 * @param[out] out_vao_
 * 生成されたVAO管理Objectの格納先。
 * 呼び出し前に*out_vao_はNULLでなければならない。
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * VAO管理Objectの生成処理が成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * backend_context_またはout_vao_がNULL、
 * または*out_vao_がNULLではない。
 * 下位Allocator由来の引数不正も含む。
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * VAO vtableが設定されていない、
 * または下位Allocatorの操作条件を満たしていない
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * VAO管理Objectのメモリを確保できなかった
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
renderer_backend_result_t renderer_backend_vao_create(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** out_vao_);

/**
 * @brief Renderer Backend VAOを破棄する
 *
 * @details
 * backend_context_が保持するVAO vtableを使用し、
 * Concrete側の破棄処理を呼び出す。
 *
 * 現在のGL33 Concreteでは、OpenGL VAO Handleの削除を要求し、
 * VAO管理Objectのメモリ解放を行う。
 *
 * backend_context_がNULL、またはVAO vtableがNULLの場合は、
 * 破棄処理を行わずに終了する。
 *
 * GL33 Concreteでは、vao_または*vao_がNULLの場合も
 * 何も行わない。
 *
 * 有効なBackend ContextとVAO管理Objectを指定した場合、
 * Concrete側の破棄処理を実行し、*vao_をNULLへ設定する。
 *
 * 現在の実装では、OpenGL側の削除結果および
 * General Allocatorによる解放結果を
 * Callerへ通知しない。
 *
 * @param[in] backend_context_
 * VAO破棄用vtableを保持するRenderer Backend Context
 *
 * @param[in,out] vao_
 * 破棄するVAO管理Objectを保持するPointerの格納先
 */
void renderer_backend_vao_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vao_);

/**
 * @brief VAOをbindする
 *
 * @param[in] backend_context_ bind用vtable保有構造体インスタンスへのポインタ
 * @param[in] vao_ bind対象VAOハンドル構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - vao_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION 以下のいずれか
 * - backend_context_->vao_vtable == NULL
 * - VAOが未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_vao_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_vao_t* vao_);

/**
 * @brief VAOをunbindする
 *
 * @param[in] backend_context_ unbind用vtable保有構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT backend_context_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION VAO vtableが設定されていない
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_vao_unbind(const renderer_backend_context_t* backend_context_);

/**
 * @brief 現在Bind中のVAOへVertex Attributeを設定する
 *
 * @details
 * VAO vtableを介してConcrete側のAttribute設定処理を呼び出す。
 *
 * 現在のGL33 Concreteでは、type_に対応するOpenGL型を選択し、
 * glVertexAttribPointer()とglEnableVertexAttribArray()を呼び出す。
 *
 * 対応するtype_は次のとおり。
 * - RENDERER_TYPE_FLOAT
 * - RENDERER_TYPE_UNSIGNED_BYTE
 * - RENDERER_TYPE_BYTE
 *
 * layout_、size_、stride_、offset_については、
 * GL33 Concrete側で個別の値域検証を行わない。
 *
 * また、OpenGL側の実行結果は検証しない。
 *
 * @warning
 * 呼び出し前に設定対象VAOと参照元VBOをBindしておくこと。
 * 本API自身はBind状態を検証しない。
 *
 * @param[in] backend_context_
 * Attribute設定用vtableを保持するRenderer Backend Context
 *
 * @param[in] layout_
 * 設定対象となるVertex AttributeのLocation
 *
 * @param[in] size_
 * Vertex AttributeのComponent数
 *
 * @param[in] type_
 * Vertex AttributeのData Type
 *
 * @param[in] normalized_
 * 整数型Attributeを浮動小数点値として取得する際の正規化指定
 *
 * @param[in] stride_
 * 連続するVertex Attribute間のバイト単位のStride
 *
 * @param[in] offset_
 * 現在Bind中のVBO先頭から対象AttributeまでのバイトOffset
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * Concrete側のAttribute設定処理がSUCCESSを返した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * backend_context_がNULL
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * VAO vtableが設定されていない
 *
 * @retval RENDERER_BACKEND_RUNTIME_ERROR
 * GL33 Concreteでtype_がサポート対象外の値である
 */
renderer_backend_result_t renderer_backend_vao_attribute_set(const renderer_backend_context_t* backend_context_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

/**
 * @brief Renderer Backend VAOの構造的妥当性を検証する
 *
 * @details
 * 次の条件を検証する。
 *
 * - vao_がNULLではない
 * - Renderer Backend Contextがvalidである
 * - Concrete側のVAO Validatorがtrueを返す
 *
 * Renderer Backend ContextのValidationでは、
 * Shader、VAO、VBO、Textureの各vtableが、
 * 現在選択されているConcreteのvtableと一致することを確認する。
 *
 * 現在のGL33 Concreteでは、VAO Handleが0ではないことを
 * 構造的妥当性の条件とする。
 *
 * 本関数はglIsVertexArray()等による
 * OpenGL側のResource存在確認を行わない。
 *
 * @pre
 * NULLではないvao_は、読み取り可能な有効期間中の
 * VAO管理Objectを指していること。
 *
 * @param[in] backend_context_
 * 検証対象のRenderer Backend Context
 *
 * @param[in] vao_
 * 検証対象のVAO管理Object
 *
 * @retval true
 * 検査対象の構造的妥当性が成立している
 *
 * @retval false
 * 検査対象の構造的妥当性が成立していない
 *
 * @note
 * 本関数はRenderer Backend Contextおよび
 * VAO管理Objectを変更しない。
 */
bool renderer_backend_vao_is_valid(const renderer_backend_context_t* backend_context_, const renderer_backend_vao_t* vao_);

#ifdef __cplusplus
}
#endif
#endif
