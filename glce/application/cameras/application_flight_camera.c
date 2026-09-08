// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/cameras/application_flight_camera.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdalign.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/linear_allocator.h"
#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/window_event.h"

#include "engine/camera/core/camera_types.h"
#include "engine/camera/flight_camera.h"

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"
#include "engine/systems/camera/camera_registries/flight_camera_registry.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

struct application_flight_camera {
    flight_camera_registry_t* flight_camera_registry;

    flight_camera_t* active_camera;
    uint16_t active_camera_id;   // id = 0はデフォルトカメラでデフォルトキーバインドのflight cmaera
};

static flight_camera_key_bind_t s_default_keybinds[FLIGHT_CAMERA_COMMAND_MAX];
static const float s_default_fovy = 45.0f;
static const float s_default_near_clip = 0.1f;
static const float s_default_far_clip = 50.0f;

static bool is_valid_shallow(const application_flight_camera_t* application_flight_camera_);

// id = 0はデフォルトカメラでデフォルトキーバインドのflight cmaeraが生成され(*out_application_camera_)->active_cameraにアドレスが格納される
application_result_t application_flight_camera_initialize(size_t max_flight_camera_count_, linear_alloc_t* allocator_, int framebuffer_width_, int framebuffer_height_, application_flight_camera_t** out_application_flight_camera_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    camera_registry_result_t ret_camera_registry = CAMERA_REGISTRY_INVALID_ARGUMENT;
    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    application_flight_camera_t* tmp_application_flight_camera = NULL;
    flight_camera_registry_t* tmp_flight_camera_registry = NULL;
    flight_camera_t* tmp_flight_camera = NULL;

    uint16_t tmp_active_camera_id = 0;
    float aspect = 0.0f;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_application_flight_camera_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_initialize", "out_application_flight_camera_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_application_flight_camera_, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "application_flight_camera_initialize", "*out_application_flight_camera_")
    if(0 == max_flight_camera_count_) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_flight_camera_initialize(%s) - Provided max_flight_camera_count_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 >= framebuffer_width_ || 0 >= framebuffer_height_) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_flight_camera_initialize(%s) - Provided framebuffer_width_ or framebuffer_height_ is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(application_flight_camera_t), alignof(application_flight_camera_t), (void**)&tmp_application_flight_camera);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = app_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("application_flight_camera_initialize(%s) - Failed to allocate application_flight_camera_t instance.", app_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_application_flight_camera, 0, sizeof(application_flight_camera_t));

    ret_camera_registry = flight_camera_registry_initialize(max_flight_camera_count_, allocator_, &tmp_flight_camera_registry);
    if(CAMERA_REGISTRY_SUCCESS != ret_camera_registry) {
        ret = app_rslt_convert_camera_registry(ret_camera_registry);
        ERROR_MESSAGE("application_flight_camera_initialize(%s) - flight_camera_registry_initialize failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    s_default_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].key = KEY_W;         // カメラ前進コマンド(キーバインド: KEY_W)
    s_default_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].key = KEY_S;        // カメラ後進コマンド(キーバインド: KEY_S)
    s_default_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].key = KEY_D;           // カメラ右移動コマンド(キーバインド: KEY_D)
    s_default_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].key = KEY_A;            // カメラ左移動コマンド(キーバインド: KEY_A)
    s_default_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_UP].key = KEY_E;              // カメラ上方向移動コマンド(キーバインド: KEY_E)
    s_default_keybinds[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].key = KEY_Q;            // カメラ下方向移動コマンド(キーバインド: KEY_Q)
    s_default_keybinds[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].key = KEY_UP;      // カメラピッチ方向(+)回転コマンド(キーバインド: KEY_UP)
    s_default_keybinds[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].key = KEY_DOWN;   // カメラピッチ方向(-)回転コマンド(キーバインド: KEY_DOWN)
    s_default_keybinds[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].key = KEY_LEFT;      // カメラヨー方向(+)回転コマンド(キーバインド: KEY_LEFT)
    s_default_keybinds[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].key = KEY_RIGHT;    // カメラヨー方向(-)回転コマンド(キーバインド: KEY_RIGHT)

    aspect = (float)framebuffer_width_ / (float)framebuffer_height_;
    ret_camera = flight_camera_create(s_default_keybinds, s_default_fovy, aspect, s_default_near_clip, s_default_far_clip, &tmp_flight_camera);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_flight_camera_initialize(%s) - flight_camera_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // 成功するとtmp_flight_camera == NULLになる
    ret_camera_registry = flight_camera_registry_register(tmp_flight_camera_registry, "default_flight_camera", &tmp_flight_camera, &tmp_active_camera_id);
    if(CAMERA_REGISTRY_SUCCESS != ret_camera_registry) {
        ret = app_rslt_convert_camera_registry(ret_camera_registry);
        ERROR_MESSAGE("application_flight_camera_initialize(%s) - flight_camera_registry_register failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_application_flight_camera->active_camera = flight_camera_registry_flight_camera_get(tmp_flight_camera_registry, tmp_active_camera_id);
    if(NULL == tmp_application_flight_camera->active_camera) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_flight_camera_initialize(%s) - flight_camera_registry_flight_camera_get failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_application_flight_camera->active_camera_id = tmp_active_camera_id;
    tmp_application_flight_camera->flight_camera_registry = tmp_flight_camera_registry;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_flight_camera_is_valid(tmp_application_flight_camera)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_flight_camera_initialize(%s) - Postcondition validation failed for 'tmp_application_flight_camera'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_application_flight_camera_ = tmp_application_flight_camera;

    tmp_flight_camera_registry = NULL;
    tmp_flight_camera = NULL;
    tmp_application_flight_camera = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED != ret) {
        if(NULL != tmp_flight_camera_registry) {
            flight_camera_registry_deinitialize(tmp_flight_camera_registry);
        }
        if(NULL != tmp_flight_camera) {
            flight_camera_destroy(&tmp_flight_camera);
        }
    }

    return ret;
}

