// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_CAMERA_FLIGHT_CAMERA_H
#define GLCE_ENGINE_CAMERA_FLIGHT_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/core/event/keyboard_event.h"
#include "engine/camera/core/camera_types.h"

typedef struct flight_camera flight_camera_t;

typedef struct mat4x4f mat4x4f_t;

typedef enum {
    FLIGHT_CAMERA_COMMAND_MOVE_FORWARD = 0,   /**< フライトカメラ制御コマンド: 前方移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD,      /**< フライトカメラ制御コマンド: 後方移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_RIGHT,         /**< フライトカメラ制御コマンド: 右方向移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_LEFT,          /**< フライトカメラ制御コマンド: 左方向移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_UP,            /**< フライトカメラ制御コマンド: 上方向移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_DOWN,          /**< フライトカメラ制御コマンド: 下方向移動 */
    FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS,     /**< フライトカメラ制御コマンド: ピッチ+方向回転 */
    FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS,    /**< フライトカメラ制御コマンド: ピッチ-方向回転 */
    FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS,       /**< フライトカメラ制御コマンド: ヨー+方向回転 */
    FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS,      /**< フライトカメラ制御コマンド: ヨー-方向回転 */
    FLIGHT_CAMERA_COMMAND_MAX,
} flight_camera_command_t;

typedef struct flight_camera_key_bind {
    keycode_t key;
} flight_camera_key_bind_t;

camera_result_t flight_camera_create(const flight_camera_key_bind_t keybinds_[FLIGHT_CAMERA_COMMAND_MAX], float fovy_, float aspect_, float near_clip_, float far_clip_, flight_camera_t** out_flight_camera_);

void flight_camera_destroy(flight_camera_t** out_flight_camera_);

camera_result_t flight_camera_command_update(flight_camera_t* flight_camera_, const keyboard_event_t* keyboard_event_);

camera_result_t flight_camera_command_execute(flight_camera_t* flight_camera_, float speed_, float delta_time_, bool* out_view_dirty_);

camera_result_t flight_camera_viewing_frustum_update(flight_camera_t* flight_camera_, float fovy_, float aspect_, float near_clip_, float far_clip_);

camera_result_t flight_camera_perspective_matrix_get(flight_camera_t* flight_camera_, mat4x4f_t* out_mat_);

camera_result_t flight_camera_view_matrix_get(flight_camera_t* flight_camera_, mat4x4f_t* out_mat_);

bool flight_camera_is_valid(const flight_camera_t* flight_camera_);

#ifdef __cplusplus
}
#endif
#endif
