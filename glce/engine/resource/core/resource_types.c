#include "engine/resource/core/resource_types.h"

#include <stdbool.h>
#include <stddef.h>

bool texture_resource_info_is_valid(const texture_resource_info_t* resource_info_) {
    size_t pixel_count = 0;
    size_t expected_pixel_data_size = 0;

    size_t width = 0;
    size_t height = 0;
    size_t channel_count = 0;

    if(NULL == resource_info_) {
        return false;
    }

    width = (size_t)resource_info_->width;
    height = (size_t)resource_info_->height;
    channel_count = resource_info_->channel_count;
    if(0 >= width || 0 >= height) {
        return false;
    }
    if(3 != resource_info_->channel_count && 4 != resource_info_->channel_count) {
        return false;
    }

    // width, height, channel_countからpixel data sizeを再計算
    if((SIZE_MAX / height) < width) {
        return false;
    }
    pixel_count = width * height;
    if((SIZE_MAX / channel_count) < pixel_count) {
        return false;
    }
    expected_pixel_data_size = pixel_count * channel_count;
    if(expected_pixel_data_size != resource_info_->pixel_data_size) {
        return false;
    }

    return true;
}
