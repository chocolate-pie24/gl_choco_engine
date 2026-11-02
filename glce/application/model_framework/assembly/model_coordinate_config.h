// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_MODEL_COORDINATE_CONFIG_H
#define GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_MODEL_COORDINATE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "application/model_framework/assembly/core/model_framework_types.h"

typedef struct model_coordinate_config {
    uint16_t coordinate_id;
    uint16_t parent_coordinate_id;

    vec3f_t translate;
    vec3f_t rotation;
} model_coordinate_config_t;

model_framework_result_t model_coordinate_config_initialize(uint16_t coordinate_id_, uint16_t parent_coordinate_id_, vec3f_t translate_, vec3f_t rotation_, model_coordinate_config_t* out_config_);

void model_coordinate_config_deinitialize(model_coordinate_config_t* config_);

bool model_coordinate_config_is_valid(const model_coordinate_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
