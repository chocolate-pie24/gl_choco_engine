// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_CONFIG_PLATFORM_CONFIG_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_CONFIG_PLATFORM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct platform_config {
    size_t max_window_event_count;
    size_t max_keyboard_event_count;
    size_t max_mouse_event_count;

    const char* window_label;

    int window_width;
    int window_height;
} platform_config_t;

bool platform_config_is_valid(const platform_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
