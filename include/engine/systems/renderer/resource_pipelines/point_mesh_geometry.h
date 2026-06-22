#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_POINT_MESH_GEOMETRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_POINT_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resource_pipelines/resource_pipelines_core/resource_pipeline_types.h"

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_resources/shaders/point_mesh_shader.h"
#include "engine/systems/renderer/resource_registries/geometries/point_mesh_geometry_registry.h"

resource_pipeline_result_t resource_pipelines_point_mesh_geometry_import_from_vertices(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* shader_, point_mesh_geometry_registry_t* geometry_registry_, const char* name_, const point_vertex_t* vertices_, size_t vertex_count_, int16_t* out_geometry_id_);

resource_pipeline_result_t resource_pipelines_point_mesh_geometry_release(int16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
