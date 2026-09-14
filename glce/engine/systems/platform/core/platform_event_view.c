// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/platform/core/platform_event_view.h"

#include <stdbool.h>
#include <stddef.h>

bool platform_event_view_is_valid(const platform_event_view_t* event_view_) {
    if(NULL == event_view_) {
        return false;
    }
    if(NULL == event_view_->keyboard_events) {
        return false;
    }
    if(NULL == event_view_->mouse_events) {
        return false;
    }
    if(NULL == event_view_->window_events) {
        return false;
    }
    return true;
}
