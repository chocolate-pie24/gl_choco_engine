#include "engine/systems/platform/config/platform_config.h"

#include <stdbool.h>
#include <stddef.h>

static bool is_valid_shallow(const platform_config_t* config_);

bool platform_config_is_valid(const platform_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(!is_valid_shallow(config_)) {
        return false;
    }
    return true;
}

static bool is_valid_shallow(const platform_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->max_keyboard_event_count || 0 == config_->max_mouse_event_count || 0 == config_->max_window_event_count) {
        return false;
    }
    if(NULL == config_->window_label) {
        return false;
    }
    if('\0' == config_->window_label[0]) {
        return false;
    }
    if(0 >= config_->window_height || 0 >= config_->window_width) {
        return false;
    }
    return true;
}
