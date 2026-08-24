// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup camera_system
 *
 * @file camera_manager.c
 * @author chocolate-pie24
 * @brief カメラ管理システムで、カメラ構造体インスタンスの追加 / 削除 / 取得APIの実装
 *
 * @date 2026-03-25
 *
 */
#include "engine/systems/camera_system/camera_manager/camera_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h> // for memset
#include <stdalign.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/camera_system/camera_core/camera_types.h"
#include "engine/systems/camera_system/camera_core/camera_err_utils.h"

#include "engine/systems/camera_system/camera/camera.h"

/**
 * @brief カメラ管理システム内部状態管理構造体
 *
 */
struct camera_manager {
    int16_t max_camera_count;   /**< 管理システムに登録可能なカメラ数上限値 */
    camera_t** camera_array;    /**< カメラ構造体インスタンス格納配列 */
};

camera_result_t camera_manager_initialize(int16_t max_camera_count_, linear_alloc_t* allocator_, camera_manager_t** out_camera_manager_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    camera_manager_t* tmp_manager = NULL;
    camera_t** tmp_camera_array = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_initialize", "out_camera_manager_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_initialize", "*out_camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < max_camera_count_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_initialize", "max_camera_count_")

    // Simulation.
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(camera_manager_t), alignof(camera_manager_t), (void**)&tmp_manager);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = camera_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("camera_manager_initialize(%s) - Failed to allocate memory for camera manager.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_manager, 0, sizeof(camera_manager_t));
    tmp_manager->max_camera_count = max_camera_count_;

    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(camera_t*) * (size_t)(max_camera_count_), alignof(camera_t*), (void**)&tmp_camera_array);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = camera_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("camera_manager_initialize(%s) - Failed to allocate memory for camera_array.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_manager->camera_array = tmp_camera_array;
    for(int16_t i = 0; i != max_camera_count_; ++i) {
        tmp_manager->camera_array[i] = NULL;
    }

    // commit.
    *out_camera_manager_ = tmp_manager;

    ret = CAMERA_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void camera_manager_deinitialize(camera_manager_t* camera_manager_) {
    if(NULL == camera_manager_) {
        return;
    }
    if(0 >= camera_manager_->max_camera_count) {
        return;
    }
    for(int16_t i = 0; i != camera_manager_->max_camera_count; ++i) {
        camera_destroy(&camera_manager_->camera_array[i]);
    }
    camera_manager_->max_camera_count = 0;
}

camera_result_t camera_manager_register(camera_manager_t* camera_manager_, const char* camera_name_, int16_t* out_camera_id_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    int16_t free_slot = INVALID_CAMERA_ID;
    camera_t* tmp_camera = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(camera_name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_register", "camera_name_")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_register", "camera_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_id_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_register", "out_camera_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_register", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_register", "camera_manager_->camera_array")

    for(int16_t i = 0; i != camera_manager_->max_camera_count; ++i) {
        if(NULL == camera_manager_->camera_array[i]) {
            if(INVALID_CAMERA_ID == free_slot) {
                free_slot = i;
            }
        } else {
            const char* tmp_camera_name = camera_name_get(camera_manager_->camera_array[i]);
            if(NULL == tmp_camera_name) {
                ret = CAMERA_DATA_CORRUPTED;
                ERROR_MESSAGE("camera_manager_register(%s) - Camera manager data corrupted.", camera_rslt_to_str(ret));
                goto cleanup;
            } else if(choco_string_equal(tmp_camera_name, camera_name_)) {
                ret = CAMERA_BAD_OPERATION;
                ERROR_MESSAGE("camera_manager_register(%s) - Provided camera name '%s' is already registered.", camera_rslt_to_str(ret), camera_name_);
                goto cleanup;
            }
        }
    }
    if(INVALID_CAMERA_ID == free_slot) {
        ret = CAMERA_LIMIT_EXCEEDED;
        ERROR_MESSAGE("camera_manager_register(%s) - Camera manager has no free slot.", camera_rslt_to_str(ret));
        goto cleanup;
    } else {
        ret = camera_create(camera_name_, &tmp_camera);
        if(CAMERA_SUCCESS != ret) {
            ERROR_MESSAGE("camera_manager_register(%s) - Failed to create camera(%s).", camera_rslt_to_str(ret), camera_name_);
            goto cleanup;
        }
        camera_manager_->camera_array[free_slot] = tmp_camera;
        *out_camera_id_ = free_slot;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_manager_unregister(camera_manager_t* camera_manager_, int16_t camera_id_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_unregister", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_unregister", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_unregister", "camera_manager_->camera_array")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_id_ >= 0, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_unregister", "camera_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > camera_id_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_unregister", "camera_id")

    if(NULL == camera_manager_->camera_array[camera_id_]) {
        ret = CAMERA_BAD_OPERATION;
        ERROR_MESSAGE("camera_manager_unregister(%s) - Provided camera id '%d' is not registered.", camera_rslt_to_str(ret), camera_id_);
        goto cleanup;
    }
    camera_destroy(&camera_manager_->camera_array[camera_id_]);

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_manager_unregister_by_name(camera_manager_t* camera_manager_, const char* name_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    int16_t tmp_id = INVALID_CAMERA_ID;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_unregister_by_name", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_unregister_by_name", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_unregister_by_name", "camera_manager_->camera_array")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_unregister_by_name", "name_")

    ret = camera_manager_camera_id_get(camera_manager_, name_, &tmp_id);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_manager_unregister_by_name(%s) - Failed to get camera id. Provided camera name = '%s'.", camera_rslt_to_str(ret), name_);
        goto cleanup;
    }
    camera_destroy(&camera_manager_->camera_array[tmp_id]);

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_manager_camera_id_get(const camera_manager_t* camera_manager_, const char* name_, int16_t* out_camera_id_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    int16_t tmp_id = INVALID_CAMERA_ID;
    bool found = false;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_id_get", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_id_get", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_id_get", "camera_manager_->camera_array")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_id_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_id_get", "out_camera_id_")

    for(int16_t i = 0; i != camera_manager_->max_camera_count; ++i) {
        if(NULL != camera_manager_->camera_array[i]) {
            const char* tmp_name = camera_name_get(camera_manager_->camera_array[i]);
            if(NULL == tmp_name) {
                ret = CAMERA_DATA_CORRUPTED;
                ERROR_MESSAGE("camera_manager_camera_id_get(%s) - Camera manager data corrupted.", camera_rslt_to_str(ret));
                goto cleanup;
            } else if(choco_string_equal(name_, tmp_name)) {
                tmp_id = i;
                found = true;
                break;
            }
        }
    }
    if(!found) {
        // NOTE: カメラの存在確認に使用することも考慮し、ワーニング、エラーは出さない
        ret = CAMERA_BAD_OPERATION;
        goto cleanup;
    }

    *out_camera_id_ = tmp_id;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_manager_camera_get(const camera_manager_t* camera_manager_, int16_t camera_id_, camera_t** out_camera_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get", "camera_manager_->camera_array")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get", "out_camera_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_id_ < camera_manager_->max_camera_count, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get", "camera_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_id_ >= 0, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get", "camera_id_")

    if(NULL == camera_manager_->camera_array[camera_id_]) {
        ret = CAMERA_BAD_OPERATION;
        ERROR_MESSAGE("camera_manager_camera_get(%s) - Provided camera id '%d' not found.", camera_rslt_to_str(ret), camera_id_);
        goto cleanup;
    }
    *out_camera_ = camera_manager_->camera_array[camera_id_];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_manager_camera_get_by_name(const camera_manager_t* camera_manager_, const char* name_, camera_t** out_camera_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    int16_t tmp_id = INVALID_CAMERA_ID;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get_by_name", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get_by_name", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get_by_name", "camera_manager_->camera_array")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get_by_name", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get_by_name", "out_camera_")

    ret = camera_manager_camera_id_get(camera_manager_, name_, &tmp_id);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_manager_camera_get_by_name(%s) - Failed to get camera.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    *out_camera_ = camera_manager_->camera_array[tmp_id];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}
