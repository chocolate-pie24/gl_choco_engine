#include "engine/systems/renderer/core/renderer_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool target_graphics_api_is_valid(target_graphics_api_t graphics_api_) {
    switch(graphics_api_) {
    case GRAPHICS_API_GL33:
        return true;
    default:
        return false;
    }
}

bool buffer_usage_is_valid(buffer_usage_t usage_) {
    switch(usage_) {
    case BUFFER_USAGE_DYNAMIC:
        return true;
    case BUFFER_USAGE_STATIC:
        return true;
    default:
        return false;
    }
}

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

bool draw_range_is_valid(const draw_range_t* draw_range_) {
    if(NULL == draw_range_) {
        return false;
    }
    if(0 == draw_range_->vertex_count) {
        return false;
    }
    if((SIZE_MAX - draw_range_->vertex_count) < draw_range_->first_vertex_count) {
        return false;
    }
    return true;
}
