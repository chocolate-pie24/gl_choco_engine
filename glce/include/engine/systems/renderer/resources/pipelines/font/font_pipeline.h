// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_FONT_FONT_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_FONT_FONT_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct font_registry font_registry_t;
typedef struct font_atlas_asset font_atlas_asset_t;

resource_pipeline_result_t font_pipeline_import(const renderer_backend_context_t* backend_context_, font_registry_t* registry_, int32_t texture_unit_index_, const font_atlas_asset_t* asset_, uint16_t* out_font_id_);

resource_pipeline_result_t font_pipeline_release(font_registry_t* registry_, uint16_t font_id_);

#ifdef __cplusplus
}
#endif
#endif
