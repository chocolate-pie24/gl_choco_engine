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
#include "engine/systems/camera_system/camera/camera.h"

#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/camera_system/camera_core/camera_err_utils.h"
#include "engine/systems/camera_system/camera_core/camera_memory.h"
#include "engine/systems/camera_system/camera_core/camera_types.h"

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

    tmp_camera->euler = vec3f_initialize(0.0f, 0.0f, 0.0f);
    tmp_camera->position = vec3f_initialize(0.0f, 0.0f, 0.0f);

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

camera_result_t camera_viewing_frustum_update(camera_t* camera_, float fovy_, float aspect_, float near_clip_, float far_clip_) {
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

camera_result_t camera_euler_update(camera_t* camera_, vec3f_t euler_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_euler_update", "camera_")

    camera_->euler = euler_;
    camera_->posture_cache_dirty = true;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_position_update(camera_t* camera_, vec3f_t position_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_position_update", "camera_")

    camera_->position = position_;
    camera_->posture_cache_dirty = true;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_euler_get(const camera_t* camera_, vec3f_t* out_euler_) {
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
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t v = { 0 };  // ワールド座標系でカメラを前方に移動させるための方向ベクトル

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_forward_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_forward_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_forward_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ前方の単位ベクトル[0, 0, -1, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    v.elem[0] = -1.0f * camera_->camera_to_world_matrix.elem[2];
    v.elem[1] = -1.0f * camera_->camera_to_world_matrix.elem[6];
    v.elem[2] = -1.0f * camera_->camera_to_world_matrix.elem[10];
    v = vec3f_normalize(v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_backward_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t v = { 0 };  // ワールド座標系でカメラを後方に移動させるための方向ベクトル

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_backward_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_backward_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_backward_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ後方の単位ベクトル[0, 0, 1, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    v.elem[0] = camera_->camera_to_world_matrix.elem[2];
    v.elem[1] = camera_->camera_to_world_matrix.elem[6];
    v.elem[2] = camera_->camera_to_world_matrix.elem[10];
    v = vec3f_normalize(v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_right_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t v = { 0 };  // ワールド座標系でカメラを右に移動させるための方向ベクトル

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_right_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_right_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_right_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ右方向の単位ベクトル[1, 0, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    v.elem[0] = camera_->camera_to_world_matrix.elem[0];
    v.elem[1] = camera_->camera_to_world_matrix.elem[4];
    v.elem[2] = camera_->camera_to_world_matrix.elem[8];
    v = vec3f_normalize(v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_left_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t v = { 0 };  // ワールド座標系でカメラを左に移動させるための方向ベクトル

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_left_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_left_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_left_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ左方向の単位ベクトル[-1, 0, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    v.elem[0] = -1.0f * camera_->camera_to_world_matrix.elem[0];
    v.elem[1] = -1.0f * camera_->camera_to_world_matrix.elem[4];
    v.elem[2] = -1.0f * camera_->camera_to_world_matrix.elem[8];
    v = vec3f_normalize(v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_up_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t v = { 0 };  // ワールド座標系でカメラを上に移動させるための方向ベクトル

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_up_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_up_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_up_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ上方向の単位ベクトル[0, 1, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    v.elem[0] = camera_->camera_to_world_matrix.elem[1];
    v.elem[1] = camera_->camera_to_world_matrix.elem[5];
    v.elem[2] = camera_->camera_to_world_matrix.elem[9];
    v = vec3f_normalize(v);

    out_vec_->elem[0] = v.elem[0];
    out_vec_->elem[1] = v.elem[1];
    out_vec_->elem[2] = v.elem[2];

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_down_vector_get(camera_t* camera_, vec3f_t* out_vec_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t v = { 0 };  // ワールド座標系でカメラを下に移動させるための方向ベクトル

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_down_vector_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vec_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_down_vector_get", "out_vec_")

    ret = camera_posture_cache_sync(camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("camera_down_vector_get(%s) - Failed to sync camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }
    // カメラ座標系からワールド座標系への変換行列に対して、カメラ座標系におけるカメラ下方向の単位ベクトル[0, -1, 0, 0]を掛けて得られる値をカメラワールド座標に加算すれば新しいカメラ座標になる。
    v.elem[0] = -1.0f * camera_->camera_to_world_matrix.elem[1];
    v.elem[1] = -1.0f * camera_->camera_to_world_matrix.elem[5];
    v.elem[2] = -1.0f * camera_->camera_to_world_matrix.elem[9];
    v = vec3f_normalize(v);

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
    mat4f_translation(camera_->position, &trans); // ある座標をtranslate分平行移動する行列 = translate分座標が増える = カメラ->ワールド座標系への変換行列

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
