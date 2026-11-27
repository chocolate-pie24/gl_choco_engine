// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/shaders/core/shader_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/cpu_allocators/general_allocator/general_allocator.h"
#include "engine/memory/cpu_allocators/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_stream.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

const char* shader_result_to_str(shader_result_t result_) {
    switch(result_) {
    case SHADER_SUCCESS:
        return STRINGIFY(SHADER_SUCCESS);
    case SHADER_INVALID_ARGUMENT:
        return STRINGIFY(SHADER_INVALID_ARGUMENT);
    case SHADER_RUNTIME_ERROR:
        return STRINGIFY(SHADER_RUNTIME_ERROR);
    case SHADER_NO_MEMORY:
        return STRINGIFY(SHADER_NO_MEMORY);
    case SHADER_COMPILE_ERROR:
        return STRINGIFY(SHADER_COMPILE_ERROR);
    case SHADER_LINK_ERROR:
        return STRINGIFY(SHADER_LINK_ERROR);
    case SHADER_LIMIT_EXCEEDED:
        return STRINGIFY(SHADER_LIMIT_EXCEEDED);
    case SHADER_BAD_OPERATION:
        return STRINGIFY(SHADER_BAD_OPERATION);
    case SHADER_DATA_CORRUPTED:
        return STRINGIFY(SHADER_DATA_CORRUPTED);
    case SHADER_OVERFLOW:
        return STRINGIFY(SHADER_OVERFLOW);
    case SHADER_UNDEFINED_ERROR:
        return STRINGIFY(SHADER_UNDEFINED_ERROR);
    default:
        return STRINGIFY(SHADER_UNDEFINED_ERROR);
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
    case SUBSYSTEM_ALLOCATOR_OVERFLOW:
        return SHADER_OVERFLOW;
    case SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED:
        return SHADER_LIMIT_EXCEEDED;
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
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return SHADER_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return SHADER_UNDEFINED_ERROR;
    default:
        return SHADER_UNDEFINED_ERROR;
    }
}
