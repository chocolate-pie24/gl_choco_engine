// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_LINE_MESH_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_LINE_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

typedef struct choco_string choco_string_t;

typedef struct line_mesh_config {
    choco_string_t* geometry_subdirectory;  // assetsからの相対パス
    choco_string_t* geometry_resource_name;
    vec4u8_t line_color;
} line_mesh_config_t;

typedef struct line_mesh {
    uint16_t geometry_id;
    vec4u8_t color;
} line_mesh_t;

#ifdef __cplusplus
}
#endif
#endif
