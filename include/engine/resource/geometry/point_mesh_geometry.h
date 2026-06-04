// TODO: pcdファイル等の点群ファイルからの初期化はそのうちやる
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_POINT_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_POINT_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct point_mesh_geometry point_mesh_geometry_t;   /**< point_mesh_geometryモジュール内部状態管理構造体 */

resource_result_t point_mesh_geometry_create(point_mesh_geometry_t** geometry_);

void point_mesh_geometry_destroy(point_mesh_geometry_t** geometry_);

resource_result_t point_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const point_vertex_t* vertices_, point_mesh_geometry_t* geometry_);

const char* point_mesh_geometry_name_get(const point_mesh_geometry_t* geometry_);

resource_result_t point_mesh_geometry_vertices_get(const point_mesh_geometry_t* geometry_, const point_vertex_t** out_vertices_);

resource_result_t point_mesh_geometry_vertex_count_get(const point_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
