// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

// NOTE: 以下のAPIは複数カメラを使用する必要が出た時に実装する
// - flight_cameraの追加API
// - activeカメラの変更API
#ifndef GLCE_APPLICATION_CAMERAS_APPLICATION_FLIGHT_CAMERA_H
#define GLCE_APPLICATION_CAMERAS_APPLICATION_FLIGHT_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "application/core/application_types.h"

typedef struct application_flight_camera application_flight_camera_t;

typedef struct mat4x4f mat4x4f_t;
typedef struct linear_alloc linear_alloc_t;
typedef struct application_event_view application_event_view_t;

application_result_t application_flight_camera_initialize(size_t max_flight_camera_count_, linear_alloc_t* allocator_, int framebuffer_width_, int framebuffer_height_, application_flight_camera_t** out_application_flight_camera_);

void application_flight_camera_deinitialize(application_flight_camera_t* application_flight_camera_);

application_result_t application_flight_camera_update(application_flight_camera_t* application_flight_camera_, float speed_, float delta_time_, const application_event_view_t* application_event_view_, bool* out_view_dirty_, bool* out_projection_dirty_);

application_result_t application_flight_camera_view_matrix_get(application_flight_camera_t* application_flight_camera_, mat4x4f_t* out_matrix_);

application_result_t application_flight_camera_perspective_matrix_get(application_flight_camera_t* application_flight_camera_, mat4x4f_t* out_matrix_);

bool application_flight_camera_is_valid(const application_flight_camera_t* application_flight_camera_);

#ifdef __cplusplus
}
#endif
#endif
