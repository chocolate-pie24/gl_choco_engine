#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/systems/renderer/resource_pipelines/geometries/point_mesh_geometry_pipeline.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/geometry/point_mesh_geometry.h"
#include "engine/resource/resource_core/resource_types.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

#include "engine/systems/renderer/renderer_resources/shaders/point_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/point_mesh_geometry_registry.h"

// append成功後の後続処理で失敗した場合、shader_に追加された頂点データは巻き戻されない
resource_pipeline_result_t point_mesh_geometry_pipeline_import_from_vertices(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* shader_, point_mesh_geometry_registry_t* geometry_registry_, const char* name_, const point_vertex_t* vertices_, size_t vertex_count_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t vertex_count = 0;
    size_t vertex_offset = 0;
    size_t vertex_array_size = 0;
    int16_t tmp_geometry_id = 0;

    point_mesh_geometry_t* geometry = NULL;

    // path_が空文字列なのは許容する
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret,RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "vertices_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < vertex_count_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "vertex_count_")

    ret_resource = point_mesh_geometry_create_from_vertices(name_, vertex_count_, vertices_, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import point mesh geometry. reason=geometry_create_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count);
        goto cleanup;
    }
    vertex_array_size = sizeof(point_vertex_t) * vertex_count_;

    // point_mesh_geometry_create_from_vertices()が成功しているのでオーバーフローチェックは不要
    ret_renderer = point_mesh_shader_vertex_buffer_point_append(backend_context_, shader_, vertex_array_size, vertices_, &vertex_offset);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = resource_pipeline_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import point mesh geometry. reason=vertex_buffer_append_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count);
        goto cleanup;
    }

    // NOTE: 一時的にverticesが2つ分必要なので、deep copyではなくmoveを検討しても良い
    ret_registry = point_mesh_geometry_registry_geometry_register(geometry, vertex_offset, geometry_registry_, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import point mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_offset=%zu, vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_offset, vertex_count);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    point_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t point_mesh_geometry_pipeline_release(int16_t geometry_id_) {
    // TODO: VBO FreeList + releaseができたら実装する
    ERROR_MESSAGE("point_mesh_geometry_pipeline_release - Failed to release point mesh geometry. reason=not_implemented, geometry_id=%d, vertex_buffer_release=not_supported", resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_RUNTIME_ERROR), geometry_id_);
    return RESOURCE_PIPELINE_RUNTIME_ERROR;
}
