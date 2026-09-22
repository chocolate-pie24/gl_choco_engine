// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_CORE_PLATFORM_SYSTEM_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_CORE_PLATFORM_SYSTEM_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/platform_system/core/platform_system_types.h"

const char* platform_system_result_to_str(platform_system_result_t result_);

platform_system_result_t platform_system_result_convert_choco_string(choco_string_result_t result_);

platform_system_result_t platform_system_result_convert_linear_allocator(linear_allocator_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
