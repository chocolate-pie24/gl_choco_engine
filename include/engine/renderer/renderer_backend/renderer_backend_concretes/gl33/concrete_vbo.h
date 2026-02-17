/**
 * @ingroup gl33
 *
 * @file concrete_vbo.h
 * @author chocolate-pie24
 * @brief gl33_vboは、renderer backendがVBO機能をOpenGL3.3で実現できるように、renderer_vbo_vtable_tのOpenGL3.3具体実装を提供する
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
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_CONCRETE_VBO_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_CONCRETE_VBO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_vbo.h"

/**
 * @brief OpenGL3.3用VBO仮想関数テーブル(vtable)を取得する
 *
 * @return const renderer_vbo_vtable_t* OpenGL3.3用VBO vtable
 */
const renderer_vbo_vtable_t* gl33_vbo_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
