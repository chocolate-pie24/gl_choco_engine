// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/event_system/config/event_system_config.h"

#include <stdbool.h>
#include <stddef.h>

bool event_system_config_is_valid(const event_system_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->max_keyboard_event_count) {
        return false;
    }
    if(0 == config_->max_mouse_event_count) {
        return false;
    }
    if(0 == config_->max_window_event_count) {
        return false;
    }
    return true;
}
