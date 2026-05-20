/**
 * @ingroup camera_system
 * @file flight_camera_controller.c
 * @author chocolate-pie24
 * @brief 空間内を自由に移動する飛行型カメラの制御機能の実装
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/systems/camera_system/camera_controller/flight_camera_controller.h"

#include "engine/systems/camera_system/camera/camera.h"

#include "engine/systems/camera_system/camera_core/camera_err_utils.h"
#include "engine/systems/camera_system/camera_core/camera_types.h"

#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>

#include "engine/core/memory/choco_memory.h"
#include "engine/systems/camera_system/camera_controller/test_flight_camera_controller.h"

#include "test_controller.h"

#include "engine/core/memory/test_choco_memory.h"

#include "engine/systems/camera_system/camera_controller/test_flight_camera_controller.h"
#include "engine/systems/camera_system/camera/test_camera.h"
#include "engine/systems/camera_system/camera_core/test_camera_memory.h"

// flight_camera_controller用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_flight_camera_controller_move_forward;     /**< flight_camera_controller_move_forward()テスト設定 */
static test_call_control_t s_test_config_flight_camera_controller_move_backward;    /**< flight_camera_controller_move_backward()テスト設定 */
static test_call_control_t s_test_config_flight_camera_controller_move_right;       /**< flight_camera_controller_move_right()テスト設定 */
static test_call_control_t s_test_config_flight_camera_controller_move_left;        /**< flight_camera_controller_move_left()テスト設定 */
static test_call_control_t s_test_config_flight_camera_controller_move_up;          /**< flight_camera_controller_move_up()テスト設定 */
static test_call_control_t s_test_config_flight_camera_controller_move_down;        /**< flight_camera_controller_move_down()テスト設定 */
static test_call_control_t s_test_config_flight_camera_controller_rot_pitch_plus;   /**< flight_camera_controller_rot_pitch_plus()テスト設定 */
static test_call_control_t s_test_config_flight_camera_controller_rot_pitch_minus;  /**< flight_camera_controller_rot_pitch_minus()テスト設定 */
static test_call_control_t s_test_config_flight_camera_controller_rot_yaw_plus;     /**< flight_camera_controller_rot_yaw_plus()テスト設定 */
static test_call_control_t s_test_config_flight_camera_controller_rot_yaw_minus;    /**< flight_camera_controller_rot_yaw_minus()テスト設定 */

