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
#ifndef GLCE_TEST_TEST_FILESYSTEM_H
#define GLCE_TEST_TEST_FILESYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD

/**
 * @brief File Systemモジュール各APIテスト設定データホルダ
 *
 */
typedef struct test_config_filesystem {
    test_call_control_t test_filesystem_create;     /**< filesystem_create()テスト設定 */
    test_call_control_t test_filesystem_open        /**< filesystem_open()テスト設定 */;
    test_call_control_t test_filesystem_close;      /**< filesystem_close()テスト設定 */
    test_call_control_t test_filesystem_byte_read;  /**< filesystem_byte_read()テスト設定 */
} test_config_filesystem_t;

/**
 * @brief 上位側で作成したテスト設定値をFile Systemモジュールに適用する
 *
 * @note API呼び出し回数についてはコピーされず、File Systemモジュール内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_filesystem_config_set(const test_config_filesystem_t* config_);

/**
 * @brief File Systemモジュールのテスト設定値を全て初期化し、テスト専用出力をなくす
 *
 */
void test_filesystem_config_reset(void);

/**
 * @brief File SystemモジュールAPIのテストを行う
 *
 */
void test_filesystem(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
