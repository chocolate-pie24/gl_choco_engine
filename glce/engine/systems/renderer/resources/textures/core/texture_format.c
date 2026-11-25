// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/textures/core/texture_format.h"

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/systems/renderer/resources/textures/core/texture_types.h"
#include "engine/systems/renderer/resources/textures/core/texture_err_utils.h"

size_t texture_format_bytes_per_texel_get(texture_format_t format_) {
    switch(format_) {
    case TEXTURE_FORMAT_RGB8:
        return 3;
    case TEXTURE_FORMAT_RGBA8:
        return 4;
    case TEXTURE_FORMAT_R8UI:
        return 1;
    default:
        return 0;
    }
}

texture_result_t texture_pixel_data_size_calculate(int32_t width_, int32_t height_, texture_format_t format_, size_t* out_size_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    const size_t bytes_per_texel = texture_format_bytes_per_texel_get(format_);
    size_t pixel_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_size_, ret, TEXTURE_INVALID_ARGUMENT);
    if(0 >= width_ || 0 >= height_ || 0 == bytes_per_texel) {
        ret = TEXTURE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_pixel_data_size_calculate(%s) - Provided width_, height_ or format_ is not valid.", texture_result_to_str(ret));
        goto cleanup;
    }
    if(!size_mul_checked((size_t)width_, (size_t)height_, &pixel_size)) {
        ret = TEXTURE_OVERFLOW;
        ERROR_MESSAGE("texture_pixel_data_size_calculate(%s) - Texture pixel size overflow.", texture_result_to_str(ret));
        goto cleanup;
    }
    if(!size_mul_checked(pixel_size, bytes_per_texel, &pixel_size)) {
        ret = TEXTURE_OVERFLOW;
        ERROR_MESSAGE("texture_pixel_data_size_calculate(%s) - Texture pixel size overflow.", texture_result_to_str(ret));
        goto cleanup;
    }

    *out_size_ = pixel_size;

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

bool texture_resource_info_is_valid(const texture_resource_info_t* resource_info_) {
    size_t pixel_data_size = 0;

    if(NULL == resource_info_) {
        return false;
    }
    if(0 >= resource_info_->width || 0 >= resource_info_->height) {
        return false;
    }
    if(!texture_format_is_valid(resource_info_->format)) {
        return false;
    }
    if(TEXTURE_SUCCESS != texture_pixel_data_size_calculate(resource_info_->width, resource_info_->height, resource_info_->format, &pixel_data_size)) {
        return false;
    }

    return true;
}

bool texture_format_is_valid(texture_format_t format_) {
    switch(format_) {
    case TEXTURE_FORMAT_RGB8:
        return true;
    case TEXTURE_FORMAT_RGBA8:
        return true;
    case TEXTURE_FORMAT_R8UI:
        return true;
    default:
        return false;
    }
}
