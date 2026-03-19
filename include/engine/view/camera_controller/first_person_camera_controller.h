/**
 * @file first_person_camera_controller.h
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
#ifndef GLCE_ENGINE_CAMERA_CONTROLLER_FIRST_PERSON_CAMERA_CONTROLLER_H
#define GLCE_ENGINE_CAMERA_CONTROLLER_FIRST_PERSON_CAMERA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/view/view_core/view_types.h"

#include "engine/view/camera/camera.h"

/**
 * @brief 一人称カメラを前方に動かす
 *
 * @details 移動量 = speed_ x delta_time_
 *
 * @param speed_ 移動速度
 * @param delta_time_ 移動時間
 * @param camera_ 動かすカメラ構造体インスタンスへのポインタ
 *
 * @retval VIEW_INVALID_ARGUMENT camera_ == NULL
 * @retval VIEW_RUNTIME_ERROR 逆行列が求まらない
 * @retval VIEW_RUNTIME_ERROR カメラ姿勢の取得もしくは更新に失敗
 * @retval VIEW_SUCCESS 処理に成功し、正常終了
 */
view_result_t first_person_camera_controller_move_forward(float speed_, float delta_time_, camera_t* camera_);

view_result_t first_person_camera_controller_move_backward(float speed_, float delta_time_, camera_t* camera_);

view_result_t first_person_camera_controller_move_right(float speed_, float delta_time_, camera_t* camera_);

view_result_t first_person_camera_controller_move_left(float speed_, float delta_time_, camera_t* camera_);

view_result_t first_person_camera_controller_move_up(float speed_, float delta_time_, camera_t* camera_);

view_result_t first_person_camera_controller_move_down(float speed_, float delta_time_, camera_t* camera_);

view_result_t first_person_camera_controller_rot_pitch(float speed_, float delta_time_, camera_t* camera_);

view_result_t first_person_camera_controller_rot_yaw(float speed_, float delta_time_, camera_t* camera_);

#ifdef __cplusplus
}
#endif
#endif
