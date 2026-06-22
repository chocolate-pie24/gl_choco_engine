#include <stdint.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_pipelines/lit_mesh_geometry.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/geometry/lit_mesh_geometry.h"
#include "engine/resource/loaders/stl_loader.h"
#include "engine/resource/resource_core/resource_types.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

#include "engine/systems/renderer/resource_pipelines/resource_pipelines_core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/resource_pipelines_core/resource_pipeline_err_utils.h"

#include "engine/systems/renderer/renderer_resources/shaders/lit_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/lit_mesh_geometry_registry.h"

resource_pipeline_result_t resource_pipelines_lit_mesh_geometry_import_from_file(const renderer_backend_context_t* backend_context_, lit_mesh_shader_t* shader_, lit_mesh_geometry_registry_t* geometry_registry_, const char* path_, const char* name_, const char* extension_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    stl_loader_t* stl_loader = NULL;
    point_normal_vertex_t* vertices = NULL;
    size_t vertex_count = 0;
    size_t vertex_offset = 0;
    int16_t tmp_geometry_id = 0;

    lit_mesh_geometry_t* geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "resource_pipelines_lit_mesh_geometry_import_from_file", "path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "resource_pipelines_lit_mesh_geometry_import_from_file", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "resource_pipelines_lit_mesh_geometry_import_from_file", "extension_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "resource_pipelines_lit_mesh_geometry_import_from_file", "out_geometry_id_")

    // TODO: private function: stl_geometry_import()作成
    if(choco_string_equal(extension_, ".stl")) {
        ret_resource = stl_loader_create(&stl_loader);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = resource_pipeline_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("resource_pipelines_lit_mesh_geometry_import_from_file(%s) - Failed to create stl loader. name='%s'", resource_pipeline_rslt_to_str(ret), name_);
            goto cleanup;
        }

        ret_resource = stl_loader_ascii_load(path_, name_, extension_, stl_loader);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = resource_pipeline_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("resource_pipelines_lit_mesh_geometry_import_from_file(%s) - Failed to load stl data. name='%s'", resource_pipeline_rslt_to_str(ret), name_);
            goto cleanup;
        }

        ret_resource = stl_loader_vertices_move(stl_loader, &vertices, &vertex_count);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = resource_pipeline_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("resource_pipelines_lit_mesh_geometry_import_from_file(%s) - Failed to move stl vertices. name='%s'", resource_pipeline_rslt_to_str(ret), name_);
            goto cleanup;
        }

        ret_resource = lit_mesh_geometry_create(name_, vertex_count, vertices, &geometry);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = resource_pipeline_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("resource_pipelines_lit_mesh_geometry_import_from_file(%s) - Failed to create lit mesh geometry. name='%s'", resource_pipeline_rslt_to_str(ret), name_);
            goto cleanup;
        }

        ret_renderer = lit_mesh_shader_vertex_buffer_vertex_append(backend_context_, shader_, sizeof(point_normal_vertex_t) * vertex_count, vertices, &vertex_offset);
        if(RENDERER_SUCCESS != ret_renderer) {
            ret = resource_pipeline_rslt_convert_renderer(ret_renderer);
            ERROR_MESSAGE("resource_pipelines_lit_mesh_geometry_import_from_file(%s) - Failed to append stl vertices to vertex buffer. name='%s', vertex_count=%zu.", resource_pipeline_rslt_to_str(ret), name_, vertex_count);
            goto cleanup;
        }

        ret_registry = lit_mesh_geometry_registry_geometry_register(geometry, vertex_offset, geometry_registry_, &tmp_geometry_id);
        if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
            ret = resource_pipeline_rslt_convert_resource_registries(ret_registry);
            ERROR_MESSAGE("resource_pipelines_lit_mesh_geometry_import_from_file(%s) - Failed to register geometry. name = '%s'.", resource_pipeline_rslt_to_str(ret));
            goto cleanup;
        }

        memory_system_free(vertices, sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        vertices = NULL;

        lit_mesh_geometry_destroy(&geometry);
        stl_loader_destroy(&stl_loader);

        *out_geometry_id_ = tmp_geometry_id;
    } else {
        ret = RESOURCE_PIPELINE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("resource_pipelines_lit_mesh_geometry_import_from_file(%s) - File type '%s' is not supported yet.", resource_pipeline_rslt_to_str(ret), extension_);
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}

resource_pipeline_result_t resource_pipelines_lit_mesh_geometry_release(int16_t geometry_id_) {
    // TODO: VBO FreeList + releaseができたら実装する
    ERROR_MESSAGE("resource_pipelines_lit_mesh_geometry_release - This function is not implemented yet. Because vertex buffer release is not implemented yet.");
    return RESOURCE_PIPELINE_RUNTIME_ERROR;
}
