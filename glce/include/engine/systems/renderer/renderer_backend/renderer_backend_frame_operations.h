// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_FRAME_OPERATIONS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_FRAME_OPERATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend内部状態管理構造体前方宣言 */

void renderer_backend_depth_test_set(renderer_backend_context_t* backend_context_, bool enable_);
renderer_backend_result_t renderer_backend_viewport_set(renderer_backend_context_t* backend_context_, int32_t x_, int32_t y_, int32_t framebuffer_width_, int32_t framebuffer_height_);
void renderer_backend_framebuffer_clear(renderer_backend_context_t* backend_context_, uint32_t mask_);
renderer_backend_result_t renderer_backend_draw_arrays(renderer_backend_context_t* backend_context_, rendering_primitive_t primitive_, const draw_range_t* draw_range_);

#ifdef __cplusplus
}
#endif
#endif
