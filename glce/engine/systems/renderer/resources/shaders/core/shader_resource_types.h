// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_CORE_SHADER_RESOURCE_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_SHADERS_CORE_SHADER_RESOURCE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/resources/allocators/range_allocator.h"

typedef enum {
    SHADER_SUCCESS = 0,
    SHADER_INVALID_ARGUMENT,
    SHADER_RUNTIME_ERROR,
    SHADER_NO_MEMORY,
    SHADER_COMPILE_ERROR,
    SHADER_LINK_ERROR,
    SHADER_LIMIT_EXCEEDED,
    SHADER_BAD_OPERATION,
    SHADER_DATA_CORRUPTED,
    SHADER_OVERFLOW,
    SHADER_UNDEFINED_ERROR,
} shader_result_t;

typedef struct vbo_range {
    draw_range_t draw_range;
    range_allocation_t allocation_info;
} vbo_range_t;

bool vbo_range_is_valid(const vbo_range_t* vbo_range_);

#ifdef __cplusplus
}
#endif
#endif
