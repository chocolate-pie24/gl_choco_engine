#include "engine/systems/renderer/resources/texture/texture_gpu_resource_err_utils.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/resources/texture/texture_gpu_resource_types.h"

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

const char* texture_gpu_resource_rslt_to_str(texture_gpu_resource_result_t rslt_) {
    switch(rslt_) {
    case TEXTURE_GPU_RESOURCE_SUCCESS:
        return s_rslt_str_success;
    case TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case TEXTURE_GPU_RESOURCE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case TEXTURE_GPU_RESOURCE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case TEXTURE_GPU_RESOURCE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case TEXTURE_GPU_RESOURCE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case TEXTURE_GPU_RESOURCE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case TEXTURE_GPU_RESOURCE_OVERFLOW:
        return s_rslt_str_overflow;
    case TEXTURE_GPU_RESOURCE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

texture_gpu_resource_result_t texture_gpu_resource_rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return TEXTURE_GPU_RESOURCE_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return TEXTURE_GPU_RESOURCE_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return TEXTURE_GPU_RESOURCE_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return TEXTURE_GPU_RESOURCE_NO_MEMORY;
    default:
        return TEXTURE_GPU_RESOURCE_UNDEFINED_ERROR;
    }
}

texture_gpu_resource_result_t texture_gpu_resource_rslt_convert_renderer_backend(renderer_backend_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_BACKEND_SUCCESS:
        return TEXTURE_GPU_RESOURCE_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return TEXTURE_GPU_RESOURCE_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return TEXTURE_GPU_RESOURCE_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return TEXTURE_GPU_RESOURCE_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return TEXTURE_GPU_RESOURCE_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return TEXTURE_GPU_RESOURCE_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return TEXTURE_GPU_RESOURCE_OVERFLOW;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return TEXTURE_GPU_RESOURCE_UNDEFINED_ERROR;
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return TEXTURE_GPU_RESOURCE_UNDEFINED_ERROR;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return TEXTURE_GPU_RESOURCE_UNDEFINED_ERROR;
    default:
        return TEXTURE_GPU_RESOURCE_UNDEFINED_ERROR;
    }
}
