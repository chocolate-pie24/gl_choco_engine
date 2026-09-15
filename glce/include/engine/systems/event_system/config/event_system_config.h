// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_EVENT_SYSTEM_CONFIG_EVENT_SYSTEM_CONFIG_H
#define GLCE_ENGINE_SYSTEMS_EVENT_SYSTEM_CONFIG_EVENT_SYSTEM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct event_system_config {
    size_t max_window_event_count;
    size_t max_keyboard_event_count;
    size_t max_mouse_event_count;
} event_system_config_t;

bool event_system_config_is_valid(const event_system_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
