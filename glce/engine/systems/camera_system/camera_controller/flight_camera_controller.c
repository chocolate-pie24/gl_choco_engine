/**
 * @ingroup camera_system
 * @file flight_camera_controller.c
 * @author chocolate-pie24
 * @brief 空間内を自由に移動する飛行型カメラの制御機能の実装
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/systems/camera_system/camera_controller/flight_camera_controller.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/systems/camera_system/camera/camera.h"
#include "engine/systems/camera_system/camera_core/camera_err_utils.h"
#include "engine/systems/camera_system/camera_core/camera_types.h"

static camera_result_t camera_position_movement_apply(camera_t* camera_, vec3f_t translation_);

camera_result_t flight_camera_controller_move_forward(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t forward_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_forward", "camera_")

    // カメラ前方の正規化されたベクトルを取得
    ret = camera_forward_vector_get(camera_, &forward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_forward(%s) - Failed to get forward vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    forward_vec = vec3f_scale(forward_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, forward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_forward(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_backward(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t backward_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_backward", "camera_")

    // カメラ後方の正規化されたベクトルを取得
    ret = camera_backward_vector_get(camera_, &backward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_backward(%s) - Failed to get backward vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    backward_vec = vec3f_scale(backward_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, backward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_backward(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_right(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t right_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_right", "camera_")

    // カメラ右方向の正規化されたベクトルを取得
    ret = camera_right_vector_get(camera_, &right_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_right(%s) - Failed to get right vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    right_vec = vec3f_scale(right_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, right_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_right(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_left(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t left_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_left", "camera_")

    // カメラ左方向の正規化されたベクトルを取得
    ret = camera_left_vector_get(camera_, &left_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_left(%s) - Failed to get left vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    left_vec = vec3f_scale(left_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, left_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_left(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_up(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t up_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_up", "camera_")

    // カメラ上方向の正規化されたベクトルを取得
    ret = camera_up_vector_get(camera_, &up_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_up(%s) - Failed to get up vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    up_vec = vec3f_scale(up_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, up_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_up(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_down(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t down_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_down", "camera_")

    // カメラ下方向の正規化されたベクトルを取得
    ret = camera_down_vector_get(camera_, &down_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_down(%s) - Failed to get down vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    down_vec = vec3f_scale(down_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, down_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_down(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_rot_pitch_plus(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_rot_pitch_plus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_pitch_plus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[0] += (speed_ * delta_time_);

    ret = camera_euler_update(camera_, euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_pitch_plus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_rot_pitch_minus(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_rot_pitch_minus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_pitch_minus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[0] -= (speed_ * delta_time_);

    ret = camera_euler_update(camera_, euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_pitch_minus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_rot_yaw_plus(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_rot_yaw_plus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_yaw_plus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[1] += (speed_ * delta_time_);

    ret = camera_euler_update(camera_, euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_yaw_plus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_rot_yaw_minus(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_rot_yaw_minus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_yaw_minus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[1] -= (speed_ * delta_time_);

    ret = camera_euler_update(camera_, euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_yaw_minus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief カメラ位置にカメラ移動量を適用する
 *
 * @param[in,out] camera_ 更新対象カメラ構造体インスタンスへのポインタ
 * @param[in] translation_ カメラ移動量
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 以下のいずれか
 * - カメラ位置の取得に失敗
 * - カメラ位置の更新に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
static camera_result_t camera_position_movement_apply(camera_t* camera_, vec3f_t translation_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t position = { 0 };
    vec3f_t new_pos = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_position_movement_apply", "camera_")

    // 現在のカメラ座標を取得
    if(CAMERA_SUCCESS != camera_position_get(camera_, &position)) {
        ret = CAMERA_RUNTIME_ERROR;
        ERROR_MESSAGE("camera_position_movement_apply(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // 新しいカメラ座標を計算
    new_pos = vec3f_add(translation_, position);

    // カメラ座標更新
    if(CAMERA_SUCCESS != camera_position_update(camera_, new_pos)) {
        ret = CAMERA_RUNTIME_ERROR;
        ERROR_MESSAGE("camera_position_movement_apply(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}
