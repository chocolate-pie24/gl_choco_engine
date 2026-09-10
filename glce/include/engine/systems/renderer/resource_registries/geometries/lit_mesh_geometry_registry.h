// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_LIT_MESH_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_LIT_MESH_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

typedef struct lit_mesh_geometry_registry lit_mesh_geometry_registry_t; /**< 単色ライティング描画用ジオメトリレジストリのopaque型 */

typedef struct linear_alloc linear_alloc_t;                             /**< リニアアロケータのopaque型 */
typedef struct lit_mesh_geometry lit_mesh_geometry_t;                   /**< 単色ライティング描画用ジオメトリのopaque型 */
typedef struct vbo_range vbo_range_t;
typedef struct draw_range draw_range_t;
typedef struct lit_mesh_shader lit_mesh_shader_t;

resource_registry_result_t lit_mesh_geometry_registry_initialize(size_t max_geometry_count_, linear_alloc_t* allocator_, lit_mesh_geometry_registry_t** out_registry_);

void lit_mesh_geometry_registry_deinitialize(lit_mesh_geometry_registry_t* registry_, lit_mesh_shader_t* shader_);

bool lit_mesh_geometry_registry_find(const lit_mesh_geometry_registry_t* registry_, const char* name_);

const lit_mesh_geometry_t* lit_mesh_geometry_registry_geometry_get(const lit_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

resource_registry_result_t lit_mesh_geometry_registry_id_get(const lit_mesh_geometry_registry_t* registry_, const char* name_, uint16_t* out_geometry_id_);

const draw_range_t* lit_mesh_geometry_registry_draw_range_get(const lit_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

resource_registry_result_t lit_mesh_geometry_registry_register(lit_mesh_geometry_registry_t* registry_, const char* resource_name_, lit_mesh_geometry_t** geometry_, vbo_range_t* vbo_range_, uint16_t* out_geometry_id_);

resource_registry_result_t lit_mesh_geometry_registry_unregister(lit_mesh_geometry_registry_t* registry_, lit_mesh_shader_t* shader_, uint16_t geometry_id_);

bool lit_mesh_geometry_registry_is_valid(const lit_mesh_geometry_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
