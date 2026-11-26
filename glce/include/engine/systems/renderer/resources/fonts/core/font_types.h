// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_CORE_FONT_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_CORE_FONT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FONT_SUCCESS = 0,
    FONT_NO_MEMORY,
    FONT_RUNTIME_ERROR,
    FONT_INVALID_ARGUMENT,
    FONT_DATA_CORRUPTED,
    FONT_BAD_OPERATION,
    FONT_OVERFLOW,
    FONT_LIMIT_EXCEEDED,
    FONT_UNDEFINED_ERROR,
} font_result_t;

#ifdef __cplusplus
}
#endif
#endif
