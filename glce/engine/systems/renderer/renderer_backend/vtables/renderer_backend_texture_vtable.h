// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_texture_vtable.h
 * @author chocolate-pie24
 * @brief テクスチャ操作関数をまとめたvtableを定義する
 *
 * @date 2026-05-18
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_TEXTURE_VTABLE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_TEXTURE_VTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef renderer_backend_result_t (*pfn_renderer_texture_create)(int32_t texture_unit_index_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** out_texture_handle_);    /**< renderer_texture_vtableが保持するrenderer_texture_createの前方宣言 */
typedef void (*pfn_renderer_texture_destroy)(renderer_backend_texture_t** texture_handle_); /**< renderer_texture_vtableが保持するrenderer_texture_destroyの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_texture_bind)(const renderer_backend_texture_t* texture_handle_);   /**< renderer_texture_vtableが保持するrenderer_texture_bindの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_texture_unbind)(const renderer_backend_texture_t* texture_handle_);    /**< renderer_texture_vtableが保持するrenderer_texture_unbindの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_texture_pixel_upload)(int32_t width_, int32_t height_, uint8_t channel_count_, const uint8_t* pixels_);  /**< renderer_texture_vtableが保持するrenderer_texture_pixel_uploadの前方宣言 */
typedef bool (*pfn_renderer_texture_is_valid)(const renderer_backend_texture_t* texture_handle_);

/**
 * @brief Renderer Backend GPU側テクスチャリソース操作用仮想関数テーブル
 *
 */
typedef struct renderer_texture_vtable {
    /**
     * @brief Concrete側のTexture管理Objectを生成する
     *
     * @details
     * Texture管理ObjectのStorageを確保し、
     * Texture Handleの生成とFilter / Wrap設定を行う。
     *
     * 現在のGL33 Concreteでは、
     * OpenGL APIの実行結果を検証しない。
     *
     * @param[in] texture_unit_index_
     * Texture Unitの0始まりのIndex
     *
     * @param[in] min_filter_config_ 縮小Filter設定
     * @param[in] mag_filter_config_ 拡大Filter設定
     * @param[in] wrap_config_s_axis_ S軸のWrap設定
     * @param[in] wrap_config_t_axis_ T軸のWrap設定
     *
     * @param[out] out_texture_handle_
     * 生成した管理Objectの格納先。
     * 呼び出し前に*out_texture_handle_はNULLであること。
     *
     * @retval RENDERER_BACKEND_SUCCESS
     * Concrete側の生成処理が成功した
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT
     * Output Pointer、Texture Unit Index、Filter / Wrap設定、
     * または下位Allocatorへの入力が不正
     *
     * @retval RENDERER_BACKEND_BAD_OPERATION
     * 下位Allocatorの操作条件違反
     *
     * @retval RENDERER_BACKEND_NO_MEMORY
     * 管理ObjectのStorage確保に失敗した
     *
     * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
     * 下位Allocatorの管理上限を超えた
     *
     * @retval RENDERER_BACKEND_OVERFLOW
     * 下位Allocatorで算術Overflowが検出された
     *
     * @retval RENDERER_BACKEND_DATA_CORRUPTED
     * 下位Allocatorの内部データ破損が検出された
     *
     * @retval RENDERER_BACKEND_UNDEFINED_ERROR
     * 下位Allocatorから未定義または変換対象外の結果を受け取った
     */
    pfn_renderer_texture_create renderer_texture_create;

    /**
     * @brief Texture管理Objectと対応するGPU Resourceを破棄する
     *
     * @details
     * Concrete側でTexture Handleの削除を要求し、
     * 管理ObjectのStorageを解放する。
     *
     * 現在のGL33 Concreteでは、
     * texture_handle_または*texture_handle_がNULLの場合は
     * 何も行わない。
     *
     * 解放処理を実行した場合、
     * *texture_handle_をNULLへ設定する。
     *
     * GPU Resourceの削除結果および
     * 管理Objectの解放結果は通知しない。
     *
     * @param[in,out] texture_handle_
     * 破棄対象のTexture管理Objectを保持するPointerの格納先
     */
    pfn_renderer_texture_destroy renderer_texture_destroy;

    /**
     * @brief テクスチャをactiveにし、bindする
     *
     * @param[in] texture_handle_ bind対象テクスチャハンドル保有構造体インスタンスへのポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT texture_handle_ == NULL
     * @retval RENDERER_BACKEND_DATA_CORRUPTED 以下のいずれか
     * - texture_handle_->handle == 0
     * - texture_handle_->unit_number < 0
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_texture_bind renderer_texture_bind;

    /**
     * @brief テクスチャをactiveにし、unbindする
     *
     * @param[in] texture_handle_ unbind対象テクスチャGPUリソース構造体インスタンスへのポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT texture_handle_ == NULL
     * @retval RENDERER_BACKEND_DATA_CORRUPTED 以下のいずれか
     * - texture_handle_->handle == 0
     * - texture_handle_->unit_number < 0
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_texture_unbind renderer_texture_unbind;

    /**
     * @brief 現在active / bindされている2Dテクスチャ対象に対してピクセルデータをGPUへ転送する
     *
     * @warning 本APIを呼び出す前にrenderer_texture_bindによって対象テクスチャユニットのactive化とbindを行っておくこと
     *
     * @param width_ 転送ピクセルデータの幅
     * @param height_ 転送ピクセルデータの高さ
     * @param channel_count_ 転送ピクセルデータのチャンネルカウント(RGB or RGBAのみ許可)
     * @param pixels_ 転送ピクセルデータ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
     * - pixels_ == NULL
     * - width_ == 0
     * - height_ == 0
     * - channel_count_が3, 4以外
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_texture_pixel_upload renderer_texture_pixel_upload;

    /**
     * @brief Concrete側Texture管理Objectの妥当性を検証する
     *
     * @details
     * Concreteが所有するTexture内部表現を検証する。
     *
     * 現在のGL33 Concreteでは、Texture Handle、
     * Texture Unit Index、Min / Mag Filter設定、
     * およびS / T軸のWrap設定を確認する。
     *
     * Renderer Backend Contextの妥当性や、
     * OpenGL側のTexture Resource存在確認は行わない。
     *
     * @param[in] texture_handle_
     * 検証対象のTexture管理Object
     *
     * @retval true
     * Concrete側の構造的妥当性が成立している
     *
     * @retval false
     * Concrete側の構造的妥当性が成立していない
     */
    pfn_renderer_texture_is_valid renderer_texture_is_valid;
} renderer_texture_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
