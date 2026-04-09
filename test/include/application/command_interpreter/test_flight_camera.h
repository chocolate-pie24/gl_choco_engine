/**
 * @file test_flight_camera.h
 * @author chocolate-pie24
 * @brief command_interpreter/flight_cameraモジュール用テストAPI定義
 *
 * @version 0.1
 * @date 2026-04-09
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_APPLICATION_COMMAND_INTERPRETER_TEST_FLIGHT_CAMERA_H
#define GLCE_TEST_APPLICATION_COMMAND_INTERPRETER_TEST_FLIGHT_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief flight_camera_command_initialize()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_command_initialize_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_command_update()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_command_update_config_set(const test_call_control_t* config_);

/**
 * @brief flight_camera_command_execute()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、flight_camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_flight_camera_command_execute_config_set(const test_call_control_t* config_);

/**
 * @brief flight_cameraが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_flight_camera_config_reset(void);

/**
 * @brief platform_err_utils保有APIのテストを行う
 *
 */
void test_flight_camera(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
