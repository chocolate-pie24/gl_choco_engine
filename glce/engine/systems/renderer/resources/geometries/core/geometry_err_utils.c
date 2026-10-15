// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/geometries/core/geometry_err_utils.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* geometry_result_to_str(geometry_result_t result_) {
    switch(result_) {
    case GEOMETRY_SUCCESS:
        return s_result_str_success;
    case GEOMETRY_NO_MEMORY:
        return s_result_str_no_memory;
    case GEOMETRY_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case GEOMETRY_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case GEOMETRY_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case GEOMETRY_BAD_OPERATION:
        return s_result_str_bad_operation;
    case GEOMETRY_OVERFLOW:
        return s_result_str_overflow;
    case GEOMETRY_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case GEOMETRY_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

geometry_result_t geometry_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return GEOMETRY_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return GEOMETRY_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return GEOMETRY_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return GEOMETRY_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return GEOMETRY_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return GEOMETRY_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return GEOMETRY_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return GEOMETRY_UNDEFINED_ERROR;
    default:
        return GEOMETRY_UNDEFINED_ERROR;
    }
}

geometry_result_t geometry_result_convert_aabb_3d(aabb_3d_result_t result_) {
    switch(result_) {
    case AABB_3D_SUCCESS:
        return GEOMETRY_SUCCESS;
    case AABB_3D_INVALID_ARGUMENT:
        return GEOMETRY_INVALID_ARGUMENT;
    case AABB_3D_UNDEFINED_ERROR:
        return GEOMETRY_UNDEFINED_ERROR;
    default:
        return GEOMETRY_UNDEFINED_ERROR;
    }
}
