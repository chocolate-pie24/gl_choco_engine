/** @ingroup application
 *
 * @file flight_camera.c
 * @author chocolate-pie24
 * @brief アプリケーションからのイベント情報をもとにフライトカメラを制御するAPIの実装
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
#include <stdbool.h>
#include <stddef.h>

#include "application/command_interpreter/flight_camera.h"

#include "application/application_core/application_types.h"
#include "application/application_core/application_err_utils.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/event/keyboard_event.h"

#include "engine/systems/camera_system/camera/camera.h"
#include "engine/systems/camera_system/camera_controller/flight_camera_controller.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <string.h>

#include "engine/base/choco_math/choco_math.h"

#include "test_controller.h"

#include "engine/systems/camera_system/camera/test_camera.h"
#include "engine/systems/camera_system/camera_controller/test_flight_camera_controller.h"

#include "application/command_interpreter/test_flight_camera.h"

// flight_camera用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定

static test_call_control_t s_test_config_flight_camera_command_initialize;  /**< flight_camera_command_initialize()テスト設定 */
static test_call_control_t s_test_config_flight_camera_command_update;      /**< flight_camera_command_update()テスト設定 */
static test_call_control_t s_test_config_flight_camera_command_execute;     /**< flight_camera_command_execute()テスト設定 */

// プライベート関数テスト設定

static test_call_control_bool_t s_test_config_is_valid_keybind; /**< is_valid_keybind()テスト設定 */

// 全テスト関数プロトタイプ宣言

static void test_flight_camera_command_initialize(void);
static void test_flight_camera_command_update(void);
static void test_flight_camera_command_execute(void);
static void test_s_command_to_str(void);
static void test_is_valid_keybind(void);
#endif

// ファイル内静的変数宣言

static const char* s_command_str_move_forward = "FLIGHT CAMERA: Move(Forward)";             /**< フライトカメラ制御コマンド文字列: 前方移動 */
static const char* s_command_str_move_backward = "FLIGHT CAMERA: Move(Backward)";           /**< フライトカメラ制御コマンド文字列: 後方移動 */
static const char* s_command_str_move_right = "FLIGHT CAMERA: Move(Right)";                 /**< フライトカメラ制御コマンド文字列: 右方向移動 */
static const char* s_command_str_move_left = "FLIGHT CAMERA: Move(Left)";                   /**< フライトカメラ制御コマンド文字列: 左方向移動 */
static const char* s_command_str_move_up = "FLIGHT CAMERA: Move(Up)";                       /**< フライトカメラ制御コマンド文字列: 上方向移動 */
static const char* s_command_str_move_down = "FLIGHT CAMERA: Move(Down)";                   /**< フライトカメラ制御コマンド文字列: 下方向移動 */
static const char* s_command_str_rot_pitch_plus = "FLIGHT CAMERA: Rotation(Pitch+)";        /**< フライトカメラ制御コマンド文字列: ピッチ+方向回転 */
static const char* s_command_str_rot_pitch_minus = "FLIGHT CAMERA: Rotation(Pitch-)";       /**< フライトカメラ制御コマンド文字列: ピッチ-方向回転 */
static const char* s_command_str_rot_yaw_plus = "FLIGHT CAMERA: Rotation(Yaw+)";            /**< フライトカメラ制御コマンド文字列: ヨー+方向回転 */
static const char* s_command_str_rot_yaw_minus = "FLIGHT CAMERA: Rotation(Yaw-)";           /**< フライトカメラ制御コマンド文字列: ヨー-方向回転 */
static const char* s_command_str_undefined_command = "FLIGHT CAMERA: Undefined Command";    /**< フライトカメラ制御コマンド文字列: 不明なコマンド */

// 関数プロトタイプ宣言

static const char* s_command_to_str(command_list_flight_camera_t command_);
static bool is_valid_keybind(size_t array_size_, const command_status_flight_camera_t* command_status_);

