#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_shader_compile_error = "SHADER_COMPILE_ERROR";
static const char* const s_rslt_str_shader_link_error = "SHADER_LINK_ERROR";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

const char* renderer_backend_rslt_to_str(renderer_backend_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_BACKEND_SUCCESS:
        return s_rslt_str_success;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RENDERER_BACKEND_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RENDERER_BACKEND_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RENDERER_BACKEND_OVERFLOW:
        return s_rslt_str_overflow;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return s_rslt_str_shader_compile_error;
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return s_rslt_str_shader_link_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

renderer_backend_result_t renderer_backend_rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return RENDERER_BACKEND_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return RENDERER_BACKEND_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return RENDERER_BACKEND_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return RENDERER_BACKEND_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return RENDERER_BACKEND_NO_MEMORY;
    default:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    }
}

renderer_backend_result_t renderer_backend_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return RENDERER_BACKEND_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return RENDERER_BACKEND_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return RENDERER_BACKEND_INVALID_ARGUMENT;
    default:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    }
}
