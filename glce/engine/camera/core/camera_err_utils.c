// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/camera/core/camera_err_utils.h"

#include "engine/camera/core/camera_types.h"

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

const char* camera_rslt_to_str(camera_result_t rslt_) {
    switch(rslt_) {
    case CAMERA_SUCCESS:
        return s_rslt_str_success;
    case CAMERA_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case CAMERA_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case CAMERA_NO_MEMORY:
        return s_rslt_str_no_memory;
    case CAMERA_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case CAMERA_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case CAMERA_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case CAMERA_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

camera_result_t camera_rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return CAMERA_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return CAMERA_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return CAMERA_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
}
