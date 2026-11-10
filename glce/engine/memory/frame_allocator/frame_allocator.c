// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/memory/frame_allocator/frame_allocator.h"

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"
#include "engine/memory/general_allocator/general_allocator.h"

// ============================================================
// Private Type Definitions
// ============================================================
struct frame_allocator {
    void* linear_allocator_pool;            /**< リニアアロケータ構造体インスタンスが使用するメモリプールのアドレス */
    linear_allocator_t linear_allocator;    /**< リニアアロケータ構造体インスタンス */

    // memory使用量管理
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t memory_tag_allocated[FRAME_ALLOCATOR_MEMORY_TAG_MAX];   /**< 各メモリタグごとのメモリ割り当て量 */
};

// ============================================================
// Private Constants
// ============================================================
static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

static const char* const s_memory_tag_render_packet = "RENDER_PACKET";
static const char* const s_memory_tag_undefined = "UNDEFINED";

// ============================================================
// Private Function Declarations
// ============================================================
// Initialize, Deinitialize helpers
static void accounting_reset(frame_allocator_t* allocator_);

// Utilities
static const char* result_to_str(frame_allocator_result_t result_);
static frame_allocator_result_t result_convert_linear_allocator(linear_allocator_result_t result_);
static frame_allocator_result_t result_convert_general_allocator(general_allocator_result_t result_);

// Validators
static bool memory_tag_is_valid(frame_allocator_memory_tag_t memory_tag_);
static bool accounting_is_valid(const frame_allocator_t* allocator_);