// プライベート関数テスト設定
static test_call_control_t s_test_config_camera_position_movement_apply;    /**< camera_position_movement_apply()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_flight_camera_controller_move_forward(void);
static void test_flight_camera_controller_move_backward(void);
static void test_flight_camera_controller_move_right(void);
static void test_flight_camera_controller_move_left(void);
static void test_flight_camera_controller_move_up(void);
static void test_flight_camera_controller_move_down(void);
static void test_flight_camera_controller_rot_pitch_plus(void);
static void test_flight_camera_controller_rot_pitch_minus(void);
static void test_flight_camera_controller_rot_yaw_plus(void);
static void test_flight_camera_controller_rot_yaw_minus(void);
static void test_camera_position_movement_apply(void);
#endif

static camera_result_t camera_position_movement_apply(const vec3f_t* translation_, camera_t* camera_);

camera_result_t flight_camera_controller_move_forward(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_move_forward.call_count++;
    if(s_test_config_flight_camera_controller_move_forward.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_move_forward.call_count == s_test_config_flight_camera_controller_move_forward.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_move_forward.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t forward_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_forward", "camera_")

    // カメラ前方の正規化されたベクトルを取得
    ret = camera_forward_vector_get(camera_, &forward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_forward(%s) - Failed to get forward vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    forward_vec.elem[0] *= (speed_ * delta_time_);
    forward_vec.elem[1] *= (speed_ * delta_time_);
    forward_vec.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(&forward_vec, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_forward(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_backward(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_move_backward.call_count++;
    if(s_test_config_flight_camera_controller_move_backward.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_move_backward.call_count == s_test_config_flight_camera_controller_move_backward.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_move_backward.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t backward_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_backward", "camera_")

    // カメラ後方の正規化されたベクトルを取得
    ret = camera_backward_vector_get(camera_, &backward_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_backward(%s) - Failed to get backward vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    backward_vec.elem[0] *= (speed_ * delta_time_);
    backward_vec.elem[1] *= (speed_ * delta_time_);
    backward_vec.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(&backward_vec, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_backward(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_right(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_move_right.call_count++;
    if(s_test_config_flight_camera_controller_move_right.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_move_right.call_count == s_test_config_flight_camera_controller_move_right.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_move_right.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t right_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_right", "camera_")

    // カメラ右方向の正規化されたベクトルを取得
    ret = camera_right_vector_get(camera_, &right_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_right(%s) - Failed to get right vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    right_vec.elem[0] *= (speed_ * delta_time_);
    right_vec.elem[1] *= (speed_ * delta_time_);
    right_vec.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(&right_vec, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_right(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_left(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_move_left.call_count++;
    if(s_test_config_flight_camera_controller_move_left.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_move_left.call_count == s_test_config_flight_camera_controller_move_left.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_move_left.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t left_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_left", "camera_")

    // カメラ左方向の正規化されたベクトルを取得
    ret = camera_left_vector_get(camera_, &left_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_left(%s) - Failed to get left vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    left_vec.elem[0] *= (speed_ * delta_time_);
    left_vec.elem[1] *= (speed_ * delta_time_);
    left_vec.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(&left_vec, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_left(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_up(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_move_up.call_count++;
    if(s_test_config_flight_camera_controller_move_up.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_move_up.call_count == s_test_config_flight_camera_controller_move_up.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_move_up.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t up_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_up", "camera_")

    // カメラ上方向の正規化されたベクトルを取得
    ret = camera_up_vector_get(camera_, &up_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_up(%s) - Failed to get up vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    up_vec.elem[0] *= (speed_ * delta_time_);
    up_vec.elem[1] *= (speed_ * delta_time_);
    up_vec.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(&up_vec, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_up(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_move_down(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_move_down.call_count++;
    if(s_test_config_flight_camera_controller_move_down.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_move_down.call_count == s_test_config_flight_camera_controller_move_down.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_move_down.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t down_vec = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_move_down", "camera_")

    // カメラ下方向の正規化されたベクトルを取得
    ret = camera_down_vector_get(camera_, &down_vec);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_down(%s) - Failed to get down vector.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // ワールド座標系でのカメラ移動量を計算
    down_vec.elem[0] *= (speed_ * delta_time_);
    down_vec.elem[1] *= (speed_ * delta_time_);
    down_vec.elem[2] *= (speed_ * delta_time_);

    // カメラ位置更新
    ret = camera_position_movement_apply(&down_vec, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_move_down(%s) - Failed to update camera position.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_rot_pitch_plus(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_rot_pitch_plus.call_count++;
    if(s_test_config_flight_camera_controller_rot_pitch_plus.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_rot_pitch_plus.call_count == s_test_config_flight_camera_controller_rot_pitch_plus.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_rot_pitch_plus.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_rot_pitch_plus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_pitch_plus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[0] += (speed_ * delta_time_);

    ret = camera_euler_update(&euler, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_pitch_plus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_rot_pitch_minus(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_rot_pitch_minus.call_count++;
    if(s_test_config_flight_camera_controller_rot_pitch_minus.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_rot_pitch_minus.call_count == s_test_config_flight_camera_controller_rot_pitch_minus.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_rot_pitch_minus.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_rot_pitch_minus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_pitch_minus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[0] -= (speed_ * delta_time_);

    ret = camera_euler_update(&euler, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_pitch_minus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_rot_yaw_plus(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_rot_yaw_plus.call_count++;
    if(s_test_config_flight_camera_controller_rot_yaw_plus.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_rot_yaw_plus.call_count == s_test_config_flight_camera_controller_rot_yaw_plus.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_rot_yaw_plus.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_rot_yaw_plus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_yaw_plus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[1] += (speed_ * delta_time_);

    ret = camera_euler_update(&euler, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_yaw_plus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

camera_result_t flight_camera_controller_rot_yaw_minus(float speed_, float delta_time_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_controller_rot_yaw_minus.call_count++;
    if(s_test_config_flight_camera_controller_rot_yaw_minus.fail_on_call != 0) {
        if(s_test_config_flight_camera_controller_rot_yaw_minus.call_count == s_test_config_flight_camera_controller_rot_yaw_minus.fail_on_call) {
            return (camera_result_t)s_test_config_flight_camera_controller_rot_yaw_minus.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t euler = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "flight_camera_controller_rot_yaw_minus", "camera_")

    ret = camera_euler_get(camera_, &euler);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_yaw_minus(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    euler.elem[1] -= (speed_ * delta_time_);

    ret = camera_euler_update(&euler, camera_);
    if(CAMERA_SUCCESS != ret) {
        ERROR_MESSAGE("flight_camera_controller_rot_yaw_minus(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief カメラ位置にカメラ移動量を適用する
 *
 * @param[in] translation_ カメラ移動量
 * @param[in,out] camera_ 更新対象カメラ構造体インスタンスへのポインタ
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - translation_ == NULL
 * - camera_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 以下のいずれか
 * - カメラ位置の取得に失敗
 * - カメラ位置の更新に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
static camera_result_t camera_position_movement_apply(const vec3f_t* translation_, camera_t* camera_) {
#ifdef TEST_BUILD
    s_test_config_camera_position_movement_apply.call_count++;
    if(s_test_config_camera_position_movement_apply.fail_on_call != 0) {
        if(s_test_config_camera_position_movement_apply.call_count == s_test_config_camera_position_movement_apply.fail_on_call) {
            return (camera_result_t)s_test_config_camera_position_movement_apply.forced_result;
        }
    }
#endif
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    vec3f_t position = { 0 };
    vec3f_t new_pos = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(translation_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_position_movement_apply", "translation_")
    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, CAMERA_INVALID_ARGUMENT, camera_rslt_to_str(CAMERA_INVALID_ARGUMENT), "camera_position_movement_apply", "camera_")

    // 現在のカメラ座標を取得
    if(CAMERA_SUCCESS != camera_position_get(camera_, &position)) {
        ret = CAMERA_RUNTIME_ERROR;
        ERROR_MESSAGE("camera_position_movement_apply(%s) - Failed to get camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    // 新しいカメラ座標を計算
    vec3f_add(translation_, &position, &new_pos);

    // カメラ座標更新
    if(CAMERA_SUCCESS != camera_position_update(&new_pos, camera_)) {
        ret = CAMERA_RUNTIME_ERROR;
        ERROR_MESSAGE("camera_position_movement_apply(%s) - Failed to update camera posture.", camera_rslt_to_str(ret));
        goto cleanup;
    }

    ret = CAMERA_SUCCESS;

cleanup:
    return ret;
}

#ifdef TEST_BUILD
void NO_COVERAGE test_flight_camera_controller_move_forward_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_move_forward.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_move_forward.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_move_backward_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_move_backward.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_move_backward.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_move_right_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_move_right.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_move_right.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_move_left_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_move_left.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_move_left.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_move_up_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_move_up.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_move_up.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_move_down_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_move_down.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_move_down.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_rot_pitch_plus_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_rot_pitch_plus.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_rot_pitch_plus.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_rot_pitch_minus_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_rot_pitch_minus.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_rot_pitch_minus.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_rot_yaw_plus_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_rot_yaw_plus.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_rot_yaw_plus.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_rot_yaw_minus_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_controller_rot_yaw_minus.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_controller_rot_yaw_minus.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_controller_config_reset(void) {
    test_call_control_reset(&s_test_config_flight_camera_controller_move_forward);
    test_call_control_reset(&s_test_config_flight_camera_controller_move_backward);
    test_call_control_reset(&s_test_config_flight_camera_controller_move_right);
    test_call_control_reset(&s_test_config_flight_camera_controller_move_left);
    test_call_control_reset(&s_test_config_flight_camera_controller_move_up);
    test_call_control_reset(&s_test_config_flight_camera_controller_move_down);
    test_call_control_reset(&s_test_config_flight_camera_controller_rot_pitch_plus);
    test_call_control_reset(&s_test_config_flight_camera_controller_rot_pitch_minus);
    test_call_control_reset(&s_test_config_flight_camera_controller_rot_yaw_plus);
    test_call_control_reset(&s_test_config_flight_camera_controller_rot_yaw_minus);

    test_call_control_reset(&s_test_config_camera_position_movement_apply);
}

void NO_COVERAGE test_flight_camera_controller(void) {
    test_flight_camera_controller_move_forward();
    test_flight_camera_controller_move_backward();
    test_flight_camera_controller_move_right();
    test_flight_camera_controller_move_left();
    test_flight_camera_controller_move_up();
    test_flight_camera_controller_move_down();
    test_flight_camera_controller_rot_pitch_plus();
    test_flight_camera_controller_rot_pitch_minus();
    test_flight_camera_controller_rot_yaw_plus();
    test_flight_camera_controller_rot_yaw_minus();
    test_camera_position_movement_apply();
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_move_forward(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_move_forward_config_set(&config);

        ret = flight_camera_controller_move_forward(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_move_forward(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_forward_vector_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_forward_vector_get_config_set(&config);

        ret = flight_camera_controller_move_forward(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_position_movement_apply() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-forward", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_position_movement_apply.fail_on_call = 1U;
        s_test_config_camera_position_movement_apply.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = flight_camera_controller_move_forward(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: 前方移動できること
         * 初期姿勢は (0, 0, 0)、カメラ前方は -Z 方向なので、
         * speed=2.0, delta_time=0.5 のとき移動量は 1.0 となり、
         * position.z はおおむね -1.0 になることを確認する。
         */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t position = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-forward", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = flight_camera_controller_move_forward(2.0f, 0.5f, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(position.elem[0], 0.0f));
        assert(is_equal_float(position.elem[1], 0.0f));
        assert(is_equal_float(position.elem[2], -1.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_move_backward(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_move_backward_config_set(&config);

        ret = flight_camera_controller_move_backward(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_move_backward(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_backward_vector_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_backward_vector_get_config_set(&config);

        ret = flight_camera_controller_move_backward(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_position_movement_apply() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-backward", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_position_movement_apply.fail_on_call = 1U;
        s_test_config_camera_position_movement_apply.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = flight_camera_controller_move_backward(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: 後方移動できること
         * 初期姿勢は (0, 0, 0)、カメラ後方は +Z 方向なので、
         * speed=2.0, delta_time=0.5 のとき移動量は 1.0 となり、
         * position.z はおおむね +1.0 になることを確認する。
         */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t position = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-backward", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = flight_camera_controller_move_backward(2.0f, 0.5f, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(position.elem[0], 0.0f));
        assert(is_equal_float(position.elem[1], 0.0f));
        assert(is_equal_float(position.elem[2], 1.0f));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_move_right(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_move_right_config_set(&config);

        ret = flight_camera_controller_move_right(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_move_right(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_right_vector_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_right_vector_get_config_set(&config);

        ret = flight_camera_controller_move_right(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_position_movement_apply() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-right", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_position_movement_apply.fail_on_call = 1U;
        s_test_config_camera_position_movement_apply.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = flight_camera_controller_move_right(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: 右方向へ移動できること
         * 事前に camera_right_vector_get() で右ベクトルを取得し、
         * speed * delta_time を掛けた期待移動量と、更新後座標が一致することを確認する。
         */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t right_vec = {0};
        vec3f_t position = {0};
        const float speed = 2.0f;
        const float delta_time = 0.5f;
        const float movement = speed * delta_time;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-right", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_right_vector_get(camera, &right_vec);
        assert(CAMERA_SUCCESS == ret);

        ret = flight_camera_controller_move_right(speed, delta_time, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(position.elem[0], right_vec.elem[0] * movement));
        assert(is_equal_float(position.elem[1], right_vec.elem[1] * movement));
        assert(is_equal_float(position.elem[2], right_vec.elem[2] * movement));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_move_left(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_move_left_config_set(&config);

        ret = flight_camera_controller_move_left(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_move_left(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_left_vector_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_left_vector_get_config_set(&config);

        ret = flight_camera_controller_move_left(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_position_movement_apply() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-left", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_position_movement_apply.fail_on_call = 1U;
        s_test_config_camera_position_movement_apply.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = flight_camera_controller_move_left(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: 左方向へ移動できること
         * 事前に camera_left_vector_get() で左ベクトルを取得し、
         * speed * delta_time を掛けた期待移動量と、更新後座標が一致することを確認する。
         */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t left_vec = {0};
        vec3f_t position = {0};
        const float speed = 2.0f;
        const float delta_time = 0.5f;
        const float movement = speed * delta_time;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-left", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_left_vector_get(camera, &left_vec);
        assert(CAMERA_SUCCESS == ret);

        ret = flight_camera_controller_move_left(speed, delta_time, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(position.elem[0], left_vec.elem[0] * movement));
        assert(is_equal_float(position.elem[1], left_vec.elem[1] * movement));
        assert(is_equal_float(position.elem[2], left_vec.elem[2] * movement));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_move_up(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_move_up_config_set(&config);

        ret = flight_camera_controller_move_up(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_move_up(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_up_vector_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_up_vector_get_config_set(&config);

        ret = flight_camera_controller_move_up(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_position_movement_apply() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-up", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_position_movement_apply.fail_on_call = 1U;
        s_test_config_camera_position_movement_apply.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = flight_camera_controller_move_up(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: 上方向へ移動できること
         * 事前に camera_up_vector_get() で上方向ベクトルを取得し、
         * speed * delta_time を掛けた期待移動量と、更新後座標が一致することを確認する。
         */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t up_vec = {0};
        vec3f_t position = {0};
        const float speed = 2.0f;
        const float delta_time = 0.5f;
        const float movement = speed * delta_time;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-up", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_up_vector_get(camera, &up_vec);
        assert(CAMERA_SUCCESS == ret);

        ret = flight_camera_controller_move_up(speed, delta_time, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(position.elem[0], up_vec.elem[0] * movement));
        assert(is_equal_float(position.elem[1], up_vec.elem[1] * movement));
        assert(is_equal_float(position.elem[2], up_vec.elem[2] * movement));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_move_down(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_move_down_config_set(&config);

        ret = flight_camera_controller_move_down(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_move_down(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_down_vector_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_down_vector_get_config_set(&config);

        ret = flight_camera_controller_move_down(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_position_movement_apply() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-down", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        s_test_config_camera_position_movement_apply.fail_on_call = 1U;
        s_test_config_camera_position_movement_apply.forced_result = (int)CAMERA_RUNTIME_ERROR;

        ret = flight_camera_controller_move_down(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: 下方向へ移動できること
         * 事前に camera_down_vector_get() で下方向ベクトルを取得し、
         * speed * delta_time を掛けた期待移動量と、更新後座標が一致することを確認する。
         */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t down_vec = {0};
        vec3f_t position = {0};
        const float speed = 2.0f;
        const float delta_time = 0.5f;
        const float movement = speed * delta_time;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-down", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_down_vector_get(camera, &down_vec);
        assert(CAMERA_SUCCESS == ret);

        ret = flight_camera_controller_move_down(speed, delta_time, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(position.elem[0], down_vec.elem[0] * movement));
        assert(is_equal_float(position.elem[1], down_vec.elem[1] * movement));
        assert(is_equal_float(position.elem[2], down_vec.elem[2] * movement));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_rot_pitch_plus(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_rot_pitch_plus_config_set(&config);

        ret = flight_camera_controller_rot_pitch_plus(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_rot_pitch_plus(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_euler_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_euler_get_config_set(&config);

        ret = flight_camera_controller_rot_pitch_plus(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_euler_update() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-rot-pitch-plus", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_euler_update_config_set(&config);

        ret = flight_camera_controller_rot_pitch_plus(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: pitch が加算され、yaw/roll は変化しないこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t before_euler = {0};
        vec3f_t after_euler = {0};
        const float speed = 2.0f;
        const float delta_time = 0.5f;
        const float rotation = speed * delta_time;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-rot-pitch-plus", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_euler_get(camera, &before_euler);
        assert(CAMERA_SUCCESS == ret);

        ret = flight_camera_controller_rot_pitch_plus(speed, delta_time, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_euler_get(camera, &after_euler);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(after_euler.elem[0], before_euler.elem[0] + rotation));
        assert(is_equal_float(after_euler.elem[1], before_euler.elem[1]));
        assert(is_equal_float(after_euler.elem[2], before_euler.elem[2]));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_rot_pitch_minus(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_rot_pitch_minus_config_set(&config);

        ret = flight_camera_controller_rot_pitch_minus(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_rot_pitch_minus(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_euler_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_euler_get_config_set(&config);

        ret = flight_camera_controller_rot_pitch_minus(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_euler_update() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-rot-pitch-minus", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_euler_update_config_set(&config);

        ret = flight_camera_controller_rot_pitch_minus(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: pitch が減算され、yaw/roll は変化しないこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t before_euler = {0};
        vec3f_t after_euler = {0};
        const float speed = 2.0f;
        const float delta_time = 0.5f;
        const float rotation = speed * delta_time;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-rot-pitch-minus", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_euler_get(camera, &before_euler);
        assert(CAMERA_SUCCESS == ret);

        ret = flight_camera_controller_rot_pitch_minus(speed, delta_time, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_euler_get(camera, &after_euler);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(after_euler.elem[0], before_euler.elem[0] - rotation));
        assert(is_equal_float(after_euler.elem[1], before_euler.elem[1]));
        assert(is_equal_float(after_euler.elem[2], before_euler.elem[2]));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_rot_yaw_plus(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_rot_yaw_plus_config_set(&config);

        ret = flight_camera_controller_rot_yaw_plus(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_rot_yaw_plus(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_euler_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_euler_get_config_set(&config);

        ret = flight_camera_controller_rot_yaw_plus(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_euler_update() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-rot-yaw-plus", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_euler_update_config_set(&config);

        ret = flight_camera_controller_rot_yaw_plus(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: yaw が加算され、pitch/roll は変化しないこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t before_euler = {0};
        vec3f_t after_euler = {0};
        const float speed = 2.0f;
        const float delta_time = 0.5f;
        const float rotation = speed * delta_time;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-rot-yaw-plus", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_euler_get(camera, &before_euler);
        assert(CAMERA_SUCCESS == ret);

        ret = flight_camera_controller_rot_yaw_plus(speed, delta_time, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_euler_get(camera, &after_euler);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(after_euler.elem[0], before_euler.elem[0]));
        assert(is_equal_float(after_euler.elem[1], before_euler.elem[1] + rotation));
        assert(is_equal_float(after_euler.elem[2], before_euler.elem[2]));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_controller_rot_yaw_minus(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_BAD_OPERATION;
        test_flight_camera_controller_rot_yaw_minus_config_set(&config);

        ret = flight_camera_controller_rot_yaw_minus(1.0f, 1.0f, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = flight_camera_controller_rot_yaw_minus(1.0f, 1.0f, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_euler_get() 失敗 -> そのまま返ること */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_euler_get_config_set(&config);

        ret = flight_camera_controller_rot_yaw_minus(1.0f, 1.0f, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_euler_update() 失敗 -> そのまま返ること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        test_call_control_t config = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-rot-yaw-minus", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_euler_update_config_set(&config);

        ret = flight_camera_controller_rot_yaw_minus(1.0f, 1.0f, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: yaw が減算され、pitch/roll は変化しないこと */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t before_euler = {0};
        vec3f_t after_euler = {0};
        const float speed = 2.0f;
        const float delta_time = 0.5f;
        const float rotation = speed * delta_time;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-flight-camera-rot-yaw-minus", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_euler_get(camera, &before_euler);
        assert(CAMERA_SUCCESS == ret);

        ret = flight_camera_controller_rot_yaw_minus(speed, delta_time, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_euler_get(camera, &after_euler);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(after_euler.elem[0], before_euler.elem[0]));
        assert(is_equal_float(after_euler.elem[1], before_euler.elem[1] - rotation));
        assert(is_equal_float(after_euler.elem[2], before_euler.elem[2]));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_camera_position_movement_apply(void) {
    {
        /* この関数自身の失敗注入が効くこと */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t translation = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        s_test_config_camera_position_movement_apply.fail_on_call = 1U;
        s_test_config_camera_position_movement_apply.forced_result = (int)CAMERA_BAD_OPERATION;

        ret = camera_position_movement_apply(&translation, NULL);
        assert(CAMERA_BAD_OPERATION == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* translation_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = camera_position_movement_apply(NULL, (camera_t*)0x1);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_ == NULL -> CAMERA_INVALID_ARGUMENT */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        vec3f_t translation = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret = camera_position_movement_apply(&translation, NULL);
        assert(CAMERA_INVALID_ARGUMENT == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_position_get() 失敗 -> CAMERA_RUNTIME_ERROR */
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        vec3f_t translation = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_position_get_config_set(&config);

        ret = camera_position_movement_apply(&translation, (camera_t*)0x1);
        assert(CAMERA_RUNTIME_ERROR == ret);

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* camera_position_update() 失敗 -> CAMERA_RUNTIME_ERROR */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        test_call_control_t config = {0};
        vec3f_t translation = { .elem = { 1.0f, 2.0f, 3.0f } };

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-camera-position-movement-apply", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_camera_position_update_config_set(&config);

        ret = camera_position_movement_apply(&translation, camera);
        assert(CAMERA_RUNTIME_ERROR == ret);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
    {
        /* 正常系: translation 分だけ位置が加算されること */
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        camera_result_t ret = CAMERA_UNDEFINED_ERROR;
        camera_t* camera = NULL;
        vec3f_t translation = { .elem = { 1.5f, -2.0f, 3.25f } };
        vec3f_t position = {0};

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret = camera_create("test-camera-position-movement-apply", &camera);
        assert(CAMERA_SUCCESS == ret);
        assert(NULL != camera);

        ret = camera_position_movement_apply(&translation, camera);
        assert(CAMERA_SUCCESS == ret);

        ret = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret);

        assert(is_equal_float(position.elem[0], translation.elem[0]));
        assert(is_equal_float(position.elem[1], translation.elem[1]));
        assert(is_equal_float(position.elem[2], translation.elem[2]));

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_camera_memory_config_reset();
        test_choco_memory_config_reset();
    }
}
#endif
