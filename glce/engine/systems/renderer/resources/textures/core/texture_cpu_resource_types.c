// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/textures/core/texture_cpu_resource_types.h"

#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_macros.h"

#include "engine/systems/renderer/resources/textures/core/texture_types.h"

bool texture_cpu_pixel_layout_is_valid(texture_cpu_pixel_layout_t layout_) {
    size_t tmp_size = 0;

    return TEXTURE_SUCCESS == texture_cpu_pixel_layout_data_size_calculate(layout_, &tmp_size);
}

texture_result_t texture_cpu_pixel_layout_data_size_calculate(texture_cpu_pixel_layout_t layout_, size_t* out_pixel_data_size_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    size_t tmp_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_pixel_data_size_, ret, TEXTURE_INVALID_ARGUMENT);
    if(1 != layout_.channel_count && 3 != layout_.channel_count && 4 != layout_.channel_count) {
        ret = TEXTURE_INVALID_ARGUMENT;
        goto cleanup;
    }
    if(0 >= layout_.width || 0 >= layout_.height) {
        ret = TEXTURE_INVALID_ARGUMENT;
        goto cleanup;
    }
    if(!size_mul_checked((size_t)layout_.width, (size_t)layout_.height, &tmp_size)) {
        ret = TEXTURE_OVERFLOW;
        goto cleanup;
    }
    if(!size_mul_checked((size_t)layout_.channel_count, tmp_size, &tmp_size)) {
        ret = TEXTURE_OVERFLOW;
        goto cleanup;
    }

    *out_pixel_data_size_ = tmp_size;

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}
