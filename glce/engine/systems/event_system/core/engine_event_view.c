// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/event_system/core/engine_event_view.h"

#include <stdbool.h>

bool engine_event_view_is_valid(const engine_event_view_t* event_view_) {
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
