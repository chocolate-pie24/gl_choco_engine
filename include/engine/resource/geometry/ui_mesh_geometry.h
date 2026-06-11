#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_UI_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_UI_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct ui_mesh_geometry ui_mesh_geometry_t;   /**< ui_mesh_geometryモジュール内部状態管理構造体 */

resource_result_t ui_mesh_geometry_default_create(ui_mesh_geometry_t** geometry_);

resource_result_t ui_mesh_geometry_create_from_vertices(const char* name_, size_t vertex_count_, const ui_vertex_t* vertices_, ui_mesh_geometry_t** geometry_);

void ui_mesh_geometry_destroy(ui_mesh_geometry_t** geometry_);

resource_result_t ui_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const ui_vertex_t* vertices_, ui_mesh_geometry_t* geometry_);

void ui_mesh_geometry_deinitialize(ui_mesh_geometry_t* geometry_);

const char* ui_mesh_geometry_name_get(const ui_mesh_geometry_t* geometry_);

resource_result_t ui_mesh_geometry_vertices_get(const ui_mesh_geometry_t* geometry_, const ui_vertex_t** out_vertices_);

resource_result_t ui_mesh_geometry_vertex_count_get(const ui_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
