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

static view_result_t camera_to_world_matrix_get(const camera_t* camera_, mat4x4f_t* out_mat_);
static view_result_t camera_position_update(const vec3f_t* translation_, camera_t* camera_);

view_result_t first_person_camera_controller_move_forward(float speed_, float delta_time_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "first_person_camera_controller_move_forward", "camera_")

    mat4x4f_t camera_to_world = { 0 };
    ret = camera_to_world_matrix_get(camera_, &camera_to_world);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_forward(%s) - Failed to get camera to world matrix.", view_rslt_to_str(ret));
        goto cleanup;
    }

    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ前方の単位ベクトル[0, 0, -1, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを前方に移動させるための方向ベクトル
    v.elem[0] = -1.0f * camera_to_world.elem[2];
    v.elem[1] = -1.0f * camera_to_world.elem[6];
    v.elem[2] = -1.0f * camera_to_world.elem[10];
    vec3f_normalize(&v);

    // ワールド座標系でのカメラ移動量を計算
    v.elem[0] *= (speed_ * delta_time_);
    v.elem[1] *= (speed_ * delta_time_);
    v.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_update(&v, camera_);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_forward(%s) - Failed to update camera position.", view_rslt_to_str(ret));
        goto cleanup;
    }

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t first_person_camera_controller_move_backward(float speed_, float delta_time_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "first_person_camera_controller_move_backward", "camera_")

    mat4x4f_t camera_to_world = { 0 };
    ret = camera_to_world_matrix_get(camera_, &camera_to_world);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_backward(%s) - Failed to get camera to world matrix.", view_rslt_to_str(ret));
        goto cleanup;
    }

    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ後方の単位ベクトル[0, 0, 1, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを後方に移動させるための方向ベクトル
    v.elem[0] = camera_to_world.elem[2];
    v.elem[1] = camera_to_world.elem[6];
    v.elem[2] = camera_to_world.elem[10];
    vec3f_normalize(&v);

    // ワールド座標系でのカメラ移動量を計算
    v.elem[0] *= (speed_ * delta_time_);
    v.elem[1] *= (speed_ * delta_time_);
    v.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_update(&v, camera_);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_backward(%s) - Failed to update camera position.", view_rslt_to_str(ret));
        goto cleanup;
    }

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t first_person_camera_controller_move_right(float speed_, float delta_time_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "first_person_camera_controller_move_right", "camera_")

    mat4x4f_t camera_to_world = { 0 };
    ret = camera_to_world_matrix_get(camera_, &camera_to_world);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_right(%s) - Failed to get camera to world matrix.", view_rslt_to_str(ret));
        goto cleanup;
    }

    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ右方向の単位ベクトル[1, 0, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを右に移動させるための方向ベクトル
    v.elem[0] = camera_to_world.elem[0];
    v.elem[1] = camera_to_world.elem[4];
    v.elem[2] = camera_to_world.elem[8];
    vec3f_normalize(&v);

    // ワールド座標系でのカメラ移動量を計算
    v.elem[0] *= (speed_ * delta_time_);
    v.elem[1] *= (speed_ * delta_time_);
    v.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_update(&v, camera_);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_right(%s) - Failed to update camera position.", view_rslt_to_str(ret));
        goto cleanup;
    }

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t first_person_camera_controller_move_left(float speed_, float delta_time_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "first_person_camera_controller_move_left", "camera_")

    mat4x4f_t camera_to_world = { 0 };
    ret = camera_to_world_matrix_get(camera_, &camera_to_world);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_left(%s) - Failed to get camera to world matrix.", view_rslt_to_str(ret));
        goto cleanup;
    }

    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ左方向の単位ベクトル[-1, 0, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを左に移動させるための方向ベクトル
    v.elem[0] = -1.0f * camera_to_world.elem[0];
    v.elem[1] = -1.0f * camera_to_world.elem[4];
    v.elem[2] = -1.0f * camera_to_world.elem[8];
    vec3f_normalize(&v);

    // ワールド座標系でのカメラ移動量を計算
    v.elem[0] *= (speed_ * delta_time_);
    v.elem[1] *= (speed_ * delta_time_);
    v.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_update(&v, camera_);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_left(%s) - Failed to update camera position.", view_rslt_to_str(ret));
        goto cleanup;
    }

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t first_person_camera_controller_move_up(float speed_, float delta_time_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "first_person_camera_controller_move_up", "camera_")

    mat4x4f_t camera_to_world = { 0 };
    ret = camera_to_world_matrix_get(camera_, &camera_to_world);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_up(%s) - Failed to get camera to world matrix.", view_rslt_to_str(ret));
        goto cleanup;
    }

    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ上方向の単位ベクトル[0, 1, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを上に移動させるための方向ベクトル
    v.elem[0] = camera_to_world.elem[1];
    v.elem[1] = camera_to_world.elem[5];
    v.elem[2] = camera_to_world.elem[9];
    vec3f_normalize(&v);

    // ワールド座標系でのカメラ移動量を計算
    v.elem[0] *= (speed_ * delta_time_);
    v.elem[1] *= (speed_ * delta_time_);
    v.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_update(&v, camera_);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_up(%s) - Failed to update camera position.", view_rslt_to_str(ret));
        goto cleanup;
    }

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t first_person_camera_controller_move_down(float speed_, float delta_time_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "first_person_camera_controller_move_down", "camera_")

    mat4x4f_t camera_to_world = { 0 };
    ret = camera_to_world_matrix_get(camera_, &camera_to_world);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_down(%s) - Failed to get camera to world matrix.", view_rslt_to_str(ret));
        goto cleanup;
    }

    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ下方向の単位ベクトル[0, -1, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを下に移動させるための方向ベクトル
    v.elem[0] = -1.0f * camera_to_world.elem[1];
    v.elem[1] = -1.0f * camera_to_world.elem[5];
    v.elem[2] = -1.0f * camera_to_world.elem[9];
    vec3f_normalize(&v);

    // ワールド座標系でのカメラ移動量を計算
    v.elem[0] *= (speed_ * delta_time_);
    v.elem[1] *= (speed_ * delta_time_);
    v.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_update(&v, camera_);
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("first_person_camera_controller_move_down(%s) - Failed to update camera position.", view_rslt_to_str(ret));
        goto cleanup;
    }

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

