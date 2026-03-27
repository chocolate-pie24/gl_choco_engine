/**
 * @file test_choco_string.h
 * @author chocolate-pie24
 * @brief Choco Stringモジュールテスト用API定義
 *
 * @version 0.1
 * @date 2026-03-17
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_CONTAINERS_TEST_CHOCO_STRING_H
#define GLCE_TEST_ENGINE_CONTAINERS_TEST_CHOCO_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief choco_string_default_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Choco String内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_choco_string_default_create_config_set(const test_call_control_t* config_);

/**
 * @brief choco_string_create_from_c_string()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Choco String内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_choco_string_create_from_c_string_config_set(const test_call_control_t* config_);

/**
 * @brief choco_string_copy()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Choco String内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_choco_string_copy_config_set(const test_call_control_t* config_);

/**
 * @brief choco_string_copy_from_c_string()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Choco String内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_choco_string_copy_from_c_string_config_set(const test_call_control_t* config_);

/**
 * @brief choco_string_concat()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Choco String内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_choco_string_concat_config_set(const test_call_control_t* config_);

/**
 * @brief choco_string_concat_from_c_string()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Choco String内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_choco_string_concat_from_c_string_config_set(const test_call_control_t* config_);

/**
 * @brief choco_string_length()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Choco String内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_choco_string_length_config_set(const test_call_control_size_t_t* config_);

/**
 * @brief Choco Stringが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_choco_string_config_reset(void);

/**
 * @brief Choco String保有APIのテストを行う
 *
 */
void test_choco_string(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
