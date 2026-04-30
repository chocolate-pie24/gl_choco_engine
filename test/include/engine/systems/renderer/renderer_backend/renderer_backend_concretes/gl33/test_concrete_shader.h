/**
 * @file test_concrete_shader.h
 * @author chocolate-pie24
 * @brief gl33/concrete_shaderモジュール用テストAPI定義
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
#ifndef GLCE_TEST_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_TEST_CONCRETE_SHADER_H
#define GLCE_TEST_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_TEST_CONCRETE_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief concrete_shaderが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_concrete_shader_config_reset(void);

/**
 * @brief concrete_shader保有APIのテストを行う
 *
 */
void test_concrete_shader(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
