/**
 * @file camera.c
 * @author chocolate-pie24
 * @brief カメラモジュール実装
 *
 * @version 0.1
 * @date 2026-03-09
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdbool.h>

#include "engine/camera/camera.h"

#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/containers/choco_string.h"

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
 * - Roll: Z軸回りの回転
 * - Pitch: X軸回りの回転
 * - Yaw: Y軸回りの回転
 * - カメラ前方方向: Z軸マイナス方向
 *
 * @todo 不要な行列計算を省くため、以下を検討する
 * - view_dirtyとview_matrixキャッシュを内部で持ち、view_dirty == falseで計算を行わない
 * - projection_dirtyとperspective_matrixキャッシュを内部で持ち、projection_dirty == falseで計算を行わない
 */
struct camera {
    float speed;                /**< カメラスピード */
    vec3f_t euler;              /**< カメラ姿勢オイラー角(degree) */
    vec3f_t position;           /**< カメラ位置 */

    viewing_frustum_t frustum;  /**< 視錐台パラメータ */

    choco_string_t* name;       /**< カメラ名称文字列 */
};

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コード(成功)文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(無効な引数)文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用範囲上限超過) */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(内部データ破損) */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コード(未定義エラー)文字列 */

static const char* rslt_to_str(camera_result_t rslt_);
static camera_result_t rslt_convert_choco_memory(memory_system_result_t rslt_);
static camera_result_t rslt_convert_choco_string(choco_string_result_t rslt_);
static bool is_valid_frustum(const viewing_frustum_t* frustum_);

