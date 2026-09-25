// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_shader_compile_error = "SHADER_COMPILE_ERROR";
static const char* const s_result_str_shader_link_error = "SHADER_LINK_ERROR";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* renderer_backend_result_to_str(renderer_backend_result_t result_) {
    switch(result_) {
    case RENDERER_BACKEND_SUCCESS:
        return s_result_str_success;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case RENDERER_BACKEND_NO_MEMORY:
        return s_result_str_no_memory;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case RENDERER_BACKEND_BAD_OPERATION:
        return s_result_str_bad_operation;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case RENDERER_BACKEND_OVERFLOW:
        return s_result_str_overflow;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return s_result_str_shader_compile_error;
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return s_result_str_shader_link_error;
    default:
        return s_result_str_undefined_error;
    }
}

renderer_backend_result_t renderer_backend_result_convert_subsystem_allocator(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return RENDERER_BACKEND_SUCCESS;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return RENDERER_BACKEND_BAD_OPERATION;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return RENDERER_BACKEND_DATA_CORRUPTED;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return RENDERER_BACKEND_NO_MEMORY;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return RENDERER_BACKEND_INVALID_ARGUMENT;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    default:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    }
}

renderer_backend_result_t renderer_backend_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return RENDERER_BACKEND_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return RENDERER_BACKEND_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return RENDERER_BACKEND_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return RENDERER_BACKEND_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return RENDERER_BACKEND_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return RENDERER_BACKEND_OVERFLOW;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    default:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    }
}
