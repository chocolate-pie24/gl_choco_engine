/**
 * @file test_renderer_err_utils.h
 * @author chocolate-pie24
 * @brief renderer_err_utilsモジュール用テストAPI定義
 *
 * @version 0.1
 * @date 2026-04-03
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_SYSTEMS_RENDERER_RENDERER_CORE_TEST_RENDERER_ERR_UTILS_H
#define GLCE_TEST_ENGINE_SYSTEMS_RENDERER_RENDERER_CORE_TEST_RENDERER_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief renderer_rslt_convert_linear_alloc()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_err_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_rslt_convert_linear_alloc_config_set(const test_call_control_t* config_);

/**
 * @brief renderer_rslt_convert_choco_memory()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_err_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_rslt_convert_choco_memory_config_set(const test_call_control_t* config_);

/**
 * @brief renderer_rslt_convert_choco_string()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_err_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_rslt_convert_choco_string_config_set(const test_call_control_t* config_);

/**
 * @brief renderer_rslt_convert_fs_utils()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_err_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_rslt_convert_fs_utils_config_set(const test_call_control_t* config_);

/**
 * @brief renderer_err_utilsが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_renderer_err_utils_config_reset(void);

/**
 * @brief renderer_err_utils保有APIのテストを行う
 *
 */
void test_renderer_err_utils(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
