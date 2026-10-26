// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/meshes/core/mesh_err_utils.h"

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* mesh_result_to_str(mesh_result_t result_) {
    switch(result_) {
    case MESH_SUCCESS:
        return s_result_str_success;
    case MESH_NO_MEMORY:
        return s_result_str_no_memory;
    case MESH_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case MESH_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case MESH_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case MESH_BAD_OPERATION:
        return s_result_str_bad_operation;
    case MESH_OVERFLOW:
        return s_result_str_overflow;
    case MESH_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case MESH_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

mesh_result_t mesh_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return MESH_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return MESH_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return MESH_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return MESH_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return MESH_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return MESH_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return MESH_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return MESH_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return MESH_LIMIT_EXCEEDED;
    default:
        return MESH_UNDEFINED_ERROR;
    }
}
