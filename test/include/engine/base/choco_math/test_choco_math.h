/**
 * @file test_choco_math.h
 * @author chocolate-pie24
 * @brief choco_mathモジュール用テストAPI定義
 *
 * @version 0.1
 * @date 2026-04-01
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_BASE_CHOCO_MATH_TEST_CHOCO_MATH_H
#define GLCE_TEST_ENGINE_BASE_CHOCO_MATH_TEST_CHOCO_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief mat4f_inverse()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、choco_math内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_mat4f_inverse_config_set(const test_call_control_bool_t* config_);

/**
 * @brief choco_mathが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_choco_math_config_reset(void);

/**
 * @brief choco_math保有APIのテストを行う
 *
 */
void test_choco_math(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
