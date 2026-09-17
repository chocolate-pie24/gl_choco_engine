#include "engine/core/memory/free_list_allocator.h"

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/memory_tag.h"

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

static const char* rslt_to_str(free_list_allocator_result_t rslt_);

static bool is_valid_shallow(const free_list_allocator_t* free_list_allocator_);

free_list_allocator_result_t free_list_allocator_initialize(size_t memory_pool_size_, void* memory_pool_, free_list_allocator_t* free_list_allocator_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(memory_pool_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_initialize", "memory_pool_")
    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_initialize", "free_list_allocator_")
    if(0 == memory_pool_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Provided memory_pool_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    free_list_allocator_->memory_pool = memory_pool_;
    free_list_allocator_->memory_pool_size = memory_pool_size_;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

void free_list_allocator_deinitialize(free_list_allocator_t* free_list_allocator_) {
    if(NULL == free_list_allocator_) {
        return;
    }
    free_list_allocator_->memory_pool = NULL;
    free_list_allocator_->memory_pool_size = 0;
}

bool free_list_allocator_is_valid(const free_list_allocator_t* free_list_allocator_) {
    if(NULL == free_list_allocator_) {
        return false;
    }
    if(!is_valid_shallow(free_list_allocator_)) {
        return false;
    }
    return true;
}

static const char* rslt_to_str(free_list_allocator_result_t rslt_) {
    switch(rslt_) {
    case FREE_LIST_ALLOCATOR_SUCCESS:
        return s_rslt_str_success;
    case FREE_LIST_ALLOCATOR_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case FREE_LIST_ALLOCATOR_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case FREE_LIST_ALLOCATOR_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case FREE_LIST_ALLOCATOR_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case FREE_LIST_ALLOCATOR_NO_MEMORY:
        return s_rslt_str_no_memory;
    case FREE_LIST_ALLOCATOR_OVERFLOW:
        return s_rslt_str_overflow;
    case FREE_LIST_ALLOCATOR_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

static bool is_valid_shallow(const free_list_allocator_t* free_list_allocator_) {
    if(NULL == free_list_allocator_) {
        return false;
    }
    if(NULL == free_list_allocator_->memory_pool) {
        return false;
    }
    if(0 == free_list_allocator_->memory_pool_size) {
        return false;
    }
    return true;
}
