// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_PLATFORM_SYSTEM_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_PLATFORM_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/core/memory/linear_allocator.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/systems/platform_system/core/platform_system_types.h"

typedef struct platform_system platform_system_t;
typedef struct platform_event_view platform_event_view_t;
typedef struct platform_system_config platform_system_config_t;

platform_system_result_t platform_system_create(const platform_system_config_t* config_, linear_alloc_t* allocator_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_system_t** out_platform_system_);

void platform_system_deinitialize(platform_system_t* platform_system_);

platform_system_result_t platform_system_update(platform_system_t* platform_system_, const platform_event_view_t** out_event_view_);

platform_system_result_t platform_system_swap_buffers(platform_system_t* platform_system_);

bool platform_system_is_valid(const platform_system_t* platform_system_);

#ifdef __cplusplus
}
#endif
#endif
