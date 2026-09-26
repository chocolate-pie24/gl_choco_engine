// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"
#include "engine/memory/general_allocator/general_allocator.h"

struct subsystem_allocator {
    size_t allocator_alignment_requirement;  /**< リニアアロケータ構造体インスタンスが要求するメモリアライメント */
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

static const char* const s_memory_tag_platform = "PLATFORM_SYSTEM";
static const char* const s_memory_tag_renderer = "RENDERER_SYSTEM";
static const char* const s_memory_tag_event = "EVENT_SYSTEM";
static const char* const s_memory_tag_camera = "CAMERA_SYSTEM";
static const char* const s_memory_tag_undefined = "UNDEFINED";

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

    ret_linear_allocator = linear_allocator_allocate(allocator_->linear_allocator, allocation_size_, (void**)&tmp_ptr);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_allocate(%s) - linear_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }
    memset(tmp_ptr, 0, allocation_size_);

    allocator_->total_allocated += allocation_size_;
    allocator_->memory_tag_allocated[memory_tag_] += allocation_size_;

    *out_ptr_ = tmp_ptr;
    tmp_ptr = NULL;

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// subsystem_allocator_reset Validation Policy
//
// - allocator_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、
//   allocator_およびowned Linear Allocatorを含むownership closureがvalidなStable stateであることを確認する。
//   resetはcorrupted stateを修復するためのAPIとして扱わない。
// - DEBUG_BUILD / TEST_BUILDでは、すべてのreset処理が完了したStable boundaryで
//   canonical validatorを実行し、owned Linear Allocatorを含むSubsystem Allocator全体が
//   validなStable stateへ戻ったことをPostconditionとして確認する。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてresetを実行する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
subsystem_allocator_result_t subsystem_allocator_reset(subsystem_allocator_t* allocator_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_reset", "allocator_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!subsystem_allocator_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_reset(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_linear_allocator = linear_allocator_reset(allocator_->linear_allocator);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_reset(%s) - linear_allocator_reset failed.", result_to_str(ret));
        goto cleanup;
    }

    allocator_->total_allocated = 0;
    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        allocator_->memory_tag_allocated[i] = 0;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!subsystem_allocator_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_reset(%s) - " "Postcondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// subsystem_allocator_status_get Validation Policy
//
// - allocator_およびout_status_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、
//   allocator_がvalidなStable stateであることを確認する。
// - 本APIはallocator_を変更しないread-only operationであるため、
//   成功時のPostcondition canonical validationは行わない。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてstatusを算出する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
subsystem_allocator_result_t subsystem_allocator_status_get(const subsystem_allocator_t* allocator_, subsystem_allocator_status_t* out_status_) {
    subsystem_allocator_result_t ret = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_status_t linear_allocator_status = { 0 };
    size_t memory_pool_size = 0;
    size_t used_size = 0;
    size_t free_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_status_get", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_status_, ret, SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT, result_to_str(SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT), "subsystem_allocator_status_get", "out_status_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!subsystem_allocator_is_valid(allocator_)) {
        ret = SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("subsystem_allocator_status_get(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_linear_allocator = linear_allocator_status_get(allocator_->linear_allocator, &linear_allocator_status);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("subsystem_allocator_status_get(%s) - linear_allocator_status_get failed.", result_to_str(ret));
        goto cleanup;
    }

    memory_pool_size = linear_allocator_status.memory_pool_size;
    used_size = linear_allocator_status.used_size;
    free_size = linear_allocator_status.free_size;

    out_status_->free_size = free_size;
    out_status_->memory_pool_size = memory_pool_size;
    out_status_->used_size = used_size;
    out_status_->total_allocated = allocator_->total_allocated;
    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        out_status_->memory_tag_allocated[i] = allocator_->memory_tag_allocated[i];
    }

    ret = SUBSYSTEM_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

const char* subsystem_allocator_memory_tag_to_str(subsystem_allocator_memory_tag_t memory_tag_) {
    switch(memory_tag_) {
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_PLATFORM:
        return s_memory_tag_platform;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER:
        return s_memory_tag_renderer;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_EVENT:
        return s_memory_tag_event;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_CAMERA:
        return s_memory_tag_camera;
    case SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX:
        return s_memory_tag_undefined;
    default:
        return s_memory_tag_undefined;
    }
}

bool subsystem_allocator_is_valid(const subsystem_allocator_t* allocator_) {
    size_t total_tag_allocated = 0;

    if(NULL == allocator_) {
        return false;
    }
    if(!is_valid_shallow(allocator_)) {
        return false;
    }

    if(!linear_allocator_is_valid(allocator_->linear_allocator)) {
        return false;
    }

    for(size_t i = 0; i != SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        // total_tag_allocatedに対する加算をを安全に実行できることを確認
        if(allocator_->memory_tag_allocated[i] > (allocator_->total_allocated - total_tag_allocated)) {
            return false;
        }
        total_tag_allocated += allocator_->memory_tag_allocated[i];
    }

    if(total_tag_allocated != allocator_->total_allocated) {
        return false;
    }

    return true;
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
    case LINEAR_ALLOCATOR_DATA_CORRUPTED:
        return SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED;
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

    if(NULL == allocator_->linear_allocator) {
        return false;
    }
    if(NULL == allocator_->linear_allocator_pool) {
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
