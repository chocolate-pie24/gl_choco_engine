// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/camera/core/camera_err_utils.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/camera/core/camera_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* camera_result_to_str(camera_result_t result_) {
    switch(result_) {
    case CAMERA_SUCCESS:
        return s_result_str_success;
    case CAMERA_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case CAMERA_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case CAMERA_NO_MEMORY:
        return s_result_str_no_memory;
    case CAMERA_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case CAMERA_BAD_OPERATION:
        return s_result_str_bad_operation;
    case CAMERA_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case CAMERA_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

camera_result_t camera_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return CAMERA_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return CAMERA_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return CAMERA_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return CAMERA_UNDEFINED_ERROR;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return CAMERA_UNDEFINED_ERROR;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
}