application_result_t flight_camera_command_initialize(size_t array_size_, command_status_flight_camera_t* command_status_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_command_initialize.call_count++;
    if(s_test_config_flight_camera_command_initialize.fail_on_call != 0) {
        if(s_test_config_flight_camera_command_initialize.call_count == s_test_config_flight_camera_command_initialize.fail_on_call) {
            return (application_result_t)s_test_config_flight_camera_command_initialize.forced_result;
        }
    }
#endif
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(command_status_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_initialize", "command_status_")
    IF_ARG_FALSE_GOTO_CLEANUP(array_size_ == FLIGHT_CAMERA_COMMAND_MAX, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_initialize", "array_size_")

    // カメラ前進コマンド(キーバインド: KEY_W)
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].command = FLIGHT_CAMERA_COMMAND_MOVE_FORWARD;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].keybind = KEY_W;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].pfn_command_executor = flight_camera_controller_move_forward;

    // カメラ後進コマンド(キーバインド: KEY_S)
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].command = FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].keybind = KEY_S;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].pfn_command_executor = flight_camera_controller_move_backward;

    // カメラ右移動コマンド(キーバインド: KEY_D)
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].command = FLIGHT_CAMERA_COMMAND_MOVE_RIGHT;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].keybind = KEY_D;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].pfn_command_executor = flight_camera_controller_move_right;

    // カメラ左移動コマンド(キーバインド: KEY_A)
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].command = FLIGHT_CAMERA_COMMAND_MOVE_LEFT;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].keybind = KEY_A;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].pfn_command_executor = flight_camera_controller_move_left;

    // カメラ上方向移動コマンド(キーバインド: KEY_E)
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_UP].command = FLIGHT_CAMERA_COMMAND_MOVE_UP;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_UP].keybind = KEY_E;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_UP].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_UP].pfn_command_executor = flight_camera_controller_move_up;

    // カメラ下方向移動コマンド(キーバインド: KEY_Q)
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].command = FLIGHT_CAMERA_COMMAND_MOVE_DOWN;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].keybind = KEY_Q;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].pfn_command_executor = flight_camera_controller_move_down;

    // カメラピッチ方向(+)回転コマンド(キーバインド: KEY_UP)
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].command = FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].keybind = KEY_UP;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].pfn_command_executor = flight_camera_controller_rot_pitch_plus;

    // カメラピッチ方向(-)回転コマンド(キーバインド: KEY_DOWN)
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].command = FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].keybind = KEY_DOWN;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].pfn_command_executor = flight_camera_controller_rot_pitch_minus;

    // カメラヨー方向(+)回転コマンド(キーバインド: KEY_LEFT)
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].command = FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].keybind = KEY_LEFT;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].pfn_command_executor = flight_camera_controller_rot_yaw_plus;

    // カメラヨー方向(-)回転コマンド(キーバインド: KEY_RIGHT)
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].command = FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].keybind = KEY_RIGHT;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].pfn_command_executor = flight_camera_controller_rot_yaw_minus;

    if(!is_valid_keybind(array_size_, command_status_)) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("flight_camera_command_initialize(%s) - Provided command configuration is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t flight_camera_command_update(const keyboard_event_t* keyboard_event_, command_status_flight_camera_t* command_status_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_command_update.call_count++;
    if(s_test_config_flight_camera_command_update.fail_on_call != 0) {
        if(s_test_config_flight_camera_command_update.call_count == s_test_config_flight_camera_command_update.fail_on_call) {
            return (application_result_t)s_test_config_flight_camera_command_update.forced_result;
        }
    }
#endif
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_update", "keyboard_event_")
    IF_ARG_NULL_GOTO_CLEANUP(command_status_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_update", "command_status_")

    for(size_t i = 0; i != FLIGHT_CAMERA_COMMAND_MAX; ++i) {
        if(command_status_[i].keybind == keyboard_event_->key) {
            if(keyboard_event_->event_args.pressed) {   // キーが押されたらコマンドON
                command_status_[i].status = true;
                INFO_MESSAGE("flight_camera_command_update: %s", s_command_to_str((command_list_flight_camera_t)(i)));
            } else if(command_status_[i].status && !keyboard_event_->event_args.pressed) {  // キーが離されたらコマンドOFF
                command_status_[i].status = false;
                INFO_MESSAGE("flight_camera_command_update: %s Released", s_command_to_str((command_list_flight_camera_t)(i)));
            }
            break;
        }
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t flight_camera_command_execute(float speed_, float delta_time_, command_status_flight_camera_t* command_status_, camera_t* camera_, bool* out_view_updated_) {
#ifdef TEST_BUILD
    s_test_config_flight_camera_command_execute.call_count++;
    if(s_test_config_flight_camera_command_execute.fail_on_call != 0) {
        if(s_test_config_flight_camera_command_execute.call_count == s_test_config_flight_camera_command_execute.fail_on_call) {
            return (application_result_t)s_test_config_flight_camera_command_execute.forced_result;
        }
    }
#endif
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;
    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(camera_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_execute", "camera_")
    IF_ARG_NULL_GOTO_CLEANUP(command_status_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_execute", "command_status_")
    IF_ARG_NULL_GOTO_CLEANUP(out_view_updated_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_execute", "out_view_updated_")

    for(size_t i = 0; i != FLIGHT_CAMERA_COMMAND_MAX; ++i) {
        if(command_status_[i].status) {
            ret_camera = command_status_[i].pfn_command_executor(speed_, delta_time_, camera_);
            if(CAMERA_SUCCESS != ret_camera) {
                ret = app_rslt_convert_camera(ret_camera);
                ERROR_MESSAGE("flight_camera_command_execute(%s) - Failed to execute flight camera command.", app_rslt_to_str(ret));
                goto cleanup;
            }
            *out_view_updated_ = true;
        }
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief フライトカメラ制御コマンドを文字列に変換する
 *
 * @param[in] command_ フライトカメラ制御コマンド
 * @return const char* 変換された文字列へのポインタ
 */
static const char* s_command_to_str(command_list_flight_camera_t command_) {
    switch(command_) {
    case FLIGHT_CAMERA_COMMAND_MOVE_FORWARD:
        return s_command_str_move_forward;
    case FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD:
        return s_command_str_move_backward;
    case FLIGHT_CAMERA_COMMAND_MOVE_RIGHT:
        return s_command_str_move_right;
    case FLIGHT_CAMERA_COMMAND_MOVE_LEFT:
        return s_command_str_move_left;
    case FLIGHT_CAMERA_COMMAND_MOVE_UP:
        return s_command_str_move_up;
    case FLIGHT_CAMERA_COMMAND_MOVE_DOWN:
        return s_command_str_move_down;
    case FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS:
        return s_command_str_rot_pitch_plus;
    case FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS:
        return s_command_str_rot_pitch_minus;
    case FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS:
        return s_command_str_rot_yaw_plus;
    case FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS:
        return s_command_str_rot_yaw_minus;
    default:
        return s_command_str_undefined_command;
    }
}

/**
 * @brief 設定されたキーバインド配列の正常チェック
 *
 * @note チェック項目
 * - コマンド実行関数が非NULL
 * - コマンドがcommand_list_flight_camera_tの列挙子の範囲内
 * - キーバインドに重複なし
 * - キーがkeycode_tの列挙子の範囲内
 *
 * @param[in] array_size_ 配列サイズ
 * @param[in] command_status_ フライトカメラ制御コマンド構造体インスタンス配列
 *
 * @retval true 異常なし
 * @return false 異常あり
 */
static bool is_valid_keybind(size_t array_size_, const command_status_flight_camera_t* command_status_) {
#ifdef TEST_BUILD
    s_test_config_is_valid_keybind.call_count++;
    if(s_test_config_is_valid_keybind.fail_on_call != 0) {
        if(s_test_config_is_valid_keybind.call_count == s_test_config_is_valid_keybind.fail_on_call) {
            return s_test_config_is_valid_keybind.forced_result;
        }
    }
#endif
    if(0 == array_size_) {
        return false;
    }
    if(NULL == command_status_) {
        return false;
    }

    for(size_t i = 0; i != array_size_; ++i) {
        // チェック項目: コマンド実行関数が非NULL
        if(NULL == command_status_[i].pfn_command_executor) {
            return false;
        }

        // チェック項目: コマンドがcommand_list_flight_camera_tの列挙子の範囲内
        if(FLIGHT_CAMERA_COMMAND_MAX <= command_status_[i].command) {
            return false;
        }

        // チェック項目: キーバインドに重複なし
        for(size_t j = 0; j != array_size_; ++j) {
            if(i != j) {
                if(command_status_[i].keybind == command_status_[j].keybind) {
                    return false;
                }
            }
        }

        // チェック項目: キーがkeycode_tの列挙子の範囲内
        if(KEY_CODE_MAX <= command_status_[i].keybind) {
            return false;
        }
    }

    return true;

}

#ifdef TEST_BUILD
void NO_COVERAGE test_flight_camera_command_initialize_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_command_initialize.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_command_initialize.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_command_update_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_command_update.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_command_update.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_command_execute_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_flight_camera_command_execute.fail_on_call = config_->fail_on_call;
    s_test_config_flight_camera_command_execute.forced_result = config_->forced_result;
}

void NO_COVERAGE test_flight_camera_config_reset(void) {
    test_call_control_reset(&s_test_config_flight_camera_command_initialize);
    test_call_control_reset(&s_test_config_flight_camera_command_update);
    test_call_control_reset(&s_test_config_flight_camera_command_execute);
    s_test_config_is_valid_keybind.call_count = 0;
    s_test_config_is_valid_keybind.fail_on_call = 0;
    s_test_config_is_valid_keybind.forced_result = false;
}

void NO_COVERAGE test_flight_camera(void) {
    test_flight_camera_command_initialize();
    test_flight_camera_command_update();
    test_flight_camera_command_execute();
    test_s_command_to_str();
    test_is_valid_keybind();
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_command_initialize(void) {
    {
        // flight_camera_command_initialize() 冒頭で強制的に APPLICATION_BAD_OPERATION を返させる
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));
        command_status[0].command = FLIGHT_CAMERA_COMMAND_MOVE_FORWARD;
        command_status[0].keybind = KEY_W;
        command_status[0].status = true;
        command_status[0].pfn_command_executor = flight_camera_controller_move_forward;

        test_flight_camera_config_reset();

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)APPLICATION_BAD_OPERATION;
        test_flight_camera_command_initialize_config_set(&config);

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_BAD_OPERATION == ret);
        assert(FLIGHT_CAMERA_COMMAND_MOVE_FORWARD == command_status[0].command);
        assert(KEY_W == command_status[0].keybind);
        assert(true == command_status[0].status);
        assert(flight_camera_controller_move_forward == command_status[0].pfn_command_executor);
        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // command_status_ == NULL -> APPLICATION_INVALID_ARGUMENT
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;

        test_flight_camera_config_reset();

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, NULL);
        assert(APPLICATION_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // array_size_ != FLIGHT_CAMERA_COMMAND_MAX -> APPLICATION_INVALID_ARGUMENT
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));
        command_status[0].command = FLIGHT_CAMERA_COMMAND_MOVE_FORWARD;
        command_status[0].keybind = KEY_W;
        command_status[0].status = true;
        command_status[0].pfn_command_executor = flight_camera_controller_move_forward;

        test_flight_camera_config_reset();

        ret = flight_camera_command_initialize(0U, command_status);
        assert(APPLICATION_INVALID_ARGUMENT == ret);
        assert(FLIGHT_CAMERA_COMMAND_MOVE_FORWARD == command_status[0].command);
        assert(KEY_W == command_status[0].keybind);
        assert(true == command_status[0].status);
        assert(flight_camera_controller_move_forward == command_status[0].pfn_command_executor);
        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // is_valid_keybind() が false を返した場合 -> APPLICATION_RUNTIME_ERROR
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0xFF, sizeof(command_status));

        test_flight_camera_config_reset();

        s_test_config_is_valid_keybind.fail_on_call = 1U;
        s_test_config_is_valid_keybind.forced_result = false;

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_RUNTIME_ERROR == ret);

        assert(FLIGHT_CAMERA_COMMAND_MOVE_FORWARD == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].command);
        assert(KEY_W == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status);
        assert(flight_camera_controller_move_forward == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].command);
        assert(KEY_RIGHT == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].status);
        assert(flight_camera_controller_rot_yaw_minus == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].pfn_command_executor);

        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // 正常系: 全コマンド定義が正しく初期化される
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0xFF, sizeof(command_status));

        test_flight_camera_config_reset();

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        assert(FLIGHT_CAMERA_COMMAND_MOVE_FORWARD == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].command);
        assert(KEY_W == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status);
        assert(flight_camera_controller_move_forward == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].command);
        assert(KEY_S == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status);
        assert(flight_camera_controller_move_backward == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_MOVE_RIGHT == command_status[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].command);
        assert(KEY_D == command_status[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].status);
        assert(flight_camera_controller_move_right == command_status[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_MOVE_LEFT == command_status[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].command);
        assert(KEY_A == command_status[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].status);
        assert(flight_camera_controller_move_left == command_status[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_MOVE_UP == command_status[FLIGHT_CAMERA_COMMAND_MOVE_UP].command);
        assert(KEY_E == command_status[FLIGHT_CAMERA_COMMAND_MOVE_UP].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_UP].status);
        assert(flight_camera_controller_move_up == command_status[FLIGHT_CAMERA_COMMAND_MOVE_UP].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_MOVE_DOWN == command_status[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].command);
        assert(KEY_Q == command_status[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].status);
        assert(flight_camera_controller_move_down == command_status[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].command);
        assert(KEY_UP == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].status);
        assert(flight_camera_controller_rot_pitch_plus == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].command);
        assert(KEY_DOWN == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].status);
        assert(flight_camera_controller_rot_pitch_minus == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].command);
        assert(KEY_LEFT == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].status);
        assert(flight_camera_controller_rot_yaw_plus == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].pfn_command_executor);

        assert(FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].command);
        assert(KEY_RIGHT == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].keybind);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].status);
        assert(flight_camera_controller_rot_yaw_minus == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].pfn_command_executor);

        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_command_update(void) {
    {
        // flight_camera_command_update() 冒頭で強制的に APPLICATION_BAD_OPERATION を返させる
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        keyboard_event_t keyboard_event;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(&keyboard_event, 0, sizeof(keyboard_event));
        memset(command_status, 0, sizeof(command_status));
        keyboard_event.key = KEY_W;
        keyboard_event.event_args.pressed = true;

        test_flight_camera_config_reset();

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)APPLICATION_BAD_OPERATION;
        test_flight_camera_command_update_config_set(&config);

        ret = flight_camera_command_update(&keyboard_event, command_status);
        assert(APPLICATION_BAD_OPERATION == ret);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status);
        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(1U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // keyboard_event_ == NULL -> APPLICATION_INVALID_ARGUMENT
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        ret = flight_camera_command_update(NULL, command_status);
        assert(APPLICATION_INVALID_ARGUMENT == ret);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status);
        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(1U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // command_status_ == NULL -> APPLICATION_INVALID_ARGUMENT
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        keyboard_event_t keyboard_event;

        memset(&keyboard_event, 0, sizeof(keyboard_event));
        keyboard_event.key = KEY_W;
        keyboard_event.event_args.pressed = true;

        test_flight_camera_config_reset();

        ret = flight_camera_command_update(&keyboard_event, NULL);
        assert(APPLICATION_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(1U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // KEY_W 押下で MOVE_FORWARD の status が true になる
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        keyboard_event_t keyboard_event;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(&keyboard_event, 0, sizeof(keyboard_event));
        memset(command_status, 0, sizeof(command_status));
        keyboard_event.key = KEY_W;
        keyboard_event.event_args.pressed = true;

        test_flight_camera_config_reset();

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        ret = flight_camera_command_update(&keyboard_event, command_status);
        assert(APPLICATION_SUCCESS == ret);
        assert(true == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_UP].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].status);
        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(1U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // KEY_W 離上で、既に ON だった MOVE_FORWARD の status が false になる
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        keyboard_event_t keyboard_event;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(&keyboard_event, 0, sizeof(keyboard_event));
        memset(command_status, 0, sizeof(command_status));
        keyboard_event.key = KEY_W;
        keyboard_event.event_args.pressed = false;

        test_flight_camera_config_reset();

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status = true;

        ret = flight_camera_command_update(&keyboard_event, command_status);
        assert(APPLICATION_SUCCESS == ret);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status);
        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(1U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // KEY_W 離上でも、既に OFF ならそのまま false のまま
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        keyboard_event_t keyboard_event;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(&keyboard_event, 0, sizeof(keyboard_event));
        memset(command_status, 0, sizeof(command_status));
        keyboard_event.key = KEY_W;
        keyboard_event.event_args.pressed = false;

        test_flight_camera_config_reset();

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        ret = flight_camera_command_update(&keyboard_event, command_status);
        assert(APPLICATION_SUCCESS == ret);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status);
        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(1U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // 対象キー不一致なら、どの status も変化しない
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        keyboard_event_t keyboard_event;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(&keyboard_event, 0, sizeof(keyboard_event));
        memset(command_status, 0, sizeof(command_status));
        keyboard_event.key = KEY_W;
        keyboard_event.event_args.pressed = true;

        test_flight_camera_config_reset();

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].keybind = KEY_Q;

        ret = flight_camera_command_update(&keyboard_event, command_status);
        assert(APPLICATION_SUCCESS == ret);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_UP].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].status);
        assert(false == command_status[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].status);
        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(1U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_flight_camera_command_execute(void) {
    {
        // flight_camera_command_execute() 冒頭で強制的に APPLICATION_BAD_OPERATION を返させる
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];
        bool out_view_updated = false;

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)APPLICATION_BAD_OPERATION;
        test_flight_camera_command_execute_config_set(&config);

        ret = flight_camera_command_execute(0.1f, 1.0f, command_status, (camera_t*)0x1, &out_view_updated);
        assert(APPLICATION_BAD_OPERATION == ret);
        assert(false == out_view_updated);
        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(1U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_flight_camera_config_reset();
    }
    {
        // camera_ == NULL -> APPLICATION_INVALID_ARGUMENT
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];
        bool out_view_updated = false;

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_camera_config_reset();

        ret = flight_camera_command_execute(0.1f, 1.0f, command_status, NULL, &out_view_updated);
        assert(APPLICATION_INVALID_ARGUMENT == ret);
        assert(false == out_view_updated);
        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(1U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_flight_camera_config_reset();
    }
    {
        // command_status_ == NULL -> APPLICATION_INVALID_ARGUMENT
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        bool out_view_updated = false;

        test_flight_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_camera_config_reset();

        ret = flight_camera_command_execute(0.1f, 1.0f, NULL, (camera_t*)0x1, &out_view_updated);
        assert(APPLICATION_INVALID_ARGUMENT == ret);
        assert(false == out_view_updated);
        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(1U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_flight_camera_config_reset();
    }
    {
        // out_view_updated_ == NULL -> APPLICATION_INVALID_ARGUMENT
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_camera_config_reset();

        ret = flight_camera_command_execute(0.1f, 1.0f, command_status, (camera_t*)0x1, NULL);
        assert(APPLICATION_INVALID_ARGUMENT == ret);
        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(1U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_flight_camera_config_reset();
    }
    {
        // 実行要求なし: 何も実行されず out_view_updated は false のまま
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        camera_result_t ret_camera = CAMERA_UNDEFINED_ERROR;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];
        camera_t* camera = NULL;
        bool out_view_updated = false;

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_camera_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret_camera = camera_create("test-flight-camera-execute-noop", &camera);
        assert(CAMERA_SUCCESS == ret_camera);
        assert(NULL != camera);

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        ret = flight_camera_command_execute(0.1f, 1.0f, command_status, camera, &out_view_updated);
        assert(APPLICATION_SUCCESS == ret);
        assert(false == out_view_updated);

        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(1U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_flight_camera_config_reset();
    }
    {
        // 正常系: 1件実行され、out_view_updated が true になり、カメラ位置が変化する
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        camera_result_t ret_camera = CAMERA_UNDEFINED_ERROR;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];
        camera_t* camera = NULL;
        vec3f_t position = {0};
        bool out_view_updated = false;

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_camera_config_reset();

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret_camera = camera_create("test-flight-camera-execute-success", &camera);
        assert(CAMERA_SUCCESS == ret_camera);
        assert(NULL != camera);

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status = true;

        ret = flight_camera_command_execute(2.0f, 0.5f, command_status, camera, &out_view_updated);
        assert(APPLICATION_SUCCESS == ret);
        assert(true == out_view_updated);

        ret_camera = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret_camera);
        assert(is_equal_float(position.elem[0], 0.0f));
        assert(is_equal_float(position.elem[1], 0.0f));
        assert(is_equal_float(position.elem[2], -1.0f));

        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(1U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_flight_camera_config_reset();
    }
    {
        // 先頭の active command が失敗したら、その時点で異常終了し out_view_updated は false のまま
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        application_result_t expected = APPLICATION_UNDEFINED_ERROR;
        camera_result_t ret_camera = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];
        camera_t* camera = NULL;
        vec3f_t position = {0};
        bool out_view_updated = false;

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_call_control_reset(&config);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret_camera = camera_create("test-flight-camera-execute-fail-first", &camera);
        assert(CAMERA_SUCCESS == ret_camera);
        assert(NULL != camera);

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_flight_camera_controller_move_forward_config_set(&config);
        expected = app_rslt_convert_camera(CAMERA_RUNTIME_ERROR);

        command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status = true;
        command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status = true;

        ret = flight_camera_command_execute(1.0f, 1.0f, command_status, camera, &out_view_updated);
        assert(expected == ret);
        assert(false == out_view_updated);

        ret_camera = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret_camera);
        assert(is_equal_float(position.elem[0], 0.0f));
        assert(is_equal_float(position.elem[1], 0.0f));
        assert(is_equal_float(position.elem[2], 0.0f));

        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(1U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_flight_camera_config_reset();
    }
    {
        // 途中まで成功し、その後の active command が失敗したら異常終了し out_view_updated は true のまま
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        application_result_t ret = APPLICATION_UNDEFINED_ERROR;
        application_result_t expected = APPLICATION_UNDEFINED_ERROR;
        camera_result_t ret_camera = CAMERA_UNDEFINED_ERROR;
        test_call_control_t config = {0};
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];
        camera_t* camera = NULL;
        vec3f_t position = {0};
        bool out_view_updated = false;

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_camera_config_reset();
        test_call_control_reset(&config);

        ret_mem = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);

        ret_camera = camera_create("test-flight-camera-execute-fail-after-success", &camera);
        assert(CAMERA_SUCCESS == ret_camera);
        assert(NULL != camera);

        ret = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret);

        config.fail_on_call = 1U;
        config.forced_result = (int)CAMERA_RUNTIME_ERROR;
        test_flight_camera_controller_move_backward_config_set(&config);
        expected = app_rslt_convert_camera(CAMERA_RUNTIME_ERROR);

        command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status = true;
        command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status = true;

        ret = flight_camera_command_execute(1.0f, 1.0f, command_status, camera, &out_view_updated);
        assert(expected == ret);
        assert(true == out_view_updated);

        ret_camera = camera_position_get(camera, &position);
        assert(CAMERA_SUCCESS == ret_camera);
        assert(is_equal_float(position.elem[0], 0.0f));
        assert(is_equal_float(position.elem[1], 0.0f));
        assert(is_equal_float(position.elem[2], -1.0f));

        assert(1U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(1U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        camera_destroy(&camera);
        assert(NULL == camera);

        memory_system_destroy();

        test_camera_config_reset();
        test_flight_camera_controller_config_reset();
        test_flight_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_s_command_to_str(void) {
    {
        // FLIGHT_CAMERA_COMMAND_MOVE_FORWARD -> "FLIGHT CAMERA: Move(Forward)"
        test_flight_camera_config_reset();

        assert(s_command_str_move_forward == s_command_to_str(FLIGHT_CAMERA_COMMAND_MOVE_FORWARD));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD -> "FLIGHT CAMERA: Move(Backward)"
        test_flight_camera_config_reset();

        assert(s_command_str_move_backward == s_command_to_str(FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // FLIGHT_CAMERA_COMMAND_MOVE_RIGHT -> "FLIGHT CAMERA: Move(Right)"
        test_flight_camera_config_reset();

        assert(s_command_str_move_right == s_command_to_str(FLIGHT_CAMERA_COMMAND_MOVE_RIGHT));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // FLIGHT_CAMERA_COMMAND_MOVE_LEFT -> "FLIGHT CAMERA: Move(Left)"
        test_flight_camera_config_reset();

        assert(s_command_str_move_left == s_command_to_str(FLIGHT_CAMERA_COMMAND_MOVE_LEFT));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // FLIGHT_CAMERA_COMMAND_MOVE_UP -> "FLIGHT CAMERA: Move(Up)"
        test_flight_camera_config_reset();

        assert(s_command_str_move_up == s_command_to_str(FLIGHT_CAMERA_COMMAND_MOVE_UP));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // FLIGHT_CAMERA_COMMAND_MOVE_DOWN -> "FLIGHT CAMERA: Move(Down)"
        test_flight_camera_config_reset();

        assert(s_command_str_move_down == s_command_to_str(FLIGHT_CAMERA_COMMAND_MOVE_DOWN));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS -> "FLIGHT CAMERA: Rotation(Pitch+)"
        test_flight_camera_config_reset();

        assert(s_command_str_rot_pitch_plus == s_command_to_str(FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS -> "FLIGHT CAMERA: Rotation(Pitch-)"
        test_flight_camera_config_reset();

        assert(s_command_str_rot_pitch_minus == s_command_to_str(FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS -> "FLIGHT CAMERA: Rotation(Yaw+)"
        test_flight_camera_config_reset();

        assert(s_command_str_rot_yaw_plus == s_command_to_str(FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS -> "FLIGHT CAMERA: Rotation(Yaw-)"
        test_flight_camera_config_reset();

        assert(s_command_str_rot_yaw_minus == s_command_to_str(FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // 定義外コマンド -> "FLIGHT CAMERA: Undefined Command"
        test_flight_camera_config_reset();

        assert(s_command_str_undefined_command == s_command_to_str(FLIGHT_CAMERA_COMMAND_MAX));
        assert(s_command_str_undefined_command == s_command_to_str((command_list_flight_camera_t)-1));

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(0U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_is_valid_keybind(void) {
    {
        // is_valid_keybind() 冒頭の失敗注入が効くこと
        // 無効な引数でも forced_result=true を返せることを確認する
        bool ret = false;
        command_status_flight_camera_t* command_status = NULL;

        test_flight_camera_config_reset();

        s_test_config_is_valid_keybind.fail_on_call = 1U;
        s_test_config_is_valid_keybind.forced_result = true;

        ret = is_valid_keybind(0U, command_status);
        assert(true == ret);

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // array_size_ == 0 -> false
        bool ret = true;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();

        ret = is_valid_keybind(0U, command_status);
        assert(false == ret);

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // command_status_ == NULL -> false
        bool ret = true;

        test_flight_camera_config_reset();

        ret = is_valid_keybind((size_t)FLIGHT_CAMERA_COMMAND_MAX, NULL);
        assert(false == ret);

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // pfn_command_executor == NULL を含む -> false
        application_result_t ret_init = APPLICATION_UNDEFINED_ERROR;
        bool ret = true;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();

        ret_init = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret_init);

        test_flight_camera_config_reset();

        command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].pfn_command_executor = NULL;

        ret = is_valid_keybind((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(false == ret);

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // command が列挙範囲外 -> false
        application_result_t ret_init = APPLICATION_UNDEFINED_ERROR;
        bool ret = true;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();

        ret_init = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret_init);

        test_flight_camera_config_reset();

        command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].command = FLIGHT_CAMERA_COMMAND_MAX;

        ret = is_valid_keybind((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(false == ret);

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // keybind 重複あり -> false
        application_result_t ret_init = APPLICATION_UNDEFINED_ERROR;
        bool ret = true;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();

        ret_init = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret_init);

        test_flight_camera_config_reset();

        command_status[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].keybind =
            command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].keybind;

        ret = is_valid_keybind((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(false == ret);

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // keybind が列挙範囲外 -> false
        application_result_t ret_init = APPLICATION_UNDEFINED_ERROR;
        bool ret = true;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();

        ret_init = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret_init);

        test_flight_camera_config_reset();

        command_status[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].keybind = KEY_CODE_MAX;

        ret = is_valid_keybind((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(false == ret);

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
    {
        // 正常系: initialize() 済みの command table は true
        application_result_t ret_init = APPLICATION_UNDEFINED_ERROR;
        bool ret = false;
        command_status_flight_camera_t command_status[FLIGHT_CAMERA_COMMAND_MAX];

        memset(command_status, 0, sizeof(command_status));

        test_flight_camera_config_reset();

        ret_init = flight_camera_command_initialize((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(APPLICATION_SUCCESS == ret_init);

        test_flight_camera_config_reset();

        ret = is_valid_keybind((size_t)FLIGHT_CAMERA_COMMAND_MAX, command_status);
        assert(true == ret);

        assert(0U == s_test_config_flight_camera_command_initialize.call_count);
        assert(0U == s_test_config_flight_camera_command_update.call_count);
        assert(0U == s_test_config_flight_camera_command_execute.call_count);
        assert(1U == s_test_config_is_valid_keybind.call_count);

        test_flight_camera_config_reset();
    }
}
#endif
