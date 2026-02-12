/**
 * @ingroup gl33
 *
 * @file gl33_shader.h
 * @author chocolate-pie24
 * @brief gl33_shaderは、renderer backendがシェーダー機能をOpenGL3.3で実現できるように、renderer_shader_vtable_tのOpenGL3.3具体実装を提供する
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

/**
 * @brief OpenGL3.3用シェーダー仮想関数テーブル(vtable)を取得する
 *
 * @return const renderer_shader_vtable_t* OpenGL3.3用シェーダーvtable
 */
const renderer_shader_vtable_t* gl33_shader_vtable_get(void);

#ifdef TEST_BUILD
#include "engine/renderer/renderer_core/renderer_types.h"

/**
 * @brief テスト用に、gl33_shaderの実行結果コードを指定値に固定する
 *
 * @note この関数を実行した後は、gl33_shader_fail_disableが呼ばれるまで効果が継続する
 *
 * @param result_code_ 出力実行結果コード
 */
void gl33_shader_fail_enable(renderer_result_t result_code_);

/**
 * @brief gl33_shader_fail_enableによる実行結果コードの固定を解除する
 *
 */
void gl33_shader_fail_disable(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
