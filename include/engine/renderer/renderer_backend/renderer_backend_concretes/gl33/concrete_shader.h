/**
 * @ingroup gl33
 *
 * @file concrete_shader.h
 * @author chocolate-pie24
 * @brief gl33_shaderは、renderer backendがシェーダー機能をOpenGL3.3で実現できるように、renderer_shader_vtable_tのOpenGL3.3具体実装を提供する
 *
 * @version 0.1
 * @date 2026-01-03
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_CONCRETE_SHADER_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_CONCRETE_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_shader.h"

/**
 * @brief OpenGL3.3用シェーダー仮想関数テーブル(vtable)を取得する
 *
 * @return const renderer_shader_vtable_t* OpenGL3.3用シェーダーvtable
 */
const renderer_shader_vtable_t* gl33_shader_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
