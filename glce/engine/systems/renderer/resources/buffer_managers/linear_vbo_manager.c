// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/buffer_managers/linear_vbo_manager.h"

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/gpu_allocators/linear_range_allocator/linear_range_allocator.h"
#include "engine/memory/cpu_allocators/general_allocator/general_allocator.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_vbo.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_err_utils.h"

struct linear_vbo_manager {
    renderer_backend_context_t* backend_context;
    linear_vbo_manager_config_t config;
    linear_range_allocator_t* linear_range_allocator;
    renderer_backend_vbo_t* vbo;            /**< 所有するRenderer Backend VBO */
};

buffer_manager_result_t linear_vbo_manager_create(renderer_backend_context_t* backend_context_, const linear_vbo_manager_config_t* config_, linear_vbo_manager_t** out_vbo_manager_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    linear_range_allocator_result_t ret_linear_range_allocator = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    linear_vbo_manager_t* tmp_vbo_manager = NULL;
    linear_range_allocator_t* tmp_allocator = NULL;
    renderer_backend_vbo_t* tmp_vbo = NULL;

    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, BUFFER_MANAGER_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, BUFFER_MANAGER_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vbo_manager_, ret, BUFFER_MANAGER_BAD_OPERATION);
    if(!linear_vbo_manager_config_is_valid(config_)) {
        ret = BUFFER_MANAGER_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_vbo_manager_create(%s) - Provided config_ is not valid.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(linear_vbo_manager_t), GENERAL_ALLOCATOR_MEMORY_TAG_VERTEX_RESOURCE, (void**)&tmp_vbo_manager);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = buffer_manager_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("linear_vbo_manager_create(%s) - general_allocator_allocate failed.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }

    ret_linear_range_allocator = linear_range_allocator_create(config_->vbo_size, &tmp_allocator);
    if(LINEAR_RANGE_ALLOCATOR_SUCCESS != ret_linear_range_allocator) {
        ret = buffer_manager_result_convert_linear_range_allocator(ret_linear_range_allocator);
        ERROR_MESSAGE("linear_vbo_manager_create(%s) - linear_range_allocator_create failed.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vbo_create(backend_context_, &tmp_vbo);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("linear_vbo_manager_create(%s) - Failed to create VBO Manager. reason=renderer_backend_vbo_create_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vbo_bind(backend_context_, tmp_vbo);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("linear_vbo_manager_create(%s) - Failed to create VBO Manager. reason=renderer_backend_vbo_bind_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }
    vbo_bound = true;

    ret_renderer_backend = renderer_backend_vbo_vertex_load(backend_context_, config_->vbo_size, 0, config_->buffer_usage);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("linear_vbo_manager_create(%s) - Failed to create VBO Manager. reason=renderer_backend_vbo_vertex_load_failed, vbo_size=%zu, buffer_usage=%d, renderer_result=%d", buffer_manager_result_to_str(ret), config_->vbo_size, (int)config_->buffer_usage, (int)ret_renderer_backend);
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vbo_unbind(backend_context_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("linear_vbo_manager_create(%s) - Failed to create VBO Manager. reason=renderer_backend_vbo_unbind_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }
    vbo_bound = false;

    tmp_vbo_manager->config = *config_;
    tmp_vbo_manager->backend_context = backend_context_;
    tmp_vbo_manager->linear_range_allocator = tmp_allocator;
    tmp_vbo_manager->vbo = tmp_vbo;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_vbo_manager_is_valid(tmp_vbo_manager)) {
        ret = BUFFER_MANAGER_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_vbo_manager_create(%s) - Result validation failed for 'tmp_vbo_manager'.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_vbo_manager_ = tmp_vbo_manager;
    tmp_vbo_manager = NULL;
    tmp_allocator = NULL;
    tmp_vbo = NULL;

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    if(BUFFER_MANAGER_DATA_CORRUPTED == ret || BUFFER_MANAGER_SUCCESS == ret) {
        return ret;
    }
    if(vbo_bound) {
        ret_renderer_backend = renderer_backend_vbo_unbind(backend_context_);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ERROR_MESSAGE("linear_vbo_manager_create(%s) - Failed to rollback VBO Manager creation. reason=renderer_backend_vbo_unbind_failed, original_result=%s, renderer_result=%d", buffer_manager_result_to_str(BUFFER_MANAGER_DATA_CORRUPTED), buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
            ret = BUFFER_MANAGER_DATA_CORRUPTED;
            return ret;
        }
    }
    if(NULL != tmp_vbo) {
        renderer_backend_vbo_destroy(backend_context_, &tmp_vbo);
    }
    if(NULL != tmp_allocator) {
        linear_range_allocator_destroy(&tmp_allocator);
    }
    if(NULL != tmp_vbo_manager) {
        general_allocator_free((void**)&tmp_vbo_manager, GENERAL_ALLOCATOR_MEMORY_TAG_VERTEX_RESOURCE);
    }
    return ret;
}

void linear_vbo_manager_destroy(linear_vbo_manager_t** vbo_manager_) {
    if(NULL == vbo_manager_) {
        return;
    }
    if(NULL == *vbo_manager_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_vbo_manager_is_valid(*vbo_manager_)) {
        ERROR_MESSAGE("linear_vbo_manager_destroy(%s) - Precondition validation failed for '*vbo_manager_'.", buffer_manager_result_to_str(BUFFER_MANAGER_DATA_CORRUPTED));
        return;
    }
#endif
    renderer_backend_vbo_destroy((*vbo_manager_)->backend_context, &(*vbo_manager_)->vbo);
    linear_range_allocator_destroy(&(*vbo_manager_)->linear_range_allocator);

    general_allocator_free((void**)vbo_manager_, GENERAL_ALLOCATOR_MEMORY_TAG_VERTEX_RESOURCE);
}

buffer_manager_result_t linear_vbo_manager_write(linear_vbo_manager_t* vbo_manager_, size_t size_, const void* write_data_, size_t* out_offset_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    linear_range_allocator_result_t ret_linear_range_allocator = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    size_t tmp_offset = 0;
    linear_range_allocator_rollback_point_t rollback_point = { 0 };

    bool allocate_success = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(write_data_, ret, BUFFER_MANAGER_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_offset_, ret, BUFFER_MANAGER_INVALID_ARGUMENT);
    if(0 == size_) {
        ret = BUFFER_MANAGER_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_vbo_manager_write(%s) - Provided size_ is not valid.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_vbo_manager_is_valid(vbo_manager_)) {
        ret = BUFFER_MANAGER_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_vbo_manager_write(%s) - Precondition validation failed for 'vbo_manager_'.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(0 != (size_ % vbo_manager_->config.base_align)) {
        ret = BUFFER_MANAGER_BAD_OPERATION;
        ERROR_MESSAGE("linear_vbo_manager_write(%s) - Failed to write vertex data. reason=write_size_misaligned, write_size=%zu, base_align=%zu", buffer_manager_result_to_str(ret), size_, vbo_manager_->config.base_align);
        goto cleanup;
    }

    ret_linear_range_allocator = linear_range_allocator_rollback_point_get(vbo_manager_->linear_range_allocator, &rollback_point);
    if(LINEAR_RANGE_ALLOCATOR_SUCCESS != ret_linear_range_allocator) {
        ret = buffer_manager_result_convert_linear_range_allocator(ret_linear_range_allocator);
        ERROR_MESSAGE("linear_vbo_manager_write(%s) - linear_range_allocator_rollback_point_get failed.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
    ret_linear_range_allocator = linear_range_allocator_allocate(vbo_manager_->linear_range_allocator, size_, vbo_manager_->config.base_align, &tmp_offset);
    if(LINEAR_RANGE_ALLOCATOR_SUCCESS != ret_linear_range_allocator) {
        ret = buffer_manager_result_convert_linear_range_allocator(ret_linear_range_allocator);
        ERROR_MESSAGE("linear_vbo_manager_write(%s) - Failed to write vertex data.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
    allocate_success = true;

    ret_renderer_backend = renderer_backend_vbo_bind(vbo_manager_->backend_context, vbo_manager_->vbo);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("linear_vbo_manager_write(%s) - Failed to write vertex data.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret_renderer_backend = renderer_backend_vbo_vertex_subload(vbo_manager_->backend_context, tmp_offset, size_, write_data_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("linear_vbo_manager_write(%s) - Failed to write vertex data.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vbo_unbind(vbo_manager_->backend_context);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("linear_vbo_manager_write(%s) - Failed to write vertex data.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    *out_offset_ = tmp_offset;

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    if(BUFFER_MANAGER_DATA_CORRUPTED == ret || BUFFER_MANAGER_SUCCESS == ret) {
        return ret;
    }
    if(allocate_success) {
        ret_linear_range_allocator = linear_range_allocator_rollback(vbo_manager_->linear_range_allocator, &rollback_point);
        if(LINEAR_RANGE_ALLOCATOR_SUCCESS != ret_linear_range_allocator) {
            ERROR_MESSAGE("linear_vbo_manager_write(%s) - linear_range_allocator_rollback failed.", buffer_manager_result_to_str(BUFFER_MANAGER_DATA_CORRUPTED));
            ret = BUFFER_MANAGER_DATA_CORRUPTED;
            return ret;
        }
    }
    if(vbo_bound) {
        // backend_context_の内部状態破損がなければ成功するはず。失敗はDATA_CORRUPTEDとする
        ret_renderer_backend = renderer_backend_vbo_unbind(vbo_manager_->backend_context);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ERROR_MESSAGE("linear_vbo_manager_write(%s) - Failed to rollback vertex data write.", buffer_manager_result_to_str(BUFFER_MANAGER_DATA_CORRUPTED));
            ret = BUFFER_MANAGER_DATA_CORRUPTED;
            return ret;
        }
    }

    return ret;
}

buffer_manager_result_t linear_vbo_manager_reset(linear_vbo_manager_t* vbo_manager_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    linear_range_allocator_result_t ret_linear_range_allocator = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT);

    ret_linear_range_allocator = linear_range_allocator_reset(vbo_manager_->linear_range_allocator);
    if(LINEAR_RANGE_ALLOCATOR_SUCCESS != ret_linear_range_allocator) {
        ret = buffer_manager_result_convert_linear_range_allocator(ret_linear_range_allocator);
        ERROR_MESSAGE("linear_vbo_manager_reset(%s) - linear_range_allocator_reset failed.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    return ret;
}

buffer_manager_result_t linear_vbo_manager_bind(linear_vbo_manager_t* vbo_manager_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT);

    ret_renderer_backend = renderer_backend_vbo_bind(vbo_manager_->backend_context, vbo_manager_->vbo);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("linear_vbo_manager_bind(%s) - Failed to bind vertex buffer. reason=renderer_backend_vbo_bind_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    return ret;
}

buffer_manager_result_t linear_vbo_manager_unbind(const linear_vbo_manager_t* vbo_manager_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT);

    ret_renderer_backend = renderer_backend_vbo_unbind(vbo_manager_->backend_context);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("linear_vbo_manager_unbind(%s) - Failed to unbind vertex buffer. reason=renderer_backend_vbo_unbind_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    return ret;
}

const renderer_backend_vbo_t* linear_vbo_manager_vbo_get(const linear_vbo_manager_t* vbo_manager_) {
    if(NULL == vbo_manager_) {
        return NULL;
    }
    return vbo_manager_->vbo;
}

bool linear_vbo_manager_config_is_valid(const linear_vbo_manager_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->vbo_size) {
        return false;
    }
    if(0 == config_->base_align || !IS_POWER_OF_TWO(config_->base_align)) {
        return false;
    }
    if(!buffer_usage_is_valid(config_->buffer_usage)) {
        return false;
    }
    return true;
}

bool linear_vbo_manager_is_valid(const linear_vbo_manager_t* vbo_manager_) {
    if(NULL == vbo_manager_) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(vbo_manager_)) {
        return false;
    }

    if(NULL == vbo_manager_->backend_context) {
        return false;
    }

    if(NULL == vbo_manager_->linear_range_allocator || NULL == vbo_manager_->vbo) {
        return false;
    }

    if(!linear_vbo_manager_config_is_valid(&vbo_manager_->config)) {
        return false;
    }

    // Owned ResourceのCanonical Validity
    if(!linear_range_allocator_is_valid(vbo_manager_->linear_range_allocator)) {
        return false;
    }

    if(!renderer_backend_vbo_is_valid(vbo_manager_->backend_context, vbo_manager_->vbo)) {
        return false;
    }

    return true;
}
