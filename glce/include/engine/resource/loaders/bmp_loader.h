// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file bmp_loader.h
 * @author chocolate-pie24
 * @brief BMPファイルのロード処理を行うAPIの定義
 *
 * @details 以下のファイルをサポートする
 * - 非圧縮BMPファイル
 * - ピクセルのチャンネルカウントはRGB or RGBAのみ
 * - 画像の高さがint16_tに収まること
 * - 画像の幅がが0より大きく、かつint16_tに収まること
 *
 * @date 2026-05-14
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_LOADERS_BMP_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_BMP_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/resource/core/resource_types.h"

resource_result_t bmp_loader_load(const char* fullpath_, texture_resource_info_t* out_resource_info_, uint8_t** out_pixels_);

#ifdef __cplusplus
}
#endif
#endif
