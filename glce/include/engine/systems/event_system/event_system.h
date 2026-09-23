// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_EVENT_SYSTEM_EVENT_SYSTEM_H
#define GLCE_ENGINE_SYSTEMS_EVENT_SYSTEM_EVENT_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include "engine/systems/event_system/core/event_system_types.h"

typedef struct event_system event_system_t;
typedef struct engine_event_view engine_event_view_t;
typedef struct event_system_config event_system_config_t;
typedef struct platform_system platform_system_t;

event_system_result_t event_system_create(const event_system_config_t* config_, linear_allocator_t* allocator_, platform_system_t* platform_system_, event_system_t** out_event_system_);

void event_system_deinitialize(event_system_t* event_system_);

event_system_result_t event_system_update(event_system_t* event_system_, const engine_event_view_t** out_event_view_);

bool event_system_is_valid(const event_system_t* event_system_);

#ifdef __cplusplus
}
#endif
#endif
