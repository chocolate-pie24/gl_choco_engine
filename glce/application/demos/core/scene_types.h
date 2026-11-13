// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_DEMOS_CORE_SCENE_TYPES_H
#define GLCE_APPLICATION_DEMOS_CORE_SCENE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCENE_SUCCESS = 0,
    SCENE_NO_MEMORY,
    SCENE_RUNTIME_ERROR,
    SCENE_INVALID_ARGUMENT,
    SCENE_DATA_CORRUPTED,
    SCENE_BAD_OPERATION,
    SCENE_OVERFLOW,
    SCENE_LIMIT_EXCEEDED,
    SCENE_UNDEFINED_ERROR,
} scene_result_t;

#ifdef __cplusplus
}
#endif
#endif
