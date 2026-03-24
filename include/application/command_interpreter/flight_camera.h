#ifndef GLCE_APPLICATION_COMMAND_INTERPRETER_FLIGHT_CAMERA_H
#define GLCE_APPLICATION_COMMAND_INTERPRETER_FLIGHT_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/core/event/keyboard_event.h"

#include "engine/camera_system/camera/camera.h"
#include "engine/camera_system/camera_controller/flight_camera_controller.h"

#include "application/application_core/application_types.h"

typedef enum {
    FLIGHT_CAMERA_COMMAND_MOVE_FORWARD = 0,   /**< カメラ前進コマンド(キーバインド: KEY_W) */
    FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD,      /**< カメラ後進コマンド(キーバインド: KEY_S) */
    FLIGHT_CAMERA_COMMAND_MOVE_RIGHT,         /**< カメラ右移動コマンド(キーバインド: KEY_D) */
    FLIGHT_CAMERA_COMMAND_MOVE_LEFT,          /**< カメラ左移動コマンド(キーバインド: KEY_A) */
    FLIGHT_CAMERA_COMMAND_MOVE_UP,            /**< カメラ上方向移動コマンド(キーバインド: KEY_E) */
    FLIGHT_CAMERA_COMMAND_MOVE_DOWN,          /**< カメラ下方向移動コマンド(キーバインド: KEY_Q) */
    FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS,     /**< カメラピッチ方向(+)回転コマンド(キーバインド: KEY_UP) */
    FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS,    /**< カメラピッチ方向(-)回転コマンド(キーバインド: KEY_DOWN) */
    FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS,       /**< カメラヨー方向(+)回転コマンド(キーバインド: KEY_LEFT) */
    FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS,      /**< カメラヨー方向(-)回転コマンド(キーバインド: KEY_RIGHT) */
    FLIGHT_CAMERA_COMMAND_MAX,                /**< フライトカメラ制御コマンド数 */
} command_list_flight_camera_t;

typedef struct command_status_flight_camera {
    command_list_flight_camera_t command;   /**< フライトカメラ制御コマンド */
    keycode_t keybind;                      /**< 制御コマンドに対応するキーバインド */
    bool status;                            /**< 制御コマンド状態(true: コマンドあり / false: コマンドなし) */
    camera_result_t (*pfn_command_executor)(float speed_, float delta_time_, camera_t* camera_);  /**< コマンド実行関数 */
} command_status_flight_camera_t;

application_result_t flight_camera_command_initialize(size_t array_size_, command_status_flight_camera_t* command_status_);

application_result_t flight_camera_command_update(const keyboard_event_t* keyboard_event_, command_status_flight_camera_t* command_status_);

application_result_t flight_camera_command_execute(float speed_, float delta_time_, camera_t* camera_, command_status_flight_camera_t* command_status_, bool* out_camera_updated_);

#ifdef __cplusplus
}
#endif
#endif
