#include "engine/systems/renderer/renderer_resources/shaders/core/shader_err_utils.h"

#include "engine/core/memory/linear_allocator.h"
#include "engine/core/memory/choco_memory.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

#include "engine/systems/renderer/renderer_resources/shaders/core/shader_resource_types.h"

#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_types.h"

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_shader_compile_error = "SHADER_COMPILE_ERROR";
static const char* const s_rslt_str_shader_link_error = "SHADER_LINK_ERROR";
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

const char* shader_rslt_to_str(shader_result_t rslt_) {
    switch(rslt_) {
    case SHADER_SUCCESS:
        return s_rslt_str_success;
    case SHADER_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case SHADER_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case SHADER_NO_MEMORY:
        return s_rslt_str_no_memory;
    case SHADER_COMPILE_ERROR:
        return s_rslt_str_shader_compile_error;
    case SHADER_LINK_ERROR:
        return s_rslt_str_shader_link_error;
    case SHADER_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case SHADER_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case SHADER_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case SHADER_OVERFLOW:
        return s_rslt_str_overflow;
    case SHADER_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

shader_result_t shader_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return SHADER_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return SHADER_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return SHADER_INVALID_ARGUMENT;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}

shader_result_t shader_rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return SHADER_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return SHADER_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return SHADER_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return SHADER_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return SHADER_NO_MEMORY;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}

shader_result_t shader_rslt_convert_choco_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return SHADER_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return SHADER_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return SHADER_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return SHADER_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return SHADER_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return SHADER_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return SHADER_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW: // 文字列長さオーバーフローはRUNTIME_ERRORに変換
        return SHADER_RUNTIME_ERROR;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return SHADER_LIMIT_EXCEEDED;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}

shader_result_t shader_rslt_convert_fs_utils(fs_utils_result_t rslt_) {
    switch(rslt_) {
    case FS_UTILS_SUCCESS:
        return SHADER_SUCCESS;
    case FS_UTILS_INVALID_ARGUMENT:
        return SHADER_INVALID_ARGUMENT;
    case FS_UTILS_BAD_OPERATION:
        return SHADER_BAD_OPERATION;
    case FS_UTILS_DATA_CORRUPTED:
        return SHADER_DATA_CORRUPTED;
    case FS_UTILS_NO_MEMORY:
        return SHADER_NO_MEMORY;
    case FS_UTILS_LIMIT_EXCEEDED:
        return SHADER_LIMIT_EXCEEDED;
    case FS_UTILS_OVERFLOW: // オーバーフローはRUNTIME_ERRORに変換
        return SHADER_RUNTIME_ERROR;
    case FS_UTILS_FILE_OPEN_ERROR:
        return SHADER_RUNTIME_ERROR;
    case FS_UTILS_EOF:
        return SHADER_RUNTIME_ERROR;
    case FS_UTILS_RUNTIME_ERROR:
        return SHADER_RUNTIME_ERROR;
    case FS_UTILS_UNDEFINED_ERROR:
        return SHADER_UNDEFINED_ERROR;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}

shader_result_t shader_rslt_convert_renderer_backend(renderer_backend_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_BACKEND_SUCCESS:
        return SHADER_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return SHADER_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return SHADER_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return SHADER_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return SHADER_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return SHADER_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return SHADER_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return SHADER_OVERFLOW;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return SHADER_UNDEFINED_ERROR;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return SHADER_COMPILE_ERROR;
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return SHADER_LINK_ERROR;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}

shader_result_t shader_rslt_convert_buffer_manager(buffer_manager_result_t rslt_) {
    switch(rslt_) {
    case BUFFER_MANAGER_SUCCESS:
        return SHADER_SUCCESS;
    case BUFFER_MANAGER_INVALID_ARGUMENT:
        return SHADER_INVALID_ARGUMENT;
    case BUFFER_MANAGER_RUNTIME_ERROR:
        return SHADER_RUNTIME_ERROR;
    case BUFFER_MANAGER_LIMIT_EXCEEDED:
        return SHADER_LIMIT_EXCEEDED;
    case BUFFER_MANAGER_NO_MEMORY:
        return SHADER_NO_MEMORY;
    case BUFFER_MANAGER_DATA_CORRUPTED:
        return SHADER_DATA_CORRUPTED;
    case BUFFER_MANAGER_BAD_OPERATION:
        return SHADER_BAD_OPERATION;
    case BUFFER_MANAGER_OVERFLOW:
        return SHADER_OVERFLOW;
    case BUFFER_MANAGER_UNDEFINED_ERROR:
        return SHADER_UNDEFINED_ERROR;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}
