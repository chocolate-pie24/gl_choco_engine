// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_MODEL_SAMPLE_ROBOT_SAMPLE_ROBOT_H
#define GLCE_APPLICATION_MODEL_SAMPLE_ROBOT_SAMPLE_ROBOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_math/math_types.h"

#include "application/model/core/model_types.h"

typedef struct model_resource_map model_resource_map_t;
typedef struct model_transform_tree model_transform_tree_t;
typedef struct model_assembly_config model_assembly_config_t;

typedef struct {
    float link1_angle;
    float link2_angle;
    float gripper_angle;
    float left_finger_angle;
    float right_finger_angle;
} sample_robot_posture_t;

typedef struct {
    model_transform_tree_t* transform_tree;
    sample_robot_posture_t posture;
} sample_robot_t;

model_result_t sample_robot_create(const model_resource_map_t* resource_map_, sample_robot_t** out_robot_);

void sample_robot_destroy(sample_robot_t** robot_);

model_result_t sample_robot_placement_set(vec3f_t translate_, vec3f_t rotation_, sample_robot_t* robot_);

model_result_t sample_robot_posture_update(const sample_robot_posture_t* posture_, sample_robot_t* robot_);

void sample_robot_posture_default_get(sample_robot_posture_t* posture_);

model_result_t sample_robot_dummy_config_loader(model_assembly_config_t* out_config_);

#ifdef __cplusplus
}
#endif
#endif
