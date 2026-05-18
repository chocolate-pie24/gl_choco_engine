/**
 * @ingroup renderer_backend_interface
 *
 * @file interface_texture.h
 * @author chocolate-pie24
 * @brief テクスチャ操作関数をまとめたvtableを定義する
 *
 * @version 0.1
 * @date 2026-05-18
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_TEXTURE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_core/renderer_types.h"

typedef renderer_result_t (*pfn_renderer_texture_create)(int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_);
typedef void (*pfn_renderer_texture_destroy)(renderer_backend_texture_t** texture_handle_);
typedef renderer_result_t (*pfn_renderer_texture_bind)(const renderer_backend_texture_t* texture_handle_, int32_t* out_texture_unit_, int32_t* out_texture_internal_handle_);
typedef renderer_result_t (*pfn_renderer_texture_unbind)(const renderer_backend_texture_t* texture_handle_);
typedef renderer_result_t (*pfn_renderer_texture_pixel_upload)(uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_);

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
     * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
     * - texture_handle_ == NULL
     * - *texture_handle_ != NULL
     * - min_filter_config_が規定値外
     * - mag_filter_config_が規定値外
     * - wrap_config_s_axis_が規定値外
     * - wrap_config_t_axis_が規定値外
     * - unit_num_ < 0
     * @retval RENDERER_BAD_OPERATION メモリシステム未初期化
     * @retval RENDERER_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
     * @retval RENDERER_NO_MEMORY メモリ確保失敗
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
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
     * @note 呼び出し側が保持している現在のtexture unit / texture handleキャッシュとtexture_handle_が保持するunit / handleと等しい場合はactive化 / bindを行わない
     *
     * @param[in] texture_handle_ bind対象テクスチャハンドル保有構造体インスタンスへのポインタ
     * @param[in,out] out_texture_unit_ 現在activeになっているテクスチャユニット番号
     * @param[in,out] out_texture_internal_handle_ 現在Bindされているテクスチャハンドル
     *
     * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
     * - texture_handle_ == NULL
     * - out_texture_unit_ == NULL
     * - out_texture_internal_handle_ == NULL
     * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
     * - texture_handle_->handle == 0
     * - texture_handle_->unit_number < 0
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_texture_bind renderer_texture_bind;

    /**
     * @brief テクスチャをactiveにし、unbindする
     *
     * @param[in] texture_handle_ unbind対象テクスチャGPUリソース構造体インスタンスへのポインタ
     *
     * @retval RENDERER_INVALID_ARGUMENT texture_handle_ == NULL
     * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
     * - texture_handle_->handle == 0
     * - texture_handle_->unit_number < 0
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_texture_unbind renderer_texture_unbind;

    /**
     * @brief 現在active / bindされているGL_TEXTURE_2Dに対してピクセルデータをGPUへ転送する
     *
     * @param width_ 転送ピクセルデータの幅
     * @param height_ 転送ピクセルデータの高さ
     * @param channel_count_ 転送ピクセルデータのチャンネルカウント(RGB or RGBAのみ許可)
     * @param pixels_ 転送ピクセルデータ
     *
     * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
     * - pixels_ == NULL
     * - width_ == 0
     * - height_ == 0
     * - channel_count_が3, 4以外
     * @retval RENDERER_SUCCESS 処理に成功し、正常終了
     */
    pfn_renderer_texture_pixel_upload renderer_texture_pixel_upload;
} renderer_texture_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
