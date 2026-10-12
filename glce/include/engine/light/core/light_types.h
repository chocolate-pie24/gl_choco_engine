// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_LIGHT_CORE_LIGHT_TYPES_H
#define GLCE_ENGINE_LIGHT_CORE_LIGHT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LIGHT_SUCCESS = 0,
    LIGHT_INVALID_ARGUMENT,
    LIGHT_RUNTIME_ERROR,
    LIGHT_BAD_OPERATION,
    LIGHT_DATA_CORRUPTED,
    LIGHT_UNDEFINED_ERROR,
} light_result_t;

#ifdef __cplusplus
}
#endif
#endif
