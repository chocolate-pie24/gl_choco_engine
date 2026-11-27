// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/memory/gpu_allocators/linear_range_allocator/linear_range_allocator.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "engine/base/memory_utility.h"
#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/cpu_allocators/general_allocator/general_allocator.h"

typedef struct linear_range_allocator {
    size_t capacity;    /**< アロケータが管理するメモリ容量(byte) */
    size_t head_offset;
} linear_range_allocator_t;

// Utilities
static const char* result_to_str(linear_range_allocator_result_t result_);
static linear_range_allocator_result_t result_convert_general_allocator(general_allocator_result_t result_);

linear_range_allocator_result_t linear_range_allocator_create(size_t memory_pool_size_, linear_range_allocator_t** out_allocator_) {
    linear_range_allocator_result_t ret = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    linear_range_allocator_t* tmp_allocator = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_allocator_, ret, LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_allocator_, ret, LINEAR_RANGE_ALLOCATOR_BAD_OPERATION);
    if(0 == memory_pool_size_) {
        ret = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_range_allocator_create(%s) - Provided memory_pool_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(linear_range_allocator_t), GENERAL_ALLOCATOR_MEMORY_TAG_ALLOCATOR, (void**)&tmp_allocator);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("linear_range_allocator_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    tmp_allocator->capacity = memory_pool_size_;
    tmp_allocator->head_offset = 0;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_range_allocator_is_valid(tmp_allocator)) {
        ret = LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_range_allocator_create(%s) - Result validation failed.", result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_allocator_ = tmp_allocator;
    tmp_allocator = NULL;

    ret = LINEAR_RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

void linear_range_allocator_destroy(linear_range_allocator_t** allocator_) {
    if(NULL == allocator_ || NULL == *allocator_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_range_allocator_is_valid(*allocator_)) {
        ERROR_MESSAGE("linear_range_allocator_destroy(%s) - Precondition validation failed.", result_to_str(LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#endif

    general_allocator_free((void**)allocator_, GENERAL_ALLOCATOR_MEMORY_TAG_ALLOCATOR);
}

linear_range_allocator_result_t linear_range_allocator_allocate(linear_range_allocator_t* allocator_, size_t required_size_, size_t required_align_, size_t* out_offset_) {
    linear_range_allocator_result_t ret = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;

    size_t aligned_offset = 0;
    size_t new_offset = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_offset_, ret, LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT);
    if(0 == required_size_ || 0 == required_align_ || !IS_POWER_OF_TWO(required_align_)) {
        ret = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_range_allocator_allocate(%s) - Provided required_size_ or required_align_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_range_allocator_is_valid(allocator_)) {
        ret = LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_range_allocator_allocate(%s) - Precondition validation failed.", result_to_str(ret));
        goto cleanup;
    }
#endif

    if(!memory_utility_align_up(allocator_->head_offset, required_align_, &aligned_offset)) {
        ret = LINEAR_RANGE_ALLOCATOR_OVERFLOW;  // memory_utility_align_upで失敗するのはoverflowのみ
        ERROR_MESSAGE("linear_range_allocator_allocate(%s) - memory_utility_align_up failed.", result_to_str(ret));
        goto cleanup;
    }
    if(!size_add_checked(required_size_, aligned_offset, &new_offset)) {
        ret = LINEAR_RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("linear_range_allocator_allocate(%s) - Memory capacity overflow.", result_to_str(ret));
        goto cleanup;
    }
    if(new_offset > allocator_->capacity) {
        ret = LINEAR_RANGE_ALLOCATOR_LIMIT_EXCEEDED;
        ERROR_MESSAGE("linear_range_allocator_allocate(%s) - Linear range allocator limit exceeded.", result_to_str(ret));
        goto cleanup;
    }

    *out_offset_ = aligned_offset;
    allocator_->head_offset = new_offset;

    ret = LINEAR_RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

linear_range_allocator_result_t linear_range_allocator_reset(linear_range_allocator_t* allocator_) {
    linear_range_allocator_result_t ret = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_range_allocator_is_valid(allocator_)) {
        ret = LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_range_allocator_reset(%s) - Precondition validation failed.", result_to_str(ret));
        goto cleanup;
    }
#endif

    allocator_->head_offset = 0;

    ret = LINEAR_RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

linear_range_allocator_result_t linear_range_allocator_rollback_point_get(const linear_range_allocator_t* allocator_, linear_range_allocator_rollback_point_t* out_rollback_point_) {
    linear_range_allocator_result_t ret = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_rollback_point_, ret, LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_range_allocator_is_valid(allocator_)) {
        ret = LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_range_allocator_rollback_point_get(%s) - Precondition validation failed.", result_to_str(ret));
        goto cleanup;
    }
#endif

    out_rollback_point_->offset = allocator_->head_offset;

    ret = LINEAR_RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

linear_range_allocator_result_t linear_range_allocator_rollback(linear_range_allocator_t* allocator_, const linear_range_allocator_rollback_point_t* rollback_point_) {
    linear_range_allocator_result_t ret = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(rollback_point_, ret, LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_range_allocator_is_valid(allocator_)) {
        ret = LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_range_allocator_rollback(%s) - Precondition validation failed.", result_to_str(ret));
        goto cleanup;
    }
#endif
    if(rollback_point_->offset > allocator_->head_offset) {
        ret = LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_range_allocator_rollback(%s) - Provided rollback point is not valid.", result_to_str(ret));
        goto cleanup;
    }

    allocator_->head_offset = rollback_point_->offset;

    ret = LINEAR_RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

bool linear_range_allocator_is_valid(const linear_range_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(allocator_)) {
        return false;
    }
    if(0 == allocator_->capacity) {
        return false;
    }
    if(allocator_->capacity < allocator_->head_offset) {
        return false;
    }
    return true;
}

static const char* result_to_str(linear_range_allocator_result_t result_) {
    switch(result_) {
    case LINEAR_RANGE_ALLOCATOR_SUCCESS:
        return STRINGIFY(LINEAR_RANGE_ALLOCATOR_SUCCESS);
    case LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT:
        return STRINGIFY(LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT);
    case LINEAR_RANGE_ALLOCATOR_LIMIT_EXCEEDED:
        return STRINGIFY(LINEAR_RANGE_ALLOCATOR_LIMIT_EXCEEDED);
    case LINEAR_RANGE_ALLOCATOR_NO_MEMORY:
        return STRINGIFY(LINEAR_RANGE_ALLOCATOR_NO_MEMORY);
    case LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED:
        return STRINGIFY(LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED);
    case LINEAR_RANGE_ALLOCATOR_BAD_OPERATION:
        return STRINGIFY(LINEAR_RANGE_ALLOCATOR_BAD_OPERATION);
    case LINEAR_RANGE_ALLOCATOR_OVERFLOW:
        return STRINGIFY(LINEAR_RANGE_ALLOCATOR_OVERFLOW);
    case LINEAR_RANGE_ALLOCATOR_UNDEFINED_ERROR:
        return STRINGIFY(LINEAR_RANGE_ALLOCATOR_UNDEFINED_ERROR);
    default:
        return STRINGIFY(LINEAR_RANGE_ALLOCATOR_UNDEFINED_ERROR);
    }
}

static linear_range_allocator_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return LINEAR_RANGE_ALLOCATOR_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return LINEAR_RANGE_ALLOCATOR_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return LINEAR_RANGE_ALLOCATOR_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return LINEAR_RANGE_ALLOCATOR_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return LINEAR_RANGE_ALLOCATOR_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return LINEAR_RANGE_ALLOCATOR_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return LINEAR_RANGE_ALLOCATOR_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return LINEAR_RANGE_ALLOCATOR_UNDEFINED_ERROR;
    default:
        return LINEAR_RANGE_ALLOCATOR_UNDEFINED_ERROR;
    }
}
