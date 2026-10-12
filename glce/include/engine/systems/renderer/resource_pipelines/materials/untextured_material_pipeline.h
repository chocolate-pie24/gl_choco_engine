// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_MATERIALS_UNTEXTURED_MATERIAL_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_MATERIALS_UNTEXTURED_MATERIAL_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

typedef struct untextured_material_registry untextured_material_registry_t;

resource_pipeline_result_t untextured_material_pipeline_import_from_file(untextured_material_registry_t* material_registry_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_);

resource_pipeline_result_t untextured_material_pipeline_release(untextured_material_registry_t* material_registry_, uint16_t material_id_);

#ifdef __cplusplus
}
#endif
#endif
