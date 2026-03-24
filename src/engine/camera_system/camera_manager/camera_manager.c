#include "engine/camera_system/camera_manager/camera_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h> // for memset
#include <stdalign.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/camera_system/camera_core/camera_types.h"
#include "engine/camera_system/camera_core/camera_err_utils.h"

#include "engine/camera_system/camera/camera.h"

struct camera_manager {
    int16_t max_camera_count;
    camera_t** camera_array;
};

camera_result_t camera_manager_initialize(linear_alloc_t* allocator_, int16_t max_camera_count_, camera_manager_t** out_camera_manager_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    void* backend_ptr = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_initialize", "out_camera_manager_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_initialize", "*out_camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < max_camera_count_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_initialize", "max_camera_count_")

    // Simulation.
    camera_manager_t* tmp_manager = NULL;
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(camera_manager_t), alignof(camera_manager_t), (void**)&tmp_manager);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = camera_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("camera_manager_initialize(%s) - Failed to allocate memory for camera manager.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_manager, 0, sizeof(camera_manager_t));
    tmp_manager->max_camera_count = max_camera_count_;

    camera_t** tmp_camera_array = NULL;
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(camera_t*) * max_camera_count_, alignof(camera_t*), (void**)&tmp_camera_array);
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
    if(0 == camera_manager_->max_camera_count) {
        return;
    }
    for(int16_t i = 0; i != camera_manager_->max_camera_count; ++i) {
        camera_destroy(&camera_manager_->camera_array[i]);
    }
    camera_manager_->max_camera_count = 0;
}

camera_result_t camera_manager_register(const char* camera_name_, camera_manager_t* camera_manager_, int16_t* out_camera_id_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    bool found_free_slot = false;
    int16_t free_slot = INVALID_CAMERA_ID;
    camera_t* tmp_camera = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(camera_name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_register", "camera_name_")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_register", "camera_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_id_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_register", "out_camera_id_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_register", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_register", "camera_manager_->camera_array")

    for(int16_t i = 0; i != camera_manager_->max_camera_count; ++i) {
        if(NULL == camera_manager_->camera_array[i]) {
            found_free_slot = true;
            free_slot = i;
        } else if(choco_string_equal(camera_name_, camera_name_get(camera_manager_->camera_array[i]))) {
            ret = CAMERA_BAD_OPERATION;
            ERROR_MESSAGE("camera_manager_register(%s) - Provided camera name '%s' is already registered.", camera_rslt_to_str(ret), camera_name_);
            goto cleanup;
        }
    }
    if(!found_free_slot) {
        ret = CAMERA_LIMIT_EXCEEDED;
        ERROR_MESSAGE("camera_manager_register(%s) - Camera manager has no free slot.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    if(INVALID_CAMERA_ID != free_slot) {
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

camera_result_t camera_manager_unregister(int16_t camera_id_, camera_manager_t* camera_manager_) {
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

camera_result_t camera_manager_unregister_by_name(const char* name_, camera_manager_t* camera_manager_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_unregister_by_name", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_unregister_by_name", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_unregister_by_name", "camera_manager_->camera_array")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_unregister_by_name", "name_")

    bool found = false;
    for(int16_t i = 0; i != camera_manager_->max_camera_count; ++i) {
        if(NULL != camera_manager_->camera_array[i]) {
            if(choco_string_equal(name_, camera_name_get(camera_manager_->camera_array[i]))) {
                camera_destroy(&camera_manager_->camera_array[i]);
                found = true;
                break;
            }
        }
    }
    if(!found) {
        ret = CAMERA_BAD_OPERATION;
        ERROR_MESSAGE("camera_manager_unregister_by_name(%s) - Provided camera name '%s' is not registered.", camera_rslt_to_str(ret), name_);
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

int16_t camera_manager_camera_id_get(const char* name_, const camera_manager_t* camera_manager_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    int16_t ret_id = INVALID_CAMERA_ID;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_id_get", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_id_get", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_id_get", "camera_manager_->camera_array")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_id_get", "name_")

    bool found = false;
    for(int16_t i = 0; i != camera_manager_->max_camera_count; ++i) {
        if(NULL != camera_manager_->camera_array[i]) {
            if(choco_string_equal(name_, camera_name_get(camera_manager_->camera_array[i]))) {
                ret_id = i;
                found = true;
                break;
            }
        }
    }
    if(!found) {
        ret = CAMERA_BAD_OPERATION;
        WARN_MESSAGE("camera_manager_camera_id_get(%s) - Provided camera name '%s' not found.", camera_rslt_to_str(ret), name_);
        goto cleanup;
    }

cleanup:
    return ret_id;
}

camera_result_t camera_manager_camera_get(int16_t camera_id_, camera_manager_t* camera_manager_, camera_t** out_camera_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get", "camera_manager_->camera_array")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > camera_id_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get", "camera_id_")
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

camera_result_t camera_manager_camera_get_by_name(const char* name_, camera_manager_t* camera_manager_, camera_t** out_camera_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    bool found = false;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get_by_name", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get_by_name", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get_by_name", "camera_manager_->camera_array")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get_by_name", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get_by_name", "out_camera_")

    for(int16_t i = 0; i != camera_manager_->max_camera_count; ++i) {
        if(NULL != camera_manager_->camera_array[i]) {
            if(choco_string_equal(camera_name_get(camera_manager_->camera_array[i]), name_)) {
                *out_camera_ = camera_manager_->camera_array[i];
                found = true;
                break;
            }
        }
    }
    if(!found) {
        ret = CAMERA_BAD_OPERATION;
        ERROR_MESSAGE("camera_manager_camera_get_by_name(%s) - Provided camera name '%s' is not registered.", camera_rslt_to_str(ret), name_);
        goto cleanup;
    }
    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}
