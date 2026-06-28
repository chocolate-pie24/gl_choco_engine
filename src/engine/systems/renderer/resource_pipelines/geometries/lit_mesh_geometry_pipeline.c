#include "engine/systems/renderer/resource_pipelines/geometries/lit_mesh_geometry_pipeline.h"

#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/loaders/stl_loader.h"
#include "engine/resource/geometry/lit_mesh_geometry.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

#include "engine/systems/renderer/renderer_resources/shaders/lit_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/lit_mesh_geometry_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

// append成功後の後続処理で失敗した場合、shader_に追加された頂点データは巻き戻されない
resource_pipeline_result_t lit_mesh_geometry_pipeline_import_from_file(const renderer_backend_context_t* backend_context_, lit_mesh_shader_t* shader_, lit_mesh_geometry_registry_t* geometry_registry_, const char* path_, const char* name_, const char* extension_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    stl_loader_t* stl_loader = NULL;
    point_normal_vertex_t* vertices = NULL;
    size_t vertex_count = 0;
    size_t vertex_offset = 0;
    size_t vertex_array_size = 0;
    int16_t tmp_geometry_id = 0;

    lit_mesh_geometry_t* geometry = NULL;

    // path_が空文字列なのは許容する
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "lit_mesh_geometry_pipeline_import_from_file", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "lit_mesh_geometry_pipeline_import_from_file", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret,RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "lit_mesh_geometry_pipeline_import_from_file", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "lit_mesh_geometry_pipeline_import_from_file", "path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "lit_mesh_geometry_pipeline_import_from_file", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "lit_mesh_geometry_pipeline_import_from_file", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "lit_mesh_geometry_pipeline_import_from_file", "extension_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != extension_[0], ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "lit_mesh_geometry_pipeline_import_from_file", "extension_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "lit_mesh_geometry_pipeline_import_from_file", "out_geometry_id_")

    // TODO: private function: stl_geometry_import()作成
    if(choco_string_equal(extension_, ".stl")) {
        ret_resource = stl_loader_create(&stl_loader);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = resource_pipeline_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("lit_mesh_geometry_pipeline_import_from_file(%s) - Failed to import lit mesh geometry. reason=stl_loader_create_failed, geometry_name='%s', extension='%s'", resource_pipeline_rslt_to_str(ret), name_, extension_);
            goto cleanup;
        }

        ret_resource = stl_loader_ascii_load(path_, name_, extension_, stl_loader);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = resource_pipeline_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("lit_mesh_geometry_pipeline_import_from_file(%s) - Failed to import lit mesh geometry. reason=stl_load_failed, path='%s', geometry_name='%s', extension='%s'", resource_pipeline_rslt_to_str(ret), path_, name_, extension_);
            goto cleanup;
        }

        ret_resource = stl_loader_vertices_move(stl_loader, &vertices, &vertex_count);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = resource_pipeline_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("lit_mesh_geometry_pipeline_import_from_file(%s) - Failed to import lit mesh geometry. reason=stl_vertices_move_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), name_);
            goto cleanup;
        }
        vertex_array_size = sizeof(point_normal_vertex_t) * vertex_count;

        ret_resource = lit_mesh_geometry_create(name_, vertex_count, vertices, &geometry);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = resource_pipeline_rslt_convert_resource(ret_resource);
            ERROR_MESSAGE("lit_mesh_geometry_pipeline_import_from_file(%s) - Failed to import lit mesh geometry. reason=geometry_create_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count);
            goto cleanup;
        }

        // lit_mesh_geometry_create()が成功しているのでオーバーフローチェックは不要
        ret_renderer = lit_mesh_shader_vertex_buffer_append(backend_context_, shader_, vertex_array_size, vertices, &vertex_offset);
        if(RENDERER_SUCCESS != ret_renderer) {
            ret = resource_pipeline_rslt_convert_renderer(ret_renderer);
            ERROR_MESSAGE("lit_mesh_geometry_pipeline_import_from_file(%s) - Failed to import lit mesh geometry. reason=vertex_buffer_append_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count);
            goto cleanup;
        }

        memory_system_free(vertices, vertex_array_size, MEMORY_TAG_GEOMETRY);
        vertices = NULL;

        // NOTE: 一時的にverticesが2つ分必要なので、deep copyではなくmoveを検討しても良い
        ret_registry = lit_mesh_geometry_registry_register(geometry, vertex_offset, geometry_registry_, &tmp_geometry_id);
        if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
            ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
            ERROR_MESSAGE("lit_mesh_geometry_pipeline_import_from_file(%s) - Failed to import lit mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_offset=%zu, vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_offset, vertex_count);
            goto cleanup;
        }

        *out_geometry_id_ = tmp_geometry_id;
    } else {
        ret = RESOURCE_PIPELINE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("lit_mesh_geometry_pipeline_import_from_file(%s) - Failed to import lit mesh geometry. reason=unsupported_file_extension, extension='%s', supported_extension='.stl'", resource_pipeline_rslt_to_str(ret), extension_);
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    lit_mesh_geometry_destroy(&geometry);
    stl_loader_destroy(&stl_loader);
    if(NULL != vertices) {
        memory_system_free(vertices, vertex_array_size, MEMORY_TAG_GEOMETRY);
        vertices = NULL;
    }
    return ret;
}

resource_pipeline_result_t lit_mesh_geometry_pipeline_release(int16_t geometry_id_) {
    // TODO: VBO FreeList + releaseができたら実装する
    ERROR_MESSAGE("lit_mesh_geometry_pipeline_release(%s) - Failed to release lit mesh geometry. reason=not_implemented, geometry_id=%d, vertex_buffer_release=not_supported", resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_RUNTIME_ERROR), geometry_id_);
    return RESOURCE_PIPELINE_RUNTIME_ERROR;
}
