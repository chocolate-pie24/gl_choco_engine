// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_CORE_MESH_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_CORE_MESH_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

const char* mesh_result_to_str(mesh_result_t result_);

mesh_result_t mesh_result_convert_choco_string(choco_string_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
