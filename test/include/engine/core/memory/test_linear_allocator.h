/**
 * @file test_linear_allocator.h
 * @author chocolate-pie24
 * @brief Linear Allocatorモジュールテスト用API定義
 *
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_CORE_MEMORY_TEST_LINEAR_ALLOCATOR_H
#define GLCE_TEST_ENGINE_CORE_MEMORY_TEST_LINEAR_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief linear_allocator_init()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Linear Allocator内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_linear_allocator_init_config_set(const test_call_control_t* config_);

/**
 * @brief linear_allocator_allocate()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Linear Allocator内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_linear_allocator_allocate_config_set(const test_call_control_t* config_);

/**
 * @brief Linear Allocatorモジュールのテスト設定値を全て初期化し、テスト専用出力をなくす
 *
 */
void test_linear_allocator_config_reset(void);

/**
 * @brief Linear AllocatorモジュールAPIのテストを行う
 *
 */
void test_linear_allocator(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
