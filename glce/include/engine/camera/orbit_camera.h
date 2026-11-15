// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_CAMERA_ORBIT_CAMERA_H
#define GLCE_ENGINE_CAMERA_ORBIT_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/camera/core/camera_types.h"

typedef struct orbit_camera orbit_camera_t;

typedef struct mat4x4f mat4x4f_t;
typedef struct mouse_event mouse_event_t;

typedef enum {
    ORBIT_CAMERA_COMMAND_ROT_YAW = 0,   /**< フライトカメラ制御コマンド: 前方移動 */
    ORBIT_CAMERA_COMMAND_ROT_PITCH,
    ORBIT_CAMERA_COMMAND_MOVE_LEFT,
    ORBIT_CAMERA_COMMAND_MOVE_RIGHT,
    ORBIT_CAMERA_COMMAND_MOVE_FORWARD,
    ORBIT_CAMERA_COMMAND_MOVE_BACKWARD,
    ORBIT_CAMERA_COMMAND_MAX,
} orbit_camera_command_t;

camera_result_t orbit_camera_create(float fovy_, float aspect_, float near_clip_, float far_clip_, orbit_camera_t** out_camera_);

void orbit_camera_destroy(orbit_camera_t** camera_);

camera_result_t orbit_camera_command_update(orbit_camera_t* camera_, const mouse_event_t* event_);

camera_result_t orbit_camera_command_execute(orbit_camera_t* camera_, bool* out_view_dirty_);

camera_result_t orbit_camera_viewing_frustum_update(orbit_camera_t* camera_, float fovy_, float aspect_, float near_clip_, float far_clip_);

camera_result_t orbit_camera_perspective_matrix_get(orbit_camera_t* camera_, mat4x4f_t* out_mat_);

camera_result_t orbit_camera_view_matrix_get(orbit_camera_t* camera_, mat4x4f_t* out_mat_);

bool orbit_camera_is_valid(const orbit_camera_t* camera_);

#ifdef __cplusplus
}
#endif
#endif
