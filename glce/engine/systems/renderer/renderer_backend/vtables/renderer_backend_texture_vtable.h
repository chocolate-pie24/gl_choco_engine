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

typedef renderer_backend_result_t (*pfn_renderer_texture_create)(int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_);    /**< renderer_texture_vtableが保持するrenderer_texture_createの前方宣言 */
typedef void (*pfn_renderer_texture_destroy)(renderer_backend_texture_t** texture_handle_); /**< renderer_texture_vtableが保持するrenderer_texture_destroyの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_texture_bind)(const renderer_backend_texture_t* texture_handle_);   /**< renderer_texture_vtableが保持するrenderer_texture_bindの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_texture_unbind)(const renderer_backend_texture_t* texture_handle_);    /**< renderer_texture_vtableが保持するrenderer_texture_unbindの前方宣言 */
typedef renderer_backend_result_t (*pfn_renderer_texture_pixel_upload)(uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_);  /**< renderer_texture_vtableが保持するrenderer_texture_pixel_uploadの前方宣言 */
typedef bool (*pfn_renderer_texture_is_valid)(const renderer_backend_texture_t* texture_handle_);

/**
 * @brief Renderer Backend GPU側テクスチャリソース操作用仮想関数テーブル
 *
 */
typedef struct renderer_texture_vtable {
    /**
     * @brief テクスチャGPU側リソース構造体インスタンスのメモリを確保し、テクスチャ設定を行い初期化する
     *
     * @param[in] unit_num_ シェーダーが参照するテクスチャ用スロット番号
     * @param[in] min_filter_config_ テクスチャ縮小表示の際の設定値
     * @param[in] mag_filter_config_ テクスチャ拡大表示の際の設定値
     * @param[in] wrap_config_s_axis_ テクスチャがラップする部分の表示設定値(s軸)
     * @param[in] wrap_config_t_axis_ テクスチャがラップする部分の表示設定値(t軸)
     * @param[out] texture_handle_ リソース確保、初期化対象テクスチャGPUリソース構造体インスタンスへのダブルポインタ
     *
     * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
     * - texture_handle_ == NULL
     * - *texture_handle_ != NULL
     * - min_filter_config_が規定値外
     * - mag_filter_config_が規定値外
     * - wrap_config_s_axis_が規定値外
     * - wrap_config_t_axis_が規定値外
     * - unit_num_ < 0
     * @retval RENDERER_BACKEND_BAD_OPERATION メモリシステム未初期化
     * @retval RENDERER_BACKEND_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
     * @retval RENDERER_BACKEND_NO_MEMORY メモリ確保失敗
     * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_texture_create renderer_texture_create;

    /**
     * @brief テクスチャGPUリソース構造体が保持するリソースを解放し、自身のメモリも解放する
     *
     * @note 本関数実行後、texture_handle_はNULLに初期化される
     * @note 2重destroy許可
     *
     * @param[in,out] texture_handle_ リソース解放対象構造体インスタンスへのダブルポインタ
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

    pfn_renderer_texture_is_valid renderer_texture_is_valid;
} renderer_texture_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
