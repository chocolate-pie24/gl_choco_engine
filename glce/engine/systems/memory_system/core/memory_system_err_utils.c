// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/memory_system/core/memory_system_err_utils.h"

#include "engine/systems/memory_system/core/memory_system_types.h"

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

const char* memory_system_rslt_to_str(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return s_rslt_str_success;
    case MEMORY_SYSTEM_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case MEMORY_SYSTEM_NO_MEMORY:
        return s_rslt_str_no_memory;
    case MEMORY_SYSTEM_OVERFLOW:
        return s_rslt_str_overflow;
    case MEMORY_SYSTEM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

memory_system_result_t memory_system_result_convert_free_list_allocator(free_list_allocator_result_t rslt_) {
    switch(rslt_) {
    case FREE_LIST_ALLOCATOR_SUCCESS:
        return MEMORY_SYSTEM_SUCCESS;
    case FREE_LIST_ALLOCATOR_DATA_CORRUPTED:
        return MEMORY_SYSTEM_DATA_CORRUPTED;
    case FREE_LIST_ALLOCATOR_BAD_OPERATION:
        return MEMORY_SYSTEM_BAD_OPERATION;
    case FREE_LIST_ALLOCATOR_INVALID_ARGUMENT:
        return MEMORY_SYSTEM_INVALID_ARGUMENT;
    case FREE_LIST_ALLOCATOR_NO_MEMORY:
        return MEMORY_SYSTEM_NO_MEMORY;
    case FREE_LIST_ALLOCATOR_OVERFLOW:
        return MEMORY_SYSTEM_OVERFLOW;
    case FREE_LIST_ALLOCATOR_UNDEFINED_ERROR:
        return MEMORY_SYSTEM_UNDEFINED_ERROR;
    default:
        return MEMORY_SYSTEM_UNDEFINED_ERROR;
    }
}
