// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UNTEXTURED_MESH_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UNTEXTURED_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

typedef struct choco_string choco_string_t;

typedef struct untextured_mesh_config {
    choco_string_t* geometry_subdirectory;  // assetsからの相対パス
    choco_string_t* geometry_resource_name;
    choco_string_t* geometry_extension;

    choco_string_t* material_subdirectory;  // assetsからの相対パス
    choco_string_t* material_resource_name;
    choco_string_t* material_extension;
} untextured_mesh_config_t;

typedef struct untextured_mesh {
    uint16_t geometry_id;
    uint16_t material_id;
} untextured_mesh_t;

mesh_result_t untextured_mesh_config_initialize(
    const char* geometry_subdirectory_,
    const char* geometry_resource_name_,
    const char* geometry_extension_,
    const char* material_subdirectory_,
    const char* material_resource_name_,
    const char* material_extension_,
    untextured_mesh_config_t* out_config_);

void untextured_mesh_config_deinitialize(untextured_mesh_config_t* config_);

// TODO: validator追加

#ifdef __cplusplus
}
#endif
#endif
