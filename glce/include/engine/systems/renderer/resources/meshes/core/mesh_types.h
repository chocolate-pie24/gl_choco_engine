// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_MESH_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_MESH_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MESH_SUCCESS = 0,
    MESH_NO_MEMORY,
    MESH_RUNTIME_ERROR,
    MESH_INVALID_ARGUMENT,
    MESH_DATA_CORRUPTED,
    MESH_BAD_OPERATION,
    MESH_OVERFLOW,
    MESH_LIMIT_EXCEEDED,
    MESH_UNDEFINED_ERROR,
} mesh_result_t;

#ifdef __cplusplus
}
#endif
#endif
