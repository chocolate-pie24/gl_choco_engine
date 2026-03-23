#include <stdbool.h>
#include <stddef.h>

#include "application/command_interpreter/flight_camera.h"

#include "application/application_core/application_types.h"
#include "application/application_core/application_err_utils.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/event/keyboard_event.h"

#include "engine/camera_system/camera/camera.h"
#include "engine/camera_system/camera_controller/flight_camera_controller.h"

static const char* s_command_str_move_forward = "FLIGHT CAMERA: Move(Forward)";
static const char* s_command_str_move_backward = "FLIGHT CAMERA: Move(Backward)";
static const char* s_command_str_move_right = "FLIGHT CAMERA: Move(Right)";
static const char* s_command_str_move_left = "FLIGHT CAMERA: Move(Left)";
static const char* s_command_str_move_up = "FLIGHT CAMERA: Move(Up)";
static const char* s_command_str_move_down = "FLIGHT CAMERA: Move(Down)";
static const char* s_command_str_rot_pitch_plus = "FLIGHT CAMERA: Rotation(Pitch+)";
static const char* s_command_str_rot_pitch_minus = "FLIGHT CAMERA: Rotation(Pitch-)";
static const char* s_command_str_rot_yaw_plus = "FLIGHT CAMERA: Rotation(Yaw+)";
static const char* s_command_str_rot_yaw_minus = "FLIGHT CAMERA: Rotation(Yaw-)";
static const char* s_command_str_undefined_command = "FLIGHT CAMERA: Undefined Command";

static const char* s_command_to_str(command_list_flight_camera_t command_);

application_result_t flight_camera_command_initialize(size_t array_size_, command_status_flight_camera_t* command_status_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(command_status_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_initialize", "command_status_")
    IF_ARG_FALSE_GOTO_CLEANUP(array_size_ == FLIGHT_CAMERA_COMMAND_MAX, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_initialize", "array_size_")

    // FLIGHT_CAMERA_COMMAND_MOVE_FORWARD = 0,   /**< カメラ前進コマンド(キーバインド: KEY_W) */
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].command = FLIGHT_CAMERA_COMMAND_MOVE_FORWARD;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].keybind = KEY_W;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_FORWARD].pfn_command_executor = flight_camera_controller_move_forward;

    // FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD,      /**< カメラ後進コマンド(キーバインド: KEY_S) */
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].command = FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].keybind = KEY_S;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD].pfn_command_executor = flight_camera_controller_move_backward;

    // FLIGHT_CAMERA_COMMAND_MOVE_RIGHT,         /**< カメラ右移動コマンド(キーバインド: KEY_D) */
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].command = FLIGHT_CAMERA_COMMAND_MOVE_RIGHT;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].keybind = KEY_D;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_RIGHT].pfn_command_executor = flight_camera_controller_move_right;

    // FLIGHT_CAMERA_COMMAND_MOVE_LEFT,          /**< カメラ左移動コマンド(キーバインド: KEY_A) */
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].command = FLIGHT_CAMERA_COMMAND_MOVE_LEFT;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].keybind = KEY_A;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_LEFT].pfn_command_executor = flight_camera_controller_move_left;

    // FLIGHT_CAMERA_COMMAND_MOVE_UP,            /**< カメラ上方向移動コマンド(キーバインド: KEY_E) */
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_UP].command = FLIGHT_CAMERA_COMMAND_MOVE_UP;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_UP].keybind = KEY_E;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_UP].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_UP].pfn_command_executor = flight_camera_controller_move_up;

    // FLIGHT_CAMERA_COMMAND_MOVE_DOWN,          /**< カメラ下方向移動コマンド(キーバインド: KEY_Q) */
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].command = FLIGHT_CAMERA_COMMAND_MOVE_DOWN;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].keybind = KEY_Q;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_MOVE_DOWN].pfn_command_executor = flight_camera_controller_move_down;

    // FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS,     /**< カメラピッチ方向(+)回転コマンド(キーバインド: KEY_UP) */
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].command = FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].keybind = KEY_UP;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS].pfn_command_executor = flight_camera_controller_rot_pitch_plus;

    // FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS,    /**< カメラピッチ方向(-)回転コマンド(キーバインド: KEY_DOWN) */
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].command = FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].keybind = KEY_DOWN;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS].pfn_command_executor = flight_camera_controller_rot_pitch_minus;

    // FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS,       /**< カメラヨー方向(+)回転コマンド(キーバインド: KEY_LEFT) */
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].command = FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].keybind = KEY_LEFT;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS].pfn_command_executor = flight_camera_controller_rot_yaw_plus;

    // FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS,      /**< カメラヨー方向(-)回転コマンド(キーバインド: KEY_RIGHT) */
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].command = FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].keybind = KEY_RIGHT;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].status = false;
    command_status_[FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS].pfn_command_executor = flight_camera_controller_rot_yaw_minus;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t flight_camera_command_update(const keyboard_event_t* keyboard_event_, command_status_flight_camera_t* command_status_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_update", "keyboard_event_")
    IF_ARG_NULL_GOTO_CLEANUP(command_status_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "flight_camera_command_update", "command_status_")

    for(size_t i = 0; i != FLIGHT_CAMERA_COMMAND_MAX; ++i) {
        if(command_status_[i].keybind == keyboard_event_->key) {
            if(keyboard_event_->event_args.pressed) {   // キーが押されたらコマンドON
                command_status_[i].status = true;
                INFO_MESSAGE("flight_camera_command_update: %s", s_command_to_str(i));
            } else if(command_status_[i].status && !keyboard_event_->event_args.pressed) {  // キーが離されたらコマンドOFF
                command_status_[i].status = false;
                INFO_MESSAGE("flight_camera_command_update: %s Released", s_command_to_str(i));
            }
        }
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

application_result_t flight_camera_command_execute(float speed_, float delta_time_, camera_t* camera_, command_status_flight_camera_t* command_status_, bool* out_view_updated_) {
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
