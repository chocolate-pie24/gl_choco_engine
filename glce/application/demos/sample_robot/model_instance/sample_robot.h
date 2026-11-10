// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_DEMOS_SAMPLE_ROBOT_MODEL_INSTANCE_SAMPLE_ROBOT_H
#define GLCE_APPLICATION_DEMOS_SAMPLE_ROBOT_MODEL_INSTANCE_SAMPLE_ROBOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/base/choco_math/math_types.h"

#include "application/model_framework/core/model_framework_types.h"

typedef struct untextured_model_resource_map untextured_model_resource_map_t;
typedef struct model_transform_tree model_transform_tree_t;
typedef struct untextured_model_assembly_config untextured_model_assembly_config_t;

typedef struct {
    float base_motor_angle;
    float shoulder_motor_angle;
    float elbow_motor_angle;
    float wrist_motor_angle;

    float left_finger_angle;
    float right_finger_angle;
} sample_robot_posture_t;

typedef struct {
    model_transform_tree_t* transform_tree;
    sample_robot_posture_t posture;
} sample_robot_t;

model_framework_result_t sample_robot_create(const untextured_model_resource_map_t* resource_map_, sample_robot_t** out_robot_);

void sample_robot_destroy(sample_robot_t** robot_);

model_framework_result_t sample_robot_placement_set(vec3f_t translate_, vec3f_t rotation_, sample_robot_t* robot_);

model_framework_result_t sample_robot_posture_update(const sample_robot_posture_t* posture_, sample_robot_t* robot_);

void sample_robot_posture_default_get(sample_robot_posture_t* posture_);

model_framework_result_t sample_robot_dummy_config_loader(untextured_model_assembly_config_t** out_config_);

#ifdef __cplusplus
}
#endif
#endif
