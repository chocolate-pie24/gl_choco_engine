#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_LIT_MESH_GEOMETRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_LIT_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resource_pipelines/resource_pipelines_core/resource_pipeline_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_resources/shaders/lit_mesh_shader.h"
#include "engine/systems/renderer/resource_registries/geometries/lit_mesh_geometry_registry.h"

resource_pipeline_result_t resource_pipelines_lit_mesh_geometry_import_from_file(const renderer_backend_context_t* backend_context_, lit_mesh_shader_t* shader_, lit_mesh_geometry_registry_t* geometry_registry_, const char* path_, const char* name_, const char* extension_, int16_t* out_geometry_id_);

// resource_pipeline_result_t resource_pipelines_lit_mesh_geometry_import_from_vertices();

resource_pipeline_result_t resource_pipelines_lit_mesh_geometry_release(int16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
