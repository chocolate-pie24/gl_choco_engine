/** @ingroup application
 *
 * @file flight_camera.h
 * @author chocolate-pie24
 * @brief アプリケーションからのイベント情報をもとにフライトカメラを制御するAPIを提供する
 *
 * @note フライトカメラ制御キーバインド
 * - KEY_W: カメラ前進
 * - KEY_S: カメラ後進
 * - KEY_D: カメラ右移動
 * - KEY_A: カメラ左移動
 * - KEY_E: カメラ上方向移動
 * - KEY_Q: カメラ下方向移動コマンド
 * - KEY_UP: カメラピッチ方向(+)回転
 * - KEY_DOWN: カメラピッチ方向(-)回転
 * - KEY_LEFT: カメラヨー方向(+)回転
 * - KEY_RIGHT: カメラヨー方向(-)回転
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

/**
 * @brief フライトカメラ制御コマンドリスト
 *
 */
typedef enum {
    FLIGHT_CAMERA_COMMAND_MOVE_FORWARD = 0,   /**< フライトカメラ制御コマンド: 前方移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_BACKWARD,      /**< フライトカメラ制御コマンド: 後方移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_RIGHT,         /**< フライトカメラ制御コマンド: 右方向移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_LEFT,          /**< フライトカメラ制御コマンド: 左方向移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_UP,            /**< フライトカメラ制御コマンド: 上方向移動 */
    FLIGHT_CAMERA_COMMAND_MOVE_DOWN,          /**< フライトカメラ制御コマンド: 下方向移動 */
    FLIGHT_CAMERA_COMMAND_ROT_PITCH_PLUS,     /**< フライトカメラ制御コマンド: ピッチ+方向回転 */
    FLIGHT_CAMERA_COMMAND_ROT_PITCH_MINUS,    /**< フライトカメラ制御コマンド: ピッチ-方向回転 */
    FLIGHT_CAMERA_COMMAND_ROT_YAW_PLUS,       /**< フライトカメラ制御コマンド: ヨー+方向回転 */
    FLIGHT_CAMERA_COMMAND_ROT_YAW_MINUS,      /**< フライトカメラ制御コマンド: ヨー-方向回転 */
    FLIGHT_CAMERA_COMMAND_MAX,                /**< フライトカメラ制御コマンド数 */
} command_list_flight_camera_t;

/**
 * @brief フライトカメラ制御コマンド実行用構造体
 *
 */
typedef struct command_status_flight_camera {
    command_list_flight_camera_t command;   /**< フライトカメラ制御コマンド */
    keycode_t keybind;                      /**< 制御コマンドに割り当てられたキーバインド */
    bool status;                            /**< 制御コマンド実行要求有無(true: 実行要求あり / false: 実行要求なし) */
    camera_result_t (*pfn_command_executor)(float speed_, float delta_time_, camera_t* camera_);  /**< コマンド実行関数 */
} command_status_flight_camera_t;

/**
 * @brief フライトカメラ制御コマンド実行用構造体インスタンス配列を初期化する
 *
 * @param[in] array_size_ 構造体配列サイズ
 * @param[out] command_status_ 初期化対象構造体インスタンス配列
 *
 * @retval APPLICATION_INVALID_ARGUMENT 以下のいずれか
 * - command_status_ == NULL
 * - array_size_ != FLIGHT_CAMERA_COMMAND_MAX
 * @retval APPLICATION_SUCCESS 処理に成功し、正常終了
 */
application_result_t flight_camera_command_initialize(size_t array_size_, command_status_flight_camera_t* command_status_);

/**
 * @brief キーボードイベント1件を受け取り、イベントに応じて制御コマンド実行用構造体インスタンスのフィールドを更新する
 *
 * @details
 * - フライトカメラ制御対象キーが押下されたら対象コマンド実行要求フラグをONし、コマンド更新メッセージを標準出力に出力する
 * - フライトカメラ制御対象キーが離されたら対象コマンド実行要求フラグをOFFし、コマンド更新メッセージを標準出力に出力する
 *
 * @param[in] keyboard_event_ キーボードイベント構造体インスタンスへのポインタ
 * @param[in,out] command_status_ 制御コマンド実行用構造体インスタンス配列
 *
 * @retval APPLICATION_INVALID_ARGUMENT 以下のいずれか
 * - keyboard_event_ == NULL
 * - command_status_ == NULL
 * @retval APPLICATION_SUCCESS 処理に成功し、正常終了
 */
application_result_t flight_camera_command_update(const keyboard_event_t* keyboard_event_, command_status_flight_camera_t* command_status_);

/**
 * @brief フライトカメラ制御コマンド実行用構造体インスタンス配列のフィールド状態に基づいて制御を実行する
 *
 * @note
 * - 制御が実行され、カメラが動いた場合はcamera_構造体フィールドが更新される
 *
 * @param[in] speed_ カメラ移動速度
 * @param[in] delta_time_ カメラ移動時間
 * @param[in] command_status_ フライトカメラ制御コマンド実行用構造体インスタンス配列
 * @param[in,out] camera_ 制御対象カメラ構造体インスタンスへのポインタ
 * @param[out] out_view_updated_ フライトカメラが動いたことの通知フラグへのポインタ(true: カメラ動作あり / false: カメラ動作なし)
 *
 * @retval APPLICATION_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - command_status_ == NULL
 * - out_view_updated_ == NULL
 * @retval APPLICATION_RUNTIME_ERROR 以下のいずれか
 * - カメラ位置の取得,更新に失敗
 * - 逆行列計算に失敗
 * @retval APPLICATION_SUCCESS 処理に成功し、正常終了
 */
application_result_t flight_camera_command_execute(float speed_, float delta_time_, command_status_flight_camera_t* command_status_, camera_t* camera_, bool* out_view_updated_);

#ifdef __cplusplus
}
#endif
#endif
