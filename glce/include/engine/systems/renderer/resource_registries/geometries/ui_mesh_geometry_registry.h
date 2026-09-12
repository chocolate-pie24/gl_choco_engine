// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_UI_MESH_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_GEOMETRIES_UI_MESH_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

typedef struct ui_mesh_geometry_registry ui_mesh_geometry_registry_t;   /**< UI描画用ジオメトリレジストリのopaque型 */

typedef struct linear_alloc linear_alloc_t;                             /**< リニアアロケータのopaque型 */
typedef struct ui_mesh_geometry ui_mesh_geometry_t;                     /**< UI描画用ジオメトリのopaque型 */
typedef struct vbo_range vbo_range_t;
typedef struct draw_range draw_range_t;
typedef struct ui_mesh_shader ui_mesh_shader_t;

resource_registry_result_t ui_mesh_geometry_registry_initialize(size_t max_geometry_count_, linear_alloc_t* allocator_, ui_mesh_geometry_registry_t** out_registry_);

void ui_mesh_geometry_registry_deinitialize(ui_mesh_geometry_registry_t* registry_, ui_mesh_shader_t* shader_);

bool ui_mesh_geometry_registry_find(const ui_mesh_geometry_registry_t* registry_, const char* name_);

const ui_mesh_geometry_t* ui_mesh_geometry_registry_geometry_get(const ui_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

resource_registry_result_t ui_mesh_geometry_registry_id_get(const ui_mesh_geometry_registry_t* registry_, const char* name_, uint16_t* out_geometry_id_);

const draw_range_t* ui_mesh_geometry_registry_draw_range_get(const ui_mesh_geometry_registry_t* registry_, uint16_t geometry_id_);

resource_registry_result_t ui_mesh_geometry_registry_register(ui_mesh_geometry_registry_t* registry_, const char* resource_name_, ui_mesh_geometry_t** geometry_, vbo_range_t* vbo_range_, uint16_t* out_geometry_id_);

resource_registry_result_t ui_mesh_geometry_registry_unregister(ui_mesh_geometry_registry_t* registry_, ui_mesh_shader_t* shader_, uint16_t geometry_id_);

bool ui_mesh_geometry_registry_is_valid(const ui_mesh_geometry_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
