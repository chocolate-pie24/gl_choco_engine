// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/model_framework/core/model_framework_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"
#include "engine/containers/ring_queue.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

#include "application/model_framework/core/model_framework_types.h"

const char* model_framework_result_to_str(model_framework_result_t result_) {
    switch(result_) {
    case MODEL_FRAMEWORK_SUCCESS:
        return STRINGIFY(MODEL_FRAMEWORK_SUCCESS);
    case MODEL_FRAMEWORK_INVALID_ARGUMENT:
        return STRINGIFY(MODEL_FRAMEWORK_INVALID_ARGUMENT);
    case MODEL_FRAMEWORK_RUNTIME_ERROR:
        return STRINGIFY(MODEL_FRAMEWORK_RUNTIME_ERROR);
    case MODEL_FRAMEWORK_NO_MEMORY:
        return STRINGIFY(MODEL_FRAMEWORK_NO_MEMORY);
    case MODEL_FRAMEWORK_LIMIT_EXCEEDED:
        return STRINGIFY(MODEL_FRAMEWORK_LIMIT_EXCEEDED);
    case MODEL_FRAMEWORK_BAD_OPERATION:
        return STRINGIFY(MODEL_FRAMEWORK_BAD_OPERATION);
    case MODEL_FRAMEWORK_DATA_CORRUPTED:
        return STRINGIFY(MODEL_FRAMEWORK_DATA_CORRUPTED);
    case MODEL_FRAMEWORK_OVERFLOW:
        return STRINGIFY(MODEL_FRAMEWORK_OVERFLOW);
    case MODEL_FRAMEWORK_UNDEFINED_ERROR:
        return STRINGIFY(MODEL_FRAMEWORK_UNDEFINED_ERROR);
    default:
        return STRINGIFY(MODEL_FRAMEWORK_UNDEFINED_ERROR);
    }
}

model_framework_result_t model_framework_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return MODEL_FRAMEWORK_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return MODEL_FRAMEWORK_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return MODEL_FRAMEWORK_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return MODEL_FRAMEWORK_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return MODEL_FRAMEWORK_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return MODEL_FRAMEWORK_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return MODEL_FRAMEWORK_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return MODEL_FRAMEWORK_LIMIT_EXCEEDED;
    default:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    }
}

model_framework_result_t model_framework_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return MODEL_FRAMEWORK_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return MODEL_FRAMEWORK_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return MODEL_FRAMEWORK_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return MODEL_FRAMEWORK_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return MODEL_FRAMEWORK_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return MODEL_FRAMEWORK_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return MODEL_FRAMEWORK_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    default:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    }
}

model_framework_result_t model_framework_result_convert_ring_queue(ring_queue_result_t result_) {
    switch(result_) {
    case RING_QUEUE_SUCCESS:
        return MODEL_FRAMEWORK_SUCCESS;
    case RING_QUEUE_INVALID_ARGUMENT:
        return MODEL_FRAMEWORK_INVALID_ARGUMENT;
    case RING_QUEUE_NO_MEMORY:
        return MODEL_FRAMEWORK_NO_MEMORY;
    case RING_QUEUE_RUNTIME_ERROR:
        return MODEL_FRAMEWORK_RUNTIME_ERROR;
    case RING_QUEUE_LIMIT_EXCEEDED:
        return MODEL_FRAMEWORK_LIMIT_EXCEEDED;
    case RING_QUEUE_BAD_OPERATION:
        return MODEL_FRAMEWORK_BAD_OPERATION;
    case RING_QUEUE_DATA_CORRUPTED:
        return MODEL_FRAMEWORK_DATA_CORRUPTED;
    case RING_QUEUE_OVERFLOW:
        return MODEL_FRAMEWORK_OVERFLOW;
    case RING_QUEUE_EMPTY:
        return MODEL_FRAMEWORK_RUNTIME_ERROR;
    case RING_QUEUE_UNDEFINED_ERROR:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    default:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    }
}

model_framework_result_t model_framework_result_convert_renderer_frontend(renderer_frontend_result_t result_) {
    switch(result_) {
    case RENDERER_FRONTEND_SUCCESS:
        return MODEL_FRAMEWORK_SUCCESS;
    case RENDERER_FRONTEND_INVALID_ARGUMENT:
        return MODEL_FRAMEWORK_INVALID_ARGUMENT;
    case RENDERER_FRONTEND_RUNTIME_ERROR:
        return MODEL_FRAMEWORK_RUNTIME_ERROR;
    case RENDERER_FRONTEND_OVERFLOW:
        return MODEL_FRAMEWORK_OVERFLOW;
    case RENDERER_FRONTEND_BAD_OPERATION:
        return MODEL_FRAMEWORK_BAD_OPERATION;
    case RENDERER_FRONTEND_DATA_CORRUPTED:
        return MODEL_FRAMEWORK_DATA_CORRUPTED;
    case RENDERER_FRONTEND_NO_MEMORY:
        return MODEL_FRAMEWORK_NO_MEMORY;
    case RENDERER_FRONTEND_LIMIT_EXCEEDED:
        return MODEL_FRAMEWORK_LIMIT_EXCEEDED;
    case RENDERER_FRONTEND_UNDEFINED_ERROR:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    default:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    }
}

model_framework_result_t model_framework_result_convert_mesh(mesh_result_t result_) {
    switch(result_) {
    case MESH_SUCCESS:
        return MODEL_FRAMEWORK_SUCCESS;
    case MESH_NO_MEMORY:
        return MODEL_FRAMEWORK_NO_MEMORY;
    case MESH_RUNTIME_ERROR:
        return MODEL_FRAMEWORK_RUNTIME_ERROR;
    case MESH_INVALID_ARGUMENT:
        return MODEL_FRAMEWORK_INVALID_ARGUMENT;
    case MESH_DATA_CORRUPTED:
        return MODEL_FRAMEWORK_DATA_CORRUPTED;
    case MESH_BAD_OPERATION:
        return MODEL_FRAMEWORK_BAD_OPERATION;
    case MESH_OVERFLOW:
        return MODEL_FRAMEWORK_OVERFLOW;
    case MESH_LIMIT_EXCEEDED:
        return MODEL_FRAMEWORK_LIMIT_EXCEEDED;
    case MESH_UNDEFINED_ERROR:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    default:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    }
}

model_framework_result_t model_framework_result_convert_geometry(geometry_result_t result_) {
    switch(result_) {
    case GEOMETRY_SUCCESS:
        return MODEL_FRAMEWORK_SUCCESS;
    case GEOMETRY_NO_MEMORY:
        return MODEL_FRAMEWORK_NO_MEMORY;
    case GEOMETRY_RUNTIME_ERROR:
        return MODEL_FRAMEWORK_RUNTIME_ERROR;
    case GEOMETRY_INVALID_ARGUMENT:
        return MODEL_FRAMEWORK_INVALID_ARGUMENT;
    case GEOMETRY_DATA_CORRUPTED:
        return MODEL_FRAMEWORK_DATA_CORRUPTED;
    case GEOMETRY_BAD_OPERATION:
        return MODEL_FRAMEWORK_BAD_OPERATION;
    case GEOMETRY_OVERFLOW:
        return MODEL_FRAMEWORK_OVERFLOW;
    case GEOMETRY_LIMIT_EXCEEDED:
        return MODEL_FRAMEWORK_LIMIT_EXCEEDED;
    case GEOMETRY_UNDEFINED_ERROR:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    default:
        return MODEL_FRAMEWORK_UNDEFINED_ERROR;
    }
}
