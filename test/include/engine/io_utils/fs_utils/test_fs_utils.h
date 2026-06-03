/**
 * @file test_fs_utils.h
 * @author chocolate-pie24
 * @brief fs_utilsモジュールテスト用API定義
 *
 * @version 0.1
 * @date 2026-04-02
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_IO_UTILS_FS_UTILS_TEST_FS_UTILS_H
#define GLCE_TEST_ENGINE_IO_UTILS_FS_UTILS_TEST_FS_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief fs_utils_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、fs_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_fs_utils_create_config_set(const test_call_control_t* config_);

/**
 * @brief fs_utils_text_file_read()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、fs_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_fs_utils_text_file_read_config_set(const test_call_control_t* config_);

/**
 * @brief fs_utils_text_file_line_read()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、fs_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_fs_utils_text_file_line_read_config_set(const test_call_control_t* config_);

/**
 * @brief fs_utils_fullpath_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、fs_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_fs_utils_fullpath_get_config_set(const test_call_control_t* config_);

/**
 * @brief fs_utilsが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_fs_utils_config_reset(void);

/**
 * @brief fs_utils保有APIのテストを行う
 *
 */
void test_fs_utils(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
