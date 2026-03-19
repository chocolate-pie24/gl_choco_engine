/**
 * @file test_filesystem.h
 * @author chocolate-pie24
 * @brief File Systemモジュールテスト用API定義
 *
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_CORE_FILESYSTEM_TEST_FILESYSTEM_H
#define GLCE_TEST_ENGINE_CORE_FILESYSTEM_TEST_FILESYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief filesystem_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、File System内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_filesystem_create_config_set(const test_call_control_t* config_);

/**
 * @brief filesystem_open()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、File System内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_filesystem_open_config_set(const test_call_control_t* config_);

/**
 * @brief filesystem_close()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、File System内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_filesystem_close_config_set(const test_call_control_t* config_);

/**
 * @brief filesystem_byte_read()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、File System内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_filesystem_byte_read_config_set(const test_call_control_t* config_);

/**
 * @brief File Systemが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_filesystem_config_reset(void);

/**
 * @brief File System保有APIのテストを行う
 *
 */
void test_filesystem(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
