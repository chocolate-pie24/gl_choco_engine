// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_err_utils.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* renderer_frontend_result_to_str(renderer_frontend_result_t result_) {
    switch(result_) {
    case RENDERER_FRONTEND_SUCCESS:
        return s_result_str_success;
    case RENDERER_FRONTEND_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case RENDERER_FRONTEND_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case RENDERER_FRONTEND_OVERFLOW:
        return s_result_str_overflow;
    case RENDERER_FRONTEND_BAD_OPERATION:
        return s_result_str_bad_operation;
    case RENDERER_FRONTEND_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case RENDERER_FRONTEND_NO_MEMORY:
        return s_result_str_no_memory;
    case RENDERER_FRONTEND_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case RENDERER_FRONTEND_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

renderer_frontend_result_t renderer_frontend_result_convert_subsystem_allocator(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return RENDERER_FRONTEND_SUCCESS;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return RENDERER_FRONTEND_BAD_OPERATION;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return RENDERER_FRONTEND_DATA_CORRUPTED;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return RENDERER_FRONTEND_NO_MEMORY;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return RENDERER_FRONTEND_INVALID_ARGUMENT;
    case SUBSYSTEM_ALLOCATOR_OVERFLOW:
        return RENDERER_FRONTEND_OVERFLOW;
    case SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED:
        return RENDERER_FRONTEND_LIMIT_EXCEEDED;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    default:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    }
}

renderer_frontend_result_t renderer_frontend_result_convert_renderer_backend(renderer_backend_result_t result_) {
    switch(result_) {
    case RENDERER_BACKEND_SUCCESS:
        return RENDERER_FRONTEND_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return RENDERER_FRONTEND_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return RENDERER_FRONTEND_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return RENDERER_FRONTEND_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return RENDERER_FRONTEND_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
            return RENDERER_FRONTEND_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return RENDERER_FRONTEND_OVERFLOW;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    default:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    }
}

renderer_frontend_result_t render_resource_result_convert_resource_registry(buffer_manager_result_t result_) {
    switch(result_) {
    case RESOURCE_REGISTRY_SUCCESS:
        return RENDERER_FRONTEND_SUCCESS;
    case RESOURCE_REGISTRY_NO_MEMORY:
        return RENDERER_FRONTEND_NO_MEMORY;
    case RESOURCE_REGISTRY_RUNTIME_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case RESOURCE_REGISTRY_INVALID_ARGUMENT:
        return RENDERER_FRONTEND_INVALID_ARGUMENT;
    case RESOURCE_REGISTRY_DATA_CORRUPTED:
        return RENDERER_FRONTEND_DATA_CORRUPTED;
    case RESOURCE_REGISTRY_BAD_OPERATION:
        return RENDERER_FRONTEND_BAD_OPERATION;
    case RESOURCE_REGISTRY_OVERFLOW:
        return RENDERER_FRONTEND_OVERFLOW;
    case RESOURCE_REGISTRY_LIMIT_EXCEEDED:
        return RENDERER_FRONTEND_LIMIT_EXCEEDED;
    case RESOURCE_REGISTRY_UNDEFINED_ERROR:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    default:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    }
}
