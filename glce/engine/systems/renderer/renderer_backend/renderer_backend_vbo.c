#include "engine/systems/renderer/renderer_backend/renderer_backend_vbo.h"

#include "engine/systems/renderer/renderer_backend/internal/renderer_backend_context_internal.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_vbo_vtable.h"

renderer_backend_result_t renderer_backend_vbo_create(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vbo_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vbo_create", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vbo_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_create", "vbo_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vbo_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_create", "*vbo_")

    ret = backend_context_->vbo_vtable->vbo_create(vbo_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vbo_create(%s) - Failed to create vbo.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_vbo_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vbo_) {
    if(NULL == backend_context_ || NULL == backend_context_->vbo_vtable) {
        return;
    }
    // NOTE: vbo_のNULLチェックは下位に任せる
    backend_context_->vbo_vtable->vbo_destroy(vbo_);
}

renderer_backend_result_t renderer_backend_vbo_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_vbo_t* vbo_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vbo_bind", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vbo_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_bind", "vbo_")

    ret = backend_context_->vbo_vtable->vbo_bind(vbo_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vbo_bind(%s) - Failed to bind vbo.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vbo_unbind(const renderer_backend_context_t* backend_context_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vbo_unbind", "backend_context_->vbo_vtable")

    ret = backend_context_->vbo_vtable->vbo_unbind();
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vbo_unbind(%s) - Failed to unbind vbo.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vbo_vertex_load(const renderer_backend_context_t* backend_context_, size_t load_size_, const void* load_data_, buffer_usage_t usage_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_vertex_load", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vbo_vertex_load", "backend_context_->vbo_vtable")

    ret = backend_context_->vbo_vtable->vbo_vertex_load(load_size_, load_data_, usage_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vbo_vertex_load(%s) - Failed to load vertex.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vbo_vertex_subload(const renderer_backend_context_t* backend_context_, size_t offset_, size_t size_, const void* load_data_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_vertex_subload", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vbo_vertex_subload", "backend_context_->vbo_vtable")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_vertex_subload", "size_")
    IF_ARG_NULL_GOTO_CLEANUP(load_data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vbo_vertex_subload", "load_data_")

    ret = backend_context_->vbo_vtable->vbo_vertex_subload(offset_, size_, load_data_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vbo_vertex_subload(%s) - Failed to load vertex.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}
