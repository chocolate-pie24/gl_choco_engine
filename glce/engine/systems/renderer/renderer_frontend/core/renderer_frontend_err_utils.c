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

renderer_frontend_result_t renderer_frontend_result_convert_resource_registry(resource_registry_result_t result_) {
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

renderer_frontend_result_t renderer_frontend_result_convert_resource_pipeline(resource_pipeline_result_t result_) {
    switch(result_) {
    case RESOURCE_PIPELINE_SUCCESS:
        return RENDERER_FRONTEND_SUCCESS;
    case RESOURCE_PIPELINE_NO_MEMORY:
        return RENDERER_FRONTEND_NO_MEMORY;
    case RESOURCE_PIPELINE_RUNTIME_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case RESOURCE_PIPELINE_INVALID_ARGUMENT:
        return RENDERER_FRONTEND_INVALID_ARGUMENT;
    case RESOURCE_PIPELINE_DATA_CORRUPTED:
        return RENDERER_FRONTEND_DATA_CORRUPTED;
    case RESOURCE_PIPELINE_BAD_OPERATION:
        return RENDERER_FRONTEND_BAD_OPERATION;
    case RESOURCE_PIPELINE_OVERFLOW:
        return RENDERER_FRONTEND_OVERFLOW;
    case RESOURCE_PIPELINE_LIMIT_EXCEEDED:
        return RENDERER_FRONTEND_LIMIT_EXCEEDED;
    case RESOURCE_PIPELINE_FILE_OPEN_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case RESOURCE_PIPELINE_FILE_READ_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case RESOURCE_PIPELINE_UNSUPPORTED_FILE:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case RESOURCE_PIPELINE_UNDEFINED_ERROR:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    default:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    }
}

renderer_frontend_result_t renderer_frontend_result_convert_buffer_manager(buffer_manager_result_t result_) {
    switch(result_) {
    case BUFFER_MANAGER_SUCCESS:
        return RENDERER_FRONTEND_SUCCESS;
    case BUFFER_MANAGER_INVALID_ARGUMENT:
        return RENDERER_FRONTEND_INVALID_ARGUMENT;
    case BUFFER_MANAGER_RUNTIME_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case BUFFER_MANAGER_LIMIT_EXCEEDED:
        return RENDERER_FRONTEND_LIMIT_EXCEEDED;
    case BUFFER_MANAGER_NO_MEMORY:
        return RENDERER_FRONTEND_NO_MEMORY;
    case BUFFER_MANAGER_DATA_CORRUPTED:
        return RENDERER_FRONTEND_DATA_CORRUPTED;
    case BUFFER_MANAGER_BAD_OPERATION:
        return RENDERER_FRONTEND_BAD_OPERATION;
    case BUFFER_MANAGER_OVERFLOW:
        return RENDERER_FRONTEND_OVERFLOW;
    case BUFFER_MANAGER_UNDEFINED_ERROR:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    default:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    }
}

renderer_frontend_result_t renderer_frontend_result_convert_fs_path(fs_path_result_t result_) {
    switch(result_) {
    case FS_PATH_SUCCESS:
        return RENDERER_FRONTEND_SUCCESS;
    case FS_PATH_INVALID_ARGUMENT:
        return RENDERER_FRONTEND_INVALID_ARGUMENT;
    case FS_PATH_BAD_OPERATION:
        return RENDERER_FRONTEND_BAD_OPERATION;
    case FS_PATH_DATA_CORRUPTED:
        return RENDERER_FRONTEND_DATA_CORRUPTED;
    case FS_PATH_NO_MEMORY:
        return RENDERER_FRONTEND_NO_MEMORY;
    case FS_PATH_LIMIT_EXCEEDED:
        return RENDERER_FRONTEND_LIMIT_EXCEEDED;
    case FS_PATH_OVERFLOW:
        return RENDERER_FRONTEND_OVERFLOW;
    case FS_PATH_RUNTIME_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case FS_PATH_UNDEFINED_ERROR:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    default:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    }
}

renderer_frontend_result_t renderer_frontend_result_convert_shader(shader_result_t result_) {
    switch(result_) {
    case SHADER_SUCCESS:
        return RENDERER_FRONTEND_SUCCESS;
    case SHADER_INVALID_ARGUMENT:
        return RENDERER_FRONTEND_INVALID_ARGUMENT;
    case SHADER_RUNTIME_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case SHADER_NO_MEMORY:
        return RENDERER_FRONTEND_NO_MEMORY;
    case SHADER_COMPILE_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case SHADER_LINK_ERROR:
        return RENDERER_FRONTEND_RUNTIME_ERROR;
    case SHADER_LIMIT_EXCEEDED:
        return RENDERER_FRONTEND_LIMIT_EXCEEDED;
    case SHADER_BAD_OPERATION:
        return RENDERER_FRONTEND_BAD_OPERATION;
    case SHADER_DATA_CORRUPTED:
        return RENDERER_FRONTEND_DATA_CORRUPTED;
    case SHADER_OVERFLOW:
        return RENDERER_FRONTEND_OVERFLOW;
    case SHADER_UNDEFINED_ERROR:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    default:
        return RENDERER_FRONTEND_UNDEFINED_ERROR;
    }
}
