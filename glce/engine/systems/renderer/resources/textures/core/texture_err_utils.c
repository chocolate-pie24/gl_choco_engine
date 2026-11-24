// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/textures/core/texture_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/systems/renderer/resources/textures/core/texture_types.h"

const char* texture_result_to_str(texture_result_t result_) {
    switch(result_) {
    case TEXTURE_SUCCESS:
        return STRINGIFY(TEXTURE_SUCCESS);
    case TEXTURE_INVALID_ARGUMENT:
        return STRINGIFY(TEXTURE_INVALID_ARGUMENT);
    case TEXTURE_RUNTIME_ERROR:
        return STRINGIFY(TEXTURE_RUNTIME_ERROR);
    case TEXTURE_NO_MEMORY:
        return STRINGIFY(TEXTURE_NO_MEMORY);
    case TEXTURE_LIMIT_EXCEEDED:
        return STRINGIFY(TEXTURE_LIMIT_EXCEEDED);
    case TEXTURE_BAD_OPERATION:
        return STRINGIFY(TEXTURE_BAD_OPERATION);
    case TEXTURE_DATA_CORRUPTED:
        return STRINGIFY(TEXTURE_DATA_CORRUPTED);
    case TEXTURE_OVERFLOW:
        return STRINGIFY(TEXTURE_OVERFLOW);
    case TEXTURE_UNDEFINED_ERROR:
        return STRINGIFY(TEXTURE_UNDEFINED_ERROR);
    default:
        return STRINGIFY(TEXTURE_UNDEFINED_ERROR);
    }
}

texture_result_t texture_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return TEXTURE_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return TEXTURE_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return TEXTURE_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return TEXTURE_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return TEXTURE_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return TEXTURE_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return TEXTURE_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return TEXTURE_UNDEFINED_ERROR;
    default:
        return TEXTURE_UNDEFINED_ERROR;
    }
}

texture_result_t texture_result_convert_renderer_backend(renderer_backend_result_t result_) {
    switch(result_) {
    case RENDERER_BACKEND_SUCCESS:
        return TEXTURE_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return TEXTURE_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return TEXTURE_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return TEXTURE_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return TEXTURE_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return TEXTURE_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return TEXTURE_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return TEXTURE_OVERFLOW;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return TEXTURE_UNDEFINED_ERROR;
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return TEXTURE_UNDEFINED_ERROR;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return TEXTURE_UNDEFINED_ERROR;
    default:
        return TEXTURE_UNDEFINED_ERROR;
    }
}
