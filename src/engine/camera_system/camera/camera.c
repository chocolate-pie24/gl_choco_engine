/**
 * @ingroup camera_system
 * @file camera.c
 * @author chocolate-pie24
 * @brief カメラモジュール実装
 *
 * @version 0.1
 * @date 2026-03-09
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdbool.h>

#include "engine/camera_system/camera/camera.h"

#include "engine/camera_system/camera_core/camera_err_utils.h"
#include "engine/camera_system/camera_core/camera_memory.h"
#include "engine/camera_system/camera_core/camera_types.h"

#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/containers/choco_string.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include "test_controller.h"

#include "engine/base/choco_macros.h"

#include "engine/core/memory/test_choco_memory.h"

#include "engine/camera_system/camera/test_camera.h"
#include "engine/camera_system/camera_core/test_camera_memory.h"

#include "engine/containers/test_choco_string.h"

// camera用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_camera_create;                 /**< camera_create()テスト設定 */
static test_call_control_t s_test_config_camera_viewing_frustum_update; /**< camera_viewing_frustum_update()テスト設定 */
static test_call_control_t s_test_config_camera_euler_update;           /**< camera_euler_update()テスト設定 */
static test_call_control_t s_test_config_camera_position_update;        /**< camera_position_update()テスト設定 */
static test_call_control_t s_test_config_camera_euler_get;              /**< camera_euler_get()テスト設定 */
static test_call_control_t s_test_config_camera_position_get;           /**< camera_position_get()テスト設定 */
static test_call_control_t s_test_config_camera_perspective_matrix_get; /**< camera_perspective_matrix_get()テスト設定 */
static test_call_control_t s_test_config_camera_view_matrix_get;        /**< camera_view_matrix_get()テスト設定 */
static test_call_control_t s_test_config_camera_forward_vector_get;     /**< camera_forward_vector_get()テスト設定 */
static test_call_control_t s_test_config_camera_backward_vector_get;    /**< camera_backward_vector_get()テスト設定 */
static test_call_control_t s_test_config_camera_right_vector_get;       /**< camera_right_vector_get()テスト設定 */
static test_call_control_t s_test_config_camera_left_vector_get;        /**< camera_left_vector_get()テスト設定 */
static test_call_control_t s_test_config_camera_up_vector_get;          /**< camera_up_vector_get()テスト設定 */
static test_call_control_t s_test_config_camera_down_vector_get;        /**< camera_down_vector_get()テスト設定 */

// プライベート関数テスト設定
static test_call_control_t s_test_config_camera_frustum_cache_sync; /**< camera_frustum_cache_sync()テスト設定 */
static test_call_control_t s_test_config_camera_posture_cache_sync; /**< camera_posture_cache_sync()テスト設定 */
static test_call_control_bool_t s_test_config_view_matrix_update;   /**< view_matrix_update()テスト設定 */
static test_call_control_bool_t s_test_config_is_valid_frustum;     /**< is_valid_frustum()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_camera_create(void);
static void test_camera_destroy(void);
static void test_camera_name_get(void);
static void test_camera_viewing_frustum_update(void);
static void test_camera_euler_update(void);
static void test_camera_position_update(void);
static void test_camera_euler_get(void);
static void test_camera_position_get(void);
static void test_camera_perspective_matrix_get(void);
static void test_camera_view_matrix_get(void);
static void test_camera_forward_vector_get(void);
static void test_camera_backward_vector_get(void);
static void test_camera_right_vector_get(void);
static void test_camera_left_vector_get(void);
static void test_camera_up_vector_get(void);
static void test_camera_down_vector_get(void);
static void test_camera_frustum_cache_sync(void);
static void test_camera_posture_cache_sync(void);
static void test_perspective_matrix_update(void);
static void test_camera_to_world_matrix_update(void);
static void test_view_matrix_update(void);
static void test_is_valid_frustum(void);
#endif

/**
 * @brief 視錐台データホルダ
 *
 */
typedef struct viewing_frustum {
    float aspect;       /**< 画面縦横比 */
    float fovy;         /**< 画角(degree) */
    float near_clip;    /**< 描画範囲(near) */
    float far_clip;     /**< 描画範囲(far) */
} viewing_frustum_t;

/**
 * @brief カメラ内部状態管理構造体
 *
 * @note カメラ姿勢に関しては以下を念頭に置くこと
 * - 座標系: 右手座標系
 * - Roll: Z軸回りの回転
 * - Pitch: X軸回りの回転
 * - Yaw: Y軸回りの回転
 * - カメラ前方方向: Z軸マイナス方向
 */
struct camera {
    vec3f_t euler;                      /**< カメラ姿勢オイラー角(degree) */
    vec3f_t position;                   /**< カメラ位置 */

    mat4x4f_t camera_to_world_matrix;   /**< カメラ座標系のある座標をワールド座標系へ変換する行列 */
    mat4x4f_t view_matrix;              /**< ビュー行列 */
    mat4x4f_t perspective_matrix;       /**< プロジェクション行列(透視投影) */

    viewing_frustum_t frustum;          /**< 視錐台パラメータ */

    choco_string_t* name;               /**< カメラ名称文字列 */

    bool posture_cache_dirty;           /**< true: 姿勢が更新されているが、姿勢由来の行列が更新されていない, false: 姿勢と姿勢由来の行列が同期済み */
    bool frustum_cache_dirty;           /**< true: 視錐台が更新されているが、視錐台由来の行列が更新されていない, false: 視錐台と視錐台由来の行列が同期済み */
};

static camera_result_t camera_frustum_cache_sync(camera_t* camera_);
static camera_result_t camera_posture_cache_sync(camera_t* camera_);

static void perspective_matrix_update(camera_t* camera_);
static void camera_to_world_matrix_update(camera_t* camera_);
static bool view_matrix_update(camera_t* camera_);
static bool is_valid_frustum(const viewing_frustum_t* frustum_);