void application_flight_camera_deinitialize(application_flight_camera_t* application_flight_camera_) {
    if(NULL == application_flight_camera_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_flight_camera_is_valid(application_flight_camera_)) {
        ERROR_MESSAGE("application_flight_camera_deinitialize(%s) - Precondition validation failed for 'application_flight_camera_'.", app_rslt_to_str(APPLICATION_DATA_CORRUPTED));
        return;
    }
#endif

    flight_camera_registry_deinitialize(application_flight_camera_->flight_camera_registry);
    application_flight_camera_->active_camera = NULL;
    application_flight_camera_->active_camera_id = 0;
}

application_result_t application_flight_camera_update(application_flight_camera_t* application_flight_camera_, float speed_, float delta_time_, const application_event_view_t* application_event_view_, bool* out_view_dirty_, bool* out_projection_dirty_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    bool window_resized = false;
    bool projection_changed = false;
    bool view_changed = false;
    int framebuffer_width = 0;
    int framebuffer_height = 0;
    float aspect = 0.0f;

    IF_ARG_NULL_GOTO_CLEANUP(application_flight_camera_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_update", "application_flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(application_event_view_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_update", "application_event_view_")
    IF_ARG_NULL_GOTO_CLEANUP(out_view_dirty_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_update", "out_view_dirty_")
    IF_ARG_NULL_GOTO_CLEANUP(out_projection_dirty_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_update", "out_projection_dirty_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_flight_camera_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_flight_camera_update(%s) - Precondition validation failed for 'application_flight_camera_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    for(size_t i = 0; i != application_event_view_->window_event_count; ++i) {
        if(WINDOW_EVENT_RESIZE == application_event_view_->window_events[i].event_code) {
            framebuffer_width = application_event_view_->window_events[i].event_args.framebuffer_width;
            framebuffer_height = application_event_view_->window_events[i].event_args.framebuffer_height;
            window_resized = true;
        }
    }

    for(size_t i = 0; i != application_event_view_->keyboard_event_count; ++i) {
        ret_camera = flight_camera_command_update(application_flight_camera_->active_camera, &application_event_view_->keyboard_events[i]);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = app_rslt_convert_camera(ret_camera);
            ERROR_MESSAGE("application_flight_camera_update(%s) - flight_camera_command_update failed.", app_rslt_to_str(ret));
            goto cleanup;
        }
    }
    ret_camera = flight_camera_command_execute(application_flight_camera_->active_camera, speed_, delta_time_, &view_changed);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_flight_camera_update(%s) - flight_camera_command_execute failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    if(window_resized && 0 < framebuffer_height && 0 < framebuffer_width) { // window最小化等でframebuffer_heightが0の場合は視錐台の更新は行わない
        aspect = (float)framebuffer_width / (float)framebuffer_height;
        ret_camera = flight_camera_viewing_frustum_update(application_flight_camera_->active_camera, s_default_fovy, aspect, s_default_near_clip, s_default_far_clip);
        if(CAMERA_SUCCESS != ret_camera) {
            ret = app_rslt_convert_camera(ret_camera);
            ERROR_MESSAGE("application_flight_camera_update(%s) - flight_camera_viewing_frustum_update failed.", app_rslt_to_str(ret));
            goto cleanup;
        }
        projection_changed = true;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_flight_camera_is_valid(application_flight_camera_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_flight_camera_update(%s) - Postcondition validation failed for 'application_flight_camera_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_projection_dirty_ = projection_changed;
    *out_view_dirty_ = view_changed;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_flight_camera_view_matrix_get(application_flight_camera_t* application_flight_camera_, mat4x4f_t* out_matrix_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_flight_camera_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_view_matrix_get", "application_flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_view_matrix_get", "out_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_flight_camera_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_flight_camera_view_matrix_get(%s) - Precondition validation failed for 'application_flight_camera_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_camera = flight_camera_view_matrix_get(application_flight_camera_->active_camera, out_matrix_);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_flight_camera_view_matrix_get(%s) - flight_camera_view_matrix_get failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t application_flight_camera_perspective_matrix_get(application_flight_camera_t* application_flight_camera_, mat4x4f_t* out_matrix_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(application_flight_camera_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_perspective_matrix_get", "application_flight_camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_matrix_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_flight_camera_perspective_matrix_get", "out_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(application_flight_camera_)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_flight_camera_perspective_matrix_get(%s) - Precondition validation failed for 'application_flight_camera_'.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_camera = flight_camera_perspective_matrix_get(application_flight_camera_->active_camera, out_matrix_);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_flight_camera_perspective_matrix_get(%s) - flight_camera_perspective_matrix_get failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

bool application_flight_camera_is_valid(const application_flight_camera_t* application_flight_camera_) {
    flight_camera_t* tmp_flight_camera = NULL;

    if(NULL == application_flight_camera_) {
        return false;
    }
    if(!is_valid_shallow(application_flight_camera_)) {
        return false;
    }
    if(!flight_camera_registry_is_valid(application_flight_camera_->flight_camera_registry)) {
        return false;
    }

    // active_camera自身のvalidationはregistry_is_validと以下のactive_cameraのアドレスチェックでチェック可能なので行わない
    tmp_flight_camera = flight_camera_registry_flight_camera_get(application_flight_camera_->flight_camera_registry, application_flight_camera_->active_camera_id);
    if(application_flight_camera_->active_camera != tmp_flight_camera) {
        return false;
    }
    return true;
}

static bool is_valid_shallow(const application_flight_camera_t* application_flight_camera_) {
    if(NULL == application_flight_camera_) {
        return false;
    }
    if(NULL == application_flight_camera_->flight_camera_registry) {
        return false;
    }
    if(NULL == application_flight_camera_->active_camera) {
        return false;
    }
    return true;
}
