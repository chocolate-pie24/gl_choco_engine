// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_REGISTRIES_FONT_FONT_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_REGISTRIES_FONT_FONT_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"

typedef struct font_registry font_registry_t;

typedef struct font_atlas_asset font_atlas_asset_t;
typedef struct texture_gpu_resource texture_gpu_resource_t;
typedef struct subsystem_allocator subsystem_allocator_t;

resource_registry_result_t font_registry_create(size_t max_font_count_, subsystem_allocator_t* allocator_, font_registry_t** out_registry_);

void font_registry_deinitialize(font_registry_t* registry_);

resource_registry_result_t font_registry_exists(const font_registry_t* registry_, const char* font_name_, int32_t cell_width_, int32_t cell_height_, bool* out_exists_);

const font_atlas_asset_t* font_registry_atlas_asset_get(const font_registry_t* registry_, uint16_t font_id_);

const texture_gpu_resource_t* font_registry_gpu_resource_get(const font_registry_t* registry_, uint16_t font_id_);

resource_registry_result_t font_registry_acquire(font_registry_t* registry_, const char* font_name_, int32_t cell_width_, int32_t cell_height_, uint16_t* out_font_id_);

resource_registry_result_t font_registry_register(font_registry_t* registry_, const char* font_name_, const font_atlas_asset_t* atlas_asset_, texture_gpu_resource_t** gpu_resource_, uint16_t* out_font_id_);

resource_registry_result_t font_registry_release(font_registry_t* registry_, uint16_t font_id_);

bool font_registry_is_valid(const font_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
