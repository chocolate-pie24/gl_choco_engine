// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_CORE_TEXTURE_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_CORE_TEXTURE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TEXTURE_SUCCESS = 0,
    TEXTURE_INVALID_ARGUMENT,
    TEXTURE_RUNTIME_ERROR,
    TEXTURE_NO_MEMORY,
    TEXTURE_LIMIT_EXCEEDED,
    TEXTURE_BAD_OPERATION,
    TEXTURE_DATA_CORRUPTED,
    TEXTURE_OVERFLOW,
    TEXTURE_UNDEFINED_ERROR,
} texture_result_t;

typedef struct texture_resource_info {
    size_t pixel_data_size;
    int32_t width;
    int32_t height;
    uint8_t channel_count;
} texture_resource_info_t;

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

#ifdef __cplusplus
}
#endif
#endif
