#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_LIT_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_LIT_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct lit_mesh_geometry lit_mesh_geometry_t;

resource_result_t lit_mesh_geometry_create(lit_mesh_geometry_t** geometry_);

void lit_mesh_geometry_destroy(lit_mesh_geometry_t** geometry_);

// geometry_にvertices_をdeep copy, vertices_の所有権は呼び出し側
resource_result_t lit_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t* geometry_);

resource_result_t lit_mesh_geometry_initialize_from_file(const char* path_, const char* name_, const char* extension_, lit_mesh_geometry_t* geometry_);

const char* lit_mesh_geometry_name_get(const lit_mesh_geometry_t* geometry_);

// verticesへの参照を取得(委譲ではない)
resource_result_t lit_mesh_geometry_vertices_get(const lit_mesh_geometry_t* geometry_, const point_normal_vertex_t** out_vertices_);

resource_result_t lit_mesh_geometry_vertex_count_get(const lit_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
