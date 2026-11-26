// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/fonts/font_atlas_asset.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/renderer/resources/fonts/core/font_types.h"
#include "engine/systems/renderer/resources/fonts/core/font_err_utils.h"

font_result_t font_atlas_asset_packed_dimensions_get(const font_atlas_asset_t* asset_, int32_t* out_width_, int32_t* out_height_) {
    font_result_t ret = FONT_INVALID_ARGUMENT;

    int32_t tmp_width = 0;

    IF_ARG_NULL_GOTO_CLEANUP(asset_, ret, FONT_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_width_, ret, FONT_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_height_, ret, FONT_INVALID_ARGUMENT);
    if(0 >= asset_->atlas_width || 0 >= asset_->atlas_height) {
        ret = FONT_INVALID_ARGUMENT;
        ERROR_MESSAGE("font_atlas_asset_packed_dimensions_get(%s) - Provided atlas width or atlas height is not valid.", font_result_to_str(ret));
        goto cleanup;
    }

    tmp_width = asset_->atlas_width / 8;
    if(0 != (asset_->atlas_width % 8)) {
        tmp_width++;
    }

    *out_width_ = tmp_width;
    *out_height_ = asset_->atlas_height;

    ret = FONT_SUCCESS;

cleanup:
    return ret;
}
