/** @ingroup gl33
 *
 * @file gl33_shader.h
 * @author chocolate-pie24
 * @brief OpenGL3.3用のシェーダープログラムのコンパイル、リンク、使用開始APIを提供する
 *
 * @version 0.1
 * @date 2026-01-03
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_SHADER_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_backend/renderer_backend_interface/shader.h"

const renderer_shader_vtable_t* gl33_shader_vtable_get(void);

#ifdef TEST_BUILD
#include "engine/renderer/renderer_core/renderer_types.h"

// 引数で与えた実行結果コードを強制的に出力させる
void gl33_shader_fail_enable(renderer_result_t result_code_);
void gl33_shader_fail_disable(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
