// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_MEMORY_SYSTEM_MEMORY_SYSTEM_H
#define GLCE_ENGINE_SYSTEMS_MEMORY_SYSTEM_MEMORY_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/systems/memory_system/core/memory_system_types.h"

typedef struct memory_system memory_system_t;

memory_system_result_t memory_system_create(void);

void memory_system_destroy(void);

bool memory_system_is_valid(const memory_system_t* memory_system_);

#ifdef __cplusplus
}
#endif
#endif
