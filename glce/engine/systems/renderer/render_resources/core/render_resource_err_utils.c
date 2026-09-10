// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/render_resources/core/render_resource_err_utils.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/io_utils/fs_path.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

const char* render_resource_rslt_to_str(render_resource_result_t rslt_) {
    switch(rslt_) {
    case RENDER_RESOURCE_SUCCESS:
        return s_rslt_str_success;
    case RENDER_RESOURCE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RENDER_RESOURCE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RENDER_RESOURCE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RENDER_RESOURCE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RENDER_RESOURCE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RENDER_RESOURCE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RENDER_RESOURCE_OVERFLOW:
        return s_rslt_str_overflow;
    case RENDER_RESOURCE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

render_resource_result_t render_resource_rslt_convert_linear_allocator(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return RENDER_RESOURCE_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return RENDER_RESOURCE_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return RENDER_RESOURCE_INVALID_ARGUMENT;
    default:
        return RENDER_RESOURCE_UNDEFINED_ERROR;
    }
}

render_resource_result_t render_resource_rslt_convert_fs_path(fs_path_result_t rslt_) {
    switch(rslt_) {
    case FS_PATH_SUCCESS:
        return RENDER_RESOURCE_SUCCESS;
    case FS_PATH_INVALID_ARGUMENT:
        return RENDER_RESOURCE_INVALID_ARGUMENT;
    case FS_PATH_BAD_OPERATION:
        return RENDER_RESOURCE_BAD_OPERATION;
    case FS_PATH_DATA_CORRUPTED:
        return RENDER_RESOURCE_DATA_CORRUPTED;
    case FS_PATH_NO_MEMORY:
        return RENDER_RESOURCE_NO_MEMORY;
    case FS_PATH_LIMIT_EXCEEDED:
        return RENDER_RESOURCE_LIMIT_EXCEEDED;
    case FS_PATH_OVERFLOW:
        return RENDER_RESOURCE_OVERFLOW;
    case FS_PATH_RUNTIME_ERROR:
        return RENDER_RESOURCE_RUNTIME_ERROR;
    case FS_PATH_UNDEFINED_ERROR:
        return RENDER_RESOURCE_UNDEFINED_ERROR;
    default:
        return RENDER_RESOURCE_UNDEFINED_ERROR;
    }
}

render_resource_result_t render_resource_rslt_convert_shader(shader_result_t rslt_) {
    switch(rslt_) {
    case SHADER_SUCCESS:
        return RENDER_RESOURCE_SUCCESS;
    case SHADER_INVALID_ARGUMENT:
        return RENDER_RESOURCE_INVALID_ARGUMENT;
    case SHADER_RUNTIME_ERROR:
        return RENDER_RESOURCE_RUNTIME_ERROR;
    case SHADER_NO_MEMORY:
        return RENDER_RESOURCE_NO_MEMORY;
    case SHADER_COMPILE_ERROR:
        return RENDER_RESOURCE_RUNTIME_ERROR;
    case SHADER_LINK_ERROR:
        return RENDER_RESOURCE_RUNTIME_ERROR;
    case SHADER_LIMIT_EXCEEDED:
        return RENDER_RESOURCE_LIMIT_EXCEEDED;
    case SHADER_BAD_OPERATION:
        return RENDER_RESOURCE_BAD_OPERATION;
    case SHADER_DATA_CORRUPTED:
        return RENDER_RESOURCE_DATA_CORRUPTED;
    case SHADER_OVERFLOW:
        return RENDER_RESOURCE_OVERFLOW;
    case SHADER_UNDEFINED_ERROR:
        return RENDER_RESOURCE_UNDEFINED_ERROR;
    default:
        return RENDER_RESOURCE_UNDEFINED_ERROR;
    }
}

render_resource_result_t render_resource_rslt_convert_resource_registry(resource_registry_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_REGISTRY_SUCCESS:
        return RENDER_RESOURCE_SUCCESS;
    case RESOURCE_REGISTRY_NO_MEMORY:
        return RENDER_RESOURCE_NO_MEMORY;
    case RESOURCE_REGISTRY_RUNTIME_ERROR:
        return RENDER_RESOURCE_RUNTIME_ERROR;
    case RESOURCE_REGISTRY_INVALID_ARGUMENT:
        return RENDER_RESOURCE_INVALID_ARGUMENT;
    case RESOURCE_REGISTRY_DATA_CORRUPTED:
        return RENDER_RESOURCE_DATA_CORRUPTED;
    case RESOURCE_REGISTRY_BAD_OPERATION:
        return RENDER_RESOURCE_BAD_OPERATION;
    case RESOURCE_REGISTRY_OVERFLOW:
        return RENDER_RESOURCE_OVERFLOW;
    case RESOURCE_REGISTRY_LIMIT_EXCEEDED:
        return RENDER_RESOURCE_LIMIT_EXCEEDED;
    case RESOURCE_REGISTRY_UNDEFINED_ERROR:
        return RENDER_RESOURCE_UNDEFINED_ERROR;
    default:
        return RENDER_RESOURCE_UNDEFINED_ERROR;
    }
}

render_resource_result_t render_resource_rslt_convert_resource_pipeline(resource_pipeline_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_PIPELINE_SUCCESS:
        return RENDER_RESOURCE_SUCCESS;
    case RESOURCE_PIPELINE_NO_MEMORY:
        return RENDER_RESOURCE_NO_MEMORY;
    case RESOURCE_PIPELINE_RUNTIME_ERROR:
        return RENDER_RESOURCE_RUNTIME_ERROR;
    case RESOURCE_PIPELINE_INVALID_ARGUMENT:
        return RENDER_RESOURCE_INVALID_ARGUMENT;
    case RESOURCE_PIPELINE_DATA_CORRUPTED:
        return RENDER_RESOURCE_DATA_CORRUPTED;
    case RESOURCE_PIPELINE_BAD_OPERATION:
        return RENDER_RESOURCE_BAD_OPERATION;
    case RESOURCE_PIPELINE_OVERFLOW:
        return RENDER_RESOURCE_OVERFLOW;
    case RESOURCE_PIPELINE_LIMIT_EXCEEDED:
        return RENDER_RESOURCE_LIMIT_EXCEEDED;
    case RESOURCE_PIPELINE_FILE_OPEN_ERROR:
        return RENDER_RESOURCE_RUNTIME_ERROR;
    case RESOURCE_PIPELINE_FILE_READ_ERROR:
        return RENDER_RESOURCE_RUNTIME_ERROR;
    case RESOURCE_PIPELINE_UNSUPPORTED_FILE:
        return RENDER_RESOURCE_RUNTIME_ERROR;
    case RESOURCE_PIPELINE_UNDEFINED_ERROR:
        return RENDER_RESOURCE_UNDEFINED_ERROR;
    default:
        return RENDER_RESOURCE_UNDEFINED_ERROR;
    }
}
