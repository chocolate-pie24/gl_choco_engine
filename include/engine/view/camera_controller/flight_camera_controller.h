/**
 * @ingroup view
 * @file flight_camera_controller.h
 * @author chocolate-pie24
 * @brief 空間内を自由に移動する飛行型カメラの制御機能を提供する
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
#ifndef GLCE_ENGINE_CAMERA_CONTROLLER_FLIGHT_CAMERA_CONTROLLER_H
#define GLCE_ENGINE_CAMERA_CONTROLLER_FLIGHT_CAMERA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/view/view_core/view_types.h"

#include "engine/view/camera/camera.h"

view_result_t flight_camera_controller_move_forward(float speed_, float delta_time_, camera_t* camera_);

view_result_t flight_camera_controller_move_backward(float speed_, float delta_time_, camera_t* camera_);

view_result_t flight_camera_controller_move_right(float speed_, float delta_time_, camera_t* camera_);

view_result_t flight_camera_controller_move_left(float speed_, float delta_time_, camera_t* camera_);

view_result_t flight_camera_controller_move_up(float speed_, float delta_time_, camera_t* camera_);

view_result_t flight_camera_controller_move_down(float speed_, float delta_time_, camera_t* camera_);

view_result_t flight_camera_controller_rot_pitch(float speed_, float delta_time_, camera_t* camera_);

view_result_t flight_camera_controller_rot_yaw(float speed_, float delta_time_, camera_t* camera_);

#ifdef __cplusplus
}
#endif
#endif
