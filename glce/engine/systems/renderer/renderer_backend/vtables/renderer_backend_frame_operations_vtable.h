// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_FRAME_OPERATIONS_VTABLE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_VTABLES_RENDERER_BACKEND_FRAME_OPERATIONS_VTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/core/renderer_types.h"

typedef struct draw_range draw_range_t;

typedef void (*pfn_renderer_depth_test_set)(bool enable_);
typedef renderer_backend_result_t (*pfn_renderer_viewport_set)(int32_t x_, int32_t y_, int32_t framebuffer_width_, int32_t framebuffer_height_);
typedef void (*pfn_renderer_framebuffer_clear)(uint32_t mask_);
typedef renderer_backend_result_t (*pfn_renderer_draw_arrays)(rendering_primitive_t primitive_, const draw_range_t* draw_range_);

typedef struct renderer_frame_operations_vtable {
    pfn_renderer_depth_test_set renderer_set_depth_test;
    pfn_renderer_viewport_set renderer_viewport_set;
    pfn_renderer_framebuffer_clear renderer_framebuffer_clear;
    pfn_renderer_draw_arrays renderer_draw_arrays;
} renderer_frame_operations_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
