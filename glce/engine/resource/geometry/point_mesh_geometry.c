// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file point_mesh_geometry.c
 * @author chocolate-pie24
 * @brief point_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの実装
 *
 * @note point_mesh_shader: 複数の点を描画する
 * @note point_mesh_geometryは点群の幾何情報のみを保持し、色情報はpoint_mesh_geometryを保持する親構造体で扱う
 *
 * @todo TODO: pcdファイル等の点群ファイルからの初期化はそのうちやる
 *
 * @date 2026-06-06
 *
 */
#include "engine/resource/geometry/point_mesh_geometry.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief point_mesh_geometry内部状態管理構造体
 *
 */
struct point_mesh_geometry {
    size_t vertex_count;        /**< point_mesh_geometryが所有する頂点数 */
    point_vertex_t* vertices;    /**< point_mesh_geometryが所有する頂点配列 */
};

static resource_result_t initialize_from_vertices(point_mesh_geometry_t* geometry_, size_t vertex_count_, const point_vertex_t* vertices_);

resource_result_t point_mesh_geometry_create_from_vertices(size_t vertex_count_, const point_vertex_t* vertices_, point_mesh_geometry_t** out_geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    point_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "point_mesh_geometry_create_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "point_mesh_geometry_create_from_vertices", "out_geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_geometry_, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "point_mesh_geometry_create_from_vertices", "*out_geometry_")
    if(0 == vertex_count_) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("point_mesh_geometry_create_from_vertices(%s) - Provided vertex_count_ is not valid.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_memory_system = memory_system_allocate(sizeof(point_mesh_geometry_t), MEMORY_TAG_GEOMETRY, (void**)&tmp_geometry);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = resource_rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("point_mesh_geometry_create_from_vertices(%s) - Failed to allocate point_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_geometry, 0, sizeof(point_mesh_geometry_t));

    ret = initialize_from_vertices(tmp_geometry, vertex_count_, vertices_);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_geometry_create_from_vertices(%s) - Failed to initialize point_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *out_geometry_ = tmp_geometry;
    tmp_geometry = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != tmp_geometry) {
        point_mesh_geometry_destroy(&tmp_geometry);
    }
    return ret;
}

void point_mesh_geometry_destroy(point_mesh_geometry_t** geometry_) {
    if(NULL == geometry_) {
        return;
    }
    if(NULL == *geometry_) {
        return;
    }

    if(NULL != (*geometry_)->vertices && 0 == (*geometry_)->vertex_count) {
        ERROR_MESSAGE("point_mesh_geometry_destroy(%s) - point_mesh_geometry internal state is inconsistent: vertices is not NULL but vertex_count is 0. CPU-side vertex array was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != (*geometry_)->vertices) {
        memory_system_free((*geometry_)->vertices, sizeof(point_vertex_t) * (*geometry_)->vertex_count, MEMORY_TAG_GEOMETRY);
        (*geometry_)->vertices = NULL;
        (*geometry_)->vertex_count = 0;
    }

    memory_system_free(*geometry_, sizeof(point_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
    *geometry_ = NULL;
}

resource_result_t point_mesh_geometry_vertices_get(const point_mesh_geometry_t* geometry_, const point_vertex_t** out_vertices_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "point_mesh_geometry_vertices_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "point_mesh_geometry_vertices_get", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "point_mesh_geometry_vertices_get", "*out_vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "point_mesh_geometry_vertices_get", "geometry_->vertices")
    if(0 == geometry_->vertex_count) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("point_mesh_geometry_vertices_get(%s) - Provided geometry_ is not initialized.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *out_vertices_ = geometry_->vertices;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t point_mesh_geometry_vertex_count_get(const point_mesh_geometry_t* geometry_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "point_mesh_geometry_vertex_count_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "point_mesh_geometry_vertex_count_get", "out_vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "point_mesh_geometry_vertex_count_get", "geometry_->vertices")
    if(0 == geometry_->vertex_count) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("point_mesh_geometry_vertex_count_get(%s) - Provided geometry_ is not initialized.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *out_vertex_count_ = geometry_->vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

static resource_result_t initialize_from_vertices(point_mesh_geometry_t* geometry_, size_t vertex_count_, const point_vertex_t* vertices_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    point_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "initialize_from_vertices", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "initialize_from_vertices", "geometry_->vertices")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "initialize_from_vertices", "vertices_")
    if(0 == vertex_count_) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("initialize_from_vertices(%s) - Provided vertex_count_ is not valid.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 != geometry_->vertex_count) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("initialize_from_vertices(%s) - Provided geometry_ is already initialized.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX / vertex_count_) < sizeof(point_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("initialize_from_vertices(%s) - CPU-side vertex array size overflow. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(point_vertex_t));
        goto cleanup;
    }
    ret_memory_system = memory_system_allocate(sizeof(point_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = resource_rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("initialize_from_vertices(%s) - Failed to allocate CPU-side vertex array. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(point_vertex_t));
        goto cleanup;
    }

    for(size_t i = 0; i != vertex_count_; ++i) {
        tmp_vertices[i] = vertices_[i];
    }

    geometry_->vertex_count = vertex_count_;
    geometry_->vertices = tmp_vertices;
    tmp_vertices = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != tmp_vertices) {
        memory_system_free(tmp_vertices, sizeof(point_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY);
        tmp_vertices = NULL;
    }
    return ret;
}
