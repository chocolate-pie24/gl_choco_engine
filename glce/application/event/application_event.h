// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_EVENT_APPLICATION_EVENT_H
#define GLCE_APPLICATION_EVENT_APPLICATION_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "application/core/application_types.h"

typedef struct platform_context platform_context_t;
typedef struct linear_alloc linear_alloc_t;
typedef struct window_event window_event_t;
typedef struct keyboard_event keyboard_event_t;
typedef struct mouse_event mouse_event_t;

typedef struct application_event_view {
    bool window_close_requested;

    size_t window_event_count;
    const window_event_t* window_events;

    size_t keyboard_event_count;
    const keyboard_event_t* keyboard_events;

    size_t mouse_event_count;
    const mouse_event_t* mouse_events;
} application_event_view_t;

application_result_t application_event_initialize(platform_context_t* platform_context_, size_t max_window_event_count_, size_t max_keyboard_event_count_, size_t max_mouse_event_count_, linear_alloc_t* allocator_);

void application_event_deinitialize(void);

application_result_t application_event_update(const application_event_view_t** out_event_view_);

bool application_event_is_valid(void);

#ifdef __cplusplus
}
#endif
#endif
