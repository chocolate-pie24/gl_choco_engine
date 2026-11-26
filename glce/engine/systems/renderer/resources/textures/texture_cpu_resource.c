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
#include "engine/systems/renderer/resources/textures/texture_cpu_resource.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/textures/core/texture_types.h"
#include "engine/systems/renderer/resources/textures/core/texture_cpu_resource_types.h"
#include "engine/systems/renderer/resources/textures/core/texture_err_utils.h"

// ============================================================
// Private Type Definitions
// ============================================================
/**
 * @brief テクスチャCPU側リソース構造体
 *
 */
struct texture_cpu_resource {
    texture_cpu_pixel_layout_t pixel_layout;
    uint8_t* pixels;        /**< テクスチャピクセルデータ */
};

// ============================================================
// Public API
// ============================================================
texture_result_t texture_cpu_resource_create(const texture_cpu_pixel_layout_t* layout_, uint8_t** pixels_, texture_cpu_resource_t** out_texture_resource_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    texture_cpu_resource_t* tmp_cpu_resource = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(layout_, ret, TEXTURE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_resource_, ret, TEXTURE_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_resource_, ret, TEXTURE_BAD_OPERATION);
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, TEXTURE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(*pixels_, ret, TEXTURE_INVALID_ARGUMENT);
    if(!texture_cpu_pixel_layout_is_valid(*layout_)) {
        ret = TEXTURE_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Provided layout_ is not valid.", texture_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(texture_cpu_resource_t), GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&tmp_cpu_resource);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = texture_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - general_allocator_allocate failed.", texture_result_to_str(ret));
        goto cleanup;
    }

    tmp_cpu_resource->pixel_layout = *layout_;
    tmp_cpu_resource->pixels = *pixels_;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_cpu_resource_is_valid(tmp_cpu_resource)) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Result validation failed for 'tmp_cpu_resource'.", texture_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    *out_texture_resource_ = tmp_cpu_resource;
    tmp_cpu_resource = NULL;
    *pixels_ = NULL;

    ret = TEXTURE_SUCCESS;

cleanup:
    if(TEXTURE_DATA_CORRUPTED != ret) {
        if(NULL != tmp_cpu_resource) {
            general_allocator_free((void**)&tmp_cpu_resource, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        }
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
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_cpu_resource_is_valid(*texture_resource_)) {
        ERROR_MESSAGE("texture_cpu_resource_destroy(%s) - Precondition validation failed for '*texture_resource_'.", texture_result_to_str(TEXTURE_DATA_CORRUPTED));
        return;
    }
#endif

    general_allocator_free((void**)&(*texture_resource_)->pixels, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
    general_allocator_free((void**)texture_resource_, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
}

texture_result_t texture_cpu_resource_pixels_get(const texture_cpu_resource_t* texture_resource_, const uint8_t** out_pixels_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(texture_resource_, ret, TEXTURE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, TEXTURE_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, TEXTURE_BAD_OPERATION);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_cpu_resource_is_valid(texture_resource_)) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixels_get(%s) - Precondition validation failed for 'texture_resource_'.", texture_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_pixels_ = texture_resource_->pixels;

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

bool texture_cpu_resource_is_valid(const texture_cpu_resource_t* texture_resource_) {
    if(NULL == texture_resource_) {
        return false;
    }
    if(NULL == texture_resource_->pixels) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated((const void*) texture_resource_->pixels)) {
        return false;
    }
    if(!texture_cpu_pixel_layout_is_valid(texture_resource_->pixel_layout)) {
        return false;
    }
    return true;
}
