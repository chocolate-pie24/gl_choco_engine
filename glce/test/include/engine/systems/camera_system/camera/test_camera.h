/**
 * @file test_camera.h
 * @author chocolate-pie24
 * @brief cameraモジュール用テストAPI定義
 *
 * @version 0.1
 * @date 2026-03-30
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_TEST_CAMERA_H
#define GLCE_TEST_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_TEST_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief camera_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_create_config_set(const test_call_control_t* config_);

/**
 * @brief camera_viewing_frustum_update()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_viewing_frustum_update_config_set(const test_call_control_t* config_);

/**
 * @brief camera_euler_update()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_euler_update_config_set(const test_call_control_t* config_);

/**
 * @brief camera_position_update()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_position_update_config_set(const test_call_control_t* config_);

/**
 * @brief camera_euler_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_euler_get_config_set(const test_call_control_t* config_);

/**
 * @brief camera_position_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_position_get_config_set(const test_call_control_t* config_);

/**
 * @brief camera_perspective_matrix_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_perspective_matrix_get_config_set(const test_call_control_t* config_);

/**
 * @brief camera_view_matrix_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_view_matrix_get_config_set(const test_call_control_t* config_);

/**
 * @brief camera_forward_vector_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_forward_vector_get_config_set(const test_call_control_t* config_);

/**
 * @brief camera_backward_vector_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_backward_vector_get_config_set(const test_call_control_t* config_);

/**
 * @brief camera_right_vector_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_right_vector_get_config_set(const test_call_control_t* config_);

/**
 * @brief camera_left_vector_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_left_vector_get_config_set(const test_call_control_t* config_);

/**
 * @brief camera_up_vector_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_up_vector_get_config_set(const test_call_control_t* config_);

/**
 * @brief camera_down_vector_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、camera内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_camera_down_vector_get_config_set(const test_call_control_t* config_);

/**
 * @brief cameraが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_camera_config_reset(void);

/**
 * @brief camera保有APIのテストを行う
 *
 */
void test_camera(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
