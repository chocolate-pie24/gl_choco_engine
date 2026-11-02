// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_UNTEXTURED_MODEL_MODEL_ASSEMBLY_CONFIG_H
#define GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_UNTEXTURED_MODEL_MODEL_ASSEMBLY_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "application/model_framework/assembly/core/model_framework_types.h"

typedef struct model_coordinate_config model_coordinate_config_t;
typedef struct untextured_mesh_config untextured_mesh_config_t;

typedef struct untextured_model_assembly_config {
    size_t coordinate_count;
    model_coordinate_config_t* coordinate_configs;

    size_t mesh_count;
    uint16_t* mesh_coordinate_ids;
    untextured_mesh_config_t* mesh_configs;
} untextured_model_assembly_config_t;

model_framework_result_t untextured_model_assembly_config_create(
    size_t coordinate_count_,
    model_coordinate_config_t** coordinate_configs_,
    size_t mesh_count_,
    uint16_t** mesh_coordinate_ids_,
    untextured_mesh_config_t** mesh_configs_,
    untextured_model_assembly_config_t** out_config_);

void untextured_model_assembly_config_destroy(untextured_model_assembly_config_t** config_);

bool untextured_model_assembly_config_is_valid(const untextured_model_assembly_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
