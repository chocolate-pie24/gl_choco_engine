/** @ingroup renderer
 *
 * @file line_mesh_geometry_pipeline.c
 * @author chocolate-pie24
 *
 * @brief line_mesh用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIの実装
 *
 * @note 本pipelineはCPU側ジオメトリリソース生成、shader resourceへの頂点転送、geometry registryへの登録を一連の手順として実行する
 * @note GPU頂点バッファ自体はshader resourceが所有し、本pipelineは所有しない
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
#include "engine/systems/renderer/resource_pipelines/geometries/line_mesh_geometry_pipeline.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/geometry/line_mesh_geometry.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

#include "engine/systems/renderer/renderer_resources/shaders/line_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/line_mesh_geometry_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_vertices(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* name_, const line_vertex_t* vertices_, size_t vertex_count_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t vertex_offset = 0;
    size_t vertex_array_size = 0;
    int16_t tmp_geometry_id = 0;
    vertex_buffer_range_t tmp_buffer_range = { 0 };

    line_mesh_geometry_t* geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "vertices_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < vertex_count_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "vertex_count_")

    ret_resource = line_mesh_geometry_create_from_vertices(name_, vertex_count_, vertices_, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import line mesh geometry. reason=geometry_create_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count_);
        goto cleanup;
    }
    vertex_array_size = sizeof(line_vertex_t) * vertex_count_;

    // line_mesh_geometry_create_from_vertices()が成功しているのでオーバーフローチェックは不要
    ret_renderer = line_mesh_shader_vbo_write(backend_context_, shader_, vertex_array_size, vertices_, &tmp_buffer_range);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = resource_pipeline_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import line mesh geometry. reason=vertex_buffer_append_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count_);
        goto cleanup;
    }

    // NOTE: 一時的にverticesが2つ分必要なので、deep copyではなくmoveを検討しても良い
    ret_registry = line_mesh_geometry_registry_register(geometry_registry_, geometry, &tmp_buffer_range, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import line mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_offset=%zu, vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_offset, vertex_count_);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    // TODO: vbo_writeに成功した後、registerが失敗した場合のロールバックでvbo_freeを使用する必要があるが、vbo_freeが失敗する可能性がある。
    // - registerをreserve -> commit / abort方式の2-phase registerに変更
    // - range free listの2-phase allocation化も実施する予定なので、vbo_writeの2-phase writeも検討する
    line_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_aabb(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* name_, const aabb_3d_t* aabb_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t vertex_count = 0;
    size_t vertex_offset = 0;
    size_t vertex_array_size = 0;
    int16_t tmp_geometry_id = 0;
    vertex_buffer_range_t tmp_buffer_range = { 0 };

    line_mesh_geometry_t* geometry = NULL;
    const line_vertex_t* vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "aabb_")

    ret_resource = line_mesh_geometry_create_from_aabbs(name_, 1, aabb_, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=geometry_create_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), name_);
        goto cleanup;
    }
    ret_resource = line_mesh_geometry_vertex_count_get(geometry, &vertex_count);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=vertex_count_get_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), name_);
        goto cleanup;
    }
    ret_resource = line_mesh_geometry_vertices_get(geometry, &vertices);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=vertices_get_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), name_);
        goto cleanup;
    }
    vertex_array_size = sizeof(line_vertex_t) * vertex_count;   // 単体のAABBなのでオーバーフローチェックは不要

    ret_renderer = line_mesh_shader_vbo_write(backend_context_, shader_, vertex_array_size, vertices, &tmp_buffer_range);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = resource_pipeline_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=vertex_buffer_append_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count);
        goto cleanup;
    }

    // NOTE: 一時的にverticesが2つ分必要なので、deep copyではなくmoveを検討しても良い
    ret_registry = line_mesh_geometry_registry_register(geometry_registry_, geometry, &tmp_buffer_range, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_offset=%zu, vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_offset, vertex_count);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    // TODO: vbo_writeに成功した後、registerが失敗した場合のロールバックでvbo_freeを使用する必要があるが、vbo_freeが失敗する可能性がある。
    // - registerをreserve -> commit / abort方式の2-phase registerに変更
    // - range free listの2-phase allocation化も実施する予定なので、vbo_writeの2-phase writeも検討する
    line_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t line_mesh_geometry_pipeline_release(line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, int16_t geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;

    vertex_buffer_range_t vertex_buffer_range = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_release", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_release", "geometry_registry_")

    ret_registry = line_mesh_geometry_registry_vertex_buffer_range_get(geometry_registry_, geometry_id_, &vertex_buffer_range);
    if(RESOURCE_REGISTRY_INVALID_ARGUMENT == ret_registry || RESOURCE_REGISTRY_BAD_OPERATION == ret_registry) { // geometry_idが異常
        ret = RESOURCE_PIPELINE_RUNTIME_ERROR;
        ERROR_MESSAGE("line_mesh_geometry_pipeline_release(%s) - line_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    } else if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_geometry_pipeline_release(%s) - line_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer = line_mesh_shader_vbo_free(shader_, &vertex_buffer_range);
    if(RENDERER_SUCCESS != ret_renderer) {
        // TODO: buffer_manager周りの仕様が安定したら適切なエラーコードに変換する
        ret = RESOURCE_PIPELINE_RUNTIME_ERROR;
        ERROR_MESSAGE("line_mesh_geometry_pipeline_release(%s) - line_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret_registry = line_mesh_geometry_registry_unregister(geometry_registry_, geometry_id_);
    if(RESOURCE_REGISTRY_INVALID_ARGUMENT == ret_registry || RESOURCE_REGISTRY_BAD_OPERATION == ret_registry) { // geometry_idが異常
        ret = RESOURCE_PIPELINE_RUNTIME_ERROR;
        ERROR_MESSAGE("line_mesh_geometry_pipeline_release(%s) - line_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    } else if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_geometry_pipeline_release(%s) - line_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}
