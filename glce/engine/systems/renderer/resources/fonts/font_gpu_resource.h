// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_FONT_GPU_RESOURCE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_FONT_GPU_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/fonts/core/font_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct font_atlas_asset font_atlas_asset_t;
typedef struct texture_gpu_resource texture_gpu_resource_t;

font_result_t font_gpu_resource_create(const renderer_backend_context_t* backend_context_, int32_t texture_unit_index_, const font_atlas_asset_t* asset_, texture_gpu_resource_t** out_gpu_resource_);

#ifdef __cplusplus
}
#endif

#endif
