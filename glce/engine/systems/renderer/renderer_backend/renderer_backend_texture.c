// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_backend/renderer_backend_texture.h"

#include "engine/systems/renderer/renderer_backend/internal/renderer_backend_context_internal.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_texture_vtable.h"

renderer_backend_result_t renderer_backend_texture_create(renderer_backend_context_t* backend_context_, int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_texture_create", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_create", "texture_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_create", "*texture_handle_")

    ret = backend_context_->texture_vtable->renderer_texture_create(unit_num_, min_filter_config_, mag_filter_config_, wrap_config_s_axis_, wrap_config_t_axis_, texture_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_create(%s) - Failed to create texture.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_texture_destroy(renderer_backend_context_t* backend_context_, renderer_backend_texture_t** texture_handle_) {
    if(NULL == backend_context_ || NULL == backend_context_->texture_vtable) {
        WARN_MESSAGE("renderer_backend_texture_destroy - Provided backend_context_ or backend_context_->texture_vtable is not valid.");
        return;
    }
    if(NULL == texture_handle_ || NULL == *texture_handle_) {
        WARN_MESSAGE("renderer_backend_texture_destroy - Provided texture_handle_ or *texture_handle_ is not valid.");
        return;
    }
    backend_context_->texture_vtable->renderer_texture_destroy(texture_handle_);
}

renderer_backend_result_t renderer_backend_texture_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_texture_bind", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_bind", "texture_handle_")

    ret = backend_context_->texture_vtable->renderer_texture_bind(texture_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_bind(%s) - Failed to bind texture.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_texture_unbind(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_texture_unbind", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_unbind", "texture_handle_")

    ret = backend_context_->texture_vtable->renderer_texture_unbind(texture_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_unbind(%s) - Failed to unbind texture.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_texture_pixel_upload(const renderer_backend_context_t* backend_context_, uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_texture_pixel_upload", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != width_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "width_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != height_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "height_")

    if(3 != channel_count_ && 4 != channel_count_) {
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_backend_texture_pixel_upload(%s) - Invalid channel_count_. expected = 3(RGB) or 4(RGBA), actual = %d", renderer_backend_rslt_to_str(ret), channel_count_);
        goto cleanup;
    }

    ret = backend_context_->texture_vtable->renderer_texture_pixel_upload(width_, height_, channel_count_, pixels_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_pixel_upload(%s) - Failed to upload texture pixels.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}
