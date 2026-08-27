// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file gl33_texture.h
 * @author chocolate-pie24
 * @brief renderer backendがテクスチャ機能をOpenGL3.3で実現できるように、renderer_texture_vtable_tのOpenGL3.3具体実装を提供する
 *
 * @date 2026-05-15
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_TEXTURE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct renderer_texture_vtable renderer_texture_vtable_t;

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
