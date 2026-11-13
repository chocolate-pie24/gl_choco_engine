// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_DEMOS_SAMPLE_ROBOT_SAMPLE_ROBOT_H
#define GLCE_APPLICATION_DEMOS_SAMPLE_ROBOT_SAMPLE_ROBOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "application/demos/core/scene_types.h"

typedef struct sample_robot sample_robot_t;

typedef struct renderer_frontend renderer_frontend_t;
typedef struct engine_event_view engine_event_view_t;

scene_result_t sample_robot_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, sample_robot_t** out_sample_robot_);

void sample_robot_destroy(renderer_frontend_t* renderer_frontend_, sample_robot_t** sample_robot_);

scene_result_t sample_robot_update(
    sample_robot_t* sample_robot_,
    const engine_event_view_t* event_view_,
    float delta_time_,
    bool window_resized_,
    int framebuffer_width_,
    int framebuffer_height_);

scene_result_t sample_robot_draw(sample_robot_t* sample_robot_, renderer_frontend_t* renderer_frontend_);

// TODO: 実装
bool sample_robot_is_valid(const sample_robot_t* sample_robot_);

#ifdef __cplusplus
}
#endif
#endif
