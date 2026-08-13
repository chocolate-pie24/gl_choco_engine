#include "engine/systems/renderer/renderer_backend/renderer_backend_vao.h"

#include "engine/systems/renderer/renderer_backend/internal/renderer_backend_context_internal.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_vao_vtable.h"

renderer_backend_result_t renderer_backend_vao_create(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vao_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vao_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vao_create", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vao_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vao_create", "vao_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vao_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vao_create", "*vao_")

    ret = backend_context_->vao_vtable->vao_create(vao_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vao_create(%s) - Failed to create vao.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_vao_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vao_) {
    if(NULL == backend_context_ || NULL == backend_context_->vao_vtable) {
        return;
    }
    // NOTE: vao_のNULLチェックは下位で行う
    backend_context_->vao_vtable->vao_destroy(vao_);
}

renderer_backend_result_t renderer_backend_vao_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_vao_t* vao_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vao_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vao_bind", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vao_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vao_bind", "vao_")

    ret = backend_context_->vao_vtable->vao_bind(vao_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vao_bind(%s) - Failed to bind vao.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vao_unbind(const renderer_backend_context_t* backend_context_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vao_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vao_unbind", "backend_context_->vao_vtable")

    ret = backend_context_->vao_vtable->vao_unbind();
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vao_unbind(%s) - Failed to unbind vao.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vao_attribute_set(const renderer_backend_context_t* backend_context_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vao_attribute_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vao_attribute_set", "backend_context_->vao_vtable")

    ret = backend_context_->vao_vtable->vao_attribute_set(layout_, size_, type_, normalized_, stride_, offset_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vao_attribute_set(%s) - Failed to set vao attribute.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}
