// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/geometries/core/geometry_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"

const char* geometry_result_to_str(geometry_result_t result_) {
    switch(result_) {
    case GEOMETRY_SUCCESS:
        return STRINGIFY(GEOMETRY_SUCCESS);
    case GEOMETRY_NO_MEMORY:
        return STRINGIFY(GEOMETRY_NO_MEMORY);
    case GEOMETRY_RUNTIME_ERROR:
        return STRINGIFY(GEOMETRY_RUNTIME_ERROR);
    case GEOMETRY_INVALID_ARGUMENT:
        return STRINGIFY(GEOMETRY_INVALID_ARGUMENT);
    case GEOMETRY_DATA_CORRUPTED:
        return STRINGIFY(GEOMETRY_DATA_CORRUPTED);
    case GEOMETRY_BAD_OPERATION:
        return STRINGIFY(GEOMETRY_BAD_OPERATION);
    case GEOMETRY_OVERFLOW:
        return STRINGIFY(GEOMETRY_OVERFLOW);
    case GEOMETRY_LIMIT_EXCEEDED:
        return STRINGIFY(GEOMETRY_LIMIT_EXCEEDED);
    case GEOMETRY_UNDEFINED_ERROR:
        return STRINGIFY(GEOMETRY_UNDEFINED_ERROR);
    default:
        return STRINGIFY(GEOMETRY_UNDEFINED_ERROR);
    }
}

geometry_result_t geometry_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return GEOMETRY_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return GEOMETRY_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return GEOMETRY_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return GEOMETRY_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return GEOMETRY_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return GEOMETRY_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return GEOMETRY_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return GEOMETRY_UNDEFINED_ERROR;
    default:
        return GEOMETRY_UNDEFINED_ERROR;
    }
}

geometry_result_t geometry_result_convert_aabb_3d(aabb_3d_result_t result_) {
    switch(result_) {
    case AABB_3D_SUCCESS:
        return GEOMETRY_SUCCESS;
    case AABB_3D_INVALID_ARGUMENT:
        return GEOMETRY_INVALID_ARGUMENT;
    case AABB_3D_UNDEFINED_ERROR:
        return GEOMETRY_UNDEFINED_ERROR;
    default:
        return GEOMETRY_UNDEFINED_ERROR;
    }
}
