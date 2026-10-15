// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_GEOMETRIES_CORE_GEOMETRY_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_GEOMETRIES_CORE_GEOMETRY_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"

const char* geometry_result_to_str(geometry_result_t result_);

geometry_result_t geometry_result_convert_general_allocator(general_allocator_result_t result_);

geometry_result_t geometry_result_convert_aabb_3d(aabb_3d_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
