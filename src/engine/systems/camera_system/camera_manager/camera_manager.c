/** @ingroup camera_system
 *
 * @file camera_manager.c
 * @author chocolate-pie24
 * @brief カメラ管理システムで、カメラ構造体インスタンスの追加 / 削除 / 取得APIの実装
 *
 * @version 0.1
 * @date 2026-03-25
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
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

// #define TEST_BUILD

/**
 * @brief カメラ管理システム内部状態管理構造体
 *
 */
struct camera_manager {
    int16_t max_camera_count;   /**< 管理システムに登録可能なカメラ数上限値 */
    camera_t** camera_array;    /**< カメラ構造体インスタンス格納配列 */
};

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include "test_controller.h"

#include "engine/core/memory/test_choco_memory.h"
#include "engine/core/memory/test_linear_allocator.h"

#include "engine/systems/camera_system/camera_manager/test_camera_manager.h"
#include "engine/systems/camera_system/camera/test_camera.h"
#include "engine/systems/camera_system/camera_core/test_camera_memory.h"

// camera_manager用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_camera_manager_initialize;         /**< camera_manager_initialize()テスト設定 */
static test_call_control_t s_test_config_camera_manager_register;           /**< camera_manager_register()テスト設定 */
static test_call_control_t s_test_config_camera_manager_unregister;         /**< camera_manager_unregister()テスト設定 */
static test_call_control_t s_test_config_camera_manager_unregister_by_name; /**< camera_manager_unregister_by_name()テスト設定 */
static test_call_control_t s_test_config_camera_manager_camera_id_get;      /**< camera_manager_camera_id_get()テスト設定 */
static test_call_control_t s_test_config_camera_manager_camera_get;         /**< camera_manager_camera_get()テスト設定 */
static test_call_control_t s_test_config_camera_manager_camera_get_by_name; /**< camera_manager_camera_get_by_name()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_camera_manager_initialize(void);
static void test_camera_manager_deinitialize(void);
static void test_camera_manager_register(void);
static void test_camera_manager_unregister(void);
static void test_camera_manager_unregister_by_name(void);
static void test_camera_manager_camera_id_get(void);
static void test_camera_manager_camera_get(void);
static void test_camera_manager_camera_get_by_name(void);
#endif

camera_result_t camera_manager_initialize(int16_t max_camera_count_, linear_alloc_t* allocator_, camera_manager_t** out_camera_manager_) {
#ifdef TEST_BUILD
    s_test_config_camera_manager_initialize.call_count++;
    if(s_test_config_camera_manager_initialize.fail_on_call != 0) {
        if(s_test_config_camera_manager_initialize.call_count == s_test_config_camera_manager_initialize.fail_on_call) {
            return (camera_result_t)s_test_config_camera_manager_initialize.forced_result;
        }
    }
#endif
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

camera_result_t camera_manager_register(const char* camera_name_, camera_manager_t* camera_manager_, int16_t* out_camera_id_) {
#ifdef TEST_BUILD
    s_test_config_camera_manager_register.call_count++;
    if(s_test_config_camera_manager_register.fail_on_call != 0) {
        if(s_test_config_camera_manager_register.call_count == s_test_config_camera_manager_register.fail_on_call) {
            return (camera_result_t)s_test_config_camera_manager_register.forced_result;
        }
    }
#endif
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

camera_result_t camera_manager_unregister(int16_t camera_id_, camera_manager_t* camera_manager_) {
#ifdef TEST_BUILD
    s_test_config_camera_manager_unregister.call_count++;
    if(s_test_config_camera_manager_unregister.fail_on_call != 0) {
        if(s_test_config_camera_manager_unregister.call_count == s_test_config_camera_manager_unregister.fail_on_call) {
            return (camera_result_t)s_test_config_camera_manager_unregister.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_camera_manager_unregister_by_name.call_count++;
    if(s_test_config_camera_manager_unregister_by_name.fail_on_call != 0) {
        if(s_test_config_camera_manager_unregister_by_name.call_count == s_test_config_camera_manager_unregister_by_name.fail_on_call) {
            return (camera_result_t)s_test_config_camera_manager_unregister_by_name.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    int16_t tmp_id = INVALID_CAMERA_ID;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_unregister_by_name", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_unregister_by_name", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_unregister_by_name", "camera_manager_->camera_array")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_unregister_by_name", "name_")

    ret = camera_manager_camera_id_get(name_, camera_manager_, &tmp_id);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_manager_unregister_by_name(%s) - Failed to get camera id. Provided camera name = '%s'.", camera_rslt_to_str(ret), name_);
        goto cleanup;
    }
    camera_destroy(&camera_manager_->camera_array[tmp_id]);

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_manager_camera_id_get(const char* name_, const camera_manager_t* camera_manager_, int16_t* out_camera_id_) {
#ifdef TEST_BUILD
    s_test_config_camera_manager_camera_id_get.call_count++;
    if(s_test_config_camera_manager_camera_id_get.fail_on_call != 0) {
        if(s_test_config_camera_manager_camera_id_get.call_count == s_test_config_camera_manager_camera_id_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_manager_camera_id_get.forced_result;
        }
    }
#endif
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

camera_result_t camera_manager_camera_get(int16_t camera_id_, const camera_manager_t* camera_manager_, camera_t** out_camera_) {
#ifdef TEST_BUILD
    s_test_config_camera_manager_camera_get.call_count++;
    if(s_test_config_camera_manager_camera_get.fail_on_call != 0) {
        if(s_test_config_camera_manager_camera_get.call_count == s_test_config_camera_manager_camera_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_manager_camera_get.forced_result;
        }
    }
#endif
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

camera_result_t camera_manager_camera_get_by_name(const char* name_, const camera_manager_t* camera_manager_, camera_t** out_camera_) {
#ifdef TEST_BUILD
    s_test_config_camera_manager_camera_get_by_name.call_count++;
    if(s_test_config_camera_manager_camera_get_by_name.fail_on_call != 0) {
        if(s_test_config_camera_manager_camera_get_by_name.call_count == s_test_config_camera_manager_camera_get_by_name.fail_on_call) {
            return (camera_result_t)s_test_config_camera_manager_camera_get_by_name.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    int16_t tmp_id = INVALID_CAMERA_ID;

    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get_by_name", "camera_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(camera_manager_->max_camera_count > 0, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get_by_name", "camera_manager_->max_camera_count")
    IF_ARG_NULL_GOTO_CLEANUP(camera_manager_->camera_array, ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_manager_camera_get_by_name", "camera_manager_->camera_array")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get_by_name", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_manager_camera_get_by_name", "out_camera_")

    ret = camera_manager_camera_id_get(name_, camera_manager_, &tmp_id);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_manager_camera_get_by_name(%s) - Failed to get camera.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    *out_camera_ = camera_manager_->camera_array[tmp_id];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

#ifdef TEST_BUILD
void NO_COVERAGE test_camera_manager_initialize_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_manager_initialize.fail_on_call = config_->fail_on_call;
    s_test_config_camera_manager_initialize.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_manager_register_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_manager_register.fail_on_call = config_->fail_on_call;
    s_test_config_camera_manager_register.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_manager_unregister_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_manager_unregister.fail_on_call = config_->fail_on_call;
    s_test_config_camera_manager_unregister.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_manager_unregister_by_name_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_manager_unregister_by_name.fail_on_call = config_->fail_on_call;
    s_test_config_camera_manager_unregister_by_name.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_manager_camera_id_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_manager_camera_id_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_manager_camera_id_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_manager_camera_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_manager_camera_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_manager_camera_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_manager_camera_get_by_name_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_manager_camera_get_by_name.fail_on_call = config_->fail_on_call;
    s_test_config_camera_manager_camera_get_by_name.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_manager_config_reset(void) {
    test_call_control_reset(&s_test_config_camera_manager_initialize);
    test_call_control_reset(&s_test_config_camera_manager_register);
    test_call_control_reset(&s_test_config_camera_manager_unregister);
    test_call_control_reset(&s_test_config_camera_manager_unregister_by_name);
    test_call_control_reset(&s_test_config_camera_manager_camera_id_get);
    test_call_control_reset(&s_test_config_camera_manager_camera_get);
    test_call_control_reset(&s_test_config_camera_manager_camera_get_by_name);
}

void NO_COVERAGE test_camera_manager(void) {
    test_camera_manager_initialize();
    test_camera_manager_deinitialize();
    test_camera_manager_register();
    test_camera_manager_unregister();
    test_camera_manager_unregister_by_name();
    test_camera_manager_camera_id_get();
    test_camera_manager_camera_get();
    test_camera_manager_camera_get_by_name();
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_manager_initialize(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t* camera_manager = NULL;

        test_camera_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_manager_initialize_config_set(&config);

        ret = camera_manager_initialize(4, (linear_alloc_t*)0x1, &camera_manager);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(NULL == camera_manager);

        test_camera_manager_config_reset();
    }
    {
        /* allocator_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t* camera_manager = NULL;

        test_camera_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_initialize(4, NULL, &camera_manager);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == camera_manager);
    }
    {
        /* out_camera_manager_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        linear_alloc_t* allocator = (linear_alloc_t*)0x1;

        test_camera_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_initialize(4, allocator, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);
    }
    {
        /* *out_camera_manager_ != NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        linear_alloc_t* allocator = (linear_alloc_t*)0x1;
        camera_manager_t* camera_manager = (camera_manager_t*)0x1;

        test_camera_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_initialize(4, allocator, &camera_manager);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert((camera_manager_t*)0x1 == camera_manager);
    }
    {
        /* max_camera_count_ <= 0 */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        linear_alloc_t* allocator = (linear_alloc_t*)0x1;
        camera_manager_t* camera_manager = NULL;

        test_camera_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_initialize(0, allocator, &camera_manager);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == camera_manager);

        ret = camera_manager_initialize(-1, allocator, &camera_manager);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == camera_manager);
    }
    {
        /* 下位層 linear_allocator_allocate() の1回目失敗が伝播すること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        linear_alloc_t* allocator = NULL;
        void* linear_alloc_pool = NULL;
        size_t linear_alloc_pool_size = 1024U;
        size_t linear_alloc_mem_req = 0U;
        size_t linear_alloc_align_req = 0U;
        camera_manager_t* camera_manager = NULL;

        test_camera_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        linear_allocator_preinit(&linear_alloc_mem_req, &linear_alloc_align_req);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret_mem = memory_system_allocate(linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&allocator);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != allocator);

        ret_mem = memory_system_allocate(linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &linear_alloc_pool);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != linear_alloc_pool);

        ret_linear = linear_allocator_init(allocator, linear_alloc_pool_size, linear_alloc_pool);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        config.fail_on_call = 1U;
        config.forced_result = (int)LINEAR_ALLOC_NO_MEMORY;
        test_linear_allocator_allocate_config_set(&config);

        ret = camera_manager_initialize(4, allocator, &camera_manager);
        assert(CAMERA_NO_MEMORY == ret);
        assert(NULL == camera_manager);

        test_linear_allocator_config_reset();

        memory_system_free(linear_alloc_pool, linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
        linear_alloc_pool = NULL;

        memory_system_free(allocator, linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
        allocator = NULL;

        memory_system_destroy();
    }
    {
        /* 下位層 linear_allocator_allocate() の2回目失敗が伝播すること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        linear_alloc_t* allocator = NULL;
        void* linear_alloc_pool = NULL;
        size_t linear_alloc_pool_size = 1024U;
        size_t linear_alloc_mem_req = 0U;
        size_t linear_alloc_align_req = 0U;
        camera_manager_t* camera_manager = NULL;

        test_camera_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        linear_allocator_preinit(&linear_alloc_mem_req, &linear_alloc_align_req);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret_mem = memory_system_allocate(linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&allocator);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != allocator);

        ret_mem = memory_system_allocate(linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &linear_alloc_pool);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != linear_alloc_pool);

        ret_linear = linear_allocator_init(allocator, linear_alloc_pool_size, linear_alloc_pool);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        config.fail_on_call = 2U;
        config.forced_result = (int)LINEAR_ALLOC_NO_MEMORY;
        test_linear_allocator_allocate_config_set(&config);

        ret = camera_manager_initialize(4, allocator, &camera_manager);
        assert(CAMERA_NO_MEMORY == ret);
        assert(NULL == camera_manager);

        test_linear_allocator_config_reset();

        memory_system_free(linear_alloc_pool, linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
        linear_alloc_pool = NULL;

        memory_system_free(allocator, linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
        allocator = NULL;

        memory_system_destroy();
    }
    {
        /* 正常系: camera_manager が初期化され、内部フィールドが正しく設定されること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        linear_alloc_t* allocator = NULL;
        void* linear_alloc_pool = NULL;
        size_t linear_alloc_pool_size = 1024U;
        size_t linear_alloc_mem_req = 0U;
        size_t linear_alloc_align_req = 0U;
        camera_manager_t* camera_manager = NULL;
        int16_t max_camera_count = 4;

        test_camera_manager_config_reset();
        test_linear_allocator_config_reset();
        test_choco_memory_config_reset();

        linear_allocator_preinit(&linear_alloc_mem_req, &linear_alloc_align_req);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret_mem = memory_system_allocate(linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&allocator);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != allocator);

        ret_mem = memory_system_allocate(linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &linear_alloc_pool);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != linear_alloc_pool);

        ret_linear = linear_allocator_init(allocator, linear_alloc_pool_size, linear_alloc_pool);
        assert(LINEAR_ALLOC_SUCCESS == ret_linear);

        ret = camera_manager_initialize(max_camera_count, allocator, &camera_manager);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_manager);

        assert(max_camera_count == camera_manager->max_camera_count);
        assert(NULL != camera_manager->camera_array);

        for(int16_t i = 0; i != max_camera_count; ++i) {
            assert(NULL == camera_manager->camera_array[i]);
        }

        camera_manager_deinitialize(camera_manager);
        camera_manager = NULL;

        memory_system_free(linear_alloc_pool, linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
        linear_alloc_pool = NULL;

        memory_system_free(allocator, linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
        allocator = NULL;

        memory_system_destroy();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_manager_deinitialize(void) {
    {
        /* camera_manager_ == NULL の場合は no-op であること */
        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        camera_manager_deinitialize(NULL);
    }
    {
        /* max_camera_count == 0 の場合は no-op であること */
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 0;
        manager.camera_array = camera_array;

        camera_manager_deinitialize(&manager);

        assert(0 == manager.max_camera_count);
        assert(camera_array == manager.camera_array);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
    }
    {
        /* max_camera_count < 0 の場合は no-op であること */
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { (camera_t*)0x1, (camera_t*)0x2 };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = -1;
        manager.camera_array = camera_array;

        camera_manager_deinitialize(&manager);

        assert(-1 == manager.max_camera_count);
        assert(camera_array == manager.camera_array);
        assert((camera_t*)0x1 == manager.camera_array[0]);
        assert((camera_t*)0x2 == manager.camera_array[1]);
    }
    {
        /* 登録カメラがない場合でも全スロットを処理し、max_camera_count が 0 になること */
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        camera_manager_deinitialize(&manager);

        assert(0 == manager.max_camera_count);
        assert(camera_array == manager.camera_array);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL == manager.camera_array[2]);
    }
    {
        /* 登録済みカメラを全て破棄し、各スロットが NULL になること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("main_camera", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("debug_camera", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("ui_camera", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        camera_manager_deinitialize(&manager);

        assert(0 == manager.max_camera_count);
        assert(camera_array == manager.camera_array);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL == manager.camera_array[2]);

        memory_system_destroy();
    }
    {
        /* NULLスロットと登録済みカメラが混在していても全走査できること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[4] = { NULL, NULL, NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("main_camera", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("sub_camera", &camera_array[3]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[3]);

        manager.max_camera_count = 4;
        manager.camera_array = camera_array;

        camera_manager_deinitialize(&manager);

        assert(0 == manager.max_camera_count);
        assert(camera_array == manager.camera_array);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL == manager.camera_array[2]);
        assert(NULL == manager.camera_array[3]);

        memory_system_destroy();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_manager_register(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_manager_register_config_set(&config);

        ret = camera_manager_register("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);

        test_camera_manager_config_reset();
    }
    {
        /* camera_name_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[1] = { NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 1;
        manager.camera_array = camera_array;

        ret = camera_manager_register(NULL, &manager, &camera_id);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(123 == camera_id);
        assert(NULL == manager.camera_array[0]);
    }
    {
        /* camera_manager_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_register("main_camera", NULL, &camera_id);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(123 == camera_id);
    }
    {
        /* out_camera_id_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[1] = { NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 1;
        manager.camera_array = camera_array;

        ret = camera_manager_register("main_camera", &manager, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == manager.camera_array[0]);
    }
    {
        /* camera_manager_->max_camera_count <= 0 */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[1] = { NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 0;
        manager.camera_array = camera_array;

        ret = camera_manager_register("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);
        assert(NULL == manager.camera_array[0]);

        manager.max_camera_count = -1;

        ret = camera_manager_register("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);
        assert(NULL == manager.camera_array[0]);
    }
    {
        /* camera_manager_->camera_array == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = NULL;

        ret = camera_manager_register("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);
    }
    {
        /* 先に空きがあっても、後ろに同名カメラがあれば重複名で失敗すること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("main_camera", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_register("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);

        assert(NULL == manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(NULL == manager.camera_array[2]);
        assert(choco_string_equal("main_camera", camera_name_get(manager.camera_array[1])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 空きスロットがない場合は CAMERA_LIMIT_EXCEEDED を返すこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("camera_1", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_register("new_camera", &manager, &camera_id);
        assert(CAMERA_LIMIT_EXCEEDED == ret);
        assert(123 == camera_id);

        assert(NULL != manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("camera_1", camera_name_get(manager.camera_array[1])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 下位層 camera_create() の失敗が伝播すること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_NO_MEMORY;
        test_camera_create_config_set(&config);

        ret = camera_manager_register("main_camera", &manager, &camera_id);
        assert(CAMERA_NO_MEMORY == ret);
        assert(123 == camera_id);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);

        test_camera_config_reset();
    }
    {
        /* 正常系: 空配列なら先頭スロットに登録されること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        int16_t camera_id = INVALID_CAMERA_ID;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_register("main_camera", &manager, &camera_id);
        assert(CAMERA_SUCCESS == ret);
        assert(0 == camera_id);

        assert(NULL != manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(choco_string_equal("main_camera", camera_name_get(manager.camera_array[0])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 正常系: 複数空きがあっても最初の空きスロットに登録されること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[4] = { NULL, NULL, NULL, NULL };
        int16_t camera_id = INVALID_CAMERA_ID;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("existing_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("existing_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 4;
        manager.camera_array = camera_array;

        ret = camera_manager_register("new_camera", &manager, &camera_id);
        assert(CAMERA_SUCCESS == ret);
        assert(1 == camera_id);

        assert(NULL != manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(NULL == manager.camera_array[3]);

        assert(choco_string_equal("existing_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("new_camera", camera_name_get(manager.camera_array[1])));
        assert(choco_string_equal("existing_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_manager_unregister(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_manager_unregister_config_set(&config);

        ret = camera_manager_unregister(0, &manager);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);

        test_camera_manager_config_reset();
    }
    {
        /* camera_manager_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_unregister(0, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);
    }
    {
        /* camera_manager_->max_camera_count <= 0 */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[1] = { NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 0;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister(0, &manager);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(NULL == manager.camera_array[0]);

        manager.max_camera_count = -1;

        ret = camera_manager_unregister(0, &manager);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(NULL == manager.camera_array[0]);
    }
    {
        /* camera_manager_->camera_array == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = NULL;

        ret = camera_manager_unregister(0, &manager);
        assert(CAMERA_BAD_OPERATION == ret);
    }
    {
        /* camera_id_ < 0 */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister(-1, &manager);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
    }
    {
        /* camera_id_ >= camera_manager_->max_camera_count */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister(2, &manager);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
    }
    {
        /* 指定IDが未登録スロットなら BAD_OPERATION を返すこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister(1, &manager);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(NULL != manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("camera_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 正常系: 指定IDのカメラだけが削除されること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("camera_1", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister(1, &manager);
        assert(CAMERA_SUCCESS == ret);

        assert(NULL != manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);

        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("camera_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_manager_unregister_by_name(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_manager_unregister_by_name_config_set(&config);

        ret = camera_manager_unregister_by_name("main_camera", &manager);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);

        test_camera_manager_config_reset();
    }
    {
        /* camera_manager_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_unregister_by_name("main_camera", NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);
    }
    {
        /* camera_manager_->max_camera_count <= 0 */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[1] = { NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 0;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister_by_name("main_camera", &manager);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(NULL == manager.camera_array[0]);

        manager.max_camera_count = -1;

        ret = camera_manager_unregister_by_name("main_camera", &manager);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(NULL == manager.camera_array[0]);
    }
    {
        /* camera_manager_->camera_array == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = NULL;

        ret = camera_manager_unregister_by_name("main_camera", &manager);
        assert(CAMERA_BAD_OPERATION == ret);
    }
    {
        /* name_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister_by_name(NULL, &manager);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
    }
    {
        /* 下位層 camera_manager_camera_id_get() の失敗が伝播し、既存カメラが破棄されないこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("main_camera", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("sub_camera", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_DATA_CORRUPTED;
        test_camera_manager_camera_id_get_config_set(&config);

        ret = camera_manager_unregister_by_name("main_camera", &manager);
        assert(CAMERA_DATA_CORRUPTED == ret);

        assert(NULL != manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(choco_string_equal("main_camera", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("sub_camera", camera_name_get(manager.camera_array[1])));

        test_camera_manager_config_reset();

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 指定名が未登録なら BAD_OPERATION を返し、既存カメラが破棄されないこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister_by_name("not_found_camera", &manager);
        assert(CAMERA_BAD_OPERATION == ret);

        assert(NULL != manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("camera_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 先頭に NULL スロットがあっても、後ろの一致名を削除できること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[4] = { NULL, NULL, NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_1", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("target_camera", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        ret = camera_create("camera_3", &camera_array[3]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[3]);

        manager.max_camera_count = 4;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister_by_name("target_camera", &manager);
        assert(CAMERA_SUCCESS == ret);

        assert(NULL == manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(NULL == manager.camera_array[2]);
        assert(NULL != manager.camera_array[3]);

        assert(choco_string_equal("camera_1", camera_name_get(manager.camera_array[1])));
        assert(choco_string_equal("camera_3", camera_name_get(manager.camera_array[3])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 正常系: 指定名のカメラだけが削除されること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("target_camera", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_unregister_by_name("target_camera", &manager);
        assert(CAMERA_SUCCESS == ret);

        assert(NULL != manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);

        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("camera_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_manager_camera_id_get(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_manager_camera_id_get_config_set(&config);

        ret = camera_manager_camera_id_get("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);

        test_camera_manager_config_reset();
    }
    {
        /* camera_manager_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_camera_id_get("main_camera", NULL, &camera_id);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(123 == camera_id);
    }
    {
        /* camera_manager_->max_camera_count <= 0 */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[1] = { NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 0;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_id_get("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);
        assert(NULL == manager.camera_array[0]);

        manager.max_camera_count = -1;

        ret = camera_manager_camera_id_get("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);
        assert(NULL == manager.camera_array[0]);
    }
    {
        /* camera_manager_->camera_array == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = NULL;

        ret = camera_manager_camera_id_get("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);
    }
    {
        /* name_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_id_get(NULL, &manager, &camera_id);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(123 == camera_id);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
    }
    {
        /* out_camera_id_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_id_get("main_camera", &manager, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
    }
    {
        /* 指定名が未登録なら BAD_OPERATION を返し、out_camera_id_ を変更しないこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_id_get("not_found_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);

        assert(NULL != manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("camera_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 空の管理配列では BAD_OPERATION を返し、out_camera_id_ を変更しないこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };
        int16_t camera_id = 123;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_id_get("main_camera", &manager, &camera_id);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(123 == camera_id);

        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL == manager.camera_array[2]);
    }
    {
        /* 先頭に NULL スロットがあっても後ろの一致名を探索できること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[4] = { NULL, NULL, NULL, NULL };
        int16_t camera_id = INVALID_CAMERA_ID;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_1", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("target_camera", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        ret = camera_create("camera_3", &camera_array[3]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[3]);

        manager.max_camera_count = 4;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_id_get("target_camera", &manager, &camera_id);
        assert(CAMERA_SUCCESS == ret);
        assert(2 == camera_id);

        assert(NULL == manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(NULL != manager.camera_array[3]);
        assert(choco_string_equal("camera_1", camera_name_get(manager.camera_array[1])));
        assert(choco_string_equal("target_camera", camera_name_get(manager.camera_array[2])));
        assert(choco_string_equal("camera_3", camera_name_get(manager.camera_array[3])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 正常系: 一致したカメラ名称のIDを返すこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };
        int16_t camera_id = INVALID_CAMERA_ID;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("target_camera", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_id_get("target_camera", &manager, &camera_id);
        assert(CAMERA_SUCCESS == ret);
        assert(1 == camera_id);

        /* 読み取り系なので内部状態は変化しないこと */
        assert(NULL != manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("target_camera", camera_name_get(manager.camera_array[1])));
        assert(choco_string_equal("camera_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_manager_camera_get(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_manager_camera_get_config_set(&config);

        ret = camera_manager_camera_get(0, &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);

        test_camera_manager_config_reset();
    }
    {
        /* camera_manager_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_camera_get(0, NULL, &out_camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert((camera_t*)0x1 == out_camera);
    }
    {
        /* camera_manager_->max_camera_count <= 0 */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[1] = { NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 0;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get(0, &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);

        manager.max_camera_count = -1;

        ret = camera_manager_camera_get(0, &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);
    }
    {
        /* camera_manager_->camera_array == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = NULL;

        ret = camera_manager_camera_get(0, &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);
    }
    {
        /* out_camera_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get(0, &manager, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);
    }
    {
        /* camera_id_ >= camera_manager_->max_camera_count */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get(2, &manager, &out_camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert((camera_t*)0x1 == out_camera);
    }
    {
        /* camera_id_ < 0 */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get(-1, &manager, &out_camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert((camera_t*)0x1 == out_camera);
    }
    {
        /* 指定IDが未登録スロットなら BAD_OPERATION を返すこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get(1, &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);

        assert(NULL != manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 正常系: 指定IDのカメラポインタを取得できること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };
        camera_t* out_camera = NULL;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("target_camera", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get(1, &manager, &out_camera);
        assert(CAMERA_SUCCESS == ret);
        assert(camera_array[1] == out_camera);
        assert(choco_string_equal("target_camera", camera_name_get(out_camera)));

        /* 読み取り系なので内部状態は変化しないこと */
        assert(NULL != manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("target_camera", camera_name_get(manager.camera_array[1])));
        assert(choco_string_equal("camera_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_manager_camera_get_by_name(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_manager_camera_get_by_name_config_set(&config);

        ret = camera_manager_camera_get_by_name("main_camera", &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);

        test_camera_manager_config_reset();
    }
    {
        /* camera_manager_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret = camera_manager_camera_get_by_name("main_camera", NULL, &out_camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert((camera_t*)0x1 == out_camera);
    }
    {
        /* camera_manager_->max_camera_count <= 0 */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[1] = { NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 0;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get_by_name("main_camera", &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);
        assert(NULL == manager.camera_array[0]);

        manager.max_camera_count = -1;

        ret = camera_manager_camera_get_by_name("main_camera", &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);
        assert(NULL == manager.camera_array[0]);
    }
    {
        /* camera_manager_->camera_array == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = NULL;

        ret = camera_manager_camera_get_by_name("main_camera", &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);
    }
    {
        /* name_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get_by_name(NULL, &manager, &out_camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert((camera_t*)0x1 == out_camera);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
    }
    {
        /* out_camera_ == NULL */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get_by_name("main_camera", &manager, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
    }
    {
        /* 下位層 camera_manager_camera_id_get() の失敗が伝播し、out_camera_ が変更されないこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        camera_manager_t manager = {0};
        camera_t* camera_array[2] = { NULL, NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("main_camera", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("sub_camera", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        manager.max_camera_count = 2;
        manager.camera_array = camera_array;

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_DATA_CORRUPTED;
        test_camera_manager_camera_id_get_config_set(&config);

        ret = camera_manager_camera_get_by_name("main_camera", &manager, &out_camera);
        assert(CAMERA_DATA_CORRUPTED == ret);
        assert((camera_t*)0x1 == out_camera);

        assert(NULL != manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(choco_string_equal("main_camera", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("sub_camera", camera_name_get(manager.camera_array[1])));

        test_camera_manager_config_reset();

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 指定名が未登録なら BAD_OPERATION を返し、out_camera_ を変更しないこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get_by_name("not_found_camera", &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);

        assert(NULL != manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("camera_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 空の管理配列では BAD_OPERATION を返し、out_camera_ を変更しないこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };
        camera_t* out_camera = (camera_t*)0x1;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get_by_name("main_camera", &manager, &out_camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert((camera_t*)0x1 == out_camera);

        assert(NULL == manager.camera_array[0]);
        assert(NULL == manager.camera_array[1]);
        assert(NULL == manager.camera_array[2]);
    }
    {
        /* 先頭に NULL スロットがあっても後ろの一致名を取得できること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[4] = { NULL, NULL, NULL, NULL };
        camera_t* out_camera = NULL;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_1", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("target_camera", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        ret = camera_create("camera_3", &camera_array[3]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[3]);

        manager.max_camera_count = 4;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get_by_name("target_camera", &manager, &out_camera);
        assert(CAMERA_SUCCESS == ret);
        assert(camera_array[2] == out_camera);
        assert(choco_string_equal("target_camera", camera_name_get(out_camera)));

        assert(NULL == manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(NULL != manager.camera_array[3]);
        assert(choco_string_equal("camera_1", camera_name_get(manager.camera_array[1])));
        assert(choco_string_equal("target_camera", camera_name_get(manager.camera_array[2])));
        assert(choco_string_equal("camera_3", camera_name_get(manager.camera_array[3])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
    {
        /* 正常系: 指定名のカメラポインタを取得できること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_manager_t manager = {0};
        camera_t* camera_array[3] = { NULL, NULL, NULL };
        camera_t* out_camera = NULL;

        test_camera_manager_config_reset();
        test_camera_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("camera_0", &camera_array[0]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[0]);

        ret = camera_create("target_camera", &camera_array[1]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[1]);

        ret = camera_create("camera_2", &camera_array[2]);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera_array[2]);

        manager.max_camera_count = 3;
        manager.camera_array = camera_array;

        ret = camera_manager_camera_get_by_name("target_camera", &manager, &out_camera);
        assert(CAMERA_SUCCESS == ret);
        assert(camera_array[1] == out_camera);
        assert(choco_string_equal("target_camera", camera_name_get(out_camera)));

        /* 読み取り系なので内部状態は変化しないこと */
        assert(NULL != manager.camera_array[0]);
        assert(NULL != manager.camera_array[1]);
        assert(NULL != manager.camera_array[2]);
        assert(choco_string_equal("camera_0", camera_name_get(manager.camera_array[0])));
        assert(choco_string_equal("target_camera", camera_name_get(manager.camera_array[1])));
        assert(choco_string_equal("camera_2", camera_name_get(manager.camera_array[2])));

        camera_manager_deinitialize(&manager);
        memory_system_destroy();
    }
}
#endif
