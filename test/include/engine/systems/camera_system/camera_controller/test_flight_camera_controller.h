/**
 * @file test_flight_camera_controller.h
 * @author chocolate-pie24
 * @brief フライトカメラ制御モジュール用テストAPI定義
 *
 * @version 0.1
 * @date 2026-03-31
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_CONTROLLER_TEST_FLIGHT_CAMERA_CONTROLLER_H
#define GLCE_TEST_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_CONTROLLER_TEST_FLIGHT_CAMERA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief flight_camera_controller_move_forward()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_move_forward_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controller_move_backward()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_move_backward_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controller_move_right()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_move_right_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controller_move_left()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_move_left_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controller_move_up()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_move_up_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controller_move_down()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_move_down_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controller_rot_pitch_plus()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_rot_pitch_plus_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controller_rot_pitch_minus()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_rot_pitch_minus_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controller_rot_yaw_plus()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_rot_yaw_plus_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controller_rot_yaw_minus()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera_controller内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_controller_rot_yaw_minus_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_controllerが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_flight_camera_controller_config_reset(void);

/**
 * @brief flight_camera_controller保有APIのテストを行う
 *
 */
void test_flight_camera_controller(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
