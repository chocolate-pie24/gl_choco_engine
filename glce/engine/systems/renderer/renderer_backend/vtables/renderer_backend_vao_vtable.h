// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_vao_vtable.h
 * @author chocolate-pie24
 * @brief VAOモジュールが提供する機能をグラフィックスAPIによって差し替え可能な仮想関数テーブルを提供する
 *
 * @note renderer_backendはStrategyパターンによるグラフィックスAPI抽象化を行っている。
 * renderer_backend_vao_vtableはVAO機能についてのStrategy Interfaceに相当する。
 *
 * @note 本モジュールのリソース管理責務はモジュールが負う。ユーザー側でのメモリ確保、解放は行わないこと。
 *
 * @date 2026-02-06
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_VAO_VTABLE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_VAO_VTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef renderer_backend_result_t (*pfn_vao_create)(renderer_backend_vao_t** out_vao_);   /**< renderer_vao_vtableが保持するvao_createの前方宣言 */
typedef void (*pfn_vao_destroy)(renderer_backend_vao_t** vao_);   /**< renderer_vao_vtableが保持するvao_destroyの前方宣言 */
typedef renderer_backend_result_t (*pfn_vao_bind)(const renderer_backend_vao_t* vao_); /**< renderer_vao_vtableが保持するvao_bindの前方宣言 */
typedef renderer_backend_result_t (*pfn_vao_unbind)(void);  /**< renderer_vao_vtableが保持するvao_unbindの前方宣言 */
typedef renderer_backend_result_t (*pfn_vao_attribute_set)(uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_); /**< renderer_vao_vtableが保持するvao_attribute_setの前方宣言 */
typedef bool (*pfn_vao_is_valid)(const renderer_backend_vao_t* vao_);

/**
 * @brief VAO機能仮想関数テーブル
 *
 */
typedef struct renderer_vao_vtable {
    /**
     * @brief Concrete側のVAO管理Object生成処理
     *
     * @details
     * 管理ObjectのStorageを確保し、
     * 対応するGraphics APIのVAO生成処理を実行する。
     *
     * @param[out] out_vao_
     * 生成されたVAO管理Objectの格納先。
     * 呼び出し前に*out_vao_はNULLでなければならない。
     *
     * @retval RENDERER_BACKEND_SUCCESS
     * Concrete側の生成処理が成功した
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT
     * out_vao_がNULL、または*out_vao_がNULLではない。
     * 下位Allocator由来の引数不正も含む。
     *
     * @retval RENDERER_BACKEND_NO_MEMORY
     * 管理ObjectのStorageを確保できなかった
     *
     * @retval RENDERER_BACKEND_BAD_OPERATION
     * 下位Allocatorの操作条件を満たしていない
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
    pfn_vao_create vao_create;

    /**
     * @brief renderer_backend_vao_t構造体インスタンスのメモリを解放し、VAOも削除する
     *
     * @param[in,out] vao_ renderer_backend_vao_t構造体インスタンスへのダブルポインタ
     */
    pfn_vao_destroy vao_destroy;

    /**
     * @brief VAOをbindする
     *
     * @param[in] vao_ bind対象vao
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
     * - vao_ == NULL
     * @retval RENDERER_BACKEND_BAD_OPERATION 未初期化のvao_が渡された
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_vao_bind vao_bind;

    /**
     * @brief VAOをunbindする
     *
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_vao_unbind vao_unbind;

    /**
     * @brief 現在bind中のVAOに対して、VAOアトリビュート設定を行う
     *
     * @warning 呼び出し側は、本APIを呼ぶ前に設定対象VAOと参照元VBOをbindしておく必要がある
     *
     * @param[in] layout_ 設定対象変数のlayoutロケーション番号
     * @param[in] size_ 頂点属性のコンポーネントの数
     * @param[in] type_ 頂点属性のデータ型
     * @param[in] normalized_ true: アクセス時に固定小数点データ値を正規化する / false: 正規化しない
     * @param[in] stride_ 連続する頂点属性間のバイトオフセット
     * @param[in] offset_ 設定対象頂点属性が格納されているバイトオフセット
     *
     * @retval RENDERER_BACKEND_RUNTIME_ERROR type_が規定値外
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_vao_attribute_set vao_attribute_set;

    /**
     * @brief Concrete側のVAO管理Objectを検証する
     *
     * @details
     * Concreteが所有するVAO内部表現の妥当性を検証する。
     *
     * 現在のGL33 Concreteでは、VAO管理ObjectがNULLではなく、
     * VAO Handleが0ではないことを検証する。
     *
     * Renderer Backend Contextの妥当性は本関数では検証しない。
     *
     * OpenGL側のVAO Resource存在確認も行わない。
     *
     * @param[in] vao_
     * 検証対象のVAO管理Object
     *
     * @retval true
     * Concrete側の構造的妥当性が成立している
     *
     * @retval false
     * Concrete側の構造的妥当性が成立していない
     */
    pfn_vao_is_valid vao_is_valid;
} renderer_vao_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
