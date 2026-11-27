// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/camera/core/camera_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/cpu_allocators/general_allocator/general_allocator.h"

#include "engine/camera/core/camera_types.h"

const char* camera_result_to_str(camera_result_t result_) {
    switch(result_) {
    case CAMERA_SUCCESS:
        return STRINGIFY(CAMERA_SUCCESS);
    case CAMERA_INVALID_ARGUMENT:
        return STRINGIFY(CAMERA_INVALID_ARGUMENT);
    case CAMERA_RUNTIME_ERROR:
        return STRINGIFY(CAMERA_RUNTIME_ERROR);
    case CAMERA_NO_MEMORY:
        return STRINGIFY(CAMERA_NO_MEMORY);
    case CAMERA_LIMIT_EXCEEDED:
        return STRINGIFY(CAMERA_LIMIT_EXCEEDED);
    case CAMERA_BAD_OPERATION:
        return STRINGIFY(CAMERA_BAD_OPERATION);
    case CAMERA_DATA_CORRUPTED:
        return STRINGIFY(CAMERA_DATA_CORRUPTED);
    case CAMERA_UNDEFINED_ERROR:
        return STRINGIFY(CAMERA_UNDEFINED_ERROR);
    default:
        return STRINGIFY(CAMERA_UNDEFINED_ERROR);
    }
}

camera_result_t camera_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return CAMERA_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return CAMERA_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return CAMERA_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return CAMERA_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return CAMERA_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return CAMERA_UNDEFINED_ERROR;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return CAMERA_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return CAMERA_UNDEFINED_ERROR;
    default:
        return CAMERA_UNDEFINED_ERROR;
    }
}
