// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file gl33_shader.h
 * @author chocolate-pie24
 * @brief renderer backendがシェーダー機能をOpenGL3.3で実現できるように、renderer_shader_vtable_tのOpenGL3.3具体実装を提供する
 *
 * @date 2026-01-03
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_SHADER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONCRETES_GL33_GL33_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct renderer_shader_vtable renderer_shader_vtable_t;

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
