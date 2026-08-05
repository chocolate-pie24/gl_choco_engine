/**
 * @file test_ring_queue.h
 * @author chocolate-pie24
 * @brief Ring Queueモジュールテスト用API定義
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
#ifndef GLCE_TEST_ENGINE_CONTAINERS_TEST_RING_QUEUE_H
#define GLCE_TEST_ENGINE_CONTAINERS_TEST_RING_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief ring_queue_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Ring Queue内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_ring_queue_create_config_set(const test_call_control_t* config_);

/**
 * @brief ring_queue_push()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Ring Queue内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_ring_queue_push_config_set(const test_call_control_t* config_);

/**
 * @brief ring_queue_pop()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Ring Queue内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_ring_queue_pop_config_set(const test_call_control_t* config_);

/**
 * @brief ring_queue_empty()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、Ring Queue内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_ring_queue_empty_config_set(const test_call_control_bool_t* config_);

/**
 * @brief Ring Queueが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_ring_queue_config_reset(void);

void test_ring_queue(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
