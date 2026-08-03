/** @ingroup renderer
 *
 * @file point_mesh_geometry_pipeline.c
 * @author chocolate-pie24
 *
 * @brief 点描画用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIの実装
 *
 * @version 0.1
 * @date 2026-06-30
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/systems/renderer/resource_pipelines/geometries/point_mesh_geometry_pipeline.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/geometry/point_mesh_geometry.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

#include "engine/systems/renderer/renderer_resources/shaders/point_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/point_mesh_geometry_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

resource_pipeline_result_t point_mesh_geometry_pipeline_import_from_vertices(const renderer_backend_context_t* backend_context_, point_mesh_shader_t* shader_, point_mesh_geometry_registry_t* geometry_registry_, const char* name_, const point_vertex_t* vertices_, size_t vertex_count_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t vertex_offset = 0;
    int16_t tmp_geometry_id = 0;
    vertex_buffer_range_t tmp_buffer_range = { 0 };
    bool vbo_written = false;

    point_mesh_geometry_t* geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "vertices_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < vertex_count_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "vertex_count_")

    ret_resource = point_mesh_geometry_create_from_vertices(name_, vertex_count_, vertices_, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import point mesh geometry. reason=geometry_create_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count_);
        goto cleanup;
    }

    ret_shader = point_mesh_shader_vbo_write(backend_context_, shader_, vertex_count_, vertices_, &tmp_buffer_range);
    if(SHADER_SUCCESS != ret_shader) {
        ret = resource_pipeline_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import point mesh geometry. reason=vertex_buffer_append_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count_);
        goto cleanup;
    }
    vbo_written = true;

    // NOTE: 一時的にverticesが2つ分必要なので、deep copyではなくmoveを検討しても良い
    ret_registry = point_mesh_geometry_registry_register(geometry_registry_, geometry, &tmp_buffer_range, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import point mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_offset=%zu, vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_offset, vertex_count_);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_SUCCESS != ret && vbo_written) {
        ret_shader = point_mesh_shader_vbo_free(shader_, &tmp_buffer_range);
        if(SHADER_SUCCESS != ret_shader) {
            // NOTE: point_mesh_shader_vbo_freeが失敗した場合はbuffer_managerにデータ不整合が発生しているため、
            // point_mesh_geometry_pipeline_import_from_vertices失敗理由に関わらず、重大エラーのDATA_CORRUPTEDを返す
            ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
            ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - point mesh geometry import failed.", resource_pipeline_rslt_to_str(ret));
        }
    }
    point_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t point_mesh_geometry_pipeline_release(point_mesh_shader_t* shader_, point_mesh_geometry_registry_t* geometry_registry_, int16_t geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    vertex_buffer_range_t vertex_buffer_range = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_release", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_release", "geometry_registry_")

    ret_registry = point_mesh_geometry_registry_vertex_buffer_range_get(geometry_registry_, geometry_id_, &vertex_buffer_range);
    if(RESOURCE_REGISTRY_INVALID_ARGUMENT == ret_registry || RESOURCE_REGISTRY_BAD_OPERATION == ret_registry) { // geometry_idが異常
        ret = RESOURCE_PIPELINE_RUNTIME_ERROR;
        ERROR_MESSAGE("point_mesh_geometry_pipeline_release(%s) - point_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    } else if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_pipeline_release(%s) - point_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // unregisterに失敗した場合はgeometry_registry_は不変となる。そのため、vbo_freeの後でunregisterに失敗するとgeometry_registry_に解放済みallocationへの参照が残る。よってvbo_freeの前で実行する
    ret_registry = point_mesh_geometry_registry_unregister(geometry_registry_, geometry_id_);
    if(RESOURCE_REGISTRY_INVALID_ARGUMENT == ret_registry || RESOURCE_REGISTRY_BAD_OPERATION == ret_registry) { // geometry_idが異常
        ret = RESOURCE_PIPELINE_RUNTIME_ERROR;
        ERROR_MESSAGE("point_mesh_geometry_pipeline_release(%s) - point_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    } else if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
        ERROR_MESSAGE("point_mesh_geometry_pipeline_release(%s) - point_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = point_mesh_shader_vbo_free(shader_, &vertex_buffer_range);
    if(SHADER_SUCCESS != ret_shader) {
        ret = resource_pipeline_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_release(%s) - point_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}
