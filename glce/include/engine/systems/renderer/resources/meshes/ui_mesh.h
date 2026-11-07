// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UI_MESH_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UI_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

typedef struct choco_string choco_string_t;

typedef struct ui_mesh_config {
    choco_string_t* geometry_subdirectory;  // assetsからの相対パス
    choco_string_t* geometry_resource_name;
    choco_string_t* geometry_extension;

    choco_string_t* material_subdirectory;  // assetsからの相対パス
    choco_string_t* texture_resource_name;
    choco_string_t* texture_extension;
} ui_mesh_config_t;

typedef struct ui_mesh {
    uint16_t geometry_id;
    uint16_t texture_id;
} ui_mesh_t;

mesh_result_t ui_mesh_config_initialize(
    const char* geometry_subdirectory_,
    const char* geometry_resource_name_,
    const char* geometry_extension_,
    const char* material_subdirectory_,
    const char* texture_resource_name_,
    const char* texture_extension_,
    ui_mesh_config_t* out_config_);

void ui_mesh_config_deinitialize(ui_mesh_config_t* config_);

bool ui_mesh_config_is_valid(const ui_mesh_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
