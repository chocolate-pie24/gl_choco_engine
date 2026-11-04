// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_GEOMETRIES_GENERATORS_UNTEXTURED_CYLINDER_GEOMETRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_GEOMETRIES_GENERATORS_UNTEXTURED_CYLINDER_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/cylinder.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"

geometry_result_t untextured_cylinder_geometry_vertices_create(const cylinder_t* cylinder_, uint16_t segment_count_, point_normal_vertex_t** out_vertices_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
