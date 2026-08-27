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

#include "engine/core/memory/choco_memory.h"

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

resource_result_t texture_cpu_resource_create(uint16_t width_, uint16_t height_, uint8_t channel_count_, size_t pixel_data_size_, uint8_t** pixels_, texture_cpu_resource_t** texture_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    texture_cpu_resource_t* tmp_cpu_resource = NULL;

    size_t expected_pixel_data_size = 0;
    const size_t width_size_t = (size_t)width_;
    const size_t height_size_t = (size_t)height_;
    const size_t channel_count_size_t = (size_t)channel_count_;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "texture_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "*texture_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(*pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "*pixels_")
    if(0 == width_ || 0 == height_ || (3 != channel_count_ && 4 != channel_count_)) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Provided width_, height_ or channel_count_ is not valid.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX / width_size_t) < height_size_t) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Pixel data size overflow.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX / channel_count_size_t) < (width_size_t * height_size_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Pixel data size overflow.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    expected_pixel_data_size = width_size_t * height_size_t * channel_count_size_t;
    if(expected_pixel_data_size != pixel_data_size_) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Provided pixel_data_size_ is not valid.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_mem = memory_system_allocate(sizeof(texture_cpu_resource_t), MEMORY_TAG_TEXTURE, (void**)&tmp_cpu_resource);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Failed to allocate memory for texture_cpu_resource_t.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_cpu_resource->channel_count = channel_count_;
    tmp_cpu_resource->height = height_;
    tmp_cpu_resource->width = width_;
    tmp_cpu_resource->pixels = *pixels_;
    tmp_cpu_resource->pixel_data_size = pixel_data_size_;

    if(!texture_cpu_resource_is_valid(tmp_cpu_resource)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Postcondition validation failed for 'tmp_cpu_resource'.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *texture_ = tmp_cpu_resource;

    tmp_cpu_resource = NULL;
    *pixels_ = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != tmp_cpu_resource) {
        memory_system_free(tmp_cpu_resource, sizeof(texture_cpu_resource_t), MEMORY_TAG_TEXTURE);
        tmp_cpu_resource = NULL;
    }
    return ret;
}

void texture_cpu_resource_destroy(texture_cpu_resource_t** texture_) {
    if(NULL == texture_) {
        return;
    }
    if(NULL == *texture_) {
        return;
    }
    if(!texture_cpu_resource_is_valid(*texture_)) {
        ERROR_MESSAGE("texture_cpu_resource_destroy(%s) - Provided texture_cpu_resource is corrupted.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else {
        memory_system_free((*texture_)->pixels, (*texture_)->pixel_data_size, MEMORY_TAG_TEXTURE);
        (*texture_)->pixels = NULL;

        memory_system_free(*texture_, sizeof(texture_cpu_resource_t), MEMORY_TAG_TEXTURE);
        *texture_ = NULL;
    }
}

resource_result_t texture_cpu_resource_pixel_get(const texture_cpu_resource_t* texture_, const uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_get", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_get", "out_pixels_")

    if(!texture_cpu_resource_is_valid(texture_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixel_get(%s) - provided texture_ is corrupted.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    *out_pixels_ = texture_->pixels;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t texture_cpu_resource_pixel_size_get(const texture_cpu_resource_t* texture_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(width_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "width_")
    IF_ARG_NULL_GOTO_CLEANUP(height_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "height_")
    IF_ARG_NULL_GOTO_CLEANUP(channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "channel_count_")

    if(!texture_cpu_resource_is_valid(texture_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixel_size_get(%s) - provided texture_ is corrupted.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *width_ = texture_->width;
    *height_ = texture_->height;
    *channel_count_ = texture_->channel_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

bool texture_cpu_resource_is_valid(const texture_cpu_resource_t* texture_) {
    size_t expected_pixel_data_size = 0;
    size_t tmp_height = 0;
    size_t tmp_width = 0;
    size_t tmp_channel_count = 0;

    if(NULL == texture_) {
        return false;
    }
    if(0 == texture_->height || 0 == texture_->width || (3 != texture_->channel_count && 4 != texture_->channel_count)) {
        return false;
    }
    if(NULL == texture_->pixels) {
        return false;
    }

    tmp_height = texture_->height;
    tmp_width = texture_->width;
    tmp_channel_count = texture_->channel_count;
    if((SIZE_MAX / tmp_height) < tmp_width) {
        return false;
    }
    if((SIZE_MAX / tmp_channel_count) < (tmp_width * tmp_height)) {
        return false;
    }
    expected_pixel_data_size = tmp_width * tmp_height * tmp_channel_count;
    if(expected_pixel_data_size != texture_->pixel_data_size) {
        return false;
    }
    return true;
}
