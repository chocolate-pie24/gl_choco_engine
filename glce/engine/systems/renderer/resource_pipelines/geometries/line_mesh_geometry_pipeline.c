// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

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
 * @date 2026-06-30
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

#include "engine/resource/core/resource_types.h"
#include "engine/resource/geometry/line_mesh_geometry.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/shaders/line_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/line_mesh_geometry_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_vertices(line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const line_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;
    vbo_range_t tmp_buffer_range = { 0 };

    line_mesh_geometry_t* geometry = NULL;
    bool vbo_written = false;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "vertices_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Provided resource_name_ is not valid.", resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT));
        goto cleanup;
    }
    if(0 == vertex_count_) {
        ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Provided vertex_count_ is not valid.", resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT));
        goto cleanup;
    }

    // Geometry CPUリソース生成
    ret_resource = line_mesh_geometry_create_from_vertices(vertex_count_, vertices_, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import line mesh geometry. reason=geometry_create_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), resource_name_, vertex_count_);
        goto cleanup;
    }

    // Geometry VBO書き込み
    // line_mesh_geometry_create_from_vertices()が成功しているのでオーバーフローチェックは不要
    ret_shader = line_mesh_shader_vbo_write(shader_, vertex_count_, vertices_, &tmp_buffer_range);
    if(SHADER_SUCCESS != ret_shader) {
        ret = resource_pipeline_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import line mesh geometry. reason=vbo_write_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), resource_name_, vertex_count_);
        goto cleanup;
    }
    vbo_written = true;

    // Geometry Registryへ登録
    ret_registry = line_mesh_geometry_registry_register(geometry_registry_, resource_name_, &geometry, &tmp_buffer_range, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import line mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), resource_name_, vertex_count_);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_SUCCESS != ret && vbo_written) {
        ret_shader = line_mesh_shader_vbo_free(shader_, &tmp_buffer_range);
        if(SHADER_SUCCESS != ret_shader) {
            // NOTE: line_mesh_shader_vbo_freeが失敗した場合はbuffer_managerにデータ不整合が発生しているため、
            // line_mesh_geometry_pipeline_import_from_vertices失敗理由に関わらず、重大エラーのDATA_CORRUPTEDを返す
            ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
            ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - line mesh geometry import failed.", resource_pipeline_rslt_to_str(ret));
        }
    }
    line_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_aabb(line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const aabb_3d_t* aabb_, uint16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    size_t vertex_count = 0;
    uint16_t tmp_geometry_id = 0;
    vbo_range_t tmp_buffer_range = { 0 };

    line_mesh_geometry_t* geometry = NULL;
    const line_vertex_t* vertices = NULL;

    bool vbo_written = false;

    // Preconditions
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "aabb_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Provided resource_name_ is not valid.", resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT));
        goto cleanup;
    }

    // Geometry CPUリソース生成
    ret_resource = line_mesh_geometry_create_from_aabbs(1, aabb_, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=geometry_create_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), resource_name_);
        goto cleanup;
    }

    ret_resource = line_mesh_geometry_vertex_count_get(geometry, &vertex_count);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=vertex_count_get_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), resource_name_);
        goto cleanup;
    }
    ret_resource = line_mesh_geometry_vertices_get(geometry, &vertices);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=vertices_get_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), resource_name_);
        goto cleanup;
    }

    ret_shader = line_mesh_shader_vbo_write(shader_, vertex_count, vertices, &tmp_buffer_range);
    if(SHADER_SUCCESS != ret_shader) {
        ret = resource_pipeline_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=vbo_write_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), resource_name_, vertex_count);
        goto cleanup;
    }
    vbo_written = true;

    ret_registry = line_mesh_geometry_registry_register(geometry_registry_, resource_name_, &geometry, &tmp_buffer_range, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), resource_name_, vertex_count);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_SUCCESS != ret && vbo_written) {
        ret_shader = line_mesh_shader_vbo_free(shader_, &tmp_buffer_range);
        if(SHADER_SUCCESS != ret_shader) {
            // NOTE: line_mesh_shader_vbo_freeが失敗した場合はbuffer_managerにデータ不整合が発生しているため、
            // line_mesh_geometry_pipeline_import_from_aabb失敗理由に関わらず、重大エラーのDATA_CORRUPTEDを返す
            ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
            ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - line mesh geometry import failed.", resource_pipeline_rslt_to_str(ret));
        }
    }
    line_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t line_mesh_geometry_pipeline_release(line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, uint16_t geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_release", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_release", "geometry_registry_")

    ret_registry = line_mesh_geometry_registry_unregister(geometry_registry_, shader_, geometry_id_);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_release(%s) - line_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}
