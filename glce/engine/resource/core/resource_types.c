#include "engine/resource/core/resource_types.h"

#include <stdbool.h>
#include <stddef.h>

bool texture_resource_info_is_valid(const texture_resource_info_t* resource_info_) {
    if(NULL == resource_info_) {
        return false;
    }
    if(0 == resource_info_->pixel_data_size) {
        return false;
    }
    if(0 == resource_info_->height || 0 == resource_info_->width) {
        return false;
    }
    if(3 != resource_info_->channel_count && 4 != resource_info_->channel_count) {
        return false;
    }
    return true;
}
