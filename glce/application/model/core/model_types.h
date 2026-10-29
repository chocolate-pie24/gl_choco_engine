// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_MODEL_CORE_MODE_TYPES_H
#define GLCE_APPLICATION_MODEL_CORE_MODE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MODEL_SUCCESS = 0,
    MODEL_INVALID_ARGUMENT,
    MODEL_RUNTIME_ERROR,
    MODEL_NO_MEMORY,
    MODEL_LIMIT_EXCEEDED,
    MODEL_BAD_OPERATION,
    MODEL_DATA_CORRUPTED,
    MODEL_OVERFLOW,
    MODEL_UNDEFINED_ERROR,
} model_result_t;

#ifdef __cplusplus
}
#endif
#endif
