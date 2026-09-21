// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/memory_system/memory_system.h"

#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>

#include "config/build_config.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/core/memory_tag.h"
#include "engine/memory/allocators/free_list_allocator.h"

#include "engine/systems/memory_system/core/memory_system_types.h"
#include "engine/systems/memory_system/core/memory_system_err_utils.h"

struct memory_system {
    // Allocator
    free_list_allocator_t free_list_allocator;
    void* memory_pool;

    // memory使用量管理
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t mem_tag_allocated[MEMORY_TAG_MAX];   /**< 各メモリタグごとのメモリ割り当て量 */
};

static memory_system_t s_memory_system;

// 組み込み向けに静的領域でmemory poolを用意(アライメントはmax_align_tでfree list allocatorのメモリ要件を満たす)
#if defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
alignas(max_align_t)
static unsigned char s_memory_pool[GLCE_BUILD_MEMORY_POOL_SIZE];
#endif

memory_system_result_t memory_system_create(void) {
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // memory poolの初期化
#if defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
    s_memory_system.memory_pool = (void*)s_memory_pool;
#elif defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
    s_memory_system.memory_pool = malloc(GLCE_BUILD_MEMORY_POOL_SIZE);
#else
    ret = MEMORY_SYSTEM_UNDEFINED_ERROR;
    ERROR_MESSAGE("memory_system_create(%s) - Undefined build config.", memory_system_rslt_to_str(ret));
    goto cleanup;
#endif
    if(NULL == s_memory_system.memory_pool) {
        ret = MEMORY_SYSTEM_NO_MEMORY;
        ERROR_MESSAGE("memory_system_create(%s) - Failed to allocate memory for memory pool.", memory_system_rslt_to_str(ret));
        goto cleanup;
    }

    ret_free_list_allocator = free_list_allocator_initialize(GLCE_BUILD_MEMORY_POOL_SIZE, s_memory_system.memory_pool, &s_memory_system.free_list_allocator);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ret = memory_system_result_convert_free_list_allocator(ret_free_list_allocator);
        ERROR_MESSAGE("memory_system_create(%s) - free_list_allocator_initialize failed.", memory_system_rslt_to_str(ret));
        goto cleanup;
    }

    s_memory_system.total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        s_memory_system.mem_tag_allocated[i] = 0;
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!memory_system_is_valid(&s_memory_system)) {
        ret = MEMORY_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("memory_system_create(%s) - Postcondition validation failed for 's_memory_system'.", memory_system_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    if(MEMORY_SYSTEM_SUCCESS != ret) {
#if defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
        free(s_memory_system.memory_pool);
        s_memory_system.memory_pool = NULL;
#endif
    }
    return ret;
}

void memory_system_destroy(void) {
    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!memory_system_is_valid(&s_memory_system)) {
        ERROR_MESSAGE("memory_system_destroy(%s) - Precondition validation failed for 's_memory_system'.", memory_system_rslt_to_str(MEMORY_SYSTEM_DATA_CORRUPTED));
        return;
    }
#endif

    ret_free_list_allocator = free_list_allocator_deinitialize(&s_memory_system.free_list_allocator);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("memory_system_destroy(%s) - free_list_allocator_deinitialize failed.", memory_system_rslt_to_str(MEMORY_SYSTEM_DATA_CORRUPTED));
        return;
    }
#if defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
    free(s_memory_system.memory_pool);
    s_memory_system.memory_pool = NULL;
#elif defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
    s_memory_system.memory_pool = NULL;
#else
    // memory systemがvalidであることを確認済みなので処理は不要
#endif

    s_memory_system.total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        s_memory_system.mem_tag_allocated[i] = 0;
    }
}

memory_system_result_t memory_system_allocate(size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_) {
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    void* tmp_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, memory_system_rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, MEMORY_SYSTEM_BAD_OPERATION, memory_system_rslt_to_str(MEMORY_SYSTEM_BAD_OPERATION), "memory_system_allocate", "*out_ptr_")
    if(0 == allocation_size_) {
        ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("memory_system_allocate(%s) - Provided allocation_size_ is not valid.", memory_system_rslt_to_str(ret));
        goto cleanup;
    }
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("memory_system_allocate(%s) - Provided memory_tag_ is not valid.", memory_system_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!memory_system_is_valid(&s_memory_system)) {
        ret = MEMORY_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("memory_system_allocate(%s) - Precondition validation failed for 's_memory_system'.", memory_system_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_free_list_allocator = free_list_allocator_allocate(&s_memory_system.free_list_allocator, allocation_size_, memory_tag_, &tmp_ptr);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ret = memory_system_result_convert_free_list_allocator(ret_free_list_allocator);
        ERROR_MESSAGE("memory_system_allocate(%s) - free_list_allocator_allocate failed.", memory_system_rslt_to_str(ret));
        goto cleanup;
    }

    s_memory_system.mem_tag_allocated[memory_tag_] += allocation_size_;
    s_memory_system.total_allocated += allocation_size_;

    *out_ptr_ = tmp_ptr;

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

void memory_system_free(void** ptr_) {
    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t allocation_size = 0;
    memory_tag_t memory_tag;

    if(NULL == ptr_ || NULL == *ptr_) {
        ERROR_MESSAGE("memory_system_free(%s) - Provided ptr_ is not valid.", memory_system_rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!memory_system_is_valid(&s_memory_system)) {
        ERROR_MESSAGE("memory_system_free(%s) - Precondition validation failed for 's_memory_system'.", memory_system_rslt_to_str(MEMORY_SYSTEM_DATA_CORRUPTED));
        return;
    }
#endif

    ret_free_list_allocator = free_list_allocator_allocation_info_get(&s_memory_system.free_list_allocator, *ptr_, &allocation_size, &memory_tag);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("memory_system_free(%s) - memory system is corrupted.", memory_system_rslt_to_str(MEMORY_SYSTEM_DATA_CORRUPTED));
        return;
    }
    if(!memory_tag_is_valid(memory_tag)) {
        ERROR_MESSAGE("memory_system_free(%s) - memory system is corrupted.", memory_system_rslt_to_str(MEMORY_SYSTEM_DATA_CORRUPTED));
        return;
    }
    if(s_memory_system.total_allocated < allocation_size) {
        ERROR_MESSAGE("memory_system_free(%s) - memory system is corrupted.", memory_system_rslt_to_str(MEMORY_SYSTEM_DATA_CORRUPTED));
        return;
    }
    if(s_memory_system.mem_tag_allocated[memory_tag] < allocation_size) {
        ERROR_MESSAGE("memory_system_free(%s) - memory system is corrupted.", memory_system_rslt_to_str(MEMORY_SYSTEM_DATA_CORRUPTED));
        return;
    }

    ret_free_list_allocator = free_list_allocator_free(&s_memory_system.free_list_allocator, *ptr_);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("memory_system_free(%s) - memory system is corrupted.", memory_system_rslt_to_str(MEMORY_SYSTEM_DATA_CORRUPTED));
        return;
    }

    s_memory_system.total_allocated -= allocation_size;
    s_memory_system.mem_tag_allocated[memory_tag] -= allocation_size;

    *ptr_ = NULL;
}

bool memory_system_is_valid(const memory_system_t* memory_system_) {
    if(NULL == memory_system_) {
        return false;
    }
    // 後で実装
    return true;
}
