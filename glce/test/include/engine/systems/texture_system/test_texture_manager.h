/**
 * @file test_texture_manager.h
 * @author chocolate-pie24
 * @brief texture_managerモジュール用テストAPI定義
 *
 * @version 0.1
 * @date 2026-05-12
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_SYSTEMS_TEXTURE_SYSTEM_TEST_TEXTURE_MANAGER_H
#define GLCE_TEST_ENGINE_SYSTEMS_TEXTURE_SYSTEM_TEST_TEXTURE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief texture_manager_initialize()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture_manager内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_manager_initialize_config_set(const test_call_control_t* config_);

/**
 * @brief texture_manager_register()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture_manager内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_manager_register_config_set(const test_call_control_t* config_);

/**
 * @brief texture_manager_unregister()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture_manager内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_manager_unregister_config_set(const test_call_control_t* config_);

/**
 * @brief texture_manager_unregister_by_name()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture_manager内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_manager_unregister_by_name_config_set(const test_call_control_t* config_);

/**
 * @brief texture_manager_texture_id_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture_manager内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_manager_texture_id_get_config_set(const test_call_control_t* config_);

/**
 * @brief texture_manager_gpu_resource_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture_manager内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_manager_gpu_resource_get_config_set(const test_call_control_t* config_);

/**
 * @brief texture_manager_gpu_resource_get_by_name()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture_manager内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_manager_gpu_resource_get_by_name_config_set(const test_call_control_t* config_);

/**
 * @brief texture_managerが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_texture_manager_config_reset(void);

/**
 * @brief texture_manager保有APIのテストを行う
 *
 */
void test_texture_manager(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
