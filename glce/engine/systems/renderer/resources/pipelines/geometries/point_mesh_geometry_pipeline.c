// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file point_mesh_geometry_pipeline.c
 * @author chocolate-pie24
 *
 * @brief 点描画用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIの実装
 *
 * @date 2026-06-30
 *
 */
#include "engine/systems/renderer/resources/pipelines/geometries/point_mesh_geometry_pipeline.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"
#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/geometries/point_mesh_geometry.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/registries/geometries/point_mesh_geometry_registry.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_err_utils.h"

resource_pipeline_result_t point_mesh_geometry_pipeline_import_from_vertices(vbo_manager_t* vbo_manager_, point_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const point_vertex_t* vertices_, size_t vertex_count_, uint16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    geometry_result_t ret_geometry = GEOMETRY_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    buffer_manager_result_t ret_buffer_manager = BUFFER_MANAGER_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;
    vbo_range_t tmp_buffer_range = { 0 };
    bool vbo_written = false;
    size_t write_size = 0;

    point_mesh_geometry_t* geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_import_from_vertices", "vertices_")

    ret_geometry = point_mesh_geometry_create_from_vertices(vertex_count_, vertices_, &geometry);
    if(GEOMETRY_SUCCESS != ret_geometry) {
        ret = resource_pipeline_result_convert_geometry(ret_geometry);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import point mesh geometry. reason=geometry_create_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_result_to_str(ret), resource_name_, vertex_count_);
        goto cleanup;
    }

    if(!size_mul_checked(vertex_count_, sizeof(point_vertex_t), &write_size)) {
        ret = RESOURCE_PIPELINE_OVERFLOW;
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - vbo write size overflow failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }
    ret_buffer_manager = vbo_manager_write(vbo_manager_, write_size, vertices_, &tmp_buffer_range.allocation_info);
    if(BUFFER_MANAGER_SUCCESS != ret_buffer_manager) {
        ret = resource_pipeline_result_convert_buffer_manager(ret_buffer_manager);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import untextured mesh geometry. reason=vbo_write_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_result_to_str(ret), resource_name_, vertex_count_);
        goto cleanup;
    }
    vbo_written = true;
    if(0 != (tmp_buffer_range.allocation_info.offset % sizeof(line_vertex_t))) {
        ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
        goto cleanup;
    }
    tmp_buffer_range.draw_range.first_vertex_count = tmp_buffer_range.allocation_info.offset / sizeof(line_vertex_t);
    tmp_buffer_range.draw_range.vertex_count = vertex_count_;

    ret_resource_registry = point_mesh_geometry_registry_register(geometry_registry_, resource_name_, &geometry, &tmp_buffer_range, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import point mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_result_to_str(ret), resource_name_, vertex_count_);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_DATA_CORRUPTED == ret) {
        return ret;
    }
    if(RESOURCE_PIPELINE_SUCCESS != ret && vbo_written) {
        ret_buffer_manager = vbo_manager_free(vbo_manager_, &tmp_buffer_range.allocation_info);
        if(BUFFER_MANAGER_SUCCESS != ret_buffer_manager) {
            ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
            ERROR_MESSAGE("point_mesh_geometry_pipeline_import_from_vertices(%s) - VBO rollback failed.", resource_pipeline_result_to_str(ret));
            return ret;
        }
    }
    point_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t point_mesh_geometry_pipeline_release(vbo_manager_t* vbo_manager_, point_mesh_geometry_registry_t* geometry_registry_, uint16_t geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_release", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "point_mesh_geometry_pipeline_release", "geometry_registry_")

    // unregisterに失敗した場合はgeometry_registry_は不変となる。そのため、vbo_freeの後でunregisterに失敗するとgeometry_registry_に解放済みallocationへの参照が残る。よってvbo_freeの前で実行する
    ret_resource_registry = point_mesh_geometry_registry_release(geometry_registry_, vbo_manager_, geometry_id_);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("point_mesh_geometry_pipeline_release(%s) - point_mesh_geometry_pipeline_release failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}
