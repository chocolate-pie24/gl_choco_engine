// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file gl33_vbo.h
 * @author chocolate-pie24
 * @brief renderer backendがVBO機能をOpenGL3.3で実現できるように、renderer_vbo_vtable_tのOpenGL3.3具体実装を提供する
 *
 * @date 2026-01-03
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_VBO_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_VBO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct renderer_vbo_vtable renderer_vbo_vtable_t;

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
