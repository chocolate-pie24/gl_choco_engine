// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file texture_cpu_resource.c
 * @author chocolate-pie24
 * @brief テクスチャCPU側リソースを操作するモジュールAPIの実装
 *
 * @date 2026-05-14
 *
 */
#include "engine/resource/texture/texture_cpu_resource.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

/**
 * @brief テクスチャCPU側リソース構造体
 *
 */
struct texture_cpu_resource {
    size_t pixel_data_size;
    uint16_t width;         /**< テクスチャ幅 */
    uint16_t height;        /**< テクスチャ高さ(左上原点の画像を基準にする) */
    uint8_t channel_count;  /**< チャンネルカウント(RGB or RGBAのみサポート) */
    uint8_t* pixels;        /**< テクスチャピクセルデータ */
};

resource_result_t texture_cpu_resource_create(uint16_t width_, uint16_t height_, uint8_t channel_count_, size_t pixel_data_size_, uint8_t** pixels_, texture_cpu_resource_t** out_texture_resource_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    texture_cpu_resource_t* tmp_cpu_resource = NULL;

    size_t expected_pixel_data_size = 0;
    const size_t width_size_t = (size_t)width_;
    const size_t height_size_t = (size_t)height_;
    const size_t channel_count_size_t = (size_t)channel_count_;

    IF_ARG_NULL_GOTO_CLEANUP(out_texture_resource_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "out_texture_resource_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_resource_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "*out_texture_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(*pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "*pixels_")
    if(0 == width_ || 0 == height_ || (3 != channel_count_ && 4 != channel_count_)) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Provided width_, height_ or channel_count_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX / width_size_t) < height_size_t) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Pixel data size overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX / channel_count_size_t) < (width_size_t * height_size_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Pixel data size overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    expected_pixel_data_size = width_size_t * height_size_t * channel_count_size_t;
    if(expected_pixel_data_size != pixel_data_size_) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Provided pixel_data_size_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(texture_cpu_resource_t), GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&tmp_cpu_resource);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    tmp_cpu_resource->channel_count = channel_count_;
    tmp_cpu_resource->height = height_;
    tmp_cpu_resource->width = width_;
    tmp_cpu_resource->pixels = *pixels_;
    tmp_cpu_resource->pixel_data_size = pixel_data_size_;

    if(!texture_cpu_resource_is_valid(tmp_cpu_resource)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Postcondition validation failed for 'tmp_cpu_resource'.", resource_result_to_str(ret));
        goto cleanup;
    }

    *out_texture_resource_ = tmp_cpu_resource;

    tmp_cpu_resource = NULL;
    *pixels_ = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != tmp_cpu_resource) {
        general_allocator_free((void**)&tmp_cpu_resource, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
    }
    return ret;
}

void texture_cpu_resource_destroy(texture_cpu_resource_t** texture_resource_) {
    if(NULL == texture_resource_) {
        return;
    }
    if(NULL == *texture_resource_) {
        return;
    }
    if(!texture_cpu_resource_is_valid(*texture_resource_)) {
        ERROR_MESSAGE("texture_cpu_resource_destroy(%s) - Provided texture_cpu_resource is corrupted.", resource_result_to_str(RESOURCE_DATA_CORRUPTED));
    } else {
        general_allocator_free((void**)&(*texture_resource_)->pixels, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        general_allocator_free((void**)texture_resource_, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
    }
}

resource_result_t texture_cpu_resource_pixels_get(const texture_cpu_resource_t* texture_resource_, const uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixels_get", "texture_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixels_get", "out_pixels_")

    if(!texture_cpu_resource_is_valid(texture_resource_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixels_get(%s) - provided texture_resource_ is corrupted.", resource_result_to_str(ret));
        goto cleanup;
    }
    *out_pixels_ = texture_resource_->pixels;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t texture_cpu_resource_pixel_size_get(const texture_cpu_resource_t* texture_resource_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "texture_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(out_width_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "out_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_height_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "out_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "out_channel_count_")

    if(!texture_cpu_resource_is_valid(texture_resource_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixel_size_get(%s) - provided texture_resource_ is corrupted.", resource_result_to_str(ret));
        goto cleanup;
    }

    *out_width_ = texture_resource_->width;
    *out_height_ = texture_resource_->height;
    *out_channel_count_ = texture_resource_->channel_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

bool texture_cpu_resource_is_valid(const texture_cpu_resource_t* texture_resource_) {
    size_t expected_pixel_data_size = 0;
    size_t tmp_height = 0;
    size_t tmp_width = 0;
    size_t tmp_channel_count = 0;

    if(NULL == texture_resource_) {
        return false;
    }
    if(0 == texture_resource_->height || 0 == texture_resource_->width || (3 != texture_resource_->channel_count && 4 != texture_resource_->channel_count)) {
        return false;
    }
    if(NULL == texture_resource_->pixels) {
        return false;
    }

    tmp_height = texture_resource_->height;
    tmp_width = texture_resource_->width;
    tmp_channel_count = texture_resource_->channel_count;
    if((SIZE_MAX / tmp_height) < tmp_width) {
        return false;
    }
    if((SIZE_MAX / tmp_channel_count) < (tmp_width * tmp_height)) {
        return false;
    }
    expected_pixel_data_size = tmp_width * tmp_height * tmp_channel_count;
    if(expected_pixel_data_size != texture_resource_->pixel_data_size) {
        return false;
    }
    return true;
}
