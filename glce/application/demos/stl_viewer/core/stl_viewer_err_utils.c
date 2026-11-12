// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/demos/stl_viewer/core/stl_viewer_types.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

#include "engine/systems/renderer/render_packet/core/render_packet_types.h"
#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

#include "application/demos/stl_viewer/core/stl_viewer_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* stl_viewer_result_to_str(stl_viewer_result_t result_) {
    switch(result_) {
    case STL_VIEWER_SUCCESS:
        return s_result_str_success;
    case STL_VIEWER_NO_MEMORY:
        return s_result_str_no_memory;
    case STL_VIEWER_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case STL_VIEWER_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case STL_VIEWER_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case STL_VIEWER_BAD_OPERATION:
        return s_result_str_bad_operation;
    case STL_VIEWER_OVERFLOW:
        return s_result_str_overflow;
    case STL_VIEWER_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case STL_VIEWER_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

stl_viewer_result_t stl_viewer_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return STL_VIEWER_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return STL_VIEWER_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return STL_VIEWER_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return STL_VIEWER_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return STL_VIEWER_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return STL_VIEWER_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return STL_VIEWER_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return STL_VIEWER_UNDEFINED_ERROR;
    default:
        return STL_VIEWER_UNDEFINED_ERROR;
    }
}

stl_viewer_result_t stl_viewer_result_convert_mesh(mesh_result_t result_) {
    switch(result_) {
    case MESH_SUCCESS:
        return STL_VIEWER_SUCCESS;
    case MESH_NO_MEMORY:
        return STL_VIEWER_NO_MEMORY;
    case MESH_RUNTIME_ERROR:
        return STL_VIEWER_RUNTIME_ERROR;
    case MESH_INVALID_ARGUMENT:
        return STL_VIEWER_INVALID_ARGUMENT;
    case MESH_DATA_CORRUPTED:
        return STL_VIEWER_DATA_CORRUPTED;
    case MESH_BAD_OPERATION:
        return STL_VIEWER_BAD_OPERATION;
    case MESH_OVERFLOW:
        return STL_VIEWER_OVERFLOW;
    case MESH_LIMIT_EXCEEDED:
        return STL_VIEWER_LIMIT_EXCEEDED;
    case MESH_UNDEFINED_ERROR:
        return STL_VIEWER_UNDEFINED_ERROR;
    default:
        return STL_VIEWER_UNDEFINED_ERROR;
    }
}

stl_viewer_result_t stl_viewer_result_convert_geometry(geometry_result_t result_) {
    switch(result_) {
    case GEOMETRY_SUCCESS:
        return STL_VIEWER_SUCCESS;
    case GEOMETRY_NO_MEMORY:
        return STL_VIEWER_NO_MEMORY;
    case GEOMETRY_RUNTIME_ERROR:
        return STL_VIEWER_RUNTIME_ERROR;
    case GEOMETRY_INVALID_ARGUMENT:
        return STL_VIEWER_INVALID_ARGUMENT;
    case GEOMETRY_DATA_CORRUPTED:
        return STL_VIEWER_DATA_CORRUPTED;
    case GEOMETRY_BAD_OPERATION:
        return STL_VIEWER_BAD_OPERATION;
    case GEOMETRY_OVERFLOW:
        return STL_VIEWER_OVERFLOW;
    case GEOMETRY_LIMIT_EXCEEDED:
        return STL_VIEWER_LIMIT_EXCEEDED;
    case GEOMETRY_UNDEFINED_ERROR:
        return STL_VIEWER_UNDEFINED_ERROR;
    default:
        return STL_VIEWER_UNDEFINED_ERROR;
    }
}

stl_viewer_result_t stl_viewer_result_convert_renderer_frontend(renderer_frontend_result_t result_) {
    switch(result_) {
    case RENDERER_FRONTEND_SUCCESS:
        return STL_VIEWER_SUCCESS;
    case RENDERER_FRONTEND_INVALID_ARGUMENT:
        return STL_VIEWER_INVALID_ARGUMENT;
    case RENDERER_FRONTEND_RUNTIME_ERROR:
        return STL_VIEWER_RUNTIME_ERROR;
    case RENDERER_FRONTEND_OVERFLOW:
        return STL_VIEWER_OVERFLOW;
    case RENDERER_FRONTEND_BAD_OPERATION:
        return STL_VIEWER_BAD_OPERATION;
    case RENDERER_FRONTEND_DATA_CORRUPTED:
        return STL_VIEWER_DATA_CORRUPTED;
    case RENDERER_FRONTEND_NO_MEMORY:
        return STL_VIEWER_NO_MEMORY;
    case RENDERER_FRONTEND_LIMIT_EXCEEDED:
        return STL_VIEWER_LIMIT_EXCEEDED;
    case RENDERER_FRONTEND_UNDEFINED_ERROR:
        return STL_VIEWER_UNDEFINED_ERROR;
    default:
        return STL_VIEWER_UNDEFINED_ERROR;
    }
}

stl_viewer_result_t stl_viewer_result_convert_render_packet(render_packet_result_t result_) {
    switch(result_) {
    case RENDER_PACKET_SUCCESS:
        return STL_VIEWER_SUCCESS;
    case RENDER_PACKET_INVALID_ARGUMENT:
        return STL_VIEWER_INVALID_ARGUMENT;
    case RENDER_PACKET_RUNTIME_ERROR:
        return STL_VIEWER_RUNTIME_ERROR;
    case RENDER_PACKET_OVERFLOW:
        return STL_VIEWER_OVERFLOW;
    case RENDER_PACKET_BAD_OPERATION:
        return STL_VIEWER_BAD_OPERATION;
    case RENDER_PACKET_DATA_CORRUPTED:
        return STL_VIEWER_DATA_CORRUPTED;
    case RENDER_PACKET_NO_MEMORY:
        return STL_VIEWER_NO_MEMORY;
    case RENDER_PACKET_LIMIT_EXCEEDED:
        return STL_VIEWER_LIMIT_EXCEEDED;
    case RENDER_PACKET_UNDEFINED_ERROR:
        return STL_VIEWER_UNDEFINED_ERROR;
    default:
        return STL_VIEWER_UNDEFINED_ERROR;
    }
}
