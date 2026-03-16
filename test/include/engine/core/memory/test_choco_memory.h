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
#ifndef GLCE_TEST_ENGINE_CORE_MEMORY_TEST_CHOCO_MEMORY_H
#define GLCE_TEST_ENGINE_CORE_MEMORY_TEST_CHOCO_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief memory_system_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、メモリーシステム内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_memory_system_create_config_set(const test_call_control_t* config_);

/**
 * @brief memory_system_allocate()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、メモリーシステム内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_memory_system_allocate_config_set(const test_call_control_t* config_);

/**
 * @brief メモリーシステムが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_choco_memory_config_reset(void);

/**
 * @brief メモリーシステム保有APIのテストを行う
 *
 */
void test_choco_memory(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
