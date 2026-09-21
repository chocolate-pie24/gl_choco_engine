// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_MEMORY_SYSTEM_CORE_MEMORY_SYSTEM_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_MEMORY_SYSTEM_CORE_MEMORY_SYSTEM_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/memory_system/core/memory_system_types.h"

#include "engine/memory/allocators/free_list_allocator.h"

const char* memory_system_rslt_to_str(memory_system_result_t rslt_);

memory_system_result_t memory_system_result_convert_free_list_allocator(free_list_allocator_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
