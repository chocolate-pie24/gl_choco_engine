// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_POINT_MESH_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_POINT_MESH_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

typedef struct point_mesh_geometry_registry point_mesh_geometry_registry_t; /**< 点描画用ジオメトリレジストリのopaque型 */

typedef struct linear_alloc linear_alloc_t;                                 /**< リニアアロケータのopaque型 */
typedef struct point_mesh_geometry point_mesh_geometry_t;                   /**< 点描画用ジオメトリのopaque型 */
typedef struct vbo_range vbo_range_t;
typedef struct draw_range draw_range_t;
typedef struct point_mesh_shader point_mesh_shader_t;

resource_registry_result_t point_mesh_geometry_registry_initialize(size_t max_geometry_count_, linear_alloc_t* allocator_, point_mesh_geometry_registry_t** out_registry_);

void point_mesh_geometry_registry_deinitialize(point_mesh_geometry_registry_t* registry_, point_mesh_shader_t* shader_);

bool point_mesh_geometry_registry_find(const point_mesh_geometry_registry_t* registry_, const char* name_);

const point_mesh_geometry_t* point_mesh_geometry_registry_geometry_get(const point_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

resource_registry_result_t point_mesh_geometry_registry_id_get(const point_mesh_geometry_registry_t* registry_, const char* name_, uint16_t* out_geometry_id_);

const draw_range_t* point_mesh_geometry_registry_draw_range_get(const point_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

resource_registry_result_t point_mesh_geometry_registry_register(point_mesh_geometry_registry_t* registry_, const char* resource_name_, point_mesh_geometry_t** geometry_, vbo_range_t* vbo_range_, uint16_t* out_geometry_id_);

resource_registry_result_t point_mesh_geometry_registry_unregister(point_mesh_geometry_registry_t* registry_, point_mesh_shader_t* shader_, uint16_t geometry_id_);

bool point_mesh_geometry_registry_is_valid(const point_mesh_geometry_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
