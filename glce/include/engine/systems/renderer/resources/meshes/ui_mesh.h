// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UI_MESH_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UI_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct ui_mesh {
    uint16_t geometry_id;
    uint16_t texture_id;
} ui_mesh_t;

#ifdef __cplusplus
}
#endif
#endif
