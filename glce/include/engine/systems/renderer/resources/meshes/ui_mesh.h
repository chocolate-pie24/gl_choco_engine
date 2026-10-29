// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UI_MESH_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UI_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct choco_string choco_string_t;

typedef struct ui_mesh_config {
    choco_string_t* geometry_subdirectory;  // assetsからの相対パス
    choco_string_t* geometry_resource_name;

    choco_string_t* material_subdirectory;  // assetsからの相対パス
    choco_string_t* texture_resource_name;
} ui_mesh_config_t;

typedef struct ui_mesh {
    uint16_t geometry_id;
    uint16_t texture_id;
} ui_mesh_t;

#ifdef __cplusplus
}
#endif
#endif
