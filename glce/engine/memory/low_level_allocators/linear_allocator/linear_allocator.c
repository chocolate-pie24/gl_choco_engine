// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/memory_utility.h"

static const char* const s_result_str_success = "SUCCESS";                     /**< 実行結果種別文字列(処理成功) */
static const char* const s_result_str_no_memory = "NO_MEMORY";                 /**< 実行結果種別文字列(メモリ確保失敗) */
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";   /**< 実行結果種別文字列(無効な引数) */
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";     /**< 実行結果種別文字列(不明なエラー) */

static linear_allocator_result_t allocation_layout_calc(size_t allocation_size_, size_t* out_payload_offset_, size_t* out_required_block_size_);
static linear_allocator_result_t allocation_is_ready(const linear_allocator_t* allocator_, size_t required_block_size_);

static const char* result_to_str(linear_allocator_result_t result_);

static bool is_valid_shallow(const linear_allocator_t* allocator_);

linear_allocator_result_t linear_allocator_initialize(linear_allocator_t* allocator_, size_t capacity_, void* memory_pool_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    bool is_aligned = false;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(memory_pool_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_initialize", "memory_pool_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != capacity_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_initialize", "capacity_")
    if(!memory_utility_is_aligned((uintptr_t)(memory_pool_), alignof(max_align_t), &is_aligned)) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_initialize(%s) - memory_utility_is_aligned failed.", result_to_str(ret));
        goto cleanup;
    }
    if(!is_aligned) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_initialize(%s) - Provided memory_pool_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    allocator_->capacity = capacity_;
    allocator_->head_ptr = memory_pool_;
    allocator_->memory_pool = memory_pool_;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

linear_allocator_result_t linear_allocator_allocate(linear_allocator_t* allocator_, size_t required_size_, void** out_ptr_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    linear_allocator_allocation_metadata_t* metadata = NULL;
    uintptr_t start_addr = 0;
    size_t payload_offset = 0;
    size_t block_size = 0;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "out_ptr_")
    if(0 == required_size_) {
        WARN_MESSAGE("linear_allocator_allocate - No-op: required_size_ is 0.");
        ret = LINEAR_ALLOCATOR_SUCCESS;
        goto cleanup;
    }

    ret = allocation_layout_calc(required_size_, &payload_offset, &block_size);
    if(LINEAR_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("linear_allocator_allocate(%s) - allocation_layout_calc failed.", result_to_str(ret));
        goto cleanup;
    }

    ret = allocation_is_ready(allocator_, block_size);
    if(LINEAR_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("linear_allocator_allocate(%s) - allocation_is_ready failed.", result_to_str(ret));
        goto cleanup;
    }

    metadata = (linear_allocator_allocation_metadata_t*)allocator_->head_ptr;
    metadata->allocation_size = required_size_;

    *out_ptr_ = (void*)((uintptr_t)allocator_->head_ptr + payload_offset);
    start_addr = (uintptr_t)allocator_->head_ptr;
    start_addr += (uintptr_t)block_size;
    allocator_->head_ptr = (void*)start_addr;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// linear_allocator_reset Validation Policy
//
// - allocator_はNULLでないことを要求する。
// - DEBUG_BUILD / TEST_BUILDではPreconditionsでcanonical validatorを使用し、allocator_がvalidなStable stateであることを確認する。
// - Commitでは、canonical validation済みのmemory_poolをhead_ptrへ代入するだけであり、
//   複数field間のmutation、下位APIの呼び出し、複雑なstate transitionを伴わない。そのため、成功時のPostcondition canonical validationは行わない。
// - RELEASE_BUILDではautomatic canonical validationを行わず、Module Internal Contractが成立していることを前提としてresetを実行する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
linear_allocator_result_t linear_allocator_reset(linear_allocator_t* allocator_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_reset", "allocator_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ret = LINEAR_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_allocator_reset(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    allocator_->head_ptr = allocator_->memory_pool;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// linear_allocator_status_get Validation Policy
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
linear_allocator_result_t linear_allocator_status_get(const linear_allocator_t* allocator_, linear_allocator_status_t* out_status_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    uintptr_t head_address = 0;
    uintptr_t pool_address = 0;

    size_t memory_pool_size = 0;
    size_t used_size = 0;
    size_t free_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_status_get", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_status_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_status_get", "out_status_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!linear_allocator_is_valid(allocator_)) {
        ret = LINEAR_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("linear_allocator_status_get(%s) - Precondition validation failed for 'allocator_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    head_address = (uintptr_t)allocator_->head_ptr;
    pool_address = (uintptr_t)allocator_->memory_pool;

    memory_pool_size = allocator_->capacity;
    used_size = (size_t)(head_address - pool_address);
    free_size = memory_pool_size - used_size;

    out_status_->free_size = free_size;
    out_status_->memory_pool_size = memory_pool_size;
    out_status_->used_size = used_size;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

bool linear_allocator_is_valid(const linear_allocator_t* allocator_) {
    if(NULL == allocator_) {
        return false;
    }
    if(!is_valid_shallow(allocator_)) {
        return false;
    }
    return true;
}

static linear_allocator_result_t allocation_layout_calc(size_t allocation_size_, size_t* out_payload_offset_, size_t* out_required_block_size_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    size_t payload_offset = 0;
    size_t required_block_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_payload_offset_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "allocation_layout_calc", "out_payload_offset_")
    IF_ARG_NULL_GOTO_CLEANUP(out_required_block_size_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "allocation_layout_calc", "out_required_block_size_")
    if(0 == allocation_size_) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_layout_calc(%s) - Provided allocation_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    if(!memory_utility_align_up(sizeof(linear_allocator_allocation_metadata_t), alignof(max_align_t), &payload_offset)) {
        ret = LINEAR_ALLOCATOR_OVERFLOW;
        goto cleanup;
    }

    if((SIZE_MAX - allocation_size_) < payload_offset) {
        ret = LINEAR_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("allocation_layout_calc(%s) - block size overflow.", result_to_str(ret));
        goto cleanup;
    }

    if(!memory_utility_align_up(payload_offset + allocation_size_, alignof(max_align_t), &required_block_size)) {
        ret = LINEAR_ALLOCATOR_OVERFLOW;
        goto cleanup;
    }

    *out_payload_offset_ = payload_offset;
    *out_required_block_size_ = required_block_size;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static linear_allocator_result_t allocation_is_ready(const linear_allocator_t* allocator_, size_t required_block_size_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    uintptr_t head_address = 0;
    uintptr_t pool_address = 0;

    size_t memory_pool_size = 0;
    size_t used_size = 0;
    size_t free_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "allocation_is_ready", "allocator_")
    if(0 == required_block_size_) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_layout_calc(%s) - Provided required_block_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    head_address = (uintptr_t)allocator_->head_ptr;
    pool_address = (uintptr_t)allocator_->memory_pool;

    memory_pool_size = allocator_->capacity;
    used_size = (size_t)(head_address - pool_address);
    free_size = memory_pool_size - used_size;

    if(required_block_size_ > free_size) {
        ret = LINEAR_ALLOCATOR_NO_MEMORY;
        ERROR_MESSAGE("allocation_layout_calc(%s) - no memory.", result_to_str(ret));
        goto cleanup;
    }

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param[in] result_ 文字列に変換する実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* result_to_str(linear_allocator_result_t result_) {
    switch(result_) {
    case LINEAR_ALLOCATOR_SUCCESS:
        return s_result_str_success;
    case LINEAR_ALLOCATOR_NO_MEMORY:
        return s_result_str_no_memory;
    case LINEAR_ALLOCATOR_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case LINEAR_ALLOCATOR_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case LINEAR_ALLOCATOR_OVERFLOW:
        return s_result_str_overflow;
    default:
        return s_result_str_undefined_error;
    }
}

static bool is_valid_shallow(const linear_allocator_t* allocator_) {
    uintptr_t head_address = 0;
    uintptr_t pool_address = 0;
    uintptr_t end_address = 0;

    if(NULL == allocator_) {
        return false;
    }

    if(NULL == allocator_->head_ptr) {
        return false;
    }
    if(NULL == allocator_->memory_pool) {
        return false;
    }
    if(0 == allocator_->capacity) {
        return false;
    }

    head_address = (uintptr_t)allocator_->head_ptr;
    pool_address = (uintptr_t)allocator_->memory_pool;
    if((UINTPTR_MAX - pool_address) < allocator_->capacity) {
        return false;
    }
    end_address = pool_address + allocator_->capacity;
    if(head_address < pool_address || head_address > end_address) {
        return false;
    }

    return true;
}
