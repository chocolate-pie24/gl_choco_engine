// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/memory/general_allocator/general_allocator.h"

#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "config/build_config.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/low_level_allocators/free_list_allocator/free_list_allocator.h"

struct general_allocator {
    // Allocator
    free_list_allocator_t free_list_allocator;
    void* memory_pool;

    // memory使用量管理
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t mem_tag_allocated[GENERAL_ALLOCATOR_MEMORY_TAG_MAX];   /**< 各メモリタグごとのメモリ割り当て量 */
};

static general_allocator_t s_general_allocator;

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

static const char* const s_memory_tag_system = "SYSTEM";
static const char* const s_memory_tag_string = "STRING";
static const char* const s_memory_tag_ring_queue = "RING_QUEUE";
static const char* const s_memory_tag_renderer = "RENDERER";
static const char* const s_memory_tag_file_io = "FILE_IO";
static const char* const s_memory_tag_camera = "CAMERA";
static const char* const s_memory_tag_texture = "TEXTURE";
static const char* const s_memory_tag_geometry = "GEOMETRY";
static const char* const s_memory_tag_undefined = "UNDEFINED";

// 組み込み向けに静的領域でmemory poolを用意(アライメントはmax_align_tでfree list allocatorのメモリ要件を満たす)
#if defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
alignas(max_align_t)
static unsigned char s_memory_pool[GLCE_BUILD_MEMORY_POOL_SIZE];
#endif

static const char* memory_tag_c_str(general_allocator_memory_tag_t memory_tag_);
static const char* result_to_str(general_allocator_result_t result_);
static general_allocator_result_t result_convert_free_list_allocator(free_list_allocator_result_t result_);

static bool is_valid_shallow(void);
static bool memory_tag_is_valid(general_allocator_memory_tag_t memory_tag_);

