// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/shaders/core/shader_err_utils.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_stream.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_shader_compile_error = "SHADER_COMPILE_ERROR";
static const char* const s_result_str_shader_link_error = "SHADER_LINK_ERROR";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* shader_result_to_str(shader_result_t result_) {
    switch(result_) {
    case SHADER_SUCCESS:
        return s_result_str_success;
    case SHADER_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case SHADER_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case SHADER_NO_MEMORY:
        return s_result_str_no_memory;
    case SHADER_COMPILE_ERROR:
        return s_result_str_shader_compile_error;
    case SHADER_LINK_ERROR:
        return s_result_str_shader_link_error;
    case SHADER_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case SHADER_BAD_OPERATION:
        return s_result_str_bad_operation;
    case SHADER_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case SHADER_OVERFLOW:
        return s_result_str_overflow;
    case SHADER_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

shader_result_t shader_result_convert_subsystem_allocator(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return SHADER_SUCCESS;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return SHADER_BAD_OPERATION;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return SHADER_DATA_CORRUPTED;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return SHADER_NO_MEMORY;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return SHADER_INVALID_ARGUMENT;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return SHADER_UNDEFINED_ERROR;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}

shader_result_t shader_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
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
    case CHOCO_STRING_OVERFLOW:
        return SHADER_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return SHADER_LIMIT_EXCEEDED;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}

shader_result_t shader_result_convert_fs_stream(fs_stream_result_t result_) {
    switch(result_) {
    case FS_STREAM_SUCCESS:
        return SHADER_SUCCESS;
    case FS_STREAM_INVALID_ARGUMENT:
        return SHADER_INVALID_ARGUMENT;
    case FS_STREAM_BAD_OPERATION:
        return SHADER_BAD_OPERATION;
    case FS_STREAM_DATA_CORRUPTED:
        return SHADER_DATA_CORRUPTED;
    case FS_STREAM_NO_MEMORY:
        return SHADER_NO_MEMORY;
    case FS_STREAM_LIMIT_EXCEEDED:
        return SHADER_LIMIT_EXCEEDED;
    case FS_STREAM_OVERFLOW:
        return SHADER_OVERFLOW;
    case FS_STREAM_FILE_OPEN_ERROR:
        return SHADER_RUNTIME_ERROR;
    case FS_STREAM_EOF:
        return SHADER_RUNTIME_ERROR;
    case FS_STREAM_RUNTIME_ERROR:
        return SHADER_RUNTIME_ERROR;
    case FS_STREAM_UNDEFINED_ERROR:
        return SHADER_UNDEFINED_ERROR;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}

shader_result_t shader_result_convert_renderer_backend(renderer_backend_result_t result_) {
    switch(result_) {
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

shader_result_t shader_result_convert_buffer_manager(buffer_manager_result_t result_) {
    switch(result_) {
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

shader_result_t shader_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return SHADER_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return SHADER_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return SHADER_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return SHADER_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return SHADER_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return SHADER_OVERFLOW;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return SHADER_UNDEFINED_ERROR;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}
