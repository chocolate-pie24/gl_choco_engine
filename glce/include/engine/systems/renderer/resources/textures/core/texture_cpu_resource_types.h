// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_CORE_TEXTURE_CPU_RESOURCE_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_CORE_TEXTURE_CPU_RESOURCE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resources/textures/core/texture_types.h"

typedef struct texture_cpu_pixel_layout {
    int32_t width;
    int32_t height;
    uint8_t channel_count;
} texture_cpu_pixel_layout_t;

bool texture_cpu_pixel_layout_is_valid(texture_cpu_pixel_layout_t layout_);

texture_result_t texture_cpu_pixel_layout_data_size_calculate(texture_cpu_pixel_layout_t layout_, size_t* out_pixel_data_size_);

#ifdef __cplusplus
}
#endif
#endif
