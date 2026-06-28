#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_POINT_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_POINT_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct renderer_backend_context renderer_backend_context_t;         /**< Renderer Backend Contextのopaque型 */
typedef struct point_mesh_shader point_mesh_shader_t;                       /**< 点描画用シェーダーリソースのopaque型 */
typedef struct point_mesh_geometry_registry point_mesh_geometry_registry_t; /**< 点描画用ジオメトリレジストリのopaque型 */

resource_pipeline_result_t point_mesh_geometry_pipeline_import_from_vertices(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* shader_, point_mesh_geometry_registry_t* geometry_registry_, const char* name_, const point_vertex_t* vertices_, size_t vertex_count_, int16_t* out_geometry_id_);

resource_pipeline_result_t point_mesh_geometry_pipeline_release(int16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