camera_result_t camera_create(const char* name_, camera_t** out_camera_) {
#ifdef TEST_BUILD
    s_test_config_camera_create.call_count++;
    if(s_test_config_camera_create.fail_on_call != 0) {
        if(s_test_config_camera_create.call_count == s_test_config_camera_create.fail_on_call) {
            return (camera_result_t)s_test_config_camera_create.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    camera_t* tmp_camera = NULL;
    choco_string_result_t string_ret = CHOCO_STRING_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_create", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_create", "out_camera_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_create", "*out_camera_")

    ret = camera_mem_allocate(sizeof(camera_t), (void**)&tmp_camera);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_create(%s) - Failed to allocate memory for camera.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_camera->name = NULL;

    string_ret = choco_string_create_from_c_string(name_, &tmp_camera->name);
    if(CHOCO_STRING_SUCCESS != string_ret) {
        ret = camera_rslt_convert_choco_string(string_ret);
        ERROR_MESSAGE("camera_create(%s) - Failed to create string for camera name.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_camera->frustum.aspect = 0.0f;
    tmp_camera->frustum.far_clip = 0.0f;
    tmp_camera->frustum.near_clip = 0.0f;
    tmp_camera->frustum.fovy = 0.0f;

    mat4f_identity(&tmp_camera->view_matrix);
    mat4f_identity(&tmp_camera->camera_to_world_matrix);
    mat4f_identity(&tmp_camera->perspective_matrix);

    tmp_camera->posture_cache_dirty = true;
    tmp_camera->frustum_cache_dirty = true;

    vec3f_initialize(0.0f, 0.0f, 0.0f, &tmp_camera->euler);
    vec3f_initialize(0.0f, 0.0f, 0.0f, &tmp_camera->position);

    *out_camera_ = tmp_camera;

    ret = CAMERA_SUCCESS;

cleanup:
    if(CAMERA_SUCCESS != ret) {
        if(NULL != tmp_camera) {
            if(NULL != tmp_camera->name) {
                // ここは現状では通ることがないためカバレッジは100にならないが許容
                choco_string_destroy(&tmp_camera->name);
            }
            camera_destroy(&tmp_camera);
        }
    }
    return ret;
}

void camera_destroy(camera_t** camera_) {
    if(NULL == camera_) {
        return;
    }
    if(NULL == *camera_) {
        return;
    }
    if(NULL != (*camera_)->name) {
        choco_string_destroy(&(*camera_)->name);
    }
    camera_mem_free(*camera_, sizeof(camera_t));
    *camera_ = NULL;
}

const char* camera_name_get(const camera_t* camera_) {
    if(NULL == camera_) {
        ERROR_MESSAGE("camera_name_get(%s) - Argument camera_ requires a valid pointer.", camera_rslt_to_str(CAMERA_INVALID_ARGUMENT));
        return NULL;
    }
    if(NULL == camera_->name) {
        ERROR_MESSAGE("camera_name_get(%s) - Provided camera_ is corrupted.", camera_rslt_to_str(CAMERA_DATA_CORRUPTED));
        return NULL;
    }
    return choco_string_c_str(camera_->name);
}

camera_result_t camera_viewing_frustum_update(float fovy_, float aspect_, float near_clip_, float far_clip_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_camera_viewing_frustum_update.call_count++;
    if(s_test_config_camera_viewing_frustum_update.fail_on_call != 0) {
        if(s_test_config_camera_viewing_frustum_update.call_count == s_test_config_camera_viewing_frustum_update.fail_on_call) {
            return (camera_result_t)s_test_config_camera_viewing_frustum_update.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    viewing_frustum_t frustum = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_viewing_frustum_update", "camera_")

    frustum.aspect = aspect_;
    frustum.far_clip = far_clip_;
    frustum.fovy = fovy_;
    frustum.near_clip = near_clip_;
    if(!is_valid_frustum(&frustum)) {
        ret = CAMERA_INVALID_ARGUMENT;
        ERROR_MESSAGE("camera_viewing_frustum_update(%s) - Invalid frustum parameter.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    camera_->frustum = frustum;
    camera_->frustum_cache_dirty = true;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_euler_update(const vec3f_t* euler_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_camera_euler_update.call_count++;
    if(s_test_config_camera_euler_update.fail_on_call != 0) {
        if(s_test_config_camera_euler_update.call_count == s_test_config_camera_euler_update.fail_on_call) {
            return (camera_result_t)s_test_config_camera_euler_update.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(euler_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_euler_update", "euler_")
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_euler_update", "camera_")

    camera_->euler.elem[0] = euler_->elem[0];
    camera_->euler.elem[1] = euler_->elem[1];
    camera_->euler.elem[2] = euler_->elem[2];

    camera_->posture_cache_dirty = true;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_position_update(const vec3f_t* position_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_camera_position_update.call_count++;
    if(s_test_config_camera_position_update.fail_on_call != 0) {
        if(s_test_config_camera_position_update.call_count == s_test_config_camera_position_update.fail_on_call) {
            return (camera_result_t)s_test_config_camera_position_update.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(position_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_position_update", "position_")
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_position_update", "camera_")

    camera_->position.elem[0] = position_->elem[0];
    camera_->position.elem[1] = position_->elem[1];
    camera_->position.elem[2] = position_->elem[2];

    camera_->posture_cache_dirty = true;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_euler_get(const camera_t* camera_, vec3f_t* out_euler_) {
#ifdef TEST_BUILD
    s_test_config_camera_euler_get.call_count++;
    if(s_test_config_camera_euler_get.fail_on_call != 0) {
        if(s_test_config_camera_euler_get.call_count == s_test_config_camera_euler_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_euler_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_euler_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_euler_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_euler_get", "out_euler_")

    out_euler_->elem[0] = camera_->euler.elem[0];
    out_euler_->elem[1] = camera_->euler.elem[1];
    out_euler_->elem[2] = camera_->euler.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_position_get(const camera_t* camera_, vec3f_t* out_position_) {
#ifdef TEST_BUILD
    s_test_config_camera_position_get.call_count++;
    if(s_test_config_camera_position_get.fail_on_call != 0) {
        if(s_test_config_camera_position_get.call_count == s_test_config_camera_position_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_position_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_position_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_position_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_position_get", "out_position_")

    out_position_->elem[0] = camera_->position.elem[0];
    out_position_->elem[1] = camera_->position.elem[1];
    out_position_->elem[2] = camera_->position.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_perspective_matrix_get(camera_t* camera_, mat4x4f_t* out_mat_) {
#ifdef TEST_BUILD
    s_test_config_camera_perspective_matrix_get.call_count++;
    if(s_test_config_camera_perspective_matrix_get.fail_on_call != 0) {
        if(s_test_config_camera_perspective_matrix_get.call_count == s_test_config_camera_perspective_matrix_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_perspective_matrix_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_perspective_matrix_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_perspective_matrix_get", "out_mat_")
    IF_ARG_FALSE_GOTO_CLEANUP(is_valid_frustum(&camera_->frustum), ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_perspective_matrix_get", "camera_->frustum")

    ret = camera_frustum_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_perspective_matrix_get(%s) - Failed to sync frustum cache.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    mat4f_copy(&camera_->perspective_matrix, out_mat_);

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_view_matrix_get(camera_t* camera_, mat4x4f_t* out_mat_) {
#ifdef TEST_BUILD
    s_test_config_camera_view_matrix_get.call_count++;
    if(s_test_config_camera_view_matrix_get.fail_on_call != 0) {
        if(s_test_config_camera_view_matrix_get.call_count == s_test_config_camera_view_matrix_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_view_matrix_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_view_matrix_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_view_matrix_get", "out_mat_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_view_matrix_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    mat4f_copy(&camera_->view_matrix, out_mat_);

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_forward_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
#ifdef TEST_BUILD
    s_test_config_camera_forward_vector_get.call_count++;
    if(s_test_config_camera_forward_vector_get.fail_on_call != 0) {
        if(s_test_config_camera_forward_vector_get.call_count == s_test_config_camera_forward_vector_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_forward_vector_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_forward_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_forward_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_forward_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ前方の単位ベクトル[0, 0, -1, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを前方に移動させるための方向ベクトル
    v.elem[0] = -1.0f * camera_->camera_to_world_matrix.elem[2];
    v.elem[1] = -1.0f * camera_->camera_to_world_matrix.elem[6];
    v.elem[2] = -1.0f * camera_->camera_to_world_matrix.elem[10];
    vec3f_normalize(&v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_backward_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
#ifdef TEST_BUILD
    s_test_config_camera_backward_vector_get.call_count++;
    if(s_test_config_camera_backward_vector_get.fail_on_call != 0) {
        if(s_test_config_camera_backward_vector_get.call_count == s_test_config_camera_backward_vector_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_backward_vector_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_backward_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_backward_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_backward_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ後方の単位ベクトル[0, 0, 1, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを後方に移動させるための方向ベクトル
    v.elem[0] = camera_->camera_to_world_matrix.elem[2];
    v.elem[1] = camera_->camera_to_world_matrix.elem[6];
    v.elem[2] = camera_->camera_to_world_matrix.elem[10];
    vec3f_normalize(&v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_right_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
#ifdef TEST_BUILD
    s_test_config_camera_right_vector_get.call_count++;
    if(s_test_config_camera_right_vector_get.fail_on_call != 0) {
        if(s_test_config_camera_right_vector_get.call_count == s_test_config_camera_right_vector_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_right_vector_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_right_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_right_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_right_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ右方向の単位ベクトル[1, 0, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを右に移動させるための方向ベクトル
    v.elem[0] = camera_->camera_to_world_matrix.elem[0];
    v.elem[1] = camera_->camera_to_world_matrix.elem[4];
    v.elem[2] = camera_->camera_to_world_matrix.elem[8];
    vec3f_normalize(&v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_left_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
#ifdef TEST_BUILD
    s_test_config_camera_left_vector_get.call_count++;
    if(s_test_config_camera_left_vector_get.fail_on_call != 0) {
        if(s_test_config_camera_left_vector_get.call_count == s_test_config_camera_left_vector_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_left_vector_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_left_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_left_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_left_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ左方向の単位ベクトル[-1, 0, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを左に移動させるための方向ベクトル
    v.elem[0] = -1.0f * camera_->camera_to_world_matrix.elem[0];
    v.elem[1] = -1.0f * camera_->camera_to_world_matrix.elem[4];
    v.elem[2] = -1.0f * camera_->camera_to_world_matrix.elem[8];
    vec3f_normalize(&v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_up_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
#ifdef TEST_BUILD
    s_test_config_camera_up_vector_get.call_count++;
    if(s_test_config_camera_up_vector_get.fail_on_call != 0) {
        if(s_test_config_camera_up_vector_get.call_count == s_test_config_camera_up_vector_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_up_vector_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_up_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_up_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_up_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ上方向の単位ベクトル[0, 1, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを上に移動させるための方向ベクトル
    v.elem[0] = camera_->camera_to_world_matrix.elem[1];
    v.elem[1] = camera_->camera_to_world_matrix.elem[5];
    v.elem[2] = camera_->camera_to_world_matrix.elem[9];
    vec3f_normalize(&v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_down_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
#ifdef TEST_BUILD
    s_test_config_camera_down_vector_get.call_count++;
    if(s_test_config_camera_down_vector_get.fail_on_call != 0) {
        if(s_test_config_camera_down_vector_get.call_count == s_test_config_camera_down_vector_get.fail_on_call) {
            return (camera_result_t)s_test_config_camera_down_vector_get.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_down_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_down_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_down_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ下方向の単位ベクトル[0, -1, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    vec3f_t v = { 0 };  // ワールド座標系でカメラを下に移動させるための方向ベクトル
    v.elem[0] = -1.0f * camera_->camera_to_world_matrix.elem[1];
    v.elem[1] = -1.0f * camera_->camera_to_world_matrix.elem[5];
    v.elem[2] = -1.0f * camera_->camera_to_world_matrix.elem[9];
    vec3f_normalize(&v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief カメラ視錐台に合わせて以下の行列を更新する
 * - 透視投影行列
 *
 * @note 行列が最新のカメラ視錐台と同期が取れていれば更新は行わない
 *
 * @param[in,out] camera_ 更新対象カメラ構造体インスタンスへのポインタ
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_BAD_OPERATION 視錐台パラメータ異常
 * @retval CAMERA_SUCCESS 同期に成功し、正常終了
 */
static camera_result_t camera_frustum_cache_sync(camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_camera_frustum_cache_sync.call_count++;
    if(s_test_config_camera_frustum_cache_sync.fail_on_call != 0) {
        if(s_test_config_camera_frustum_cache_sync.call_count == s_test_config_camera_frustum_cache_sync.fail_on_call) {
            return (camera_result_t)s_test_config_camera_frustum_cache_sync.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_frustum_cache_sync", "camera_")
    IF_ARG_FALSE_GOTO_CLEANUP(is_valid_frustum(&camera_->frustum), ret, CAMERA_BAD_OPERATION, camera_rslt_to_str(CAMERA_BAD_OPERATION), "camera_frustum_cache_sync", "camera_->frustum")

    if(camera_->frustum_cache_dirty) {
        perspective_matrix_update(camera_);
        camera_->frustum_cache_dirty = false;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief カメラ姿勢に合わせて以下の行列を更新する
 * - カメラ座標系からワールド座標系への変換行列
 * - ビュー行列
 *
 * @note 行列が最新のカメラ姿勢と同期が取れていれば更新は行わない
 *
 * @param[in,out] camera_ 更新対象カメラ構造体インスタンスへのポインタ
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 逆行列計算に失敗
 * @retval CAMERA_SUCCESS 同期に成功し、正常終了
 */
static camera_result_t camera_posture_cache_sync(camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_camera_posture_cache_sync.call_count++;
    if(s_test_config_camera_posture_cache_sync.fail_on_call != 0) {
        if(s_test_config_camera_posture_cache_sync.call_count == s_test_config_camera_posture_cache_sync.fail_on_call) {
            return (camera_result_t)s_test_config_camera_posture_cache_sync.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_posture_cache_sync", "camera_")

    if(camera_->posture_cache_dirty) {
        camera_to_world_matrix_update(camera_);
        if(!view_matrix_update(camera_)) {
            ret = CAMERA_RUNTIME_ERROR;
            ERROR_MESSAGE("camera_posture_cache_sync(%s) - Matrix(view) inversion failed because the determinant is zero or near zero.", camera_rslt_to_str(ret));
            goto cleanup;
        }
        camera_->posture_cache_dirty = false;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief プロジェクション行列(透視投影)を更新する
 *
 * @warning この関数を呼び出す際は、事前に以下のチェックを必ず行うこと
 * - camera_ != NULL
 * - is_valid_frustum(&camera->frustum) == true
 *
 * @param[in,out] camera_ プロジェクション行列取得対象カメラ構造体インスタンスへのポインタ
 */
static void perspective_matrix_update(camera_t* camera_) {
    const float dz = camera_->frustum.far_clip - camera_->frustum.near_clip;

    mat4f_identity(&camera_->perspective_matrix);
    camera_->perspective_matrix.elem[5] = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(camera_->frustum.fovy) * 0.5f);
    camera_->perspective_matrix.elem[0] = camera_->perspective_matrix.elem[5] / camera_->frustum.aspect;
    camera_->perspective_matrix.elem[10] = -1.0f * (camera_->frustum.far_clip + camera_->frustum.near_clip) / dz;
    camera_->perspective_matrix.elem[11] = -2.0f * camera_->frustum.far_clip * camera_->frustum.near_clip / dz;
    camera_->perspective_matrix.elem[14] = -1.0f;
    camera_->perspective_matrix.elem[15] = 0.0f;
}

/**
 * @brief カメラ座標系からワールド座標系へ変換する行列を更新する
 *
 * @warning この関数を呼び出す際は、事前に以下のチェックを必ず行うこと
 * - camera_ != NULL
 *
 * @param[in,out] camera_ 座標変換行列取得対象カメラ構造体インスタンスへのポインタ
 */
static void camera_to_world_matrix_update(camera_t* camera_) {
    mat4x4f_t rot = { 0 };
    mat4x4f_t trans = { 0 };

    mat4f_rot_xyz(CHOCO_DEG_TO_RAD(camera_->euler.elem[0]), CHOCO_DEG_TO_RAD(camera_->euler.elem[1]), CHOCO_DEG_TO_RAD(camera_->euler.elem[2]), &rot);
    mat4f_translation(&camera_->position, &trans); // ある座標をtranslate分平行移動する行列 = translate分座標が増える = カメラ->ワールド座標系への変換行列

    // 後に変換するものを左から掛ける
    mat4f_mul(&trans, &rot, &camera_->camera_to_world_matrix);
}

/**
 * @brief ビュー行列を更新する
 *
 * @warning この関数を呼び出す際は、事前に以下のチェックを必ず行うこと
 * - camera_ != NULL
 *
 * @param[in,out] camera_ ビュー行列取得対象カメラ構造体インスタンスへのポインタ
 *
 * @return true ビュー行列更新成功
 * @return false ビュー行列の更新に失敗(逆行列の計算に失敗)
 */
static bool view_matrix_update(camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_view_matrix_update.call_count++;
    if(s_test_config_view_matrix_update.fail_on_call != 0) {
        if(s_test_config_view_matrix_update.call_count == s_test_config_view_matrix_update.fail_on_call) {
            return s_test_config_view_matrix_update.forced_result;
        }
    }
#endif
    mat4x4f_t tmp = { 0 };

    mat4f_copy(&camera_->camera_to_world_matrix, &tmp);
    const bool ret = mat4f_inverse(&tmp);
    if(ret) {
        mat4f_copy(&tmp, &camera_->view_matrix);
    }

    return ret;
}

/**
 * @brief 視錐台パラメータ有効 / 無効判定を行う
 *
 * @param[in] frustum_ 判定対象視錐台構造体インスタンスへのポインタ
 *
 * @retval true 視錐台パラメータ正常
 * @retval false 視錐台パラメータ異常
 */
static bool is_valid_frustum(const viewing_frustum_t* frustum_) {
#ifdef TEST_BUILD
    s_test_config_is_valid_frustum.call_count++;
    if(s_test_config_is_valid_frustum.fail_on_call != 0) {
        if(s_test_config_is_valid_frustum.call_count == s_test_config_is_valid_frustum.fail_on_call) {
            return s_test_config_is_valid_frustum.forced_result;
        }
    }
#endif
    if(NULL == frustum_) {
        return false;
    } else if(frustum_->near_clip >= frustum_->far_clip) {
        return false;
    } else if(frustum_->aspect <= 0.0f) {
        return false;
    } else if(frustum_->fovy <= 0.0f) {
        return false;
    } else if(frustum_->fovy >= 180.0f) {
        return false;
    } else if(frustum_->near_clip <= 0.0f) {
        return false;
    } else if(frustum_->far_clip <= 0.0f) {
        // ここはnear_clip >= far_clipで先に引っかかるため通らないけど、許容+一応残しておく
        return false;
    }
    return true;
}

#ifdef TEST_BUILD
void NO_COVERAGE test_camera_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_create.fail_on_call = config_->fail_on_call;
    s_test_config_camera_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_viewing_frustum_update_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_viewing_frustum_update.fail_on_call = config_->fail_on_call;
    s_test_config_camera_viewing_frustum_update.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_euler_update_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_euler_update.fail_on_call = config_->fail_on_call;
    s_test_config_camera_euler_update.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_position_update_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_position_update.fail_on_call = config_->fail_on_call;
    s_test_config_camera_position_update.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_euler_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_euler_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_euler_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_position_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_position_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_position_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_perspective_matrix_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_perspective_matrix_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_perspective_matrix_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_view_matrix_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_view_matrix_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_view_matrix_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_forward_vector_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_forward_vector_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_forward_vector_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_backward_vector_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_backward_vector_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_backward_vector_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_right_vector_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_right_vector_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_right_vector_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_left_vector_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_left_vector_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_left_vector_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_up_vector_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_up_vector_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_up_vector_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_down_vector_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_camera_down_vector_get.fail_on_call = config_->fail_on_call;
    s_test_config_camera_down_vector_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_camera_config_reset(void) {
    test_call_control_reset(&s_test_config_camera_create);
    test_call_control_reset(&s_test_config_camera_viewing_frustum_update);
    test_call_control_reset(&s_test_config_camera_euler_update);
    test_call_control_reset(&s_test_config_camera_position_update);
    test_call_control_reset(&s_test_config_camera_euler_get);
    test_call_control_reset(&s_test_config_camera_position_get);
    test_call_control_reset(&s_test_config_camera_perspective_matrix_get);
    test_call_control_reset(&s_test_config_camera_view_matrix_get);
    test_call_control_reset(&s_test_config_camera_forward_vector_get);
    test_call_control_reset(&s_test_config_camera_backward_vector_get);
    test_call_control_reset(&s_test_config_camera_right_vector_get);
    test_call_control_reset(&s_test_config_camera_left_vector_get);
    test_call_control_reset(&s_test_config_camera_up_vector_get);
    test_call_control_reset(&s_test_config_camera_down_vector_get);

    test_call_control_reset(&s_test_config_camera_frustum_cache_sync);
    test_call_control_reset(&s_test_config_camera_posture_cache_sync);
    test_call_control_bool_reset(&s_test_config_view_matrix_update);
    test_call_control_bool_reset(&s_test_config_is_valid_frustum);
}

void NO_COVERAGE test_camera(void) {
    test_camera_create();
    test_camera_destroy();
    test_camera_name_get();
    test_camera_viewing_frustum_update();
    test_camera_euler_update();
    test_camera_position_update();
    test_camera_euler_get();
    test_camera_position_get();
    test_camera_perspective_matrix_get();
    test_camera_view_matrix_get();
    test_camera_forward_vector_get();
    test_camera_backward_vector_get();
    test_camera_right_vector_get();
    test_camera_left_vector_get();
    test_camera_up_vector_get();
    test_camera_down_vector_get();
    test_camera_frustum_cache_sync();
    test_camera_posture_cache_sync();
    test_perspective_matrix_update();
    test_camera_to_world_matrix_update();
    test_view_matrix_update();
    test_is_valid_frustum();
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_create(void) {
    {
        // camera_create() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_create_config_set(&config);

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(NULL == camera);

        test_camera_config_reset();
    }
    {
        // name_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();

        ret = camera_create(NULL, &camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert(NULL == camera);

        test_camera_config_reset();
    }
    {
        // out_camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_camera_config_reset();

        ret = camera_create("main_camera", NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_camera_config_reset();
    }
    {
        // *out_camera_ != NULL -> CAMERA_INVALID_ARGUMENT
        // 既存ポインタは変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = (camera_t*)0x1;

        test_camera_config_reset();

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);
        assert((camera_t*)0x1 == camera);

        test_camera_config_reset();
    }
    {
        // camera_mem_allocate() 失敗 -> 注入結果がそのまま返ること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_NO_MEMORY;
        test_camera_mem_allocate_config_set(&config);

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_NO_MEMORY == ret);
        assert(NULL == camera);

        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // choco_string_create_from_c_string() 失敗 ->
        // camera_rslt_convert_choco_string() により CAMERA_LIMIT_EXCEEDED に変換されること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_LIMIT_EXCEEDED;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_LIMIT_EXCEEDED == ret);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t identity = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        // name 初期化確認
        assert(NULL != camera->name);
        assert(true == choco_string_equal("main_camera", camera_name_get(camera)));

        // frustum 初期値確認
        assert(is_equal_float(0.0f, camera->frustum.aspect));
        assert(is_equal_float(0.0f, camera->frustum.fovy));
        assert(is_equal_float(0.0f, camera->frustum.near_clip));
        assert(is_equal_float(0.0f, camera->frustum.far_clip));

        // posture 初期値確認
        assert(is_equal_float(0.0f, camera->euler.elem[0]));
        assert(is_equal_float(0.0f, camera->euler.elem[1]));
        assert(is_equal_float(0.0f, camera->euler.elem[2]));
        assert(is_equal_float(0.0f, camera->position.elem[0]));
        assert(is_equal_float(0.0f, camera->position.elem[1]));
        assert(is_equal_float(0.0f, camera->position.elem[2]));

        // dirty flag 初期値確認
        assert(true == camera->posture_cache_dirty);
        assert(true == camera->frustum_cache_dirty);

        // 行列初期値確認
        mat4f_identity(&identity);
        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(identity.elem[i], camera->view_matrix.elem[i]));
            assert(is_equal_float(identity.elem[i], camera->camera_to_world_matrix.elem[i]));
            assert(is_equal_float(identity.elem[i], camera->perspective_matrix.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_destroy(void) {
    {
        // camera_ == NULL -> no-op
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        camera_destroy(NULL);

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // *camera_ == NULL -> no-op
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        camera_destroy(&camera);
        assert(NULL == camera);

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // name != NULL を持つ通常の camera を破棄できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);
        assert(NULL != camera->name);

        camera_destroy(&camera);
        assert(NULL == camera);

        // 2回目も no-op で呼べること
        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // name == NULL の camera でも破棄できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);
        assert(NULL != camera->name);

        choco_string_destroy(&camera->name);
        assert(NULL == camera->name);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_name_get(void) {
    {
        // camera_ == NULL -> NULL
        const char* name = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        name = camera_name_get(NULL);
        assert(NULL == name);

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_->name == NULL -> NULL
        camera_t camera = {0};
        const char* name = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        name = camera_name_get(&camera);
        assert(NULL == name);

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: 設定した名前を取得できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        const char* name = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        name = camera_name_get(camera);
        assert(NULL != name);
        assert(true == choco_string_equal("main_camera", name));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: 空文字名でも取得できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        const char* name = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        name = camera_name_get(camera);
        assert(NULL != name);
        assert(true == choco_string_equal("", name));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_viewing_frustum_update(void) {
    {
        // camera_viewing_frustum_update() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_viewing_frustum_update_config_set(&config);

        ret = camera_viewing_frustum_update(60.0f, 16.0f / 9.0f, 0.1f, 100.0f, camera);
        assert(CAMERA_BAD_OPERATION == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_camera_config_reset();

        ret = camera_viewing_frustum_update(60.0f, 16.0f / 9.0f, 0.1f, 100.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_camera_config_reset();
    }
    {
        // 無効な視錐台入力 -> CAMERA_INVALID_ARGUMENT
        // 既存の frustum 状態は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->frustum.aspect = 4.0f / 3.0f;
        camera->frustum.fovy = 45.0f;
        camera->frustum.near_clip = 0.5f;
        camera->frustum.far_clip = 500.0f;
        camera->frustum_cache_dirty = false;

        // near_clip >= far_clip で無効
        ret = camera_viewing_frustum_update(60.0f, 16.0f / 9.0f, 10.0f, 1.0f, camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        // 既存状態が維持されること
        assert(is_equal_float(camera->frustum.aspect, 4.0f / 3.0f));
        assert(is_equal_float(camera->frustum.fovy, 45.0f));
        assert(is_equal_float(camera->frustum.near_clip, 0.5f));
        assert(is_equal_float(camera->frustum.far_clip, 500.0f));
        assert(false == camera->frustum_cache_dirty);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->frustum.aspect = 1.0f;
        camera->frustum.fovy = 1.0f;
        camera->frustum.near_clip = 1.0f;
        camera->frustum.far_clip = 2.0f;
        camera->frustum_cache_dirty = false;

        ret = camera_viewing_frustum_update(60.0f, 16.0f / 9.0f, 0.1f, 100.0f, camera);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(camera->frustum.aspect, 16.0f / 9.0f));
        assert(is_equal_float(camera->frustum.fovy, 60.0f));
        assert(is_equal_float(camera->frustum.near_clip, 0.1f));
        assert(is_equal_float(camera->frustum.far_clip, 100.0f));
        assert(true == camera->frustum_cache_dirty);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_euler_update(void) {
    {
        // camera_euler_update() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t euler = { .elem = { 10.0f, 20.0f, 30.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_euler_update_config_set(&config);

        ret = camera_euler_update(&euler, camera);
        assert(CAMERA_BAD_OPERATION == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // euler_ == NULL -> CAMERA_INVALID_ARGUMENT
        // 状態が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->euler.elem[0] = 1.0f;
        camera->euler.elem[1] = 2.0f;
        camera->euler.elem[2] = 3.0f;
        camera->posture_cache_dirty = false;

        ret = camera_euler_update(NULL, camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(camera->euler.elem[0], 1.0f));
        assert(is_equal_float(camera->euler.elem[1], 2.0f));
        assert(is_equal_float(camera->euler.elem[2], 3.0f));
        assert(false == camera->posture_cache_dirty);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t euler = { .elem = { 10.0f, 20.0f, 30.0f } };

        test_camera_config_reset();

        ret = camera_euler_update(&euler, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_camera_config_reset();
    }
    {
        // 正常系
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t euler = { .elem = { 15.0f, -25.0f, 35.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->euler.elem[0] = 1.0f;
        camera->euler.elem[1] = 2.0f;
        camera->euler.elem[2] = 3.0f;
        camera->position.elem[0] = 100.0f;
        camera->position.elem[1] = 200.0f;
        camera->position.elem[2] = 300.0f;
        camera->posture_cache_dirty = false;

        ret = camera_euler_update(&euler, camera);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(camera->euler.elem[0], 15.0f));
        assert(is_equal_float(camera->euler.elem[1], -25.0f));
        assert(is_equal_float(camera->euler.elem[2], 35.0f));
        assert(true == camera->posture_cache_dirty);

        // 関係ない状態は変化しないこと
        assert(is_equal_float(camera->position.elem[0], 100.0f));
        assert(is_equal_float(camera->position.elem[1], 200.0f));
        assert(is_equal_float(camera->position.elem[2], 300.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_position_update(void) {
    {
        // camera_position_update() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t position = { .elem = { 10.0f, 20.0f, 30.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_position_update_config_set(&config);

        ret = camera_position_update(&position, camera);
        assert(CAMERA_BAD_OPERATION == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // position_ == NULL -> CAMERA_INVALID_ARGUMENT
        // 状態が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->position.elem[0] = 1.0f;
        camera->position.elem[1] = 2.0f;
        camera->position.elem[2] = 3.0f;
        camera->posture_cache_dirty = false;

        ret = camera_position_update(NULL, camera);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(camera->position.elem[0], 1.0f));
        assert(is_equal_float(camera->position.elem[1], 2.0f));
        assert(is_equal_float(camera->position.elem[2], 3.0f));
        assert(false == camera->posture_cache_dirty);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t position = { .elem = { 10.0f, 20.0f, 30.0f } };

        test_camera_config_reset();

        ret = camera_position_update(&position, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_camera_config_reset();
    }
    {
        // 正常系
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t position = { .elem = { 15.0f, -25.0f, 35.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->position.elem[0] = 1.0f;
        camera->position.elem[1] = 2.0f;
        camera->position.elem[2] = 3.0f;
        camera->euler.elem[0] = 100.0f;
        camera->euler.elem[1] = 200.0f;
        camera->euler.elem[2] = 300.0f;
        camera->posture_cache_dirty = false;

        ret = camera_position_update(&position, camera);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(camera->position.elem[0], 15.0f));
        assert(is_equal_float(camera->position.elem[1], -25.0f));
        assert(is_equal_float(camera->position.elem[2], 35.0f));
        assert(true == camera->posture_cache_dirty);

        // 関係ない状態は変化しないこと
        assert(is_equal_float(camera->euler.elem[0], 100.0f));
        assert(is_equal_float(camera->euler.elem[1], 200.0f));
        assert(is_equal_float(camera->euler.elem[2], 300.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_euler_get(void) {
    {
        // camera_euler_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_euler = { .elem = { -1.0f, -1.0f, -1.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_euler_get_config_set(&config);

        ret = camera_euler_get(camera, &out_euler);
        assert(CAMERA_BAD_OPERATION == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        // out_euler_ は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t out_euler = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();

        ret = camera_euler_get(NULL, &out_euler);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(out_euler.elem[0], 1.0f));
        assert(is_equal_float(out_euler.elem[1], 2.0f));
        assert(is_equal_float(out_euler.elem[2], 3.0f));

        test_camera_config_reset();
    }
    {
        // out_euler_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->euler.elem[0] = 10.0f;
        camera->euler.elem[1] = 20.0f;
        camera->euler.elem[2] = 30.0f;

        ret = camera_euler_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_euler = { .elem = { -1.0f, -1.0f, -1.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->euler.elem[0] = 15.0f;
        camera->euler.elem[1] = -25.0f;
        camera->euler.elem[2] = 35.0f;

        ret = camera_euler_get(camera, &out_euler);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(out_euler.elem[0], 15.0f));
        assert(is_equal_float(out_euler.elem[1], -25.0f));
        assert(is_equal_float(out_euler.elem[2], 35.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_position_get(void) {
    {
        // camera_position_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_position = { .elem = { -1.0f, -1.0f, -1.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_position_get_config_set(&config);

        ret = camera_position_get(camera, &out_position);
        assert(CAMERA_BAD_OPERATION == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        // out_position_ は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t out_position = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();

        ret = camera_position_get(NULL, &out_position);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(out_position.elem[0], 1.0f));
        assert(is_equal_float(out_position.elem[1], 2.0f));
        assert(is_equal_float(out_position.elem[2], 3.0f));

        test_camera_config_reset();
    }
    {
        // out_position_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->position.elem[0] = 10.0f;
        camera->position.elem[1] = 20.0f;
        camera->position.elem[2] = 30.0f;

        ret = camera_position_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_position = { .elem = { -1.0f, -1.0f, -1.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->position.elem[0] = 15.0f;
        camera->position.elem[1] = -25.0f;
        camera->position.elem[2] = 35.0f;

        ret = camera_position_get(camera, &out_position);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(out_position.elem[0], 15.0f));
        assert(is_equal_float(out_position.elem[1], -25.0f));
        assert(is_equal_float(out_position.elem[2], 35.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_perspective_matrix_get(void) {
    {
        // camera_perspective_matrix_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t out_mat = { 0 };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_perspective_matrix_get_config_set(&config);

        ret = camera_perspective_matrix_get(camera, &out_mat);
        assert(CAMERA_BAD_OPERATION == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        mat4x4f_t out_mat = { 0 };

        test_camera_config_reset();

        ret = camera_perspective_matrix_get(NULL, &out_mat);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_camera_config_reset();
    }
    {
        // out_mat_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_perspective_matrix_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 無効な frustum -> CAMERA_BAD_OPERATION
        // out_mat_ が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t out_mat = {
            .elem = {
                1.0f,  2.0f,  3.0f,  4.0f,
                5.0f,  6.0f,  7.0f,  8.0f,
                9.0f, 10.0f, 11.0f, 12.0f,
               13.0f, 14.0f, 15.0f, 16.0f
            }
        };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        // camera_create() 直後の frustum は 0 初期化で無効
        ret = camera_perspective_matrix_get(camera, &out_mat);
        assert(CAMERA_BAD_OPERATION == ret);

        assert(is_equal_float(out_mat.elem[0], 1.0f));
        assert(is_equal_float(out_mat.elem[1], 2.0f));
        assert(is_equal_float(out_mat.elem[2], 3.0f));
        assert(is_equal_float(out_mat.elem[3], 4.0f));
        assert(is_equal_float(out_mat.elem[4], 5.0f));
        assert(is_equal_float(out_mat.elem[5], 6.0f));
        assert(is_equal_float(out_mat.elem[6], 7.0f));
        assert(is_equal_float(out_mat.elem[7], 8.0f));
        assert(is_equal_float(out_mat.elem[8], 9.0f));
        assert(is_equal_float(out_mat.elem[9], 10.0f));
        assert(is_equal_float(out_mat.elem[10], 11.0f));
        assert(is_equal_float(out_mat.elem[11], 12.0f));
        assert(is_equal_float(out_mat.elem[12], 13.0f));
        assert(is_equal_float(out_mat.elem[13], 14.0f));
        assert(is_equal_float(out_mat.elem[14], 15.0f));
        assert(is_equal_float(out_mat.elem[15], 16.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_frustum_cache_sync() 失敗 -> そのまま返ること
        // out_mat_ が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t out_mat = {
            .elem = {
                1.0f,  2.0f,  3.0f,  4.0f,
                5.0f,  6.0f,  7.0f,  8.0f,
                9.0f, 10.0f, 11.0f, 12.0f,
               13.0f, 14.0f, 15.0f, 16.0f
            }
        };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->frustum.aspect = 16.0f / 9.0f;
        camera->frustum.fovy = 60.0f;
        camera->frustum.near_clip = 0.1f;
        camera->frustum.far_clip = 100.0f;
        camera->frustum_cache_dirty = true;

        s_test_config_camera_frustum_cache_sync.fail_on_call = 1U;
        s_test_config_camera_frustum_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_perspective_matrix_get(camera, &out_mat);
        assert(CAMERA_RUNTIME_ERROR == ret);

        assert(is_equal_float(out_mat.elem[0], 1.0f));
        assert(is_equal_float(out_mat.elem[1], 2.0f));
        assert(is_equal_float(out_mat.elem[2], 3.0f));
        assert(is_equal_float(out_mat.elem[3], 4.0f));
        assert(is_equal_float(out_mat.elem[4], 5.0f));
        assert(is_equal_float(out_mat.elem[5], 6.0f));
        assert(is_equal_float(out_mat.elem[6], 7.0f));
        assert(is_equal_float(out_mat.elem[7], 8.0f));
        assert(is_equal_float(out_mat.elem[8], 9.0f));
        assert(is_equal_float(out_mat.elem[9], 10.0f));
        assert(is_equal_float(out_mat.elem[10], 11.0f));
        assert(is_equal_float(out_mat.elem[11], 12.0f));
        assert(is_equal_float(out_mat.elem[12], 13.0f));
        assert(is_equal_float(out_mat.elem[13], 14.0f));
        assert(is_equal_float(out_mat.elem[14], 15.0f));
        assert(is_equal_float(out_mat.elem[15], 16.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t out_mat = { 0 };
        mat4x4f_t expected = { 0 };
        const float aspect = 16.0f / 9.0f;
        const float fovy = 60.0f;
        const float near_clip = 0.1f;
        const float far_clip = 100.0f;
        const float dz = far_clip - near_clip;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->frustum.aspect = aspect;
        camera->frustum.fovy = fovy;
        camera->frustum.near_clip = near_clip;
        camera->frustum.far_clip = far_clip;
        camera->frustum_cache_dirty = true;

        ret = camera_perspective_matrix_get(camera, &out_mat);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->frustum_cache_dirty);

        mat4f_identity(&expected);
        expected.elem[5] = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(fovy) * 0.5f);
        expected.elem[0] = expected.elem[5] / aspect;
        expected.elem[10] = -1.0f * (far_clip + near_clip) / dz;
        expected.elem[11] = -2.0f * far_clip * near_clip / dz;
        expected.elem[14] = -1.0f;
        expected.elem[15] = 0.0f;

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera->perspective_matrix.elem[i]));
            assert(is_equal_float(expected.elem[i], out_mat.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_view_matrix_get(void) {
    {
        // camera_view_matrix_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t out_mat = {
            .elem = {
                1.0f,  2.0f,  3.0f,  4.0f,
                5.0f,  6.0f,  7.0f,  8.0f,
                9.0f, 10.0f, 11.0f, 12.0f,
               13.0f, 14.0f, 15.0f, 16.0f
            }
        };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_view_matrix_get_config_set(&config);

        ret = camera_view_matrix_get(camera, &out_mat);
        assert(CAMERA_BAD_OPERATION == ret);

        assert(is_equal_float(out_mat.elem[0], 1.0f));
        assert(is_equal_float(out_mat.elem[1], 2.0f));
        assert(is_equal_float(out_mat.elem[2], 3.0f));
        assert(is_equal_float(out_mat.elem[3], 4.0f));
        assert(is_equal_float(out_mat.elem[4], 5.0f));
        assert(is_equal_float(out_mat.elem[5], 6.0f));
        assert(is_equal_float(out_mat.elem[6], 7.0f));
        assert(is_equal_float(out_mat.elem[7], 8.0f));
        assert(is_equal_float(out_mat.elem[8], 9.0f));
        assert(is_equal_float(out_mat.elem[9], 10.0f));
        assert(is_equal_float(out_mat.elem[10], 11.0f));
        assert(is_equal_float(out_mat.elem[11], 12.0f));
        assert(is_equal_float(out_mat.elem[12], 13.0f));
        assert(is_equal_float(out_mat.elem[13], 14.0f));
        assert(is_equal_float(out_mat.elem[14], 15.0f));
        assert(is_equal_float(out_mat.elem[15], 16.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        // out_mat_ は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        mat4x4f_t out_mat = {
            .elem = {
                1.0f,  2.0f,  3.0f,  4.0f,
                5.0f,  6.0f,  7.0f,  8.0f,
                9.0f, 10.0f, 11.0f, 12.0f,
               13.0f, 14.0f, 15.0f, 16.0f
            }
        };

        test_camera_config_reset();

        ret = camera_view_matrix_get(NULL, &out_mat);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(out_mat.elem[0], 1.0f));
        assert(is_equal_float(out_mat.elem[1], 2.0f));
        assert(is_equal_float(out_mat.elem[2], 3.0f));
        assert(is_equal_float(out_mat.elem[3], 4.0f));
        assert(is_equal_float(out_mat.elem[4], 5.0f));
        assert(is_equal_float(out_mat.elem[5], 6.0f));
        assert(is_equal_float(out_mat.elem[6], 7.0f));
        assert(is_equal_float(out_mat.elem[7], 8.0f));
        assert(is_equal_float(out_mat.elem[8], 9.0f));
        assert(is_equal_float(out_mat.elem[9], 10.0f));
        assert(is_equal_float(out_mat.elem[10], 11.0f));
        assert(is_equal_float(out_mat.elem[11], 12.0f));
        assert(is_equal_float(out_mat.elem[12], 13.0f));
        assert(is_equal_float(out_mat.elem[13], 14.0f));
        assert(is_equal_float(out_mat.elem[14], 15.0f));
        assert(is_equal_float(out_mat.elem[15], 16.0f));

        test_camera_config_reset();
    }
    {
        // out_mat_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_view_matrix_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_posture_cache_sync() 失敗 -> そのまま返ること
        // out_mat_ が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t out_mat = {
            .elem = {
                1.0f,  2.0f,  3.0f,  4.0f,
                5.0f,  6.0f,  7.0f,  8.0f,
                9.0f, 10.0f, 11.0f, 12.0f,
               13.0f, 14.0f, 15.0f, 16.0f
            }
        };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_posture_cache_sync.fail_on_call = 1U;
        s_test_config_camera_posture_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_view_matrix_get(camera, &out_mat);
        assert(CAMERA_RUNTIME_ERROR == ret);

        assert(is_equal_float(out_mat.elem[0], 1.0f));
        assert(is_equal_float(out_mat.elem[1], 2.0f));
        assert(is_equal_float(out_mat.elem[2], 3.0f));
        assert(is_equal_float(out_mat.elem[3], 4.0f));
        assert(is_equal_float(out_mat.elem[4], 5.0f));
        assert(is_equal_float(out_mat.elem[5], 6.0f));
        assert(is_equal_float(out_mat.elem[6], 7.0f));
        assert(is_equal_float(out_mat.elem[7], 8.0f));
        assert(is_equal_float(out_mat.elem[8], 9.0f));
        assert(is_equal_float(out_mat.elem[9], 10.0f));
        assert(is_equal_float(out_mat.elem[10], 11.0f));
        assert(is_equal_float(out_mat.elem[11], 12.0f));
        assert(is_equal_float(out_mat.elem[12], 13.0f));
        assert(is_equal_float(out_mat.elem[13], 14.0f));
        assert(is_equal_float(out_mat.elem[14], 15.0f));
        assert(is_equal_float(out_mat.elem[15], 16.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == false のとき既存 view_matrix がそのまま返ること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t out_mat = { 0 };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->view_matrix.elem[0] = 1.0f;
        camera->view_matrix.elem[1] = 2.0f;
        camera->view_matrix.elem[2] = 3.0f;
        camera->view_matrix.elem[3] = 4.0f;
        camera->view_matrix.elem[4] = 5.0f;
        camera->view_matrix.elem[5] = 6.0f;
        camera->view_matrix.elem[6] = 7.0f;
        camera->view_matrix.elem[7] = 8.0f;
        camera->view_matrix.elem[8] = 9.0f;
        camera->view_matrix.elem[9] = 10.0f;
        camera->view_matrix.elem[10] = 11.0f;
        camera->view_matrix.elem[11] = 12.0f;
        camera->view_matrix.elem[12] = 13.0f;
        camera->view_matrix.elem[13] = 14.0f;
        camera->view_matrix.elem[14] = 15.0f;
        camera->view_matrix.elem[15] = 16.0f;
        camera->posture_cache_dirty = false;

        ret = camera_view_matrix_get(camera, &out_mat);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_mat.elem[0], 1.0f));
        assert(is_equal_float(out_mat.elem[1], 2.0f));
        assert(is_equal_float(out_mat.elem[2], 3.0f));
        assert(is_equal_float(out_mat.elem[3], 4.0f));
        assert(is_equal_float(out_mat.elem[4], 5.0f));
        assert(is_equal_float(out_mat.elem[5], 6.0f));
        assert(is_equal_float(out_mat.elem[6], 7.0f));
        assert(is_equal_float(out_mat.elem[7], 8.0f));
        assert(is_equal_float(out_mat.elem[8], 9.0f));
        assert(is_equal_float(out_mat.elem[9], 10.0f));
        assert(is_equal_float(out_mat.elem[10], 11.0f));
        assert(is_equal_float(out_mat.elem[11], 12.0f));
        assert(is_equal_float(out_mat.elem[12], 13.0f));
        assert(is_equal_float(out_mat.elem[13], 14.0f));
        assert(is_equal_float(out_mat.elem[14], 15.0f));
        assert(is_equal_float(out_mat.elem[15], 16.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == true のとき同期後の view_matrix が返ること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t out_mat = { 0 };
        mat4x4f_t identity = { 0 };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        // camera_create() 直後は euler = {0,0,0}, position = {0,0,0}, posture_cache_dirty = true
        // この場合、同期後の view_matrix は単位行列になる
        ret = camera_view_matrix_get(camera, &out_mat);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        mat4f_identity(&identity);
        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(identity.elem[i], camera->view_matrix.elem[i]));
            assert(is_equal_float(identity.elem[i], out_mat.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_forward_vector_get(void) {
    {
        // camera_forward_vector_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_forward_vector_get_config_set(&config);

        ret = camera_forward_vector_get(camera, &out_vec);
        assert(CAMERA_BAD_OPERATION == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        // out_vec_ は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();

        ret = camera_forward_vector_get(NULL, &out_vec);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        test_camera_config_reset();
    }
    {
        // out_vec_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_forward_vector_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_posture_cache_sync() 失敗 -> そのまま返ること
        // out_vec_ が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_posture_cache_sync.fail_on_call = 1U;
        s_test_config_camera_posture_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_forward_vector_get(camera, &out_vec);
        assert(CAMERA_RUNTIME_ERROR == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == false のとき
        // 既存の camera_to_world_matrix から forward vector を取得できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        mat4f_identity(&camera->camera_to_world_matrix);
        camera->posture_cache_dirty = false;

        ret = camera_forward_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 0.0f));
        assert(is_equal_float(out_vec.elem[1], 0.0f));
        assert(is_equal_float(out_vec.elem[2], -1.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == true のとき
        // camera_create() 直後の姿勢同期後、forward vector は {0, 0, -1} になること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);
        assert(true == camera->posture_cache_dirty);

        ret = camera_forward_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 0.0f));
        assert(is_equal_float(out_vec.elem[1], 0.0f));
        assert(is_equal_float(out_vec.elem[2], -1.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_backward_vector_get(void) {
    {
        // camera_backward_vector_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_backward_vector_get_config_set(&config);

        ret = camera_backward_vector_get(camera, &out_vec);
        assert(CAMERA_BAD_OPERATION == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        // out_vec_ は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();

        ret = camera_backward_vector_get(NULL, &out_vec);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        test_camera_config_reset();
    }
    {
        // out_vec_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_backward_vector_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_posture_cache_sync() 失敗 -> そのまま返ること
        // out_vec_ が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_posture_cache_sync.fail_on_call = 1U;
        s_test_config_camera_posture_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_backward_vector_get(camera, &out_vec);
        assert(CAMERA_RUNTIME_ERROR == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == false のとき
        // 既存の camera_to_world_matrix から backward vector を取得できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        mat4f_identity(&camera->camera_to_world_matrix);
        camera->posture_cache_dirty = false;

        ret = camera_backward_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 0.0f));
        assert(is_equal_float(out_vec.elem[1], 0.0f));
        assert(is_equal_float(out_vec.elem[2], 1.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == true のとき
        // camera_create() 直後の姿勢同期後、backward vector は {0, 0, 1} になること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);
        assert(true == camera->posture_cache_dirty);

        ret = camera_backward_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 0.0f));
        assert(is_equal_float(out_vec.elem[1], 0.0f));
        assert(is_equal_float(out_vec.elem[2], 1.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_right_vector_get(void) {
    {
        // camera_right_vector_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_right_vector_get_config_set(&config);

        ret = camera_right_vector_get(camera, &out_vec);
        assert(CAMERA_BAD_OPERATION == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        // out_vec_ は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();

        ret = camera_right_vector_get(NULL, &out_vec);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        test_camera_config_reset();
    }
    {
        // out_vec_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_right_vector_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_posture_cache_sync() 失敗 -> そのまま返ること
        // out_vec_ が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_posture_cache_sync.fail_on_call = 1U;
        s_test_config_camera_posture_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_right_vector_get(camera, &out_vec);
        assert(CAMERA_RUNTIME_ERROR == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == false のとき
        // 既存の camera_to_world_matrix から right vector を取得できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        mat4f_identity(&camera->camera_to_world_matrix);
        camera->posture_cache_dirty = false;

        ret = camera_right_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 0.0f));
        assert(is_equal_float(out_vec.elem[2], 0.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == true のとき
        // camera_create() 直後の姿勢同期後、right vector は {1, 0, 0} になること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);
        assert(true == camera->posture_cache_dirty);

        ret = camera_right_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 0.0f));
        assert(is_equal_float(out_vec.elem[2], 0.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_left_vector_get(void) {
    {
        // camera_left_vector_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_left_vector_get_config_set(&config);

        ret = camera_left_vector_get(camera, &out_vec);
        assert(CAMERA_BAD_OPERATION == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        // out_vec_ は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();

        ret = camera_left_vector_get(NULL, &out_vec);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        test_camera_config_reset();
    }
    {
        // out_vec_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_left_vector_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_posture_cache_sync() 失敗 -> そのまま返ること
        // out_vec_ が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_posture_cache_sync.fail_on_call = 1U;
        s_test_config_camera_posture_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_left_vector_get(camera, &out_vec);
        assert(CAMERA_RUNTIME_ERROR == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == false のとき
        // 既存の camera_to_world_matrix から left vector を取得できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        mat4f_identity(&camera->camera_to_world_matrix);
        camera->posture_cache_dirty = false;

        ret = camera_left_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], -1.0f));
        assert(is_equal_float(out_vec.elem[1], 0.0f));
        assert(is_equal_float(out_vec.elem[2], 0.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == true のとき
        // camera_create() 直後の姿勢同期後、left vector は {-1, 0, 0} になること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);
        assert(true == camera->posture_cache_dirty);

        ret = camera_left_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], -1.0f));
        assert(is_equal_float(out_vec.elem[1], 0.0f));
        assert(is_equal_float(out_vec.elem[2], 0.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_up_vector_get(void) {
    {
        // camera_up_vector_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_up_vector_get_config_set(&config);

        ret = camera_up_vector_get(camera, &out_vec);
        assert(CAMERA_BAD_OPERATION == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        // out_vec_ は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();

        ret = camera_up_vector_get(NULL, &out_vec);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        test_camera_config_reset();
    }
    {
        // out_vec_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_up_vector_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_posture_cache_sync() 失敗 -> そのまま返ること
        // out_vec_ が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_posture_cache_sync.fail_on_call = 1U;
        s_test_config_camera_posture_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_up_vector_get(camera, &out_vec);
        assert(CAMERA_RUNTIME_ERROR == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == false のとき
        // 既存の camera_to_world_matrix から up vector を取得できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        mat4f_identity(&camera->camera_to_world_matrix);
        camera->posture_cache_dirty = false;

        ret = camera_up_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 0.0f));
        assert(is_equal_float(out_vec.elem[1], 1.0f));
        assert(is_equal_float(out_vec.elem[2], 0.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == true のとき
        // camera_create() 直後の姿勢同期後、up vector は {0, 1, 0} になること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);
        assert(true == camera->posture_cache_dirty);

        ret = camera_up_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 0.0f));
        assert(is_equal_float(out_vec.elem[1], 1.0f));
        assert(is_equal_float(out_vec.elem[2], 0.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_down_vector_get(void) {
    {
        // camera_down_vector_get() 冒頭で強制的に CAMERA_BAD_OPERATION を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };
        test_call_control_t config = {0};

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();
        test_call_control_reset(&config);

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_camera_down_vector_get_config_set(&config);

        ret = camera_down_vector_get(camera, &out_vec);
        assert(CAMERA_BAD_OPERATION == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        // out_vec_ は変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();

        ret = camera_down_vector_get(NULL, &out_vec);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        test_camera_config_reset();
    }
    {
        // out_vec_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_down_vector_get(camera, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_posture_cache_sync() 失敗 -> そのまま返ること
        // out_vec_ が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_posture_cache_sync.fail_on_call = 1U;
        s_test_config_camera_posture_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_down_vector_get(camera, &out_vec);
        assert(CAMERA_RUNTIME_ERROR == ret);

        assert(is_equal_float(out_vec.elem[0], 1.0f));
        assert(is_equal_float(out_vec.elem[1], 2.0f));
        assert(is_equal_float(out_vec.elem[2], 3.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == false のとき
        // 既存の camera_to_world_matrix から down vector を取得できること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        mat4f_identity(&camera->camera_to_world_matrix);
        camera->posture_cache_dirty = false;

        ret = camera_down_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 0.0f));
        assert(is_equal_float(out_vec.elem[1], -1.0f));
        assert(is_equal_float(out_vec.elem[2], 0.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: posture_cache_dirty == true のとき
        // camera_create() 直後の姿勢同期後、down vector は {0, -1, 0} になること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t out_vec = { .elem = { 0.0f, 0.0f, 0.0f } };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);
        assert(true == camera->posture_cache_dirty);

        ret = camera_down_vector_get(camera, &out_vec);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->posture_cache_dirty);

        assert(is_equal_float(out_vec.elem[0], 0.0f));
        assert(is_equal_float(out_vec.elem[1], -1.0f));
        assert(is_equal_float(out_vec.elem[2], 0.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_frustum_cache_sync(void) {
    {
        // camera_frustum_cache_sync() 冒頭で強制的に CAMERA_RUNTIME_ERROR を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_frustum_cache_sync.fail_on_call = 1U;
        s_test_config_camera_frustum_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_frustum_cache_sync(camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_camera_config_reset();

        ret = camera_frustum_cache_sync(NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_camera_config_reset();
    }
    {
        // 無効な frustum -> CAMERA_BAD_OPERATION
        // matrix と dirty flag が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t before = { 0 };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        // 無効状態を作る
        camera->frustum.aspect = 16.0f / 9.0f;
        camera->frustum.fovy = 60.0f;
        camera->frustum.near_clip = 10.0f;
        camera->frustum.far_clip = 1.0f;   // near >= far で無効
        camera->frustum_cache_dirty = true;

        camera->perspective_matrix.elem[0] = 1.0f;
        camera->perspective_matrix.elem[1] = 2.0f;
        camera->perspective_matrix.elem[2] = 3.0f;
        camera->perspective_matrix.elem[3] = 4.0f;
        camera->perspective_matrix.elem[4] = 5.0f;
        camera->perspective_matrix.elem[5] = 6.0f;
        camera->perspective_matrix.elem[6] = 7.0f;
        camera->perspective_matrix.elem[7] = 8.0f;
        camera->perspective_matrix.elem[8] = 9.0f;
        camera->perspective_matrix.elem[9] = 10.0f;
        camera->perspective_matrix.elem[10] = 11.0f;
        camera->perspective_matrix.elem[11] = 12.0f;
        camera->perspective_matrix.elem[12] = 13.0f;
        camera->perspective_matrix.elem[13] = 14.0f;
        camera->perspective_matrix.elem[14] = 15.0f;
        camera->perspective_matrix.elem[15] = 16.0f;
        mat4f_copy(&camera->perspective_matrix, &before);

        ret = camera_frustum_cache_sync(camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(true == camera->frustum_cache_dirty);

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(before.elem[i], camera->perspective_matrix.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: frustum_cache_dirty == false のとき
        // perspective_matrix は更新されず、そのまま維持されること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t before = { 0 };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->frustum.aspect = 16.0f / 9.0f;
        camera->frustum.fovy = 60.0f;
        camera->frustum.near_clip = 0.1f;
        camera->frustum.far_clip = 100.0f;
        camera->frustum_cache_dirty = false;

        camera->perspective_matrix.elem[0] = 1.0f;
        camera->perspective_matrix.elem[1] = 2.0f;
        camera->perspective_matrix.elem[2] = 3.0f;
        camera->perspective_matrix.elem[3] = 4.0f;
        camera->perspective_matrix.elem[4] = 5.0f;
        camera->perspective_matrix.elem[5] = 6.0f;
        camera->perspective_matrix.elem[6] = 7.0f;
        camera->perspective_matrix.elem[7] = 8.0f;
        camera->perspective_matrix.elem[8] = 9.0f;
        camera->perspective_matrix.elem[9] = 10.0f;
        camera->perspective_matrix.elem[10] = 11.0f;
        camera->perspective_matrix.elem[11] = 12.0f;
        camera->perspective_matrix.elem[12] = 13.0f;
        camera->perspective_matrix.elem[13] = 14.0f;
        camera->perspective_matrix.elem[14] = 15.0f;
        camera->perspective_matrix.elem[15] = 16.0f;
        mat4f_copy(&camera->perspective_matrix, &before);

        ret = camera_frustum_cache_sync(camera);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->frustum_cache_dirty);

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(before.elem[i], camera->perspective_matrix.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: frustum_cache_dirty == true のとき
        // perspective_matrix が更新され、dirty flag が false になること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t expected = { 0 };
        const float aspect = 16.0f / 9.0f;
        const float fovy = 60.0f;
        const float near_clip = 0.1f;
        const float far_clip = 100.0f;
        const float dz = far_clip - near_clip;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->frustum.aspect = aspect;
        camera->frustum.fovy = fovy;
        camera->frustum.near_clip = near_clip;
        camera->frustum.far_clip = far_clip;
        camera->frustum_cache_dirty = true;

        ret = camera_frustum_cache_sync(camera);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->frustum_cache_dirty);

        mat4f_identity(&expected);
        expected.elem[5] = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(fovy) * 0.5f);
        expected.elem[0] = expected.elem[5] / aspect;
        expected.elem[10] = -1.0f * (far_clip + near_clip) / dz;
        expected.elem[11] = -2.0f * far_clip * near_clip / dz;
        expected.elem[14] = -1.0f;
        expected.elem[15] = 0.0f;

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera->perspective_matrix.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_posture_cache_sync(void) {
    {
        // camera_frustum_cache_sync() 冒頭で強制的に CAMERA_RUNTIME_ERROR を返させる
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_frustum_cache_sync.fail_on_call = 1U;
        s_test_config_camera_frustum_cache_sync.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = camera_frustum_cache_sync(camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // camera_ == NULL -> CAMERA_INVALID_ARGUMENT
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_camera_config_reset();

        ret = camera_frustum_cache_sync(NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_camera_config_reset();
    }
    {
        // 無効な frustum -> CAMERA_BAD_OPERATION
        // matrix と dirty flag が変更されないこと
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t before = { 0 };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        // 無効状態を作る
        camera->frustum.aspect = 16.0f / 9.0f;
        camera->frustum.fovy = 60.0f;
        camera->frustum.near_clip = 10.0f;
        camera->frustum.far_clip = 1.0f;   // near >= far で無効
        camera->frustum_cache_dirty = true;

        camera->perspective_matrix.elem[0] = 1.0f;
        camera->perspective_matrix.elem[1] = 2.0f;
        camera->perspective_matrix.elem[2] = 3.0f;
        camera->perspective_matrix.elem[3] = 4.0f;
        camera->perspective_matrix.elem[4] = 5.0f;
        camera->perspective_matrix.elem[5] = 6.0f;
        camera->perspective_matrix.elem[6] = 7.0f;
        camera->perspective_matrix.elem[7] = 8.0f;
        camera->perspective_matrix.elem[8] = 9.0f;
        camera->perspective_matrix.elem[9] = 10.0f;
        camera->perspective_matrix.elem[10] = 11.0f;
        camera->perspective_matrix.elem[11] = 12.0f;
        camera->perspective_matrix.elem[12] = 13.0f;
        camera->perspective_matrix.elem[13] = 14.0f;
        camera->perspective_matrix.elem[14] = 15.0f;
        camera->perspective_matrix.elem[15] = 16.0f;
        mat4f_copy(&camera->perspective_matrix, &before);

        ret = camera_frustum_cache_sync(camera);
        assert(CAMERA_BAD_OPERATION == ret);
        assert(true == camera->frustum_cache_dirty);

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(before.elem[i], camera->perspective_matrix.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: frustum_cache_dirty == false のとき
        // perspective_matrix は更新されず、そのまま維持されること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t before = { 0 };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->frustum.aspect = 16.0f / 9.0f;
        camera->frustum.fovy = 60.0f;
        camera->frustum.near_clip = 0.1f;
        camera->frustum.far_clip = 100.0f;
        camera->frustum_cache_dirty = false;

        camera->perspective_matrix.elem[0] = 1.0f;
        camera->perspective_matrix.elem[1] = 2.0f;
        camera->perspective_matrix.elem[2] = 3.0f;
        camera->perspective_matrix.elem[3] = 4.0f;
        camera->perspective_matrix.elem[4] = 5.0f;
        camera->perspective_matrix.elem[5] = 6.0f;
        camera->perspective_matrix.elem[6] = 7.0f;
        camera->perspective_matrix.elem[7] = 8.0f;
        camera->perspective_matrix.elem[8] = 9.0f;
        camera->perspective_matrix.elem[9] = 10.0f;
        camera->perspective_matrix.elem[10] = 11.0f;
        camera->perspective_matrix.elem[11] = 12.0f;
        camera->perspective_matrix.elem[12] = 13.0f;
        camera->perspective_matrix.elem[13] = 14.0f;
        camera->perspective_matrix.elem[14] = 15.0f;
        camera->perspective_matrix.elem[15] = 16.0f;
        mat4f_copy(&camera->perspective_matrix, &before);

        ret = camera_frustum_cache_sync(camera);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->frustum_cache_dirty);

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(before.elem[i], camera->perspective_matrix.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // posture_cache_dirty == true かつ view_matrix_update() が false を返すと
        // CAMERA_RUNTIME_ERROR で cleanup に入ること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t before_view = { 0 };

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        // posture_cache_dirty == true の経路に入れる
        camera->euler.elem[0] = 0.0f;
        camera->euler.elem[1] = 0.0f;
        camera->euler.elem[2] = 0.0f;
        camera->position.elem[0] = 10.0f;
        camera->position.elem[1] = 20.0f;
        camera->position.elem[2] = 30.0f;
        camera->posture_cache_dirty = true;

        // view_matrix が失敗時に変更されないことも確認する
        camera->view_matrix.elem[0] = 16.0f;
        camera->view_matrix.elem[1] = 15.0f;
        camera->view_matrix.elem[2] = 14.0f;
        camera->view_matrix.elem[3] = 13.0f;
        camera->view_matrix.elem[4] = 12.0f;
        camera->view_matrix.elem[5] = 11.0f;
        camera->view_matrix.elem[6] = 10.0f;
        camera->view_matrix.elem[7] = 9.0f;
        camera->view_matrix.elem[8] = 8.0f;
        camera->view_matrix.elem[9] = 7.0f;
        camera->view_matrix.elem[10] = 6.0f;
        camera->view_matrix.elem[11] = 5.0f;
        camera->view_matrix.elem[12] = 4.0f;
        camera->view_matrix.elem[13] = 3.0f;
        camera->view_matrix.elem[14] = 2.0f;
        camera->view_matrix.elem[15] = 1.0f;
        mat4f_copy(&camera->view_matrix, &before_view);

        // ここで inner branch を直接踏ませる
        s_test_config_view_matrix_update.fail_on_call = 1U;
        s_test_config_view_matrix_update.forced_result = false;

        ret = camera_posture_cache_sync(camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        // cleanup に飛ぶので dirty は false にならない
        assert(true == camera->posture_cache_dirty);

        // view_matrix_update() は false を返しただけなので、view_matrix は不変
        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(before_view.elem[i], camera->view_matrix.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
    {
        // 正常系: frustum_cache_dirty == true のとき
        // perspective_matrix が更新され、dirty flag が false になること
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        mat4x4f_t expected = { 0 };
        const float aspect = 16.0f / 9.0f;
        const float fovy = 60.0f;
        const float near_clip = 0.1f;
        const float far_clip = 100.0f;
        const float dz = far_clip - near_clip;

        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_string_config_reset();

        assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

        ret = camera_create("main_camera", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        camera->frustum.aspect = aspect;
        camera->frustum.fovy = fovy;
        camera->frustum.near_clip = near_clip;
        camera->frustum.far_clip = far_clip;
        camera->frustum_cache_dirty = true;

        ret = camera_frustum_cache_sync(camera);
        assert(CAMERA_SUCCESS == ret);
        assert(false == camera->frustum_cache_dirty);

        mat4f_identity(&expected);
        expected.elem[5] = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(fovy) * 0.5f);
        expected.elem[0] = expected.elem[5] / aspect;
        expected.elem[10] = -1.0f * (far_clip + near_clip) / dz;
        expected.elem[11] = -2.0f * far_clip * near_clip / dz;
        expected.elem[14] = -1.0f;
        expected.elem[15] = 0.0f;

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera->perspective_matrix.elem[i]));
        }

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_choco_string_config_reset();
        test_camera_memory_config_reset();
        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_perspective_matrix_update(void) {
    {
        // 正常系: 一般的な視錐台設定で透視投影行列が正しく更新されること
        camera_t camera = { 0 };
        mat4x4f_t expected = { 0 };
        const float aspect = 16.0f / 9.0f;
        const float fovy = 60.0f;
        const float near_clip = 0.1f;
        const float far_clip = 100.0f;
        const float dz = far_clip - near_clip;

        camera.frustum.aspect = aspect;
        camera.frustum.fovy = fovy;
        camera.frustum.near_clip = near_clip;
        camera.frustum.far_clip = far_clip;

        // 事前にダミー値を入れて、全要素が更新されることを見やすくする
        for(int i = 0; i < 16; ++i) {
            camera.perspective_matrix.elem[i] = -999.0f;
        }

        perspective_matrix_update(&camera);

        mat4f_identity(&expected);
        expected.elem[5] = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(fovy) * 0.5f);
        expected.elem[0] = expected.elem[5] / aspect;
        expected.elem[10] = -1.0f * (far_clip + near_clip) / dz;
        expected.elem[11] = -2.0f * far_clip * near_clip / dz;
        expected.elem[14] = -1.0f;
        expected.elem[15] = 0.0f;

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera.perspective_matrix.elem[i]));
        }
    }
    {
        // 正常系: 別パラメータでも正しく更新されること
        camera_t camera = { 0 };
        mat4x4f_t expected = { 0 };
        const float aspect = 1.0f;
        const float fovy = 90.0f;
        const float near_clip = 1.0f;
        const float far_clip = 10.0f;
        const float dz = far_clip - near_clip;

        camera.frustum.aspect = aspect;
        camera.frustum.fovy = fovy;
        camera.frustum.near_clip = near_clip;
        camera.frustum.far_clip = far_clip;

        for(int i = 0; i < 16; ++i) {
            camera.perspective_matrix.elem[i] = 123.0f;
        }

        perspective_matrix_update(&camera);

        mat4f_identity(&expected);
        expected.elem[5] = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(fovy) * 0.5f);
        expected.elem[0] = expected.elem[5] / aspect;
        expected.elem[10] = -1.0f * (far_clip + near_clip) / dz;
        expected.elem[11] = -2.0f * far_clip * near_clip / dz;
        expected.elem[14] = -1.0f;
        expected.elem[15] = 0.0f;

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera.perspective_matrix.elem[i]));
        }
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_to_world_matrix_update(void) {
    {
        // 正常系: euler = {0,0,0}, position = {0,0,0} -> 単位行列
        camera_t camera = { 0 };
        mat4x4f_t expected = { 0 };

        camera.euler.elem[0] = 0.0f;
        camera.euler.elem[1] = 0.0f;
        camera.euler.elem[2] = 0.0f;
        camera.position.elem[0] = 0.0f;
        camera.position.elem[1] = 0.0f;
        camera.position.elem[2] = 0.0f;

        for(int i = 0; i < 16; ++i) {
            camera.camera_to_world_matrix.elem[i] = -999.0f;
        }

        camera_to_world_matrix_update(&camera);

        mat4f_identity(&expected);
        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera.camera_to_world_matrix.elem[i]));
        }
    }
    {
        // 正常系: 回転なし、平行移動のみ -> translation 行列
        camera_t camera = { 0 };
        mat4x4f_t expected = { 0 };
        vec3f_t position = { .elem = { 10.0f, 20.0f, 30.0f } };

        camera.euler.elem[0] = 0.0f;
        camera.euler.elem[1] = 0.0f;
        camera.euler.elem[2] = 0.0f;
        camera.position.elem[0] = position.elem[0];
        camera.position.elem[1] = position.elem[1];
        camera.position.elem[2] = position.elem[2];

        for(int i = 0; i < 16; ++i) {
            camera.camera_to_world_matrix.elem[i] = 123.0f;
        }

        camera_to_world_matrix_update(&camera);

        mat4f_translation(&position, &expected);
        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera.camera_to_world_matrix.elem[i]));
        }
    }
    {
        // 正常系: 回転 + 平行移動 -> trans * rot の順で更新されること
        camera_t camera = { 0 };
        mat4x4f_t rot = { 0 };
        mat4x4f_t trans = { 0 };
        mat4x4f_t expected = { 0 };
        const float x_deg = 10.0f;
        const float y_deg = 20.0f;
        const float z_deg = 30.0f;
        vec3f_t position = { .elem = { 1.5f, -2.5f, 3.5f } };

        camera.euler.elem[0] = x_deg;
        camera.euler.elem[1] = y_deg;
        camera.euler.elem[2] = z_deg;
        camera.position.elem[0] = position.elem[0];
        camera.position.elem[1] = position.elem[1];
        camera.position.elem[2] = position.elem[2];

        for(int i = 0; i < 16; ++i) {
            camera.camera_to_world_matrix.elem[i] = 456.0f;
        }

        camera_to_world_matrix_update(&camera);

        mat4f_rot_xyz(
            CHOCO_DEG_TO_RAD(x_deg),
            CHOCO_DEG_TO_RAD(y_deg),
            CHOCO_DEG_TO_RAD(z_deg),
            &rot
        );
        mat4f_translation(&position, &trans);
        mat4f_mul(&trans, &rot, &expected);

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera.camera_to_world_matrix.elem[i]));
        }
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_view_matrix_update(void) {
    {
        // view_matrix_update() 冒頭で強制的に false を返させる
        camera_t camera = { 0 };
        const bool ret = false;

        test_camera_config_reset();

        camera.view_matrix.elem[0] = 1.0f;
        camera.view_matrix.elem[1] = 2.0f;
        camera.view_matrix.elem[2] = 3.0f;
        camera.view_matrix.elem[3] = 4.0f;
        camera.view_matrix.elem[4] = 5.0f;
        camera.view_matrix.elem[5] = 6.0f;
        camera.view_matrix.elem[6] = 7.0f;
        camera.view_matrix.elem[7] = 8.0f;
        camera.view_matrix.elem[8] = 9.0f;
        camera.view_matrix.elem[9] = 10.0f;
        camera.view_matrix.elem[10] = 11.0f;
        camera.view_matrix.elem[11] = 12.0f;
        camera.view_matrix.elem[12] = 13.0f;
        camera.view_matrix.elem[13] = 14.0f;
        camera.view_matrix.elem[14] = 15.0f;
        camera.view_matrix.elem[15] = 16.0f;

        s_test_config_view_matrix_update.fail_on_call = 1U;
        s_test_config_view_matrix_update.forced_result = false;

        const bool actual = view_matrix_update(&camera);
        assert(ret == actual);

        // 強制返却時は view_matrix が変更されないこと
        assert(is_equal_float(camera.view_matrix.elem[0], 1.0f));
        assert(is_equal_float(camera.view_matrix.elem[1], 2.0f));
        assert(is_equal_float(camera.view_matrix.elem[2], 3.0f));
        assert(is_equal_float(camera.view_matrix.elem[3], 4.0f));
        assert(is_equal_float(camera.view_matrix.elem[4], 5.0f));
        assert(is_equal_float(camera.view_matrix.elem[5], 6.0f));
        assert(is_equal_float(camera.view_matrix.elem[6], 7.0f));
        assert(is_equal_float(camera.view_matrix.elem[7], 8.0f));
        assert(is_equal_float(camera.view_matrix.elem[8], 9.0f));
        assert(is_equal_float(camera.view_matrix.elem[9], 10.0f));
        assert(is_equal_float(camera.view_matrix.elem[10], 11.0f));
        assert(is_equal_float(camera.view_matrix.elem[11], 12.0f));
        assert(is_equal_float(camera.view_matrix.elem[12], 13.0f));
        assert(is_equal_float(camera.view_matrix.elem[13], 14.0f));
        assert(is_equal_float(camera.view_matrix.elem[14], 15.0f));
        assert(is_equal_float(camera.view_matrix.elem[15], 16.0f));

        test_camera_config_reset();
    }
    {
        // 正常系: camera_to_world_matrix が単位行列なら view_matrix も単位行列になること
        camera_t camera = { 0 };
        mat4x4f_t expected = { 0 };

        test_camera_config_reset();

        mat4f_identity(&camera.camera_to_world_matrix);

        const bool ret = view_matrix_update(&camera);
        assert(true == ret);

        mat4f_identity(&expected);
        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera.view_matrix.elem[i]));
        }

        test_camera_config_reset();
    }
    {
        // 正常系: 平行移動行列の逆行列が view_matrix に設定されること
        camera_t camera = { 0 };
        mat4x4f_t expected = { 0 };
        vec3f_t position = { .elem = { 10.0f, 20.0f, 30.0f } };

        test_camera_config_reset();

        mat4f_translation(&position, &camera.camera_to_world_matrix);

        const bool ret = view_matrix_update(&camera);
        assert(true == ret);

        mat4f_identity(&expected);
        expected.elem[3] = -10.0f;
        expected.elem[7] = -20.0f;
        expected.elem[11] = -30.0f;

        for(int i = 0; i < 16; ++i) {
            assert(is_equal_float(expected.elem[i], camera.view_matrix.elem[i]));
        }

        test_camera_config_reset();
    }
    {
        // 異常系: 特異行列なら false を返し、view_matrix は変更されないこと
        camera_t camera = { 0 };

        test_camera_config_reset();

        // 零行列は特異
        for(int i = 0; i < 16; ++i) {
            camera.camera_to_world_matrix.elem[i] = 0.0f;
        }

        camera.view_matrix.elem[0] = 1.0f;
        camera.view_matrix.elem[1] = 2.0f;
        camera.view_matrix.elem[2] = 3.0f;
        camera.view_matrix.elem[3] = 4.0f;
        camera.view_matrix.elem[4] = 5.0f;
        camera.view_matrix.elem[5] = 6.0f;
        camera.view_matrix.elem[6] = 7.0f;
        camera.view_matrix.elem[7] = 8.0f;
        camera.view_matrix.elem[8] = 9.0f;
        camera.view_matrix.elem[9] = 10.0f;
        camera.view_matrix.elem[10] = 11.0f;
        camera.view_matrix.elem[11] = 12.0f;
        camera.view_matrix.elem[12] = 13.0f;
        camera.view_matrix.elem[13] = 14.0f;
        camera.view_matrix.elem[14] = 15.0f;
        camera.view_matrix.elem[15] = 16.0f;

        const bool ret = view_matrix_update(&camera);
        assert(false == ret);

        assert(is_equal_float(camera.view_matrix.elem[0], 1.0f));
        assert(is_equal_float(camera.view_matrix.elem[1], 2.0f));
        assert(is_equal_float(camera.view_matrix.elem[2], 3.0f));
        assert(is_equal_float(camera.view_matrix.elem[3], 4.0f));
        assert(is_equal_float(camera.view_matrix.elem[4], 5.0f));
        assert(is_equal_float(camera.view_matrix.elem[5], 6.0f));
        assert(is_equal_float(camera.view_matrix.elem[6], 7.0f));
        assert(is_equal_float(camera.view_matrix.elem[7], 8.0f));
        assert(is_equal_float(camera.view_matrix.elem[8], 9.0f));
        assert(is_equal_float(camera.view_matrix.elem[9], 10.0f));
        assert(is_equal_float(camera.view_matrix.elem[10], 11.0f));
        assert(is_equal_float(camera.view_matrix.elem[11], 12.0f));
        assert(is_equal_float(camera.view_matrix.elem[12], 13.0f));
        assert(is_equal_float(camera.view_matrix.elem[13], 14.0f));
        assert(is_equal_float(camera.view_matrix.elem[14], 15.0f));
        assert(is_equal_float(camera.view_matrix.elem[15], 16.0f));

        test_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_is_valid_frustum(void) {
    {
        // is_valid_frustum() 冒頭で強制的に true を返させる
        viewing_frustum_t frustum = {
            .aspect = 0.0f,
            .fovy = 0.0f,
            .near_clip = 0.0f,
            .far_clip = 0.0f
        };

        test_camera_config_reset();

        s_test_config_is_valid_frustum.fail_on_call = 1U;
        s_test_config_is_valid_frustum.forced_result = true;

        assert(true == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // frustum_ == NULL -> false
        test_camera_config_reset();

        assert(false == is_valid_frustum(NULL));

        test_camera_config_reset();
    }
    {
        // near_clip >= far_clip -> false (等しい場合)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 1.0f,
            .far_clip = 1.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // near_clip >= far_clip -> false (near の方が大きい場合)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 10.0f,
            .far_clip = 1.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // aspect <= 0.0f -> false (0.0f)
        viewing_frustum_t frustum = {
            .aspect = 0.0f,
            .fovy = 60.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // aspect <= 0.0f -> false (負値)
        viewing_frustum_t frustum = {
            .aspect = -1.0f,
            .fovy = 60.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // fovy <= 0.0f -> false (0.0f)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 0.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // fovy <= 0.0f -> false (負値)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = -1.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // fovy >= 180.0f -> false (180.0f ちょうど)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 180.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // fovy >= 180.0f -> false (180.0f 超過)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 181.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // near_clip <= 0.0f -> false (0.0f)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 0.0f,
            .far_clip = 100.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // near_clip <= 0.0f -> false (負値)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = -0.1f,
            .far_clip = 100.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // far_clip <= 0.0f -> false (0.0f)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 0.1f,
            .far_clip = 0.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // far_clip <= 0.0f -> false (負値)
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 0.1f,
            .far_clip = -1.0f
        };

        test_camera_config_reset();

        assert(false == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // 正常系: 一般的なパラメータ
        viewing_frustum_t frustum = {
            .aspect = 16.0f / 9.0f,
            .fovy = 60.0f,
            .near_clip = 0.1f,
            .far_clip = 100.0f
        };

        test_camera_config_reset();

        assert(true == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
    {
        // 正常系: 境界近傍 (fovy が 180 未満, near < far)
        viewing_frustum_t frustum = {
            .aspect = 1.0f,
            .fovy = 179.999f,
            .near_clip = 0.0001f,
            .far_clip = 0.0002f
        };

        test_camera_config_reset();

        assert(true == is_valid_frustum(&frustum));

        test_camera_config_reset();
    }
}
#endif
