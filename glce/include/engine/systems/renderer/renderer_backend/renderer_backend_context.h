// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_context.h
 * @author chocolate-pie24
 * @brief renderer_backend内部情報管理構造体のリソース管理APIを提供する
 *
 * @date 2026-02-23
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< renderer_backend_context内部情報管理構造体前方宣言 */

typedef struct linear_allocator linear_allocator_t;

renderer_backend_result_t renderer_backend_create(linear_allocator_t* allocator_, renderer_backend_context_t** out_renderer_backend_context_);

void renderer_backend_deinitialize(renderer_backend_context_t* renderer_backend_context_);

#ifdef __cplusplus
}
#endif
#endif