camera_result_t camera_create(const char* name_, camera_t** out_camera_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    camera_t* tmp_camera = NULL;
    memory_system_result_t mem_ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t string_ret = CHOCO_STRING_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_create", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_camera_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_create", "out_camera_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_camera_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_create", "*out_camera_")

    mem_ret = memory_system_allocate(sizeof(camera_t), MEMORY_TAG_CAMERA, (void**)&tmp_camera);
    if(MEMORY_SYSTEM_SUCCESS != mem_ret) {
        ret = rslt_convert_choco_memory(mem_ret);
        ERROR_MESSAGE("camera_create(%s) - Failed to allocate memory for camera.", rslt_to_str(ret));
        goto cleanup;
    }
    tmp_camera->name = NULL;

    string_ret = choco_string_create_from_c_string(&tmp_camera->name, name_);
    if(CHOCO_STRING_SUCCESS != string_ret) {
        ret = rslt_convert_choco_string(string_ret);
        ERROR_MESSAGE("camera_create(%s) - Failed to create string for camera name.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp_camera->speed = 0.0f;
    tmp_camera->frustum.aspect = 0.0f;
    tmp_camera->frustum.far_clip = 0.0f;
    tmp_camera->frustum.near_clip = 0.0f;
    tmp_camera->frustum.fovy = 0.0f;

    vec3f_initialize(0.0f, 0.0f, 0.0f, &tmp_camera->euler);
    vec3f_initialize(0.0f, 0.0f, 0.0f, &tmp_camera->position);

    *out_camera_ = tmp_camera;
    ret = CAMERA_SUCCESS;

cleanup:
    if(CAMERA_SUCCESS != ret) {
        if(NULL != tmp_camera) {
            if(NULL != tmp_camera->name) {
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
    memory_system_free(*camera_, sizeof(camera_t), MEMORY_TAG_CAMERA);
    *camera_ = NULL;
}

const char* camera_name_get(const camera_t* camera_) {
    if(NULL == camera_) {
        ERROR_MESSAGE("camera_name_get(%s) - Argument camera_ requires a valid pointer.", rslt_to_str(CAMERA_INVALID_ARGUMENT));
        return NULL;
    }
    if(NULL == camera_->name) {
        ERROR_MESSAGE("camera_name_get(%s) - Provided camera_ is corrupted.", rslt_to_str(CAMERA_DATA_CORRUPTED));
        return NULL;
    }
    return choco_string_c_str(camera_->name);
}

camera_result_t camera_viewing_frustum_update(float fovy_, float aspect_, float near_clip_, float far_clip_, camera_t* camera_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_viewing_frustum_update", "camera_")

    viewing_frustum_t frustum = { 0 };
    frustum.aspect = aspect_;
    frustum.far_clip = far_clip_;
    frustum.fovy = fovy_;
    frustum.near_clip = near_clip_;
    if(!is_valid_frustum(&frustum)) {
        ret = CAMERA_INVALID_ARGUMENT;
        ERROR_MESSAGE("camera_viewing_frustum_update(%s) - Invalid frustum parameter.", rslt_to_str(ret));
        goto cleanup;
    }

    camera_->frustum = frustum;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_perspective_matrix_get(const camera_t* camera_, mat4x4f_t* out_mat_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_perspective_matrix_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_perspective_matrix_get", "out_mat_")
    IF_ARG_FALSE_GOTO_CLEANUP(is_valid_frustum(&camera_->frustum), ret, CAMERA_BAD_OPERATION, rslt_to_str(CAMERA_BAD_OPERATION), "camera_perspective_matrix_get", "camera_->frustum")

    const float dz = camera_->frustum.far_clip - camera_->frustum.near_clip;

    mat4f_identity(out_mat_);
    out_mat_->elem[5] = 1.0f / choco_tanf(CHOCO_DEG_TO_RAD(camera_->frustum.fovy) * 0.5f);
    out_mat_->elem[0] = out_mat_->elem[5] / camera_->frustum.aspect;
    out_mat_->elem[10] = -1.0f * (camera_->frustum.far_clip + camera_->frustum.near_clip) / dz;
    out_mat_->elem[11] = -2.0f * camera_->frustum.far_clip * camera_->frustum.near_clip / dz;
    out_mat_->elem[14] = -1.0f;
    out_mat_->elem[15] = 0.0f;

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t camera_view_matrix_get(const camera_t* camera_, mat4x4f_t* out_mat_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_view_matrix_get", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mat_, ret, CAMERA_INVALID_ARGUMENT, rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_view_matrix_get", "out_mat_")

    mat4x4f_t rot = { 0 };
    mat4f_rot_xyz(CHOCO_DEG_TO_RAD(camera_->euler.elem[0]), CHOCO_DEG_TO_RAD(camera_->euler.elem[1]), CHOCO_DEG_TO_RAD(camera_->euler.elem[2]), &rot);

    mat4x4f_t trans = { 0 };
    mat4f_translation(&camera_->position, &trans); // ある座標をtranslate分平行移動する行列 = translate分座標が増える = カメラ->ワールド座標系への変換行列

    mat4x4f_t view = { 0 };
    // 後に変換するものを左から掛ける
    mat4f_mul(&trans, &rot, &view); // カメラ座標系のある座標をワールド座標系へ変換する行列
    if(!mat4f_inverse(&view)) {     // ワールド座標系のある座標をカメラ座標系へ変換する行列
        ERROR_MESSAGE("camera_view_matrix_get(%s) - Matrix(view) inversion failed because the determinant is zero or near zero.", rslt_to_str(CAMERA_RUNTIME_ERROR));
        ret = CAMERA_RUNTIME_ERROR;
        goto cleanup;
    }
    mat4f_copy(&view, out_mat_);

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param rslt_ 文字列に変換する実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* rslt_to_str(camera_result_t rslt_) {
    switch(rslt_) {
    case CAMERA_SUCCESS:
        return s_rslt_str_success;
    case CAMERA_NO_MEMORY:
        return s_rslt_str_no_memory;
    case CAMERA_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case CAMERA_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case CAMERA_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case CAMERA_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case CAMERA_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case CAMERA_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    default:
        return s_rslt_str_undefined_error;
    }
}

/**
 * @brief メモリシステム実行結果コードをカメラ実行結果コードに変換する
 *
 * @param[in] rslt_ メモリシステムが出力する実行結果コード
 *
 * @return camera_result_t 変換されたカメラシステム実行結果コード
 */
static camera_result_t rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return CAMERA_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return CAMERA_RUNTIME_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return CAMERA_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
}

/**
 * @brief 文字列コンテナモジュール実行結果コードをカメラ実行結果コードに変換する
 *
 * @param[in] rslt_ 文字列コンテナモジュール実行結果コード
 *
 * @return camera_result_t 変換されたカメラシステム実行結果コード
 */
static camera_result_t rslt_convert_choco_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return CAMERA_SUCCESS;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    case CHOCO_STRING_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return CAMERA_LIMIT_EXCEEDED;
    case CHOCO_STRING_OVERFLOW:
        return CAMERA_BAD_OPERATION;    // 長すぎる文字列指定はAPIの誤用とする
    case CHOCO_STRING_BAD_OPERATION:
        return CAMERA_BAD_OPERATION;
    case CHOCO_STRING_DATA_CORRUPTED:
        return CAMERA_DATA_CORRUPTED;
    case CHOCO_STRING_RUNTIME_ERROR:
        return CAMERA_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return CAMERA_UNDEFINED_ERROR;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
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
        return false;
    }
    return true;
}
