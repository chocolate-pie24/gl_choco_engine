// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/meshes/core/mesh_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

const char* mesh_result_to_str(mesh_result_t result_) {
    switch(result_) {
    case MESH_SUCCESS:
        return STRINGIFY(MESH_SUCCESS);
    case MESH_NO_MEMORY:
        return STRINGIFY(MESH_NO_MEMORY);
    case MESH_RUNTIME_ERROR:
        return STRINGIFY(MESH_RUNTIME_ERROR);
    case MESH_INVALID_ARGUMENT:
        return STRINGIFY(MESH_INVALID_ARGUMENT);
    case MESH_DATA_CORRUPTED:
        return STRINGIFY(MESH_DATA_CORRUPTED);
    case MESH_BAD_OPERATION:
        return STRINGIFY(MESH_BAD_OPERATION);
    case MESH_OVERFLOW:
        return STRINGIFY(MESH_OVERFLOW);
    case MESH_LIMIT_EXCEEDED:
        return STRINGIFY(MESH_LIMIT_EXCEEDED);
    case MESH_UNDEFINED_ERROR:
        return STRINGIFY(MESH_UNDEFINED_ERROR);
    default:
        return STRINGIFY(MESH_UNDEFINED_ERROR);
    }
}

mesh_result_t mesh_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return MESH_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return MESH_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return MESH_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return MESH_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return MESH_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return MESH_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return MESH_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return MESH_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return MESH_LIMIT_EXCEEDED;
    default:
        return MESH_UNDEFINED_ERROR;
    }
}
