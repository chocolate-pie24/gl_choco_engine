// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_TEXTURE_TEXTURE_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_TEXTURE_TEXTURE_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct texture_registry texture_registry_t;

resource_pipeline_result_t texture_pipeline_import_from_bmp(renderer_backend_context_t* backend_context_, texture_registry_t* texture_registry_, int32_t gpu_unit_num_, const char* resource_name_, const char* texture_fullpath_, int16_t* out_texture_id_);

resource_pipeline_result_t texture_pipeline_import_from_solid_color(renderer_backend_context_t* backend_context_, texture_registry_t* texture_registry_, int32_t gpu_unit_num_, const char* resource_name_, uint8_t red_, uint8_t green_, uint8_t blue_, int16_t* out_texture_id_);

resource_pipeline_result_t texture_pipeline_release(renderer_backend_context_t* backend_context_, texture_registry_t* texture_registry_, int16_t texture_id_);

#ifdef __cplusplus
}
#endif
#endif
