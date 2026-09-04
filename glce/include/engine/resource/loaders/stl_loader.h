// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_RESOURCE_LOADERS_STL_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_STL_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

resource_result_t stl_loader_load(const char* fullpath_, size_t* out_vertex_count_, point_normal_vertex_t** out_vertices_);

#ifdef __cplusplus
}
#endif
#endif
