// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/fonts/font_gpu_resource.h"

#include <stdint.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_texture_types.h"

#include "engine/systems/renderer/resources/textures/texture_gpu_resource.h"

#include "engine/systems/renderer/resources/fonts/core/font_types.h"
#include "engine/systems/renderer/resources/fonts/core/font_err_utils.h"
#include "engine/systems/renderer/resources/fonts/font_atlas_asset.h"

font_result_t font_gpu_resource_create(const renderer_backend_context_t* backend_context_, int32_t texture_unit_index_, const font_atlas_asset_t* asset_, texture_gpu_resource_t** out_gpu_resource_) {
    font_result_t ret = FONT_INVALID_ARGUMENT;

    texture_result_t ret_texture = TEXTURE_INVALID_ARGUMENT;

    texture_gpu_resource_t* tmp_gpu_resource = NULL;

    texture_gpu_config_t resource_config;
    int32_t tmp_width = 0;
    int32_t tmp_height = 0;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, FONT_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(asset_, ret, FONT_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_gpu_resource_, ret, FONT_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_gpu_resource_, ret, FONT_BAD_OPERATION);

    ret = font_atlas_asset_packed_dimensions_get(asset_, &tmp_width, &tmp_height);
    if(FONT_SUCCESS != ret) {
        ERROR_MESSAGE("font_gpu_resource_create(%s) - font_atlas_asset_packed_dimensions_get failed.", font_result_to_str(ret));
        goto cleanup;
    }

    resource_config.format = TEXTURE_FORMAT_R8UI;
    resource_config.mag_filter_config = TEXTURE_MAG_FILTER_CONFIG_NEAREST;
    resource_config.min_filter_config = TEXTURE_MIN_FILTER_CONFIG_NEAREST;
    resource_config.wrap_config_s_axis = TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE;
    resource_config.wrap_config_t_axis = TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE;

    ret_texture = texture_gpu_resource_create(backend_context_, texture_unit_index_, &resource_config, tmp_width, tmp_height, asset_->pixels, &tmp_gpu_resource);
    if(TEXTURE_SUCCESS != ret_texture) {
        ret = font_result_convert_texture(ret_texture);
        ERROR_MESSAGE("font_gpu_resource_create(%s) - texture_gpu_resource_create failed.", font_result_to_str(ret));
        goto cleanup;
    }

    *out_gpu_resource_ = tmp_gpu_resource;
    tmp_gpu_resource = NULL;

    ret = FONT_SUCCESS;

cleanup:
    return ret;
}

