// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_CONVERSION_GEOMETRY_CONVERSION_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_CONVERSION_GEOMETRY_CONVERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/resource/core/resource_types.h"

typedef struct lit_mesh_geometry lit_mesh_geometry_t;
typedef struct aabb_3d aabb_3d_t;

resource_result_t geometry_conversion_lit_mesh_geometry_to_aabb_3d(const lit_mesh_geometry_t* src_geometry_, aabb_3d_t* dst_geometry_);

#ifdef __cplusplus
}
#endif
#endif
