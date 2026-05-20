/**
 * @file test_platform_glfw.h
 * @author chocolate-pie24
 * @brief platform_glfwモジュール用テストAPI定義
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
#ifndef GLCE_TEST_ENGINE_SYSTEMS_PLATFORM_PLATFORM_CONCRETES_TEST_PLATFORM_GLFW_H
#define GLCE_TEST_ENGINE_SYSTEMS_PLATFORM_PLATFORM_CONCRETES_TEST_PLATFORM_GLFW_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief platform_glfwが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_platform_glfw_config_reset(void);

/**
 * @brief platform_glfw保有APIのテストを行う
 *
 */
void test_platform_glfw(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
