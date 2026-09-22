// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset

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
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";   /**< 実行結果種別文字列(無効な引数) */
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";     /**< 実行結果種別文字列(不明なエラー) */

static const char* result_to_str(linear_allocator_result_t result_);

void linear_allocator_preinit(size_t* memory_requirement_, size_t* align_requirement_) {
    if(NULL == memory_requirement_ || NULL == align_requirement_) {
        return;
    }
    *memory_requirement_ = sizeof(linear_allocator_t);
    *align_requirement_ = alignof(linear_allocator_t);
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

linear_allocator_result_t linear_allocator_allocate(linear_allocator_t* allocator_, size_t req_size_, size_t req_align_, void** out_ptr_) {
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
    if(0 == req_align_ || 0 == req_size_) {
        WARN_MESSAGE("linear_allocator_allocate - No-op: req_align_ or req_size_ is 0.");
        ret = LINEAR_ALLOCATOR_SUCCESS;
        goto cleanup;
    }
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(req_align_), ret, LINEAR_ALLOCATOR_INVALID_ARGUMENT, result_to_str(LINEAR_ALLOCATOR_INVALID_ARGUMENT), "linear_allocator_allocate", "req_align_")

    // Simulation
    head = (uintptr_t)allocator_->head_ptr;
    align = (uintptr_t)req_align_;
    size = (uintptr_t)req_size_;
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
        ERROR_MESSAGE("linear_allocator_allocate(%s) - Cannot allocate requested size. Requested size: %zu / Free space: %zu", result_to_str(ret), req_size_, (size_t)free_space);
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
    case LINEAR_ALLOCATOR_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    default:
        return s_result_str_undefined_error;
    }
}
