// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_texture.h
 * @author chocolate-pie24
 * @brief renderer_backendが保有するテクスチャ操作機能の窓口を上位層に提供する
 *
 * @date 2026-05-18
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_TEXTURE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

renderer_backend_result_t renderer_backend_texture_create(const renderer_backend_context_t* backend_context_, int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_);

void renderer_backend_texture_destroy(const renderer_backend_context_t* backend_context_, renderer_backend_texture_t** texture_handle_);

/**
 * @brief テクスチャをactiveにし、bindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] texture_handle_ bind対象テクスチャハンドル保有構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - texture_handle_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_->texture_vtableがNULLで未初期化
 * @retval RENDERER_BACKEND_DATA_CORRUPTED texture_handle_内部データ破損
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_texture_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_);

/**
 * @brief テクスチャをunbindする
 *
 * @param[in] backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param[in] texture_handle_ unbind対象テクスチャGPUリソース構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - texture_handle_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_->texture_vtableがNULLで未初期化
 * @retval RENDERER_BACKEND_DATA_CORRUPTED texture_handle_内部データ破損
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_texture_unbind(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_);

/**
 * @brief 現在active / bindされている2Dテクスチャ対象に対してピクセルデータをGPUへ転送する
 *
 * @warning 本APIを呼び出す前にrenderer_backend_texture_bindによって対象テクスチャユニットのactive化とbindを行っておくこと
 *
 * @param backend_context_ Renderer Backendコンテキスト構造体インスタンスへのポインタ
 * @param texture_handle_ テクスチャハンドル保有構造体インスタンスへのポインタ
 * @param width_ 転送ピクセルデータの幅
 * @param height_ 転送ピクセルデータの高さ
 * @param channel_count_ 転送ピクセルデータのチャンネルカウント(RGB or RGBAのみ許可)
 * @param pixels_ 転送ピクセルデータ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - pixels_ == NULL
 * - width_ == 0
 * - height_ == 0
 * - channel_count_が3or4以外
 * @retval RENDERER_BACKEND_BAD_OPERATION backend_context_->texture_vtableがNULLで未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
renderer_backend_result_t renderer_backend_texture_pixel_upload(const renderer_backend_context_t* backend_context_, uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_);

bool renderer_backend_texture_is_valid(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_);

#ifdef __cplusplus
}
#endif
#endif
