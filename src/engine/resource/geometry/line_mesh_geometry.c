/** @ingroup resource
 *
 * @file line_mesh_geometry.c
 * @author chocolate-pie24
 * @brief line_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの実装
 * 
 * @note line_mesh_shader: 複数の線分を描画する。色情報はuniform変数で扱い、RGBで指定する。このため、全ての線分が指定した色で描画される
 *
 * @version 0.1
 * @date 2026-06-04
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/resource/geometry/line_mesh_geometry.h"

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

#include "engine/containers/choco_string.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief line_mesh_geometry内部状態管理構造体
 *
 */
struct line_mesh_geometry {
    choco_string_t* name;       /**< line_mesh_geometry CPU側リソース名称 */

    size_t vertex_count;        /**< line_mesh_geometryが所有する頂点数(線分の端点をverticesには格納するので、必ず2の倍数) */
    line_vertex_t* vertices;    /**< line_mesh_geometryが所有する頂点配列(線分1-p1, 線分1-p2, 線分2-p1, 線分2-p2...) */
};

resource_result_t line_mesh_geometry_create(line_mesh_geometry_t** geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    line_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create", "*geometry_")

    ret_mem = memory_system_allocate(sizeof(line_mesh_geometry_t), MEMORY_TAG_GEOMETRY, (void**)&tmp_geometry);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("line_mesh_geometry_create(%s) - Failed to allocate line_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_geometry->name = NULL;
    tmp_geometry->vertex_count = 0;
    tmp_geometry->vertices = NULL;

    *geometry_ = tmp_geometry;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_geometry) {
            memory_system_free(tmp_geometry, sizeof(line_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
            tmp_geometry = NULL;
        }
    }
    return ret;
}

void line_mesh_geometry_destroy(line_mesh_geometry_t** geometry_) {
    if(NULL == geometry_) {
        return;
    }
    if(NULL == *geometry_) {
        return;
    }
    if(NULL != (*geometry_)->name) {
        choco_string_destroy(&(*geometry_)->name);
    }
    
    if(NULL != (*geometry_)->vertices && 0 == (*geometry_)->vertex_count) {
        ERROR_MESSAGE("line_mesh_geometry_destroy(%s) - line_mesh_geometry internal state is inconsistent: vertices is not NULL but vertex_count is 0. CPU-side vertex array was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != (*geometry_)->vertices && 0 != ((*geometry_)->vertex_count % 2)) {
        ERROR_MESSAGE("line_mesh_geometry_destroy(%s) - line_mesh_geometry internal state is inconsistent: vertex_count is not a multiple of 2.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != (*geometry_)->vertices) {
        memory_system_free((*geometry_)->vertices, sizeof(line_vertex_t) * (*geometry_)->vertex_count, MEMORY_TAG_GEOMETRY);
        (*geometry_)->vertices = NULL;
        (*geometry_)->vertex_count = 0;
    }

    memory_system_free(*geometry_, sizeof(line_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
    *geometry_ = NULL;
}

resource_result_t line_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const line_vertex_t* vertices_, line_mesh_geometry_t* geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    choco_string_t* tmp_name = NULL;
    line_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_vertices", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_vertices", "vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_vertices", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_initialize_from_vertices", "geometry_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_initialize_from_vertices", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_initialize_from_vertices", "geometry_->vertex_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (vertex_count_ % 2), ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_vertices", "vertex_count_")

    ret_string = choco_string_create_from_c_string(name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("line_mesh_geometry_initialize_from_vertices(%s) - Failed to create line mesh geometry name string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX / vertex_count_) < sizeof(line_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("line_mesh_geometry_initialize_from_vertices(%s) - CPU-side vertex array size overflow. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(line_vertex_t));
        goto cleanup;
    }
    ret_mem = memory_system_allocate(sizeof(line_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("line_mesh_geometry_initialize_from_vertices(%s) - Failed to allocate CPU-side vertex array. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(line_vertex_t));
        goto cleanup;
    }

    for(size_t i = 0; i != vertex_count_; ++i) {
        tmp_vertices[i] = vertices_[i];
    }

    geometry_->name = tmp_name;
    geometry_->vertex_count = vertex_count_;
    geometry_->vertices = tmp_vertices;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_name) {
            choco_string_destroy(&tmp_name);
        }
        if(NULL != tmp_vertices) {
            memory_system_free(tmp_vertices, sizeof(line_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY);
            tmp_vertices = NULL;
        }
    }
    return ret;
}

const char* line_mesh_geometry_name_get(const line_mesh_geometry_t* geometry_) {
    if(NULL == geometry_) {
        return NULL;
    }
    if(NULL == geometry_->name) {
        return NULL;
    }
    return choco_string_c_str(geometry_->name);
}

resource_result_t line_mesh_geometry_vertices_get(const line_mesh_geometry_t* geometry_, const line_vertex_t** out_vertices_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_vertices_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_vertices_get", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_vertices_get", "*out_vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_vertices_get", "geometry_->name")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_vertices_get", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_vertices_get", "geometry_->vertex_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (geometry_->vertex_count % 2), ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "line_mesh_geometry_vertices_get", "geometry_->vertex_count")

    *out_vertices_ = geometry_->vertices;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t line_mesh_geometry_vertex_count_get(const line_mesh_geometry_t* geometry_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_vertex_count_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_vertex_count_get", "out_vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_vertex_count_get", "geometry_->name")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_vertex_count_get", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_vertex_count_get", "geometry_->vertex_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (geometry_->vertex_count % 2), ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "line_mesh_geometry_vertex_count_get", "geometry_->vertex_count")

    *out_vertex_count_ = geometry_->vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}
