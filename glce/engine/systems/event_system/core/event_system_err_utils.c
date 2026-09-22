// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/event_system/core/event_system_err_utils.h"

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include "engine/systems/event_system/core/event_system_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* event_system_result_to_str(event_system_result_t result_) {
    switch(result_) {
    case EVENT_SYSTEM_SUCCESS:
        return s_result_str_success;
    case EVENT_SYSTEM_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case EVENT_SYSTEM_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case EVENT_SYSTEM_NO_MEMORY:
        return s_result_str_no_memory;
    case EVENT_SYSTEM_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case EVENT_SYSTEM_BAD_OPERATION:
        return s_result_str_bad_operation;
    case EVENT_SYSTEM_OVERFLOW:
        return s_result_str_overflow;
    case EVENT_SYSTEM_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case EVENT_SYSTEM_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

event_system_result_t event_system_result_convert_linear_allocator(linear_allocator_result_t result_) {
    switch(result_) {
    case LINEAR_ALLOCATOR_SUCCESS:
        return EVENT_SYSTEM_SUCCESS;
    case LINEAR_ALLOCATOR_NO_MEMORY:
        return EVENT_SYSTEM_NO_MEMORY;
    case LINEAR_ALLOCATOR_INVALID_ARGUMENT:
        return EVENT_SYSTEM_INVALID_ARGUMENT;
    default:
        return EVENT_SYSTEM_UNDEFINED_ERROR;
    }
}

event_system_result_t event_system_result_convert_platform_system(platform_system_result_t result_) {
    switch(result_) {
    case PLATFORM_SYSTEM_SUCCESS:
        return EVENT_SYSTEM_SUCCESS;
    case PLATFORM_SYSTEM_INVALID_ARGUMENT:
        return EVENT_SYSTEM_INVALID_ARGUMENT;
    case PLATFORM_SYSTEM_RUNTIME_ERROR:
        return EVENT_SYSTEM_RUNTIME_ERROR;
    case PLATFORM_SYSTEM_NO_MEMORY:
        return EVENT_SYSTEM_NO_MEMORY;
    case PLATFORM_SYSTEM_DATA_CORRUPTED:
        return EVENT_SYSTEM_DATA_CORRUPTED;
    case PLATFORM_SYSTEM_BAD_OPERATION:
        return EVENT_SYSTEM_BAD_OPERATION;
    case PLATFORM_SYSTEM_OVERFLOW:
        return EVENT_SYSTEM_OVERFLOW;
    case PLATFORM_SYSTEM_LIMIT_EXCEEDED:
        return EVENT_SYSTEM_LIMIT_EXCEEDED;
    case PLATFORM_SYSTEM_UNDEFINED_ERROR:
        return EVENT_SYSTEM_UNDEFINED_ERROR;
    default:
        return EVENT_SYSTEM_UNDEFINED_ERROR;
    }
}
