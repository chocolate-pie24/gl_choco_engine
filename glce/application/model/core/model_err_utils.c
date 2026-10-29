// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/model/core/model_err_utils.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"
#include "engine/containers/ring_queue.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

#include "application/model/core/model_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* model_result_to_str(model_result_t result_) {
    switch(result_) {
    case MODEL_SUCCESS:
        return s_result_str_success;
    case MODEL_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case MODEL_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case MODEL_NO_MEMORY:
        return s_result_str_no_memory;
    case MODEL_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case MODEL_BAD_OPERATION:
        return s_result_str_bad_operation;
    case MODEL_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case MODEL_OVERFLOW:
        return s_result_str_overflow;
    case MODEL_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

model_result_t model_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return MODEL_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return MODEL_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return MODEL_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return MODEL_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return MODEL_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return MODEL_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return MODEL_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return MODEL_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return MODEL_LIMIT_EXCEEDED;
    default:
        return MODEL_UNDEFINED_ERROR;
    }
}

model_result_t model_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return MODEL_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return MODEL_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return MODEL_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return MODEL_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return MODEL_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return MODEL_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return MODEL_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return MODEL_UNDEFINED_ERROR;
    default:
        return MODEL_UNDEFINED_ERROR;
    }
}

model_result_t model_result_convert_ring_queue(ring_queue_result_t result_) {
    switch(result_) {
    case RING_QUEUE_SUCCESS:
        return MODEL_SUCCESS;
    case RING_QUEUE_INVALID_ARGUMENT:
        return MODEL_INVALID_ARGUMENT;
    case RING_QUEUE_NO_MEMORY:
        return MODEL_NO_MEMORY;
    case RING_QUEUE_RUNTIME_ERROR:
        return MODEL_RUNTIME_ERROR;
    case RING_QUEUE_LIMIT_EXCEEDED:
        return MODEL_LIMIT_EXCEEDED;
    case RING_QUEUE_BAD_OPERATION:
        return MODEL_BAD_OPERATION;
    case RING_QUEUE_DATA_CORRUPTED:
        return MODEL_DATA_CORRUPTED;
    case RING_QUEUE_OVERFLOW:
        return MODEL_OVERFLOW;
    case RING_QUEUE_EMPTY:
        return MODEL_RUNTIME_ERROR;
    case RING_QUEUE_UNDEFINED_ERROR:
        return MODEL_UNDEFINED_ERROR;
    default:
        return MODEL_UNDEFINED_ERROR;
    }
}

model_result_t model_result_convert_renderer_frontend(renderer_frontend_result_t result_) {
    switch(result_) {
    case RENDERER_FRONTEND_SUCCESS:
        return MODEL_SUCCESS;
    case RENDERER_FRONTEND_INVALID_ARGUMENT:
        return MODEL_INVALID_ARGUMENT;
    case RENDERER_FRONTEND_RUNTIME_ERROR:
        return MODEL_RUNTIME_ERROR;
    case RENDERER_FRONTEND_OVERFLOW:
        return MODEL_OVERFLOW;
    case RENDERER_FRONTEND_BAD_OPERATION:
        return MODEL_BAD_OPERATION;
    case RENDERER_FRONTEND_DATA_CORRUPTED:
        return MODEL_DATA_CORRUPTED;
    case RENDERER_FRONTEND_NO_MEMORY:
        return MODEL_NO_MEMORY;
    case RENDERER_FRONTEND_LIMIT_EXCEEDED:
        return MODEL_LIMIT_EXCEEDED;
    case RENDERER_FRONTEND_UNDEFINED_ERROR:
        return MODEL_UNDEFINED_ERROR;
    default:
        return MODEL_UNDEFINED_ERROR;
    }
}

model_result_t model_result_convert_mesh(mesh_result_t result_) {
    switch(result_) {
    case MESH_SUCCESS:
        return MODEL_SUCCESS;
    case MESH_NO_MEMORY:
        return MODEL_NO_MEMORY;
    case MESH_RUNTIME_ERROR:
        return MODEL_RUNTIME_ERROR;
    case MESH_INVALID_ARGUMENT:
        return MODEL_INVALID_ARGUMENT;
    case MESH_DATA_CORRUPTED:
        return MODEL_DATA_CORRUPTED;
    case MESH_BAD_OPERATION:
        return MODEL_BAD_OPERATION;
    case MESH_OVERFLOW:
        return MODEL_OVERFLOW;
    case MESH_LIMIT_EXCEEDED:
        return MODEL_LIMIT_EXCEEDED;
    case MESH_UNDEFINED_ERROR:
        return MODEL_UNDEFINED_ERROR;
    default:
        return MODEL_UNDEFINED_ERROR;
    }
}
