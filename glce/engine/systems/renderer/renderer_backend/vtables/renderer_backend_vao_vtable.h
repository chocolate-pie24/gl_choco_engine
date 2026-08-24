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

typedef renderer_backend_result_t (*pfn_vao_create)(renderer_backend_vao_t** vao_);   /**< renderer_vao_vtableが保持するvao_createの前方宣言 */
typedef void (*pfn_vao_destroy)(renderer_backend_vao_t** vao_);   /**< renderer_vao_vtableが保持するvao_destroyの前方宣言 */
typedef renderer_backend_result_t (*pfn_vao_bind)(const renderer_backend_vao_t* vao_); /**< renderer_vao_vtableが保持するvao_bindの前方宣言 */
typedef renderer_backend_result_t (*pfn_vao_unbind)(void);  /**< renderer_vao_vtableが保持するvao_unbindの前方宣言 */
typedef renderer_backend_result_t (*pfn_vao_attribute_set)(uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_); /**< renderer_vao_vtableが保持するvao_attribute_setの前方宣言 */

/**
 * @brief VAO機能仮想関数テーブル
 *
 */
typedef struct renderer_vao_vtable {
    /**
     * @brief VAO構造体インスタンスのメモリを確保し、VAOハンドルを生成する
     *
     * @param[out] vao_ renderer_backend_vao_t構造体インスタンスへのダブルポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
     * - vao_がNULL
     * - *vao_が非NULL
     * @retval RENDERER_BACKEND_NO_MEMORY メモリ確保失敗
     * @retval RENDERER_BACKEND_UNDEFINED_ERROR メモリ確保時に不明なエラーが発生
     * @retval RENDERER_BACKEND_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
     * @retval RENDERER_BACKEND_BAD_OPERATION メモリシステム未初期化
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
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
} renderer_vao_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
