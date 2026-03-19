/**
 * @file first_person_camera.h
 * @author chocolate-pie24
 * @brief 一人称視点のカメラ制御機能を提供する
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_CAMERA_CONTROL_FIRST_PERSON_CAMERA_H
#define GLCE_ENGINE_CAMERA_CONTROL_FIRST_PERSON_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/view/view_core/view_types.h"

#include "engine/view/camera/camera.h"

view_result_t camera_control_move_forward(float speed_, float delta_time_, camera_t* camera_);

view_result_t camera_control_move_backward(float speed_, float delta_time_, camera_t* camera_);

view_result_t camera_control_move_right(float speed_, float delta_time_, camera_t* camera_);

view_result_t camera_control_move_left(float speed_, float delta_time_, camera_t* camera_);

view_result_t camera_control_pitch(float speed_, float delta_time_, camera_t* camera_);

view_result_t camera_control_yaw(float speed_, float delta_time_, camera_t* camera_);

#ifdef __cplusplus
}
#endif
#endif
