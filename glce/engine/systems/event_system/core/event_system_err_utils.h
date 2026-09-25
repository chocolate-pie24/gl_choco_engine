// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_EVENT_SYSTEM_CORE_EVENT_SYSTEM_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_EVENT_SYSTEM_CORE_EVENT_SYSTEM_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/platform_system/core/platform_system_types.h"

#include "engine/systems/event_system/core/event_system_types.h"

const char* event_system_result_to_str(event_system_result_t result_);

event_system_result_t event_system_result_convert_subsystem_allocator(subsystem_allocator_result_t result_);

event_system_result_t event_system_result_convert_platform_system(platform_system_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
