// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_INTERNAL_RENDERER_BACKEND_CONTEXT_INTERNAL_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_INTERNAL_RENDERER_BACKEND_CONTEXT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/core/renderer_types.h"

typedef struct renderer_shader_vtable renderer_shader_vtable_t;
typedef struct renderer_vao_vtable renderer_vao_vtable_t;
typedef struct renderer_vbo_vtable renderer_vbo_vtable_t;
typedef struct renderer_texture_vtable renderer_texture_vtable_t;

/**
 * @brief RendererBackend内部状態管理構造体
 *
 */
struct renderer_backend_context {
    target_graphics_api_t target_api;                   /**< 使用グラフィックスAPI */

    const renderer_shader_vtable_t* shader_vtable;      /**< シェーダー機能提供vtable */
    const renderer_vao_vtable_t* vao_vtable;            /**< VAO機能提供vtable */
    const renderer_vbo_vtable_t* vbo_vtable;            /**< VBO機能提供vtable */
    const renderer_texture_vtable_t* texture_vtable;    /**< Texture機能提供vtable */
};

#ifdef __cplusplus
}
#endif
#endif
