/**
 * @ingroup gl33
 *
 * @file concrete_texture.h
 * @author chocolate-pie24
 * @brief renderer backendがテクスチャ機能をOpenGL3.3で実現できるように、renderer_texture_vtable_tのOpenGL3.3具体実装を提供する
 *
 * @version 0.1
 * @date 2026-05-15
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_CONCRETE_TEXTURE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_CONCRETE_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/renderer_backend/renderer_backend_interface/interface_texture.h"

/**
 * @brief OpenGL3.3用texture仮想関数テーブル(vtable)を取得する
 *
 * @return const renderer_texture_vtable_t* OpenGL3.3用texture vtable
 */
const renderer_texture_vtable_t* gl33_texture_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