general_allocator_result_t general_allocator_create(void) {
    general_allocator_result_t ret = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // memory poolの初期化
#if defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
    s_general_allocator.memory_pool = (void*)s_memory_pool;
#elif defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
    s_general_allocator.memory_pool = malloc(GLCE_BUILD_MEMORY_POOL_SIZE);
#else
    ret = GENERAL_ALLOCATOR_UNDEFINED_ERROR;
    ERROR_MESSAGE("general_allocator_create(%s) - Undefined build config.", result_to_str(ret));
    goto cleanup;
#endif
    if(NULL == s_general_allocator.memory_pool) {
        ret = GENERAL_ALLOCATOR_NO_MEMORY;
        ERROR_MESSAGE("general_allocator_create(%s) - Failed to allocate memory for memory pool.", result_to_str(ret));
        goto cleanup;
    }

    ret_free_list_allocator = free_list_allocator_initialize(GLCE_BUILD_MEMORY_POOL_SIZE, s_general_allocator.memory_pool, &s_general_allocator.free_list_allocator);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ret = result_convert_free_list_allocator(ret_free_list_allocator);
        ERROR_MESSAGE("general_allocator_create(%s) - free_list_allocator_initialize failed.", result_to_str(ret));
        goto cleanup;
    }

    s_general_allocator.total_allocated = 0;
    for(size_t i = 0; i != GENERAL_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        s_general_allocator.mem_tag_allocated[i] = 0;
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!general_allocator_is_valid()) {
        ret = GENERAL_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("general_allocator_create(%s) - Postcondition validation failed for 's_general_allocator'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = GENERAL_ALLOCATOR_SUCCESS;

cleanup:
    if(GENERAL_ALLOCATOR_SUCCESS != ret) {
#if defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
        free(s_general_allocator.memory_pool);
        s_general_allocator.memory_pool = NULL;
#endif
    }
    return ret;
}

void general_allocator_destroy(void) {
    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!general_allocator_is_valid()) {
        ERROR_MESSAGE("general_allocator_destroy(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#endif

    ret_free_list_allocator = free_list_allocator_deinitialize(&s_general_allocator.free_list_allocator);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("general_allocator_destroy(%s) - free_list_allocator_deinitialize failed.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#if defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
    free(s_general_allocator.memory_pool);
    s_general_allocator.memory_pool = NULL;
#elif defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
    s_general_allocator.memory_pool = NULL;
#else
    // general allocatorがvalidであることを確認済みなので処理は不要
#endif

    s_general_allocator.total_allocated = 0;
    for(size_t i = 0; i != GENERAL_ALLOCATOR_MEMORY_TAG_MAX; ++i) {
        s_general_allocator.mem_tag_allocated[i] = 0;
    }
}

general_allocator_result_t general_allocator_allocate(size_t allocation_size_, general_allocator_memory_tag_t memory_tag_, void** out_ptr_) {
    general_allocator_result_t ret = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    void* tmp_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, GENERAL_ALLOCATOR_INVALID_ARGUMENT, result_to_str(GENERAL_ALLOCATOR_INVALID_ARGUMENT), "general_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, GENERAL_ALLOCATOR_BAD_OPERATION, result_to_str(GENERAL_ALLOCATOR_BAD_OPERATION), "general_allocator_allocate", "*out_ptr_")
    if(0 == allocation_size_) {
        ret = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("general_allocator_allocate(%s) - Provided allocation_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("general_allocator_allocate(%s) - Provided memory_tag_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!general_allocator_is_valid()) {
        ret = GENERAL_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("general_allocator_allocate(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_free_list_allocator = free_list_allocator_allocate(&s_general_allocator.free_list_allocator, allocation_size_, &tmp_ptr);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ret = result_convert_free_list_allocator(ret_free_list_allocator);
        ERROR_MESSAGE("general_allocator_allocate(%s) - free_list_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    memset(tmp_ptr, 0, allocation_size_);
    s_general_allocator.mem_tag_allocated[memory_tag_] += allocation_size_;
    s_general_allocator.total_allocated += allocation_size_;

    *out_ptr_ = tmp_ptr;

    ret = GENERAL_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

void general_allocator_free(void** ptr_, general_allocator_memory_tag_t memory_tag_) {
    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t allocation_size = 0;

    if(NULL == ptr_ || NULL == *ptr_) {
        ERROR_MESSAGE("general_allocator_free(%s) - Provided ptr_ is not valid.", result_to_str(GENERAL_ALLOCATOR_INVALID_ARGUMENT));
        return;
    }
    if(!memory_tag_is_valid(memory_tag_)) {
        ERROR_MESSAGE("general_allocator_free(%s) - Provided memory_tag_ is not valid.", result_to_str(GENERAL_ALLOCATOR_INVALID_ARGUMENT));
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!general_allocator_is_valid()) {
        ERROR_MESSAGE("general_allocator_free(%s) - Precondition validation failed for 's_general_allocator'.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#endif

    ret_free_list_allocator = free_list_allocator_allocation_info_get(&s_general_allocator.free_list_allocator, *ptr_, &allocation_size);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("general_allocator_free(%s) - general allocator is corrupted.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
    if(s_general_allocator.total_allocated < allocation_size) {
        ERROR_MESSAGE("general_allocator_free(%s) - general allocator is corrupted.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
    if(s_general_allocator.mem_tag_allocated[memory_tag_] < allocation_size) {
        ERROR_MESSAGE("general_allocator_free(%s) - general allocator is corrupted.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }

    ret_free_list_allocator = free_list_allocator_free(&s_general_allocator.free_list_allocator, *ptr_);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("general_allocator_free(%s) - general allocator is corrupted.", result_to_str(GENERAL_ALLOCATOR_DATA_CORRUPTED));
        return;
    }

    s_general_allocator.total_allocated -= allocation_size;
    s_general_allocator.mem_tag_allocated[memory_tag_] -= allocation_size;

    *ptr_ = NULL;
}

bool general_allocator_is_valid(void) {
    if(!is_valid_shallow()) {
        return false;
    }
    // 後で実装
    return true;
}

static const char* memory_tag_c_str(general_allocator_memory_tag_t memory_tag_) {
    switch(memory_tag_) {
    case GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM:
        return s_memory_tag_system;
    case GENERAL_ALLOCATOR_MEMORY_TAG_STRING:
        return s_memory_tag_string;
    case GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE:
        return s_memory_tag_ring_queue;
    case GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER:
        return s_memory_tag_renderer;
    case GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO:
        return s_memory_tag_file_io;
    case GENERAL_ALLOCATOR_MEMORY_TAG_CAMERA:
        return s_memory_tag_camera;
    case GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE:
        return s_memory_tag_texture;
    case GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY:
        return s_memory_tag_geometry;
    case GENERAL_ALLOCATOR_MEMORY_TAG_MAX:
        return s_memory_tag_undefined;
    default:
        return s_memory_tag_undefined;
    }
}

static const char* result_to_str(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return s_result_str_success;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return s_result_str_bad_operation;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return s_result_str_no_memory;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return s_result_str_overflow;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

static general_allocator_result_t result_convert_free_list_allocator(free_list_allocator_result_t result_) {
    switch(result_) {
    case FREE_LIST_ALLOCATOR_SUCCESS:
        return GENERAL_ALLOCATOR_SUCCESS;
    case FREE_LIST_ALLOCATOR_DATA_CORRUPTED:
        return GENERAL_ALLOCATOR_DATA_CORRUPTED;
    case FREE_LIST_ALLOCATOR_BAD_OPERATION:
        return GENERAL_ALLOCATOR_BAD_OPERATION;
    case FREE_LIST_ALLOCATOR_INVALID_ARGUMENT:
        return GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    case FREE_LIST_ALLOCATOR_NO_MEMORY:
        return GENERAL_ALLOCATOR_NO_MEMORY;
    case FREE_LIST_ALLOCATOR_OVERFLOW:
        return GENERAL_ALLOCATOR_OVERFLOW;
    case FREE_LIST_ALLOCATOR_UNDEFINED_ERROR:
        return GENERAL_ALLOCATOR_UNDEFINED_ERROR;
    default:
        return GENERAL_ALLOCATOR_UNDEFINED_ERROR;
    }
}

static bool is_valid_shallow(void) {
    return true;
}

static bool memory_tag_is_valid(general_allocator_memory_tag_t memory_tag_) {
    if(memory_tag_ >= GENERAL_ALLOCATOR_MEMORY_TAG_MAX || 0 > memory_tag_) {
        return false;
    }
    return true;
}
