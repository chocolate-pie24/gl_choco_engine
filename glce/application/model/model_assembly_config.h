// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_MODEL_MODEL_ASSEMBLY_CONFIG_H
#define GLCE_APPLICATION_MODEL_MODEL_ASSEMBLY_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "application/model/core/model_types.h"

typedef struct line_mesh_config line_mesh_config_t;
typedef struct point_mesh_config point_mesh_config_t;
typedef struct ui_mesh_config ui_mesh_config_t;
typedef struct untextured_mesh_config untextured_mesh_config_t;

typedef struct model_coordinate_config {
    uint16_t coordinate_id;
    uint16_t parent_coordinate_id;

    vec3f_t translate;
    vec3f_t rotation;
} model_coordinate_config_t;

typedef struct model_assembly_config {
    size_t coordinate_count;
    model_coordinate_config_t* coordinates;

    // line mesh
    size_t line_mesh_count;
    uint16_t* line_mesh_coordinate_ids;
    line_mesh_config_t* line_mesh_configs;

    // point mesh
    size_t point_mesh_count;
    uint16_t* point_mesh_coordinate_ids;
    point_mesh_config_t* point_mesh_configs;

    // ui mesh
    size_t ui_mesh_count;
    uint16_t* ui_mesh_coordinate_ids;
    ui_mesh_config_t* ui_mesh_configs;

    // untextured mesh
    size_t untextured_mesh_count;
    uint16_t* untextured_mesh_coordinate_ids;
    untextured_mesh_config_t* untextured_mesh_configs;
} model_assembly_config_t;

model_result_t model_coordinate_config_initialize(uint16_t coordinate_id_, uint16_t parent_coordinate_id_, vec3f_t translate_, vec3f_t rotation_, model_coordinate_config_t* out_config_);

void model_coordinate_config_deinitialize(model_coordinate_config_t* config_);

model_result_t model_assembly_config_create(
    size_t coordinate_count_,
    model_coordinate_config_t** coordinates_,
    size_t line_mesh_count_,
    uint16_t** line_mesh_coordinate_ids_,
    line_mesh_config_t** line_mesh_configs_,
    size_t point_mesh_count_,
    uint16_t** point_mesh_coordinate_ids_,
    point_mesh_config_t** point_mesh_configs_,
    size_t ui_mesh_count_,
    uint16_t** ui_mesh_coordinate_ids_,
    ui_mesh_config_t** ui_mesh_configs_,
    size_t untextured_mesh_count_,
    uint16_t** untextured_mesh_coordinate_ids_,
    untextured_mesh_config_t** untextured_mesh_configs_,
    model_assembly_config_t** out_config_);

void model_assembly_config_destroy(model_assembly_config_t** config_);

bool model_coordinate_config_is_valid(const model_coordinate_config_t* config_);

bool model_assembly_config_is_valid(const model_assembly_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
