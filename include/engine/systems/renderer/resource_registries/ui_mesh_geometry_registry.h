#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_UI_MESH_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_UI_MESH_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

#include "engine/resource/geometry/ui_mesh_geometry.h"

#include "engine/core/memory/linear_allocator.h"

typedef struct ui_mesh_geometry_registry ui_mesh_geometry_registry_t;

resource_registry_result_t ui_mesh_geometry_registry_initialize(size_t max_geometry_count_, linear_alloc_t* allocator_, ui_mesh_geometry_registry_t** out_registry_);

void ui_mesh_geometry_registry_deinitialize(ui_mesh_geometry_registry_t* registry_);

bool ui_mesh_geometry_registry_geometry_find(const char* name_, const ui_mesh_geometry_registry_t* registry_);

resource_registry_result_t ui_mesh_geometry_registry_geometry_id_get(const char* name_, const ui_mesh_geometry_registry_t* registry_, int16_t* out_geometry_id_);

resource_registry_result_t ui_mesh_geometry_registry_draw_range_get(int16_t geometry_id_, const ui_mesh_geometry_registry_t* registry_, size_t* out_vertex_offset_, size_t* out_vertex_count_);

// geometry_をgeometry_registry_へdeep copy
resource_registry_result_t ui_mesh_geometry_registry_geometry_register(const ui_mesh_geometry_t* geometry_, size_t vertex_offset_, ui_mesh_geometry_registry_t* registry_, int16_t* out_geometry_id_);

resource_registry_result_t ui_mesh_geometry_registry_geometry_unregister(int16_t geometry_id_, ui_mesh_geometry_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
