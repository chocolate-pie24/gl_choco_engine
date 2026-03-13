/**
 * @file test_choco_memory.h
 * @author chocolate-pie24
 * @brief メモリーシステムテスト用API定義
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
#ifndef GLCE_TEST_TEST_CHOCO_MEMORY_H
#define GLCE_TEST_TEST_CHOCO_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief メモリーシステム各APIテスト設定データホルダ
 *
 */
typedef struct test_config_choco_memory {
    test_call_control_t test_config_memory_system_create;       /**< memory_system_create()テスト設定 */
    test_call_control_t test_config_memory_system_allocate;     /**< memory_system_allocate()テスト設定 */
} test_config_choco_memory_t;

/**
 * @brief 上位側で作成したテスト設定値をメモリーシステムに適用する
 *
 * @note API呼び出し回数についてはコピーされず、メモリーシステム内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_memory_system_config_set(const test_config_choco_memory_t* config_);

/**
 * @brief メモリーシステムのテスト設定値を全て初期化し、テスト専用出力をなくす
 *
 */
void test_memory_system_config_reset(void);

/**
 * @brief メモリーシステムAPIのテストを行う
 *
 */
void test_choco_memory(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
