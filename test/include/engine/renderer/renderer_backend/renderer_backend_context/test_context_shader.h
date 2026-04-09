/**
 * @file test_context_shader.h
 * @author chocolate-pie24
 * @brief context_shaderモジュール用テストAPI定義
 *
 * @version 0.1
 * @date 2026-04-08
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_TEST_CONTEXT_SHADER_H
#define GLCE_TEST_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_TEST_CONTEXT_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief renderer_backend_shader_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_backend_context.c内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_backend_shader_create_config_set(const test_call_control_t* config_);

/**
 * @brief renderer_backend_shader_compile()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_backend_context.c内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_backend_shader_compile_config_set(const test_call_control_t* config_);

/**
 * @brief renderer_backend_shader_link()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_backend_context.c内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_backend_shader_link_config_set(const test_call_control_t* config_);

/**
 * @brief renderer_backend_shader_use()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_backend_context.c内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_backend_shader_use_config_set(const test_call_control_t* config_);

/**
 * @brief renderer_backend_shader_uniform_location_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_backend_context.c内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_backend_shader_uniform_location_get_config_set(const test_call_control_t* config_);

/**
 * @brief renderer_backend_shader_mat4f_uniform_set()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、renderer_backend_context.c内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_renderer_backend_shader_mat4f_uniform_set_config_set(const test_call_control_t* config_);

// NOTE: test_module_name_reset(), test_module_name()はtest_renderer_backend_context.hで定義

#endif

#ifdef __cplusplus
}
#endif
#endif
