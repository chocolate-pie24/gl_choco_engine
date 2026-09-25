// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"
#include "engine/memory/general_allocator/general_allocator.h"

struct subsystem_allocator {
    size_t allocator_memory_requirement;     /**< リニアアロケータ構造体インスタンスに必要なメモリ量 */
    size_t allocator_alignment_requirement;  /**< リニアアロケータ構造体インスタンスが要求するメモリアライメント */
    size_t allocator_pool_size;              /**< リニアアロケータ構造体インスタンスが使用するメモリプールのサイズ */
    void* linear_allocator_pool;             /**< リニアアロケータ構造体インスタンスが使用するメモリプールのアドレス */
    linear_allocator_t* linear_allocator;    /**< リニアアロケータ構造体インスタンス */

    // memory使用量管理
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t memory_tag_allocated[SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX];   /**< 各メモリタグごとのメモリ割り当て量 */
};

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

static const char* const s_memory_tag_platform_system = "PLATFORM_SYSTEM";
static const char* const s_memory_tag_renderer_system = "RENDERER_SYSTEM";
static const char* const s_memory_tag_event_system = "EVENT_SYSTEM";
static const char* const s_memory_tag_flight_camera_system = "FLIGHT_CAMERA_SYSTEM";
static const char* const s_memory_tag_undefined = "UNDEFINED";

static const char* memory_tag_c_str(subsystem_allocator_memory_tag_t memory_tag_);
static const char* result_to_str(subsystem_allocator_result_t result_);
static subsystem_allocator_result_t result_convert_linear_allocator(linear_allocator_result_t result_);
static subsystem_allocator_result_t result_convert_general_allocator(general_allocator_result_t result_);

static bool is_valid_shallow(const subsystem_allocator_t* allocator_);
static bool memory_tag_is_valid(subsystem_allocator_memory_tag_t memory_tag_);

subsystem_allocator_result_t subsystem_allocator_create(size_t memory_pool_size_, subsystem_allocator_t** out_allocator_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    subsystem_allocator_t* tmp_allocator = NULL;
    linear_allocator_t* tmp_linear_allocator = NULL;
    void* tmp_memory_pool = NULL;

    size_t tmp_memory_requirement = 0;
    size_t tmp_alignment_requirement = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_create", "out_allocator_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_create", "*out_allocator_")
    if(0 == memory_pool_size_) {
        ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("subsystem_allocator_create(%s) - Provided memory_pool_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(subsystem_allocator_t), GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM, (void**)&tmp_allocator);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("subsystem_allocator_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    linear_allocator_preinit(&tmp_memory_requirement, &tmp_alignment_requirement);
    ret_general_allocator = general_allocator_allocate(tmp_memory_requirement, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM, (void**)&tmp_linear_allocator);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("subsystem_allocator_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(memory_pool_size_, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM, &tmp_memory_pool);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("subsystem_allocator_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_linear_allocator = linear_allocator_initialize(tmp_linear_allocator, memory_pool_size_, tmp_memory_pool);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_create(%s) - linear_allocator_initialize failed.", result_to_str(ret));
        goto cleanup;
    }

    tmp_allocator->allocator_alignment_requirement = tmp_alignment_requirement;
    tmp_allocator->allocator_memory_requirement = tmp_memory_requirement;
    tmp_allocator->allocator_pool_size = memory_pool_size_;
    tmp_allocator->linear_allocator = tmp_linear_allocator;
    tmp_allocator->linear_allocator_pool = tmp_memory_pool;

    *out_allocator_ = tmp_allocator;

    tmp_allocator = NULL;
    tmp_memory_pool = NULL;
    tmp_linear_allocator = NULL;

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    if(NULL != tmp_linear_allocator) {
        general_allocator_free((void**)&tmp_linear_allocator, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    }
    if(NULL != tmp_memory_pool) {
        general_allocator_free((void**)&tmp_memory_pool, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    }
    if(NULL != tmp_allocator) {
        general_allocator_free((void**)&tmp_allocator, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    }
    return ret;
}

void subsystem_allocator_destroy(subsystem_allocator_t** allocator_) {
    if(NULL == allocator_) {
        return;
    }
    if(NULL == *allocator_) {
        return;
    }
    general_allocator_free((void**)&(*allocator_)->linear_allocator_pool, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    general_allocator_free((void**)&(*allocator_)->linear_allocator, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    general_allocator_free((void**)allocator_, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    *allocator_ = NULL;
}

subsystem_allocator_result_t subsystem_allocator_allocate(subsystem_allocator_t* allocator_, size_t allocation_size_, subsystem_allocator_memory_tag_t memory_tag_, void** out_ptr_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    void* tmp_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_allocate", "*out_ptr_")
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - Provided memory_tag_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(0 == allocation_size_) {
        ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - Provided allocation_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    ret_linear_allocator = linear_allocator_allocate(allocator_->linear_allocator, allocation_size_, alignof(max_align_t), (void**)&tmp_ptr);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - linear_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    allocator_->total_allocated += allocation_size_;
    allocator_->memory_tag_allocated[memory_tag_] += allocation_size_;

    *out_ptr_ = tmp_ptr;
    tmp_ptr = NULL;

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

subsystem_allocator_result_t subsystem_allocator_reset(subsystem_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    }
    // TODO: linear_allocator_reset実装後に実装
    return SUBSYSTEM_ALLOCATOR_SUCCESS;
}

bool subsystem_allocator_is_valid(const subsystem_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return false;
    }
    if(!is_valid_shallow(allocator_)) {
        return false;
    }
    return true;
}

static const char* memory_tag_c_str(subsystem_allocator_memory_tag_t memory_tag_) {
    switch(memory_tag_) {
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_PLATFORM_SYSTEM:
        return s_memory_tag_platform_system;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER_SYSTEM:
        return s_memory_tag_renderer_system;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_EVENT_SYSTEM:
        return s_memory_tag_event_system;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_FLIGHT_CAMERA_SYSTEM:
        return s_memory_tag_flight_camera_system;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX:
        return s_memory_tag_undefined;
    default:
        return s_memory_tag_undefined;
    }
}
static const char* result_to_str(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return s_result_str_success;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return s_result_str_bad_operation;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return s_result_str_no_memory;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

static subsystem_allocator_result_t result_convert_linear_allocator(linear_allocator_result_t result_) {
    switch(result_) {
    case LINEAR_ALLOCATOR_SUCCESS:
        return SUBSYSTEM_ALLOCATOR_SUCCESS;
    case LINEAR_ALLOCATOR_NO_MEMORY:
        return SUBSYSTEM_ALLOCATOR_NO_MEMORY;
    case LINEAR_ALLOCATOR_INVALID_ARGUMENT:
        return SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    default:
        return SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR;
    }
}

static subsystem_allocator_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return SUBSYSTEM_ALLOCATOR_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return SUBSYSTEM_ALLOCATOR_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return SUBSYSTEM_ALLOCATOR_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR;
    default:
        return SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR;
    }
}

static bool is_valid_shallow(const subsystem_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return false;
    }
    return true;
}

static bool memory_tag_is_valid(subsystem_allocator_memory_tag_t memory_tag_) {
    if(SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX <= memory_tag_ || 0 > (int)memory_tag_) {
        return false;
    }
    return true;
}
