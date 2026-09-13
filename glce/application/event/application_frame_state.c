// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/event/application_frame_state.h"

#include <stdbool.h>
#include <stddef.h>

static bool is_valid_shallow(const application_frame_state_t* frame_state_);

void application_frame_state_begin_frame(application_frame_state_t* frame_state_) {
    if(NULL == frame_state_) {
        return;
    }
    // NOTE: frame_state_->framebuffer_height, frame_state_->framebuffer_width の値は保持する
    frame_state_->projection_dirty = false;
    frame_state_->view_dirty = false;
    frame_state_->window_resized = false;
}

bool application_frame_state_is_valid(const application_frame_state_t* frame_state_) {
    if(NULL == frame_state_) {
        return false;
    }
    if(!is_valid_shallow(frame_state_)) {
        return false;
    }
    return true;
}

static bool is_valid_shallow(const application_frame_state_t* frame_state_) {
    if(NULL == frame_state_) {
        return false;
    }
    if(0 > frame_state_->framebuffer_height) {
        return false;
    }
    if(0 > frame_state_->framebuffer_width) {
        return false;
    }
    return true;
}
