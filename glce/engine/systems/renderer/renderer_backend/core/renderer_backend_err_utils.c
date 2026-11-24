// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

const char* renderer_backend_result_to_str(renderer_backend_result_t result_) {
    switch(result_) {
    case RENDERER_BACKEND_SUCCESS:
        return STRINGIFY(RENDERER_BACKEND_SUCCESS);
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return STRINGIFY(RENDERER_BACKEND_INVALID_ARGUMENT);
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return STRINGIFY(RENDERER_BACKEND_RUNTIME_ERROR);
    case RENDERER_BACKEND_NO_MEMORY:
        return STRINGIFY(RENDERER_BACKEND_NO_MEMORY);
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return STRINGIFY(RENDERER_BACKEND_LIMIT_EXCEEDED);
    case RENDERER_BACKEND_BAD_OPERATION:
        return STRINGIFY(RENDERER_BACKEND_BAD_OPERATION);
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return STRINGIFY(RENDERER_BACKEND_DATA_CORRUPTED);
    case RENDERER_BACKEND_OVERFLOW:
        return STRINGIFY(RENDERER_BACKEND_OVERFLOW);
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return STRINGIFY(RENDERER_BACKEND_UNDEFINED_ERROR);
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return STRINGIFY(RENDERER_BACKEND_SHADER_COMPILE_ERROR);
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return STRINGIFY(RENDERER_BACKEND_SHADER_LINK_ERROR);
    default:
        return STRINGIFY(RENDERER_BACKEND_UNDEFINED_ERROR);
    }
}

renderer_backend_result_t renderer_backend_result_convert_subsystem_allocator(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return RENDERER_BACKEND_SUCCESS;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return RENDERER_BACKEND_BAD_OPERATION;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return RENDERER_BACKEND_DATA_CORRUPTED;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return RENDERER_BACKEND_NO_MEMORY;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return RENDERER_BACKEND_INVALID_ARGUMENT;
    case SUBSYSTEM_ALLOCATOR_OVERFLOW:
        return RENDERER_BACKEND_OVERFLOW;
    case SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED:
        return RENDERER_BACKEND_LIMIT_EXCEEDED;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    default:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    }
}

renderer_backend_result_t renderer_backend_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return RENDERER_BACKEND_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return RENDERER_BACKEND_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return RENDERER_BACKEND_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return RENDERER_BACKEND_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return RENDERER_BACKEND_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return RENDERER_BACKEND_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return RENDERER_BACKEND_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    default:
        return RENDERER_BACKEND_UNDEFINED_ERROR;
    }
}