// ============================================================
// Public API
// ============================================================
frame_allocator_result_t frame_allocator_create(size_t memory_pool_size_, frame_allocator_t** out_allocator_) {
    frame_allocator_result_t ret = FRAME_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    frame_allocator_t* tmp_allocator = NULL;
    void* tmp_memory_pool = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_allocator_, ret, FRAME_ALLOCATOR_INVALID_ARGUMENT, result_to_str(FRAME_ALLOCATOR_INVALID_ARGUMENT), "frame_allocator_create", "out_allocator_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_allocator_, ret, FRAME_ALLOCATOR_BAD_OPERATION, result_to_str(FRAME_ALLOCATOR_BAD_OPERATION), "frame_allocator_create", "*out_allocator_")
    if(0 == memory_pool_size_) {
        ret = FRAME_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("frame_allocator_create(%s) - Provided memory_pool_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(frame_allocator_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_allocator);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("frame_allocator_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(memory_pool_size_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, &tmp_memory_pool);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("frame_allocator_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_linear_allocator = linear_allocator_initialize(&tmp_allocator->linear_allocator, memory_pool_size_, tmp_memory_pool);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        if(LINEAR_ALLOCATOR_INVALID_ARGUMENT == ret_linear_allocator) {
            ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("frame_allocator_create(%s) - linear_allocator_initialize failed.", result_to_str(ret));
        goto cleanup;
    }

    tmp_allocator->linear_allocator_pool = tmp_memory_pool;
    accounting_reset(tmp_allocator);

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!frame_allocator_is_valid(tmp_allocator)) {
        ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("frame_allocator_create(%s) - Result validation failed for 'tmp_allocator'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_allocator_ = tmp_allocator;
    tmp_allocator = NULL;
    tmp_memory_pool = NULL;

    ret = FRAME_ALLOCATOR_SUCCESS;

cleanup:
    if(FRAME_ALLOCATOR_DATA_CORRUPTED == ret || FRAME_ALLOCATOR_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_memory_pool) {
        general_allocator_free((void**)&tmp_memory_pool, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != tmp_allocator) {
        general_allocator_free((void**)&tmp_allocator, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void frame_allocator_destroy(frame_allocator_t** allocator_) {
    if(NULL == allocator_ || NULL == *allocator_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!frame_allocator_is_valid(*allocator_)) {
        ERROR_MESSAGE("frame_allocator_destroy(%s) - Precondition validation failed for 'allocator_'.", result_to_str(FRAME_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#endif

    general_allocator_free((void**)&(*allocator_)->linear_allocator_pool, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    general_allocator_free((void**)allocator_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

frame_allocator_result_t frame_allocator_allocate(frame_allocator_t* allocator_, size_t allocation_size_, frame_allocator_memory_tag_t memory_tag_, void** out_ptr_) {
    frame_allocator_result_t ret = FRAME_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    void* tmp_ptr = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, FRAME_ALLOCATOR_INVALID_ARGUMENT, result_to_str(FRAME_ALLOCATOR_INVALID_ARGUMENT), "frame_allocator_allocate", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, FRAME_ALLOCATOR_INVALID_ARGUMENT, result_to_str(FRAME_ALLOCATOR_INVALID_ARGUMENT), "frame_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, FRAME_ALLOCATOR_BAD_OPERATION, result_to_str(FRAME_ALLOCATOR_BAD_OPERATION), "frame_allocator_allocate", "*out_ptr_")
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = FRAME_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("frame_allocator_allocate(%s) - Provided memory_tag_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(0 == allocation_size_) {
        ret = FRAME_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("frame_allocator_allocate(%s) - Provided allocation_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!frame_allocator_is_valid(allocator_)) {
        ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("frame_allocator_allocate(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif
    if((SIZE_MAX - allocation_size_) < allocator_->total_allocated) {
        ret = FRAME_ALLOCATOR_LIMIT_EXCEEDED;
        ERROR_MESSAGE("frame_allocator_allocate(%s) - subsystem allocator limit exceeded.", result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - allocation_size_) < allocator_->memory_tag_allocated[memory_tag_]) {
        ret = FRAME_ALLOCATOR_LIMIT_EXCEEDED;
        ERROR_MESSAGE("frame_allocator_allocate(%s) - subsystem allocator limit exceeded.", result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    ret_linear_allocator = linear_allocator_allocate(&allocator_->linear_allocator, allocation_size_, (void**)&tmp_ptr);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        if(LINEAR_ALLOCATOR_INVALID_ARGUMENT == ret_linear_allocator) {
            ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("frame_allocator_allocate(%s) - linear_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }
    memset(tmp_ptr, 0, allocation_size_);

    allocator_->total_allocated += allocation_size_;
    allocator_->memory_tag_allocated[memory_tag_] += allocation_size_;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!accounting_is_valid(allocator_)) {
        ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("frame_allocator_allocate(%s) - Postcondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_ptr_ = tmp_ptr;
    tmp_ptr = NULL;

    ret = FRAME_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

frame_allocator_result_t frame_allocator_reset(frame_allocator_t* allocator_) {
    frame_allocator_result_t ret = FRAME_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, FRAME_ALLOCATOR_INVALID_ARGUMENT, result_to_str(FRAME_ALLOCATOR_INVALID_ARGUMENT), "frame_allocator_reset", "allocator_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!frame_allocator_is_valid(allocator_)) {
        ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("frame_allocator_reset(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    ret_linear_allocator = linear_allocator_reset(&allocator_->linear_allocator);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        if(LINEAR_ALLOCATOR_INVALID_ARGUMENT == ret_linear_allocator) {
            ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("frame_allocator_reset(%s) - linear_allocator_reset failed.", result_to_str(ret));
        goto cleanup;
    }
    accounting_reset(allocator_);

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!accounting_is_valid(allocator_)) {
        ret = FRAME_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("frame_allocator_reset(%s) - Postcondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = FRAME_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

const char* frame_allocator_memory_tag_to_str(frame_allocator_memory_tag_t memory_tag_) {
    switch(memory_tag_) {
    case FRAME_ALLOCATOR_MEMORY_TAG_RENDER_PACKET:
        return s_memory_tag_render_packet;
    case FRAME_ALLOCATOR_MEMORY_TAG_MAX:
        return s_memory_tag_undefined;
    default:
        return s_memory_tag_undefined;
    }
}

bool frame_allocator_is_valid(const frame_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return false;
    }
    if(NULL == allocator_->linear_allocator_pool) {
        return false;
    }
    if(!accounting_is_valid(allocator_)) {
        return false;
    }
    if(allocator_->linear_allocator_pool != allocator_->linear_allocator.memory_pool) {
        return false;
    }
    if(!linear_allocator_is_valid(&allocator_->linear_allocator)) {
        return false;
    }
    return true;
}

// ============================================================
// Initialize, Deinitialize helpers
// ============================================================
static void accounting_reset(frame_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return;
    }
    allocator_->total_allocated = 0;
    for(size_t i = 0; i != FRAME_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        allocator_->memory_tag_allocated[i] = 0;
    }
}

// ============================================================
// Utilities
// ============================================================
static const char* result_to_str(frame_allocator_result_t result_) {
    switch(result_) {
    case FRAME_ALLOCATOR_SUCCESS:
        return s_result_str_success;
    case FRAME_ALLOCATOR_BAD_OPERATION:
        return s_result_str_bad_operation;
    case FRAME_ALLOCATOR_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case FRAME_ALLOCATOR_NO_MEMORY:
        return s_result_str_no_memory;
    case FRAME_ALLOCATOR_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case FRAME_ALLOCATOR_OVERFLOW:
        return s_result_str_overflow;
    case FRAME_ALLOCATOR_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case FRAME_ALLOCATOR_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

static frame_allocator_result_t result_convert_linear_allocator(linear_allocator_result_t result_) {
    switch(result_) {
    case LINEAR_ALLOCATOR_SUCCESS:
        return FRAME_ALLOCATOR_SUCCESS;
    case LINEAR_ALLOCATOR_NO_MEMORY:
        return FRAME_ALLOCATOR_NO_MEMORY;
    case LINEAR_ALLOCATOR_DATA_CORRUPTED:
        return FRAME_ALLOCATOR_DATA_CORRUPTED;
    case LINEAR_ALLOCATOR_BAD_OPERATION:
        return FRAME_ALLOCATOR_BAD_OPERATION;
    case LINEAR_ALLOCATOR_INVALID_ARGUMENT:
        return FRAME_ALLOCATOR_INVALID_ARGUMENT;
    case LINEAR_ALLOCATOR_OVERFLOW:
        return FRAME_ALLOCATOR_OVERFLOW;
    case LINEAR_ALLOCATOR_UNDEFINED_ERROR:
        return FRAME_ALLOCATOR_UNDEFINED_ERROR;
    default:
        return FRAME_ALLOCATOR_UNDEFINED_ERROR;
    }
}

static frame_allocator_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return FRAME_ALLOCATOR_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return FRAME_ALLOCATOR_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return FRAME_ALLOCATOR_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return FRAME_ALLOCATOR_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return FRAME_ALLOCATOR_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return FRAME_ALLOCATOR_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return FRAME_ALLOCATOR_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return FRAME_ALLOCATOR_UNDEFINED_ERROR;
    default:
        return FRAME_ALLOCATOR_UNDEFINED_ERROR;
    }
}

// ============================================================
// Validators
// ============================================================
static bool memory_tag_is_valid(frame_allocator_memory_tag_t memory_tag_) {
    if(FRAME_ALLOCATOR_MEMORY_TAG_MAX <= memory_tag_ || 0 > (int)memory_tag_) {
        return false;
    }
    return true;
}

static bool accounting_is_valid(const frame_allocator_t* allocator_) {
    size_t expected_total_size = 0;
    if(NULL == allocator_) {
        return false;
    }
    for(size_t i = 0; i != FRAME_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        if((SIZE_MAX - allocator_->memory_tag_allocated[i]) < expected_total_size) {
            return false;
        }
        expected_total_size += allocator_->memory_tag_allocated[i];
    }
    if(allocator_->total_allocated != expected_total_size) {
        return false;
    }
    return true;
}
