/**
 * @file first_person_camera_controller.c
 * @author chocolate-pie24
 * @brief 一人称視点のカメラ制御機能の実装
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
#include "engine/view/camera_controller/first_person_camera_controller.h"

#include "engine/view/camera/camera.h"

#include "engine/view/view_core/view_err_utils.h"
#include "engine/view/view_core/view_types.h"

#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

view_result_t first_person_camera_controller_move_forward(float speed_, float delta_time_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "first_person_camera_controller_move_forward", "camera_")

    mat4x4f_t view_mat = { 0 };
    ret = camera_view_matrix_get(camera_, &view_mat);   // ワールド座標系からカメラ座標系への変換行列
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_forward(%s) - Failed to get view matrix.", view_rslt_to_str(ret));
        goto cleanup;
    }
    if(!mat4f_inverse(&view_mat)) { // カメラ座標系からワールド座標系への変換行列
        ret = VIEW_RUNTIME_ERROR;
        ERROR_MESSAGE("first_person_camera_controller_move_forward(%s) - Failed to get mat4f inverse.", view_rslt_to_str(ret));
        goto cleanup;
    }

    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ前方の単位ベクトル[0, 0, -1, 1]を掛けて得られる値をカメラ座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを前方に移動させるための方向ベクトル
    v.elem[0] = -1.0f * view_mat.elem[2];
    v.elem[1] = -1.0f * view_mat.elem[6];
    v.elem[2] = -1.0f * view_mat.elem[10];
    vec3f_normalize(&v);

    // ワールド座標系でのカメラ移動量を計算
    v.elem[0] *= (speed_ * delta_time_);
    v.elem[1] *= (speed_ * delta_time_);
    v.elem[2] *= (speed_ * delta_time_);

    // 新しいカメラ座標を計算
    vec3f_t euler = { 0 };
    vec3f_t position = { 0 };
    if(VIEW_SUCCESS != camera_posture_get(camera_, &euler, &position)) {
        ret = VIEW_RUNTIME_ERROR;
        ERROR_MESSAGE("first_person_camera_controller_move_forward(%s) - Failed to get camera posture.", view_rslt_to_str(ret));
        goto cleanup;
    }
    vec3f_t new_pos = { 0 };
    vec3f_add(&v, &position, &new_pos);

    // カメラ座標更新
    if(VIEW_SUCCESS != camera_posture_update(&euler, &new_pos, camera_)) {
        ret = VIEW_RUNTIME_ERROR;
        ERROR_MESSAGE("first_person_camera_controller_move_forward(%s) - Failed to update camera posture.", view_rslt_to_str(ret));
        goto cleanup;
    }

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t first_person_camera_controller_move_backward(float speed_, float delta_time_, camera_t* camera_) {
    // 後で実装
    return VIEW_SUCCESS;
}

view_result_t first_person_camera_controller_move_right(float speed_, float delta_time_, camera_t* camera_) {
    // 後で実装
    return VIEW_SUCCESS;
}

view_result_t first_person_camera_controller_move_left(float speed_, float delta_time_, camera_t* camera_) {
    // 後で実装
    return VIEW_SUCCESS;
}

view_result_t first_person_camera_controller_rot_pitch(float speed_, float delta_time_, camera_t* camera_) {
    // 後で実装
    return VIEW_SUCCESS;
}

view_result_t first_person_camera_controller_rot_yaw(float speed_, float delta_time_, camera_t* camera_) {
    // 後で実装
    return VIEW_SUCCESS;
}
