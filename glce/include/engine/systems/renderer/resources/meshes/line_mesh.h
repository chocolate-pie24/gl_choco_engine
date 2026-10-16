// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_LINE_MESH_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_LINE_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

typedef struct line_mesh {
    uint16_t geometry_id;
    vec4u8_t color;
} line_mesh_t;

#ifdef __cplusplus
}
#endif
#endif
