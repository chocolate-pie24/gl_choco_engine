// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/camera/camera_registries/core/camera_registry_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/cpu_allocators/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/camera/core/camera_types.h"

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"

const char* camera_registry_result_to_str(camera_registry_result_t result_) {
    switch(result_) {
    case CAMERA_REGISTRY_SUCCESS:
        return STRINGIFY(CAMERA_REGISTRY_SUCCESS);
    case CAMERA_REGISTRY_NO_MEMORY:
        return STRINGIFY(CAMERA_REGISTRY_NO_MEMORY);
    case CAMERA_REGISTRY_RUNTIME_ERROR:
        return STRINGIFY(CAMERA_REGISTRY_RUNTIME_ERROR);
    case CAMERA_REGISTRY_INVALID_ARGUMENT:
        return STRINGIFY(CAMERA_REGISTRY_INVALID_ARGUMENT);
    case CAMERA_REGISTRY_DATA_CORRUPTED:
        return STRINGIFY(CAMERA_REGISTRY_DATA_CORRUPTED);
    case CAMERA_REGISTRY_BAD_OPERATION:
        return STRINGIFY(CAMERA_REGISTRY_BAD_OPERATION);
    case CAMERA_REGISTRY_OVERFLOW:
        return STRINGIFY(CAMERA_REGISTRY_OVERFLOW);
    case CAMERA_REGISTRY_LIMIT_EXCEEDED:
        return STRINGIFY(CAMERA_REGISTRY_LIMIT_EXCEEDED);
    case CAMERA_REGISTRY_UNDEFINED_ERROR:
        return STRINGIFY(CAMERA_REGISTRY_UNDEFINED_ERROR);
    default:
        return STRINGIFY(CAMERA_REGISTRY_UNDEFINED_ERROR);
    }
}

camera_registry_result_t camera_registry_result_convert_subsystem_allocator(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return CAMERA_REGISTRY_SUCCESS;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return CAMERA_REGISTRY_BAD_OPERATION;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return CAMERA_REGISTRY_DATA_CORRUPTED;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return CAMERA_REGISTRY_NO_MEMORY;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return CAMERA_REGISTRY_INVALID_ARGUMENT;
    case SUBSYSTEM_ALLOCATOR_OVERFLOW:
        return CAMERA_REGISTRY_OVERFLOW;
    case SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED:
        return CAMERA_REGISTRY_LIMIT_EXCEEDED;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return CAMERA_REGISTRY_UNDEFINED_ERROR;
    default:
        return CAMERA_REGISTRY_UNDEFINED_ERROR;
    }
}

camera_registry_result_t camera_registry_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return CAMERA_REGISTRY_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return CAMERA_REGISTRY_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return CAMERA_REGISTRY_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return CAMERA_REGISTRY_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return CAMERA_REGISTRY_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return CAMERA_REGISTRY_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return CAMERA_REGISTRY_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return CAMERA_REGISTRY_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return CAMERA_REGISTRY_LIMIT_EXCEEDED;
    default:
        return CAMERA_REGISTRY_UNDEFINED_ERROR;
    }
}
