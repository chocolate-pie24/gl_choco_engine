// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_DEMOS_SAMPLE_ROBOT_SAMPLE_ROBOT_DEMO_H
#define GLCE_APPLICATION_DEMOS_SAMPLE_ROBOT_SAMPLE_ROBOT_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "application/demos/sample_robot/core/sample_robot_types.h"

typedef struct sample_robot_demo sample_robot_demo_t;

typedef struct render_packet render_packet_t;
typedef struct renderer_frontend renderer_frontend_t;

sample_robot_result_t sample_robot_demo_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, sample_robot_demo_t** out_demo_);

void sample_robot_demo_destroy(renderer_frontend_t* renderer_frontend_, sample_robot_demo_t** demo_);

sample_robot_result_t sample_robot_demo_posture_update(float delta_time_, sample_robot_demo_t* demo_);

sample_robot_result_t sample_robot_demo_render_packet_build(const sample_robot_demo_t* demo_, render_packet_t* packet_);

// TODO: 実装
bool sample_robot_demo_is_valid(const sample_robot_demo_t* demo_);

#ifdef __cplusplus
}
#endif
#endif
