/**
 * @file test_linear_allocator.h
 * @author chocolate-pie24
 * @brief Linear Allocatorモジュールテスト用API定義
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
#ifndef GLCE_TEST_TEST_LINEAR_ALLOCATOR_H
#define GLCE_TEST_TEST_LINEAR_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief Linear Allocatorモジュール各APIテスト設定データホルダ
 *
 */
typedef struct test_config_linear_allocator {
    test_call_control_t test_linear_allocator_init;         /**< linear_allocator_init()テスト設定 */
    test_call_control_t test_linear_allocator_allocate;     /**< linear_allocator_allocate()テスト設定 */
} test_config_linear_allocator_t;

/**
 * @brief 上位側で作成したテスト設定値をメモリーシステムに適用する
 *
 * @note API呼び出し回数についてはコピーされず、Linear Allocatorモジュール内で管理している値が保持される
 *
 * @param config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_linear_allocator_config_set(const test_config_linear_allocator_t* config_);

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
