// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_backend/renderer_backend_frame_operations.h"

#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"
#include "engine/systems/renderer/renderer_backend/internal/renderer_backend_context_internal.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_frame_operations_vtable.h"

void renderer_backend_depth_test_set(renderer_backend_context_t* backend_context_, bool enable_) {
    if(NULL == backend_context_ || NULL == backend_context_->frame_operations_vtable) {
        return;
    }
    backend_context_->frame_operations_vtable->renderer_set_depth_test(enable_);
}

renderer_backend_result_t renderer_backend_viewport_set(renderer_backend_context_t* backend_context_, int32_t x_, int32_t y_, int32_t framebuffer_width_, int32_t framebuffer_height_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT);
    if(NULL == backend_context_->frame_operations_vtable) {
        ret = RENDERER_BACKEND_DATA_CORRUPTED;
        ERROR_MESSAGE("renderer_backend_viewport_set(%s) - Provided backend_context_ is corrupted.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    ret = backend_context_->frame_operations_vtable->renderer_viewport_set(x_, y_, framebuffer_width_, framebuffer_height_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_viewport_set(%s) - renderer_viewport_set failed.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

void renderer_backend_framebuffer_clear(renderer_backend_context_t* backend_context_, uint32_t mask_) {
    if(NULL == backend_context_ || NULL == backend_context_->frame_operations_vtable) {
        return;
    }
    backend_context_->frame_operations_vtable->renderer_framebuffer_clear(mask_);
}

renderer_backend_result_t renderer_backend_draw_arrays(renderer_backend_context_t* backend_context_, rendering_primitive_t primitive_, const draw_range_t* draw_range_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(draw_range_, ret, RENDERER_BACKEND_INVALID_ARGUMENT);
    if(NULL == backend_context_->frame_operations_vtable) {
        ret = RENDERER_BACKEND_DATA_CORRUPTED;
        ERROR_MESSAGE("renderer_backend_draw_arrays(%s) - Provided backend_context_ is corrupted.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    ret = backend_context_->frame_operations_vtable->renderer_draw_arrays(primitive_, draw_range_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_draw_arrays(%s) - renderer_draw_arrays failed.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}
