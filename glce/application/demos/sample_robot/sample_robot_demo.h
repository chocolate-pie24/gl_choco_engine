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
typedef struct engine_event_view engine_event_view_t;
typedef struct mat4x4f mat4x4f_t;

sample_robot_result_t sample_robot_demo_create(renderer_frontend_t* renderer_frontend_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, sample_robot_demo_t** out_demo_);

void sample_robot_demo_destroy(renderer_frontend_t* renderer_frontend_, sample_robot_demo_t** demo_);

// projection, viewをshaderごとに分けて取得できるようにする。scene_untextured_mesh_shader_view_get
// そもそもapplicationがviewとprojectionを持つ必要があるのか？
// TODO: 引数整理
sample_robot_result_t sample_robot_demo_view_update(
    sample_robot_demo_t* demo_,
    const engine_event_view_t* event_view_,
    float delta_time_,
    bool window_resized_,
    int framebuffer_width_,
    int framebuffer_height_,
    bool* out_view_changed_,
    bool* out_projection_changed_);

sample_robot_result_t sample_robot_demo_posture_update(float delta_time_, sample_robot_demo_t* demo_);

sample_robot_result_t sample_robot_demo_render_packet_build(const sample_robot_demo_t* demo_, render_packet_t* packet_);

sample_robot_result_t sample_robot_demo_view_matrix_get(const sample_robot_demo_t* demo_, mat4x4f_t* out_matrix_);

sample_robot_result_t sample_robot_demo_perspective_matrix_get(const sample_robot_demo_t* demo_, mat4x4f_t* out_matrix_);

// TODO: 実装
bool sample_robot_demo_is_valid(const sample_robot_demo_t* demo_);

#ifdef __cplusplus
}
#endif
#endif
