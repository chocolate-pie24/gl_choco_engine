// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/camera/flight_camera.h"

#include <stdbool.h>
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_math/math_types.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/camera/core/camera_types.h"
#include "engine/camera/core/camera_err_utils.h"
#include "engine/camera/camera.h"

struct flight_camera {
    camera_t* camera;
    bool command_status[FLIGHT_CAMERA_COMMAND_MAX];
    flight_camera_key_bind_t keybinds[FLIGHT_CAMERA_COMMAND_MAX];
};

static camera_result_t move_forward(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t move_backward(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t move_right(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t move_left(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t move_up(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t move_down(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t rot_pitch_plus(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t rot_pitch_minus(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t rot_yaw_plus(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t rot_yaw_minus(camera_t* camera_, float speed_, float delta_time_);
static camera_result_t camera_position_movement_apply(camera_t* camera_, vec3f_t translation_);

static bool flight_camera_key_bind_is_valid(flight_camera_key_bind_t key_binds_);
static bool is_valid_shallow(const flight_camera_t* flight_camera_);
static void destroy_unchecked(flight_camera_t** flight_camera_);

camera_result_t flight_camera_create(const flight_camera_key_bind_t keybinds_[FLIGHT_CAMERA_COMMAND_MAX], float fovy_, float aspect_, float near_clip_, float far_clip_, flight_camera_t** out_flight_camera_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    camera_t* tmp_camera = NULL;
    flight_camera_t* tmp_flight_camera = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(keybinds_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_create", "keybinds_")
    IF_ARG_NULL_GOTO_CLEANUP(out_flight_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_create", "out_flight_camera_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_flight_camera_, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "flight_camera_create", "*out_flight_camera_")
    for(size_t i = 0; i != FLIGHT_CAMERA_COMMAND_MAX; ++i) {
        if(!flight_camera_key_bind_is_valid(keybinds_[i])) {
            ret = CAMERA_INVALID_ARGUMENT;
            ERROR_MESSAGE("flight_camera_create(%s) - Provided keybinds_[%zu] is not valid.", camera_rslt_to_str(ret), i);
            goto cleanup;
        }
    }
    for(size_t i = 0; i != (FLIGHT_CAMERA_COMMAND_MAX - 1); ++i) {
        for(size_t j = (i + 1); j != FLIGHT_CAMERA_COMMAND_MAX; ++j) {
            if(keybinds_[i].key == keybinds_[j].key) {
                ret = CAMERA_INVALID_ARGUMENT;
                ERROR_MESSAGE("flight_camera_create(%s) - Duplicate key bind.", camera_rslt_to_str(ret));
                goto cleanup;
            }
        }
    }

    ret_memory_system = memory_system_allocate(sizeof(flight_camera_t), MEMORY_TAG_CAMERA, (void**)&tmp_flight_camera);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = camera_rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("flight_camera_create(%s) - memory_system_allocate failed.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_flight_camera, 0, sizeof(flight_camera_t));

    ret = camera_create(fovy_, aspect_, near_clip_, far_clip_, &tmp_camera);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_create(%s) - camera_create failed.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != FLIGHT_CAMERA_COMMAND_MAX; ++i) {
        tmp_flight_camera->keybinds[i] = keybinds_[i];
        tmp_flight_camera->command_status[i] = false;
    }
    tmp_flight_camera->camera = tmp_camera;
    tmp_camera = NULL;

    *out_flight_camera_ = tmp_flight_camera;
    tmp_flight_camera = NULL;

    ret = CAMERA_SUCCESS;

cleanup:
    if(NULL != tmp_camera) {
        camera_destroy(&tmp_camera);
    }
    if(NULL != tmp_flight_camera) {
        destroy_unchecked(&tmp_flight_camera);
    }
    return ret;
}

void flight_camera_destroy(flight_camera_t** out_flight_camera_) {
    if(NULL == out_flight_camera_) {
        return;
    }
    if(NULL == *out_flight_camera_) {
        return;
    }

    destroy_unchecked(out_flight_camera_);
}

camera_result_t flight_camera_command_update(flight_camera_t* flight_camera_, const keyboard_event_t* keyboard_event_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(flight_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_command_update", "flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_command_update", "keyboard_event_")

    for(size_t i = 0; i != FLIGHT_CAMERA_COMMAND_MAX; ++i) {
        if(flight_camera_->keybinds[i].key == keyboard_event_->key) {
            if(keyboard_event_->event_args.pressed) {   // キーが押されたらコマンドON
                flight_camera_->command_status[i] = true;
            } else if(flight_camera_->command_status[i] && !keyboard_event_->event_args.pressed) {  // キーが離されたらコマンドOFF
                flight_camera_->command_status[i] = false;
            }
            break;
        }
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_command_execute(flight_camera_t* flight_camera_, float speed_, float delta_time_, bool* out_view_dirty_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    bool has_command_requested = false;

    IF_ARG_NULL_GOTO_CLEANUP(flight_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_command_execute", "flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_view_dirty_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_command_execute", "out_view_dirty_")

    for(size_t i = 0; i != FLIGHT_CAMERA_COMMAND_MAX; ++i) {
        if(flight_camera_->command_status[i]) {
            has_command_requested = true;
            switch(i) {
            case FLIGHT_CAMERA_COMMAND_MOVE_FORWARD:
                ret = move_forward(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD:
                ret = move_backward(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_MOVE_RIGHT:
                ret = move_right(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_MOVE_LEFT:
                ret = move_left(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_MOVE_UP:
                ret = move_up(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_MOVE_DOWN:
                ret = move_down(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS:
                ret = rot_pitch_plus(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS:
                ret = rot_pitch_minus(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS:
                ret = rot_yaw_plus(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS:
                ret = rot_yaw_minus(flight_camera_->camera, speed_, delta_time_);
                break;
            case FLIGHT_CAMERA_COMMAND_MAX:
                ret = CAMERA_INVALID_ARGUMENT;
                break;
            default:
                ret = CAMERA_INVALID_ARGUMENT;
                break;
            }
            if(CAMERA_SUCCESS != ret) {
                ERROR_MESSAGE("flight_camera_command_execute(%s) - command failed.", camera_rslt_to_str(ret));
                goto cleanup;
            }
        }
    }

    if(has_command_requested && CAMERA_SUCCESS == ret) {
        *out_view_dirty_ = true;
    } else if(!has_command_requested) {
        *out_view_dirty_ = false;
        ret = CAMERA_SUCCESS;
    }

cleanup:
    return ret;
}

camera_result_t flight_camera_viewing_frustum_update(flight_camera_t* flight_camera_, float fovy_, float aspect_, float near_clip_, float far_clip_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(flight_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_viewing_frustum_update", "flight_camera_")

    ret = camera_viewing_frustum_update(flight_camera_->camera, fovy_, aspect_, near_clip_, far_clip_);

cleanup:
    return ret;
}

camera_result_t flight_camera_perspective_matrix_get(flight_camera_t* flight_camera_, mat4x4f_t* out_mat_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(flight_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_perspective_matrix_get", "flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_perspective_matrix_get", "out_mat_")

    ret = camera_perspective_matrix_get(flight_camera_->camera, out_mat_);

cleanup:
    return ret;
}

camera_result_t flight_camera_view_matrix_get(flight_camera_t* flight_camera_, mat4x4f_t* out_mat_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(flight_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_view_matrix_get", "flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_view_matrix_get", "out_mat_")

    ret = camera_view_matrix_get(flight_camera_->camera, out_mat_);

cleanup:
    return ret;
}

bool flight_camera_is_valid(const flight_camera_t* flight_camera_) {
    if(NULL == flight_camera_) {
        return false;
    }
    if(!is_valid_shallow(flight_camera_)) {
        return false;
    }
    for(size_t i = 0; i != (FLIGHT_CAMERA_COMMAND_MAX - 1); ++i) {
        for(size_t j = (i + 1); j != FLIGHT_CAMERA_COMMAND_MAX; ++j) {
            if(flight_camera_->keybinds[i].key == flight_camera_->keybinds[j].key) {
                return false;
            }
        }
    }

    return true;
}

static camera_result_t move_forward(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t forward_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "move_forward", "camera_")

    // カメラ前方の正規化されたベクトルを取得
    ret = camera_forward_vector_get(camera_, &forward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_forward(%s) - Failed to get forward vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    forward_vec = vec3f_scale(forward_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, forward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_forward(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t move_backward(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t backward_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "move_backward", "camera_")

    // カメラ後方の正規化されたベクトルを取得
    ret = camera_backward_vector_get(camera_, &backward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_backward(%s) - Failed to get backward vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    backward_vec = vec3f_scale(backward_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, backward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_backward(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t move_right(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t right_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "move_right", "camera_")

    // カメラ右方向の正規化されたベクトルを取得
    ret = camera_right_vector_get(camera_, &right_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_right(%s) - Failed to get right vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    right_vec = vec3f_scale(right_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, right_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_right(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t move_left(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t left_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "move_left", "camera_")

    // カメラ左方向の正規化されたベクトルを取得
    ret = camera_left_vector_get(camera_, &left_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_left(%s) - Failed to get left vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    left_vec = vec3f_scale(left_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, left_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_left(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t move_up(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t up_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "move_up", "camera_")

    // カメラ上方向の正規化されたベクトルを取得
    ret = camera_up_vector_get(camera_, &up_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_up(%s) - Failed to get up vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    up_vec = vec3f_scale(up_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, up_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_up(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t move_down(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t down_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "move_down", "camera_")

    // カメラ下方向の正規化されたベクトルを取得
    ret = camera_down_vector_get(camera_, &down_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_down(%s) - Failed to get down vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    down_vec = vec3f_scale(down_vec, speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(camera_, down_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("move_down(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t rot_pitch_plus(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "rot_pitch_plus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("rot_pitch_plus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[0] += (speed_ * delta_time_);

    ret = camera_euler_update(camera_, euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("rot_pitch_plus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t rot_pitch_minus(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "rot_pitch_minus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("rot_pitch_minus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[0] -= (speed_ * delta_time_);

    ret = camera_euler_update(camera_, euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("rot_pitch_minus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t rot_yaw_plus(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "rot_yaw_plus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("rot_yaw_plus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[1] += (speed_ * delta_time_);

    ret = camera_euler_update(camera_, euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("rot_yaw_plus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t rot_yaw_minus(camera_t* camera_, float speed_, float delta_time_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "rot_yaw_minus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("rot_yaw_minus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[1] -= (speed_ * delta_time_);

    ret = camera_euler_update(camera_, euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("rot_yaw_minus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t camera_position_movement_apply(camera_t* camera_, vec3f_t translation_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t position = { 0 };
    vec3f_t new_pos = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_position_movement_apply", "camera_")

    // 現在のカメラ座標を取得
    ret = camera_position_get(camera_, &position);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_position_movement_apply(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // 新しいカメラ座標を計算
    new_pos = vec3f_add(translation_, position);

    // カメラ座標更新
    ret = camera_position_update(camera_, new_pos);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_position_movement_apply(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static bool flight_camera_key_bind_is_valid(flight_camera_key_bind_t key_binds_) {
    if((unsigned int)key_binds_.key >= (unsigned int)KEY_CODE_MAX) {
        return false;
    }
    return true;
}

static bool is_valid_shallow(const flight_camera_t* flight_camera_) {
    if(NULL == flight_camera_) {
        return false;
    }
    // 個数が多くはないのでshallowでもループでチェックする
    for(size_t i = 0; i != FLIGHT_CAMERA_COMMAND_MAX; ++i) {
        if(!flight_camera_key_bind_is_valid(flight_camera_->keybinds[i])) {
            return false;
        }
    }
    if(!camera_is_valid(flight_camera_->camera)) {
        return false;
    }
    return true;
}

static void destroy_unchecked(flight_camera_t** flight_camera_) {
    if(NULL == flight_camera_) {
        return;
    }
    if(NULL == *flight_camera_) {
        return;
    }
    camera_destroy(&(*flight_camera_)->camera);
    memory_system_free(*flight_camera_, sizeof(flight_camera_t), MEMORY_TAG_CAMERA);
    *flight_camera_ = NULL;
}
