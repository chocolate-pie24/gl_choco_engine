// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_CORE_PLATFORM_EVENT_VIEW_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_CORE_PLATFORM_EVENT_VIEW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct window_event window_event_t;
typedef struct keyboard_event keyboard_event_t;
typedef struct mouse_event mouse_event_t;

typedef struct platform_event_view {
    bool window_close_requested;

    size_t window_event_count;
    const window_event_t* window_events;

    size_t keyboard_event_count;
    const keyboard_event_t* keyboard_events;

    size_t mouse_event_count;
    const mouse_event_t* mouse_events;
} platform_event_view_t;

bool platform_event_view_is_valid(const platform_event_view_t* event_view_);

#ifdef __cplusplus
}
#endif
#endif
