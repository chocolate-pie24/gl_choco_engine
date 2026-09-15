// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_EVENT_SYSTEM_CORE_EVENT_SYSTEM_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_EVENT_SYSTEM_CORE_EVENT_SYSTEM_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/memory/linear_allocator.h"

#include "engine/systems/platform/core/platform_types.h"

#include "engine/systems/event_system/core/event_system_types.h"

const char* event_system_rslt_to_str(event_system_result_t rslt_);

event_system_result_t event_system_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

event_system_result_t event_system_rslt_convert_platform(platform_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
