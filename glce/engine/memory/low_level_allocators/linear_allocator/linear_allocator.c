// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief linear_allocator_t内部データ構造
 * @todo 必要であればメモリトラッキング追加(現状では入れる予定はなし)
 */
struct linear_allocator {
    size_t capacity;    /**< アロケータが管理するメモリ容量(byte) */
    void* head_ptr;     /**< 次にメモリを確保する際の先頭アドレス(実際にはアライメント要件分オフセットされたアドレスを渡す) */
    void* memory_pool;  /**< アロケータが管理するメモリ領域 */
};

static const char* const s_result_str_success = "SUCCESS";                     /**< 実行結果種別文字列(処理成功) */
static const char* const s_result_str_no_memory = "NO_MEMORY";                 /**< 実行結果種別文字列(メモリ確保失敗) */
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";   /**< 実行結果種別文字列(無効な引数) */
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";     /**< 実行結果種別文字列(不明なエラー) */

static const char* result_to_str(linear_allocator_result_t result_);

static bool is_valid_shallow(const linear_allocator_t* allocator_);

void linear_allocator_preinit(size_t* out_memory_requirement_, size_t* out_align_requirement_) {
    if(NULL == out_memory_requirement_ || NULL == out_align_requirement_) {
        return;
    }
    *out_memory_requirement_ = sizeof(linear_allocator_t);
    *out_align_requirement_ = alignof(linear_allocator_t);
}

linear_allocator_result_t linear_allocator_initialize(linear_allocator_t* allocator_, size_t capacity_, void* memory_pool_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(memory_pool_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_initialize", "memory_pool_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != capacity_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_initialize", "capacity_")

    allocator_->capacity = capacity_;
    allocator_->head_ptr = memory_pool_;
    allocator_->memory_pool = memory_pool_;

    ret = LINEAR_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

linear_allocator_result_t linear_allocator_allocate(linear_allocator_t* allocator_, size_t required_size_, size_t required_align_, void** out_ptr_) {
    linear_allocator_result_t ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    uintptr_t head = 0;
    uintptr_t align = 0;
    uintptr_t size = 0;
    uintptr_t offset = 0;
    uintptr_t start_addr = 0;
    uintptr_t pool = 0;
    uintptr_t cap = 0;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "out_ptr_")
    if(0 == required_align_ || 0 == required_size_) {
        WARN_MESSAGE("linear_allocator_allocate - No-op: required_align_ or required_size_ is 0.");
        ret = LINEAR_ALLOCATOR_SUCCESS;
        goto cleanup;
    }
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(required_align_), ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "required_align_")

    // Simulation
    head = (uintptr_t)allocator_->head_ptr;
    align = (uintptr_t)required_align_;
    size = (uintptr_t)required_size_;
    offset = head % align;
    if(0 != offset) {
        offset = align - offset;    // 要求アライメントに先頭アドレスを調整
    }
    if(UINTPTR_MAX - offset < head) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Requested alignment offset is too large.", result_to_str(ret));
        goto cleanup;
    }
    start_addr = head + offset;
    if(UINTPTR_MAX - size < start_addr) {
        ret = LINEAR_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Requested size is too large.", result_to_str(ret));
        goto cleanup;
    }
    pool = (uintptr_t)allocator_->memory_pool;
    cap = (uintptr_t)allocator_->capacity;
    if((start_addr + size) > (pool + cap)) {
        uintptr_t free_space = pool + cap - start_addr;
        ret = LINEAR_ALLOCATOR_NO_MEMORY;
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Cannot allocate requested size. Requested size: %zu / Free space: %zu", result_to_str(ret), required_size_, (size_t)free_space);
        goto cleanup;
    }

    // commit
    *out_ptr_ = (void*)start_addr;
    head += offset + size;
    allocator_->head_ptr = (void*)head;

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
