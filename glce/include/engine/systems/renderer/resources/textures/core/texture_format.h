// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_CORE_TEXTURE_FORMAT_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_CORE_TEXTURE_FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/resources/textures/core/texture_types.h"

/**
 * @brief TextureのGPU上でのデータ表現を定義する
 */
typedef enum texture_format {
    TEXTURE_FORMAT_RGB8,  /**< 各Channelが8bitの正規化値であるRGB形式。Alphaを持たないカラー画像等に使用する。 */
    TEXTURE_FORMAT_RGBA8, /**< 各Channelが8bitの正規化値であるRGBA形式。Alphaを持つカラー画像等に使用する。 */
    TEXTURE_FORMAT_R8UI,  /**< 1 Texelを8bit符号なし整数として扱う形式。Bit Packed Font Atlas等に使用する。 */
} texture_format_t;

typedef struct texture_resource_info {
    int32_t width;
    int32_t height;
    texture_format_t format;
} texture_resource_info_t;

size_t texture_format_bytes_per_texel_get(texture_format_t format_);

texture_result_t texture_pixel_data_size_calculate(int32_t width_, int32_t height_, texture_format_t format_, size_t* out_size_);

/**
 * @brief texture resource metadataがGLCE内部で使用可能なvalid stateであるか検証する
 *
 * @details
 * 外部resource境界からのload・変換等を終え、
 * GLCE内部representationとして構築されたtexture resource metadataについて、
 * GLCE内で使用可能なsemantic stateが成立していることを検証する。
 *
 * 本validatorはtexture_resource_info_tがsemantic ownerとして保持する
 * 各fieldおよびfield間relationを検証する。
 *
 * 以下の条件を検証する。
 * - resource_info_がNULLではない。
 * - widthが0より大きい。
 * - heightが0より大きい。
 * - channel_countがRGBを表す3、またはRGBAを表す4である。
 * - width * heightがsize_tで表現可能である。
 * - width * height * channel_countがsize_tで表現可能である。
 * - pixel_data_sizeがwidth * height * channel_countと一致する。
 *
 * pixel storageの存在、allocation validity、ownership relation、
 * pixel elementの内容そのものは本validatorの検証対象としない。
 *
 * @param[in] resource_info_ 検証対象のtexture resource metadata
 *
 * @retval true texture_resource_info_tとしてvalid
 * @retval false texture_resource_info_tとしてinvalid
 */
bool texture_resource_info_is_valid(const texture_resource_info_t* resource_info_);

bool texture_format_is_valid(texture_format_t format_);

#ifdef __cplusplus
}
#endif
#endif
