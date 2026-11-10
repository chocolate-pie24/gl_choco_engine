// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/demos/sample_robot/core/sample_robot_err_utils.h"

#include "application/model_framework/core/model_framework_types.h"

#include "application/demos/sample_robot/core/sample_robot_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* sample_robot_result_to_str(sample_robot_result_t result_) {
    switch(result_) {
    case SAMPLE_ROBOT_SUCCESS:
        return s_result_str_success;
    case SAMPLE_ROBOT_NO_MEMORY:
        return s_result_str_no_memory;
    case SAMPLE_ROBOT_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case SAMPLE_ROBOT_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case SAMPLE_ROBOT_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case SAMPLE_ROBOT_BAD_OPERATION:
        return s_result_str_bad_operation;
    case SAMPLE_ROBOT_OVERFLOW:
        return s_result_str_overflow;
    case SAMPLE_ROBOT_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case SAMPLE_ROBOT_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

sample_robot_result_t sample_robot_result_convert_model_framework(model_framework_result_t result_) {
    switch(result_) {
    case MODEL_FRAMEWORK_SUCCESS:
        return SAMPLE_ROBOT_SUCCESS;
    case MODEL_FRAMEWORK_INVALID_ARGUMENT:
        return SAMPLE_ROBOT_INVALID_ARGUMENT;
    case MODEL_FRAMEWORK_RUNTIME_ERROR:
        return SAMPLE_ROBOT_RUNTIME_ERROR;
    case MODEL_FRAMEWORK_NO_MEMORY:
        return SAMPLE_ROBOT_NO_MEMORY;
    case MODEL_FRAMEWORK_LIMIT_EXCEEDED:
        return SAMPLE_ROBOT_LIMIT_EXCEEDED;
    case MODEL_FRAMEWORK_BAD_OPERATION:
        return SAMPLE_ROBOT_BAD_OPERATION;
    case MODEL_FRAMEWORK_DATA_CORRUPTED:
        return SAMPLE_ROBOT_DATA_CORRUPTED;
    case MODEL_FRAMEWORK_OVERFLOW:
        return SAMPLE_ROBOT_OVERFLOW;
    case MODEL_FRAMEWORK_UNDEFINED_ERROR:
        return SAMPLE_ROBOT_UNDEFINED_ERROR;
    default:
        return SAMPLE_ROBOT_UNDEFINED_ERROR;
    }
}

sample_robot_result_t sample_robot_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return SAMPLE_ROBOT_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return SAMPLE_ROBOT_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return SAMPLE_ROBOT_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return SAMPLE_ROBOT_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return SAMPLE_ROBOT_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return SAMPLE_ROBOT_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return SAMPLE_ROBOT_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return SAMPLE_ROBOT_UNDEFINED_ERROR;
    default:
        return SAMPLE_ROBOT_UNDEFINED_ERROR;
    }
}
