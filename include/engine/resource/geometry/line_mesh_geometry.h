#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_LINE_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_LINE_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct line_mesh_geometry line_mesh_geometry_t;   /**< line_mesh_geometryモジュール内部状態管理構造体 */

resource_result_t line_mesh_geometry_create(line_mesh_geometry_t** geometry_);

void line_mesh_geometry_destroy(line_mesh_geometry_t** geometry_);

resource_result_t line_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const line_vertex_t* vertices_, line_mesh_geometry_t* geometry_);

const char* line_mesh_geometry_name_get(const line_mesh_geometry_t* geometry_);

resource_result_t line_mesh_geometry_vertices_get(const line_mesh_geometry_t* geometry_, const line_vertex_t** out_vertices_);

resource_result_t line_mesh_geometry_vertex_count_get(const line_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
