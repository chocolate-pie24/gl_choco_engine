// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/camera/orbit_camera.h"

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_math/math_types.h"

#include "engine/core/event/mouse_event.h"

#include "engine/memory/cpu_allocators/general_allocator/general_allocator.h"

#include "engine/camera/core/camera_types.h"
#include "engine/camera/core/camera_err_utils.h"
#include "engine/camera/camera.h"

typedef struct camera_command_state {
    bool rotating;

    double rotate_x_offset;
    double rotate_y_offset;

    double zoom_offset;
} command_state_t;

// カメラ位置: P = T + dB
// P: カメラ位置
// T: 注視点（Target）
// d: カメラと注視点の距離
// B: カメラの後方を向く単位ベクトル
// (T = P - dBで求まるため、注視点は保存しない)
struct orbit_camera {
    camera_t* camera;
    float distance;
    command_state_t commands;
};

static camera_result_t orbit_camera_update(orbit_camera_t* camera_, float new_distance_, vec3f_t new_euler_);
static camera_result_t position_from_target_update(camera_t* camera_, vec3f_t target_, float distance_);

static void orbit_rotation_matrix_build(vec3f_t euler_, mat4x4f_t* out_rotation_);

camera_result_t orbit_camera_create(float fovy_, float aspect_, float near_clip_, float far_clip_, vec3f_t initial_target_, float initial_distance_, orbit_camera_t** out_camera_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    camera_t* tmp_camera = NULL;
    orbit_camera_t* tmp_orbit_camera = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_camera_, ret, CAMERA_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_camera_, ret, CAMERA_BAD_OPERATION);

    ret_general_allocator = general_allocator_allocate(sizeof(orbit_camera_t), GENERAL_ALLOCATOR_MEMORY_TAG_CAMERA, (void**)&tmp_orbit_camera);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = camera_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("orbit_camera_create(%s) - general_allocator_allocate failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    ret = camera_create(fovy_, aspect_, near_clip_, far_clip_, orbit_rotation_matrix_build, &tmp_camera);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("orbit_camera_create(%s) - camera_create failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    ret = position_from_target_update(tmp_camera, initial_target_, initial_distance_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("orbit_camera_create(%s) - position_from_target_update failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    tmp_orbit_camera->commands.rotate_x_offset = 0.0;
    tmp_orbit_camera->commands.rotate_y_offset = 0.0;
    tmp_orbit_camera->commands.zoom_offset = 0.0;
    tmp_orbit_camera->commands.rotating = false;

    tmp_orbit_camera->camera = tmp_camera;
    tmp_orbit_camera->distance = initial_distance_;
    tmp_camera = NULL;

    *out_camera_ = tmp_orbit_camera;
    tmp_orbit_camera = NULL;

    ret = CAMERA_SUCCESS;

cleanup:
    if(CAMERA_DATA_CORRUPTED == ret || CAMERA_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_camera) {
        camera_destroy(&tmp_camera);
    }
    if(NULL != tmp_orbit_camera) {
        general_allocator_free((void**)&tmp_orbit_camera, GENERAL_ALLOCATOR_MEMORY_TAG_CAMERA);
    }
    return ret;
}

void orbit_camera_destroy(orbit_camera_t** camera_) {
    if(NULL == camera_ || NULL == *camera_) {
        return;
    }
    camera_destroy(&(*camera_)->camera);
    general_allocator_free((void**)camera_, GENERAL_ALLOCATOR_MEMORY_TAG_CAMERA);
}

camera_result_t orbit_camera_command_update(orbit_camera_t* camera_, const mouse_event_t* event_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(event_, ret, CAMERA_INVALID_ARGUMENT);

    switch(event_->event_code) {
    case MOUSE_EVENT_BUTTON:
        if(MOUSE_BUTTON_LEFT == event_->event_args.button.button) {
            camera_->commands.rotating = event_->event_args.button.pressed;
        }
        break;
    case MOUSE_EVENT_MOVE:
        // フレームを跨いで処理をすることがあるため累積する
        if(camera_->commands.rotating) {
            camera_->commands.rotate_x_offset += event_->event_args.move.x_offset;

            camera_->commands.rotate_y_offset += event_->event_args.move.y_offset;
        }
        break;
    case MOUSE_EVENT_SCROLL:
        // フレームを跨いで処理をすることがあるため累積する
        camera_->commands.zoom_offset += event_->event_args.scroll.y_offset;
        break;
    default:
        break;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t orbit_camera_command_execute(orbit_camera_t* camera_, bool* out_view_dirty_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t current_euler = { 0 };

    // TODO: 自動で計算できるようにする
    const double rot_sensitivity = 0.1;
    const double zoom_sensitivity = 0.1;
    const float min_distance = 0.5f;

    float new_distance = 0.0;
    bool view_dirty = false;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_view_dirty_, ret, CAMERA_INVALID_ARGUMENT);

    new_distance = camera_->distance;

    ret = camera_euler_get(camera_->camera, &current_euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("orbit_camera_command_execute(%s) - camera_euler_get failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    if(!is_equal_double(camera_->commands.rotate_x_offset, 0.0)) {
        current_euler.elem[1] += (float)(camera_->commands.rotate_x_offset * rot_sensitivity);  // Yaw
        view_dirty = true;
    }
    if(!is_equal_double(camera_->commands.rotate_y_offset, 0.0)) {
        current_euler.elem[0] -= (float)(camera_->commands.rotate_y_offset * rot_sensitivity);  // Pitch
        view_dirty = true;
    }
    if(!is_equal_double(camera_->commands.zoom_offset, 0.0)) {
        new_distance -= (float)(camera_->commands.zoom_offset * zoom_sensitivity);
        if(new_distance < min_distance) {
            new_distance = min_distance;
        }
        view_dirty = true;
    }

    if(view_dirty) {
        ret = orbit_camera_update(camera_, new_distance, current_euler);
        if(CAMERA_SUCCESS != ret) {
            ERROR_MESSAGE("orbit_camera_command_execute(%s) - orbit_camera_update failed.", camera_result_to_str(ret));
            goto cleanup;
        }
    }


    camera_->commands.rotate_x_offset = 0.0;
    camera_->commands.rotate_y_offset = 0.0;
    camera_->commands.zoom_offset = 0.0;

    *out_view_dirty_ = view_dirty;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t orbit_camera_viewing_frustum_update(orbit_camera_t* camera_, float fovy_, float aspect_, float near_clip_, float far_clip_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT);

    ret = camera_viewing_frustum_update(camera_->camera, fovy_, aspect_, near_clip_, far_clip_);

cleanup:
    return ret;
}

camera_result_t orbit_camera_perspective_matrix_get(orbit_camera_t* camera_, mat4x4f_t* out_mat_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, CAMERA_INVALID_ARGUMENT);

    ret = camera_perspective_matrix_get(camera_->camera, out_mat_);

cleanup:
    return ret;
}

camera_result_t orbit_camera_view_matrix_get(orbit_camera_t* camera_, mat4x4f_t* out_mat_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, CAMERA_INVALID_ARGUMENT);

    ret = camera_view_matrix_get(camera_->camera, out_mat_);

cleanup:
    return ret;
}

bool orbit_camera_is_valid(const orbit_camera_t* camera_) {
    if(NULL == camera_) {
        return false;
    }
    return true;
}

static camera_result_t orbit_camera_update(orbit_camera_t* camera_, float new_distance_, vec3f_t new_euler_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t current_camera_pos = { 0 };
    vec3f_t backward_vec = { 0 };
    vec3f_t target = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT);

    ret = camera_position_get(camera_->camera, &current_camera_pos);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("orbit_camera_update(%s) - camera_position_get failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    ret = camera_backward_vector_get(camera_->camera, &backward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("orbit_camera_update(%s) - camera_backward_vector_get failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    target.elem[0] = current_camera_pos.elem[0] - camera_->distance * backward_vec.elem[0];
    target.elem[1] = current_camera_pos.elem[1] - camera_->distance * backward_vec.elem[1];
    target.elem[2] = current_camera_pos.elem[2] - camera_->distance * backward_vec.elem[2];

    ret = camera_euler_update(camera_->camera, new_euler_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("orbit_camera_update(%s) - camera_euler_update failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    ret = position_from_target_update(camera_->camera, target, new_distance_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("orbit_camera_update(%s) - position_from_target_update failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    camera_->distance = new_distance_;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static camera_result_t position_from_target_update(camera_t* camera_, vec3f_t target_, float distance_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    vec3f_t new_camera_pos = { 0 };
    vec3f_t backward_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT);

    ret = camera_backward_vector_get(camera_, &backward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("position_from_target_update(%s) - camera_backward_vector_get failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    new_camera_pos = vec3f_add(target_, vec3f_scale(backward_vec, distance_));

    ret = camera_position_update(camera_, new_camera_pos);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("position_from_target_update(%s) - camera_position_update failed.", camera_result_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

static void orbit_rotation_matrix_build(vec3f_t euler_, mat4x4f_t* out_rotation_) {
    if(NULL == out_rotation_) {
        return;
    }
    mat4x4f_t rot_y;    // yaw
    mat4x4f_t rot_x;    // pitch

    mat4f_rot_y(CHOCO_DEG_TO_RAD(euler_.elem[1]), &rot_y);
    mat4f_rot_x(CHOCO_DEG_TO_RAD(euler_.elem[0]), &rot_x);
    mat4f_mul(&rot_y, &rot_x, out_rotation_);
}