view_result_t first_person_camera_controller_rot_pitch(float speed_, float delta_time_, camera_t* camera_) {
    // 後で実装
    return VIEW_SUCCESS;
}

view_result_t first_person_camera_controller_rot_yaw(float speed_, float delta_time_, camera_t* camera_) {
    // 後で実装
    return VIEW_SUCCESS;
}

/**
 * @brief カメラ座標系からワールド座標系への変換行列を取得する
 *
 * @param camera_ カメラ構造体インスタンスへのポインタ
 * @param out_mat_ 行列格納先
 *
 * @retval VIEW_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_mat_ == NULL
 * @retval VIEW_RUNTIME_ERROR 逆行列計算失敗
 * @retval VIEW_SUCCESS 処理に成功し、正常終了
 */
static view_result_t camera_to_world_matrix_get(const camera_t* camera_, mat4x4f_t* out_mat_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_to_world_matrix_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_to_world_matrix_get", "out_mat_")

    mat4x4f_t view_mat = { 0 };
    ret = camera_view_matrix_get(camera_, &view_mat);   // ワールド座標系からカメラ座標系への変換行列
    if(VIEW_SUCCESS != ret) {
        ERROR_MESSAGE("camera_to_world_matrix_get(%s) - Failed to get view matrix.", view_rslt_to_str(ret));
        goto cleanup;
    }
    if(!mat4f_inverse(&view_mat)) { // カメラ座標系からワールド座標系への変換行列
        ret = VIEW_RUNTIME_ERROR;
        ERROR_MESSAGE("camera_to_world_matrix_get(%s) - Failed to get mat4f inverse.", view_rslt_to_str(ret));
        goto cleanup;
    }
    mat4f_copy(&view_mat, out_mat_);
    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief カメラ位置を更新する
 *
 * @param translation_ カメラ移動量
 * @param camera_ 更新対象カメラ構造体インスタンスへのポインタ
 *
 * @retval VIEW_INVALID_ARGUMENT 以下のいずれか
 * - translation_ == NULL
 * - camera_ == NULL
 * @retval VIEW_RUNTIME_ERROR 以下のいずれか
 * - カメラ姿勢の取得に失敗
 * - カメラ姿勢の更新に失敗
 * @retval VIEW_SUCCESS 処理に成功し、正常終了
 */
static view_result_t camera_position_update(const vec3f_t* translation_, camera_t* camera_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(translation_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_position_update", "translation_")
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, VIEW_INVALID_ARGUMENT, view_rslt_to_str(VIEW_INVALID_ARGUMENT), "camera_position_update", "camera_")

    // 現在のカメラ座標を取得
    vec3f_t euler = { 0 };
    vec3f_t position = { 0 };
    if(VIEW_SUCCESS != camera_posture_get(camera_, &euler, &position)) {
        ret = VIEW_RUNTIME_ERROR;
        ERROR_MESSAGE("camera_position_update(%s) - Failed to get camera posture.", view_rslt_to_str(ret));
        goto cleanup;
    }
    vec3f_t new_pos = { 0 };
    vec3f_add(translation_, &position, &new_pos);

    // カメラ座標更新
    if(VIEW_SUCCESS != camera_posture_update(&euler, &new_pos, camera_)) {
        ret = VIEW_RUNTIME_ERROR;
        ERROR_MESSAGE("camera_position_update(%s) - Failed to update camera posture.", view_rslt_to_str(ret));
        goto cleanup;
    }

    ret = VIEW_SUCCESS;

cleanup:
    return ret;
}
