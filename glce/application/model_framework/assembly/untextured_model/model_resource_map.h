// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_UNTEXTURED_MODEL_MODEL_RESOURCE_MAP_H
#define GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_UNTEXTURED_MODEL_MODEL_RESOURCE_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "application/model_framework/assembly/core/model_framework_types.h"

typedef struct untextured_model_resource_map untextured_model_resource_map_t;

typedef struct untextured_model_assembly_config untextured_model_assembly_config_t;

typedef struct line_mesh line_mesh_t;
typedef struct untextured_mesh untextured_mesh_t;
typedef struct model_transform_tree model_transform_tree_t;

typedef struct renderer_frontend renderer_frontend_t;

model_framework_result_t untextured_model_resource_map_create(renderer_frontend_t* renderer_frontend_, const char* resource_map_name_, const untextured_model_assembly_config_t* config_, const char* asset_root_path_, untextured_model_resource_map_t** out_resource_map_);

model_framework_result_t untextured_model_resource_map_destroy(renderer_frontend_t* renderer_frontend_, untextured_model_resource_map_t** resource_map_);

model_framework_result_t untextured_model_resource_map_transform_tree_clone(const untextured_model_resource_map_t* resource_map_, model_transform_tree_t** out_transform_tree_);

// for Untextured Mesh.
model_framework_result_t untextured_model_resource_map_untextured_mesh_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, untextured_mesh_t* out_mesh_);

model_framework_result_t untextured_model_resource_map_untextured_mesh_coordinate_id_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, uint16_t* out_coordinate_id_);

model_framework_result_t untextured_model_resource_map_untextured_mesh_count_get(const untextured_model_resource_map_t* resource_map_, size_t* out_count_);

// for Debug AABB Line Mesh.
model_framework_result_t untextured_model_resource_map_line_mesh_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, line_mesh_t* out_mesh_);

model_framework_result_t untextured_model_resource_map_line_mesh_coordinate_id_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, uint16_t* out_coordinate_id_);

model_framework_result_t untextured_model_resource_map_line_mesh_count_get(const untextured_model_resource_map_t* resource_map_, size_t* out_count_);

// Validator.
bool untextured_model_resource_map_is_valid(const untextured_model_resource_map_t* resource_map_);

#ifdef __cplusplus
}
#endif
#endif
