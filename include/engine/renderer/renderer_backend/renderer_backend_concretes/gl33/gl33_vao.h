/**
 * @ingroup gl33
 *
 * @file gl33_vao.h
 * @author chocolate-pie24
 * @brief gl33_vaoは、renderer backendがVAO機能をOpenGL3.3で実現できるように、renderer_vao_vtable_tのOpenGL3.3具体実装を提供する
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
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_VAO_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_VAO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_backend/renderer_backend_interface/vertex_array_object.h"

/**
 * @brief OpenGL3.3用VAO仮想関数テーブル(vtable)を取得する
 *
 * @return const renderer_vao_vtable_t* OpenGL3.3用VAO vtable
 */
const renderer_vao_vtable_t* gl33_vao_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
