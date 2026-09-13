// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_EVENT_APPLICATION_FRAME_STATE_H
#define GLCE_APPLICATION_EVENT_APPLICATION_FRAME_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct application_frame_state {
    int framebuffer_width;
    int framebuffer_height;
    bool window_resized;
    bool projection_dirty;
    bool view_dirty;
} application_frame_state_t;

void application_frame_state_begin_frame(application_frame_state_t* frame_state_);

bool application_frame_state_is_valid(const application_frame_state_t* frame_state_);

#ifdef __cplusplus
}
#endif
#endif
