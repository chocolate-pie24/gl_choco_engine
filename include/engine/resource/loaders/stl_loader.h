#ifndef GLCE_ENGINE_RESOURCE_LOADERS_STL_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_STL_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct stl_loader stl_loader_t; /**< STLローダー内部状態管理構造体前方宣言 */

resource_result_t stl_loader_create(stl_loader_t** stl_loader_);

void stl_loader_destroy(stl_loader_t** stl_loader_);

resource_result_t stl_loader_load(const char* path_, const char* name_, const char* extension_, stl_loader_t* stl_loader_);

resource_result_t stl_loader_vertex_move(stl_loader_t* stl_loader_, point_normal_vertex_t** out_vertices_, size_t* out_vertex_count_);

resource_result_t stl_loader_vertex_count_get(const stl_loader_t* stl_loader_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
