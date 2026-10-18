// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_MODEL_SAMPLE_ROBOT_SAMPLE_ROBOT_H
#define GLCE_APPLICATION_MODEL_SAMPLE_ROBOT_SAMPLE_ROBOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "application/core/application_types.h"

typedef struct render_model render_model_t;
typedef struct render_model_config render_model_config_t;

typedef struct sample_robot_posture {
    float link1_angle;
    float link2_angle;
    float left_finger_angle;
    float right_finger_angle;
} sample_robot_posture_t;

typedef struct sample_robot {
    render_model_t* render_model;
    sample_robot_posture_t posture;
    bool posture_dirty;
} sample_robot_t;

application_result_t sample_robot_create(const char* model_name_, vec3f_t root_translate_, vec3f_t root_rotation_, sample_robot_t** out_robot_);

void sample_robot_destroy(sample_robot_t** robot_);

application_result_t sample_robot_posture_update(const sample_robot_posture_t* posture_, sample_robot_t* robot_);

application_result_t sample_robot_dummy_config_loader(render_model_config_t* out_config_);

#ifdef __cplusplus
}
#endif
#endif
