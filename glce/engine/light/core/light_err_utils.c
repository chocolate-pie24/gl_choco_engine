// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/light/core/light_err_utils.h"

#include "engine/light/core/light_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* light_result_to_str(light_result_t result_) {
    switch(result_) {
    case LIGHT_SUCCESS:
        return s_result_str_success;
    case LIGHT_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case LIGHT_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case LIGHT_BAD_OPERATION:
        return s_result_str_bad_operation;
    case LIGHT_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case LIGHT_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}
