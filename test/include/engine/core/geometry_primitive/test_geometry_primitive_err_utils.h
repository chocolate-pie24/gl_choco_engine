/**
 * @file test_geometry_primitive_err_utils.h
 * @author chocolate-pie24
 * @brief geometry_primitive_err_utilsテスト用API定義
 *
 * @version 0.1
 * @date 2026-06-09
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_CORE_GEOMETRY_PRIMITIVE_TEST_GEOMETRY_PRIMITIVE_ERR_UTILS_H
#define GLCE_TEST_ENGINE_CORE_GEOMETRY_PRIMITIVE_TEST_GEOMETRY_PRIMITIVE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief geometry_primitive_err_utilsが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_geometry_primitive_err_utils_config_reset(void);

/**
 * @brief geometry_primitive_err_utils保有APIのテストを行う
 *
 */
void test_geometry_primitive_err_utils(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
