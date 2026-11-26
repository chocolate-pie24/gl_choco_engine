// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_texture_types.h"

#include <stdbool.h>
#include <stddef.h>

bool texture_mag_filter_config_is_valid(texture_mag_filter_config_t config_) {
    switch(config_) {
    case TEXTURE_MAG_FILTER_CONFIG_NEAREST:
        return true;
    case TEXTURE_MAG_FILTER_CONFIG_LINEAR:
        return true;
    default:
        return false;
    }
}

bool texture_min_filter_config_is_valid(texture_min_filter_config_t config_) {
    switch(config_) {
    case TEXTURE_MIN_FILTER_CONFIG_NEAREST:
        return true;
    case TEXTURE_MIN_FILTER_CONFIG_LINEAR:
        return true;
    default:
        return false;
    }
}

bool texture_wrap_config_is_valid(texture_wrap_config_t config_) {
    switch(config_) {
    case TEXTURE_WRAP_CONFIG_REPEAT:
        return true;
    case TEXTURE_WRAP_CONFIG_MIRRORED_REPEAT:
        return true;
    case TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE:
        return true;
    case TEXTURE_WRAP_CONFIG_CLAMP_TO_BORDER:
        return true;
    default:
        return false;
    }
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

bool texture_gpu_config_is_valid(const texture_gpu_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(!texture_format_is_valid(config_->format)) {
        return false;
    }
    if(!texture_wrap_config_is_valid(config_->wrap_config_s_axis)) {
        return false;
    }
    if(!texture_wrap_config_is_valid(config_->wrap_config_t_axis)) {
        return false;
    }
    if(!texture_mag_filter_config_is_valid(config_->mag_filter_config)) {
        return false;
    }
    if(!texture_min_filter_config_is_valid(config_->min_filter_config)) {
        return false;
    }
    if(TEXTURE_FORMAT_R8UI == config_->format) {
        if(TEXTURE_MAG_FILTER_CONFIG_LINEAR == config_->mag_filter_config) {
            return false;
        }
        if(TEXTURE_MIN_FILTER_CONFIG_LINEAR == config_->min_filter_config) {
            return false;
        }
    }
    return true;
}
