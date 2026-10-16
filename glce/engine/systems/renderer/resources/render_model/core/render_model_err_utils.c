// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/render_model/core/render_model_err_utils.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* render_model_result_to_str(render_model_result_t result_) {
    switch(result_) {
    case RENDER_MODEL_SUCCESS:
        return s_result_str_success;
    case RENDER_MODEL_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case RENDER_MODEL_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case RENDER_MODEL_NO_MEMORY:
        return s_result_str_no_memory;
    case RENDER_MODEL_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case RENDER_MODEL_BAD_OPERATION:
        return s_result_str_bad_operation;
    case RENDER_MODEL_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case RENDER_MODEL_OVERFLOW:
        return s_result_str_overflow;
    case RENDER_MODEL_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

render_model_result_t render_model_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return RENDER_MODEL_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return RENDER_MODEL_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return RENDER_MODEL_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return RENDER_MODEL_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return RENDER_MODEL_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return RENDER_MODEL_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return RENDER_MODEL_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return RENDER_MODEL_UNDEFINED_ERROR;
    default:
        return RENDER_MODEL_UNDEFINED_ERROR;
    }
}
