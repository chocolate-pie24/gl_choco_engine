// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_TEXTURE_TEXTURE_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_TEXTURE_TEXTURE_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

typedef struct texture_registry texture_registry_t;
typedef struct linear_alloc linear_alloc_t;
typedef struct texture_gpu_resource texture_gpu_resource_t;
typedef struct texture_cpu_resource texture_cpu_resource_t;
typedef struct renderer_backend_context renderer_backend_context_t;

resource_registry_result_t texture_registry_initialize(size_t max_texture_count_, linear_alloc_t* allocator_, texture_registry_t** out_registry_);

void texture_registry_deinitialize(texture_registry_t* registry_, renderer_backend_context_t* backend_context_);

bool texture_registry_find(const texture_registry_t* registry_, const char* name_);

const char* texture_registry_name_get(const texture_registry_t* registry_, int16_t texture_id_);

const texture_gpu_resource_t* texture_registry_gpu_resource_get(const texture_registry_t* registry_, int16_t texture_id_);

const texture_cpu_resource_t* texture_registry_cpu_resource_get(const texture_registry_t* registry_, int16_t texture_id_);

resource_registry_result_t texture_registry_id_get(const texture_registry_t* registry_, const char* name_, int16_t* out_texture_id_);

resource_registry_result_t texture_registry_register(texture_registry_t* registry_, const char* resource_name_, texture_gpu_resource_t** gpu_resource_, texture_cpu_resource_t** cpu_resource_, int16_t* out_texture_id_);

resource_registry_result_t texture_registry_unregister(texture_registry_t* registry_, renderer_backend_context_t* backend_context_, int16_t texture_id_);

bool texture_registry_is_valid(const texture_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
