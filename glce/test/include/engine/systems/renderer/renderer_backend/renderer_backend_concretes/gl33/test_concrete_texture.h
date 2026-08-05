/**
 * @file test_concrete_texture.h
 * @author chocolate-pie24
 * @brief gl33/concrete_textureモジュール用テストAPI定義
 *
 * @version 0.1
 * @date 2026-05-11
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_TEST_CONCRETE_TEXTURE_H
#define GLCE_TEST_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_TEST_CONCRETE_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief concrete_textureが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_concrete_texture_config_reset(void);

/**
 * @brief concrete_texture保有APIのテストを行う
 *
 */
void test_concrete_texture(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
