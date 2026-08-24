// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file gl33_vao.h
 * @author chocolate-pie24
 * @brief renderer backendがVAO機能をOpenGL3.3で実現できるように、renderer_vao_vtable_tのOpenGL3.3具体実装を提供する
 *
 * @date 2026-01-03
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_VAO_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_VAO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct renderer_vao_vtable renderer_vao_vtable_t;

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
