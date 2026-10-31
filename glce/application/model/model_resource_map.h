// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_MODEL_MODEL_RESOURCE_MAP_H
#define GLCE_APPLICATION_MODEL_MODEL_RESOURCE_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "application/model/core/model_types.h"

typedef struct model_resource_map model_resource_map_t;

typedef struct model_assembly_config model_assembly_config_t;

typedef struct line_mesh line_mesh_t;
typedef struct point_mesh point_mesh_t;
typedef struct ui_mesh ui_mesh_t;
typedef struct untextured_mesh untextured_mesh_t;
typedef struct model_transform_tree model_transform_tree_t;

typedef struct renderer_frontend renderer_frontend_t;

model_result_t model_resource_map_create(renderer_frontend_t* renderer_frontend_, const model_assembly_config_t* config_, const char* asset_root_path_, model_resource_map_t** out_resource_map_);

model_result_t model_resource_map_destroy(renderer_frontend_t* renderer_frontend_, model_resource_map_t** resource_map_);

model_result_t model_resource_map_transform_tree_clone(const model_resource_map_t* resource_map_, model_transform_tree_t** out_transform_tree_);

// for Line Mesh.
model_result_t model_resource_map_line_mesh_get(const model_resource_map_t* resource_map_, uint16_t mesh_index_, line_mesh_t* out_mesh_);

model_result_t model_resource_map_line_mesh_coordinate_id_get(const model_resource_map_t* resource_map_, uint16_t mesh_index_, uint16_t* out_coordinate_id_);

model_result_t model_resource_map_line_mesh_count_get(const model_resource_map_t* resource_map_, size_t* out_count_);

// for Untextured Mesh.
model_result_t model_resource_map_untextured_mesh_get(const model_resource_map_t* resource_map_, uint16_t mesh_index_, untextured_mesh_t* out_mesh_);

model_result_t model_resource_map_untextured_mesh_coordinate_id_get(const model_resource_map_t* resource_map_, uint16_t mesh_index_, uint16_t* out_coordinate_id_);

model_result_t model_resource_map_untextured_mesh_count_get(const model_resource_map_t* resource_map_, size_t* out_count_);

// Validator.
bool model_resource_map_is_valid(const model_resource_map_t* resource_map_);

#ifdef __cplusplus
}
#endif
#endif
