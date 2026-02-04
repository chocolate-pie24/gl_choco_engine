/** @ingroup gl33
 *
 * @file gl33_vbo.h
 * @author chocolate-pie24
 * @brief OpenGL固有の型やAPIを使用せず、VBOを使用するためのラッパーAPIを提供する
 *
 * @version 0.1
 * @date 2025-12.19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @todo TODO: vertex_buffer_vertex_reload
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_GL33_GL33_VBO_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_GL33_GL33_VBO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/renderer/renderer_interface/vertex_buffer_object.h"

const renderer_vbo_vtable_t* gl33_vbo_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
