/**
 * @file test_controller.h
 * @author chocolate-pie24
 * @brief 各API単体テスト時に使用モジュールの実行結果を制御するための失敗注入機能の定義
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
#ifndef GLCE_TEST_TEST_CONTROLLER_H
#define GLCE_TEST_TEST_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 各種APIのテスト用失敗注入構造体(返り値が実行結果コード or intの関数用)
 *
 */
typedef struct test_call_control {
    uint32_t call_count;    /**< API呼び出し回数 */
    uint32_t fail_on_call;  /**< APIを何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    int forced_result;      /**< APIを強制的に失敗させる際の戻り値(各種実行結果コードをintにキャストして使用する) */
} test_call_control_t;

/**
 * @brief 各種APIのテスト用失敗注入構造体(返り値がsize_tの関数用)
 *
 */
typedef struct test_call_control_size_t {
    uint32_t call_count;    /**< API呼び出し回数 */
    uint32_t fail_on_call;  /**< APIを何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    size_t forced_result;   /**< APIを強制的に失敗させる際の戻り値 */
} test_call_control_size_t_t;

/**
 * @brief 各種APIのテスト用失敗注入構造体(返り値がboolの関数用)
 *
 */
typedef struct test_call_control_bool {
    uint32_t call_count;    /**< API呼び出し回数 */
    uint32_t fail_on_call;  /**< APIを何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はforced_resultを出力させる) */
    bool forced_result;   /**< APIを強制的に失敗させる際の戻り値 */
} test_call_control_bool_t;

/**
 * @brief 失敗注入構造体のフィールドを失敗注入なしにリセットする
 *
 * @details リセット後は構造体のフィールドは以下の値になる
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = 0(多くの場合SUCCESS)
 *
 * @param[out] control_ リセット対象構造体へのポインタ
 */
void test_call_control_reset(test_call_control_t* control_);

/**
 * @brief 失敗注入構造体のフィールドを失敗注入なしにリセットする
 *
 * @details リセット後は構造体のフィールドは以下の値になる
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = 0
 *
 * @param[out] control_ リセット対象構造体へのポインタ
 */
void test_call_control_size_t_reset(test_call_control_size_t_t* control_);

/**
 * @brief 失敗注入構造体のフィールドを失敗注入なしにリセットする
 *
 * @details リセット後は構造体のフィールドは以下の値になる
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = false
 *
 * @param[out] control_ リセット対象構造体へのポインタ
 */
void test_call_control_bool_reset(test_call_control_bool_t* control_);

#endif

#ifdef __cplusplus
}
#endif
#endif
