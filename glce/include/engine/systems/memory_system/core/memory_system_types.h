// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_MEMORY_SYSTEM_CORE_MEMORY_SYSTEM_TYPES_H
#define GLCE_ENGINE_SYSTEMS_MEMORY_SYSTEM_CORE_MEMORY_SYSTEM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    MEMORY_SYSTEM_SUCCESS = 0,
    MEMORY_SYSTEM_DATA_CORRUPTED,
    MEMORY_SYSTEM_BAD_OPERATION,
    MEMORY_SYSTEM_INVALID_ARGUMENT,
    MEMORY_SYSTEM_NO_MEMORY,
    MEMORY_SYSTEM_OVERFLOW,
    MEMORY_SYSTEM_UNDEFINED_ERROR,
} memory_system_result_t;

#ifdef __cplusplus
}
#endif
#endif
