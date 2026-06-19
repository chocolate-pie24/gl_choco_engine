#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_REGISTRIES_GEOMETRY_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_REGISTRIES_GEOMETRY_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

#include "engine/core/memory/linear_allocator.h"

#define INVALID_GEOMETRY_ID (-1)

typedef enum {
    GEOMETRY_TYPE_LIT_MESH_GEOMETRY,
    GEOMETRY_TYPE_LINE_MESH_GEOMETRY,
    GEOMETRY_TYPE_POINT_MESH_GEOMETRY,
    GEOMETRY_TYPE_UI_MESH_GEOMETRY,
} geometry_type_t;

typedef struct geometry_registry_config {
    size_t max_lit_mesh_geometry_count;
    size_t max_point_mesh_geometry_count;
    size_t max_line_mesh_geometry_count;
    size_t max_ui_mesh_geometry_count;
} geometry_registry_config_t;

typedef struct geometry_registry geometry_registry_t;

resource_registry_result_t geometry_registry_initialize(const geometry_registry_config_t* config_, linear_alloc_t* allocator_, geometry_registry_t** out_geometry_registry_);

void geometry_registry_deinitialize(geometry_registry_t* geometry_registry_);

bool geometry_registry_geometry_find(geometry_type_t geometry_type_, const char* name_, const geometry_registry_t* geometry_registry_);

resource_registry_result_t geometry_registry_geometry_id_get(geometry_type_t geometry_type_, const char* name_, const geometry_registry_t* geometry_registry_, int16_t* out_geometry_id_);

resource_registry_result_t geometry_registry_draw_range_get(geometry_type_t geometry_type_, int16_t geometry_id_, const geometry_registry_t* geometry_registry_, size_t* out_vertex_offset_, size_t* out_vertex_count_);

// geometry_をgeometry_registry_へdeep copy
resource_registry_result_t geometry_registry_geometry_register(geometry_type_t geometry_type_, const void* geometry_, geometry_registry_t* geometry_registry_, int16_t* out_geometry_id_);

resource_registry_result_t geometry_registry_geometry_unregister(geometry_type_t geometry_type_, int16_t geometry_id_, geometry_registry_t* geometry_registry_);

#ifdef __cplusplus
}
#endif
#endif
