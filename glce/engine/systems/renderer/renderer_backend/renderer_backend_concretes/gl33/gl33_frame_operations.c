// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_frame_operations.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_frame_operations_vtable.h"

static void gl33_depth_test_set(bool enable_);
static renderer_backend_result_t gl33_viewport_set(int32_t x_, int32_t y_, int32_t framebuffer_width_, int32_t framebuffer_height_);
static void gl33_framebuffer_clear(uint32_t mask_);
static renderer_backend_result_t gl33_renderer_draw_arrays(rendering_primitive_t primitive_, const draw_range_t* draw_range_);

static const renderer_frame_operations_vtable_t s_gl33_frame_operations_vtable = {
    .renderer_set_depth_test = gl33_depth_test_set,
    .renderer_viewport_set = gl33_viewport_set,
    .renderer_framebuffer_clear = gl33_framebuffer_clear,
    .renderer_draw_arrays = gl33_renderer_draw_arrays,
};

const renderer_frame_operations_vtable_t* gl33_frame_operations_vtable_get(void) {
    return &s_gl33_frame_operations_vtable;
}

static void gl33_depth_test_set(bool enable_) {
    if(enable_) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

static renderer_backend_result_t gl33_viewport_set(int32_t x_, int32_t y_, int32_t framebuffer_width_, int32_t framebuffer_height_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    if(0 > x_ || 0 > y_) {
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("gl33_viewport_set(%s) - Provided x_ or y_ is not valid.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }
    if(0 > framebuffer_width_ || 0 > framebuffer_height_) {
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("gl33_viewport_set(%s) - Provided framebuffer_width_ or framebuffer_height_ is not valid.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }
    if(0 == framebuffer_width_ || 0 == framebuffer_height_) {
        ret = RENDERER_BACKEND_SUCCESS;
        goto cleanup;
    }

    glViewport(x_, y_, framebuffer_width_, framebuffer_height_);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

static void gl33_framebuffer_clear(uint32_t mask_) {
    GLbitfield gl_mask = 0;

    if(mask_ & RENDERER_CLEAR_COLOR_BIT) {
        gl_mask |= GL_COLOR_BUFFER_BIT;
    }
    if(mask_ & RENDERER_CLEAR_DEPTH_BIT) {
        gl_mask |= GL_DEPTH_BUFFER_BIT;
    }
    if(mask_ & RENDERER_CLEAR_STENCIL_BIT) {
        gl_mask |= GL_STENCIL_BUFFER_BIT;
    }

    glClear(gl_mask);
}

static renderer_backend_result_t gl33_renderer_draw_arrays(rendering_primitive_t primitive_, const draw_range_t* draw_range_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(draw_range_, ret, RENDERER_BACKEND_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!draw_range_is_valid(draw_range_)) {
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("gl33_renderer_draw_arrays(%s) - Provided draw_range_ is not valid.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(INT32_MAX < draw_range_->first_vertex_count || INT32_MAX < draw_range_->vertex_count) {
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("gl33_renderer_draw_arrays(%s) - Provided draw_range_ is not valid.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    switch(primitive_) {
    case RENDERING_PRIMITIVE_LINES:
        glDrawArrays(GL_LINES, (GLint)draw_range_->first_vertex_count, (GLsizei)draw_range_->vertex_count);
        break;
    case RENDERING_PRIMITIVE_TRIANGLES:
        glDrawArrays(GL_TRIANGLES, (GLint)draw_range_->first_vertex_count, (GLsizei)draw_range_->vertex_count);
        break;
    case RENDERING_PRIMITIVE_POINTS:
        glDrawArrays(GL_POINTS, (GLint)draw_range_->first_vertex_count, (GLsizei)draw_range_->vertex_count);
        break;
    default:
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("gl33_renderer_draw_arrays(%s) - Provided primitive_ is not valid.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}
