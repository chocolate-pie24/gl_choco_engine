// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file line_mesh_geometry.c
 * @author chocolate-pie24
 * @brief line_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの実装
 *
 * @note line_mesh_shader: 複数の線分を描画する。色情報はuniform変数で扱い、RGB(4byte目はpadding)で指定する。このため、全ての線分が指定した色で描画される
 *
 * @date 2026-06-04
 *
 */
#include "engine/resource/geometry/line_mesh_geometry.h"

#include <stddef.h>
#include <stdint.h>

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

#include "engine/containers/choco_string.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/geometry_primitive_err_utils.h"
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

resource_result_t line_mesh_geometry_default_create(line_mesh_geometry_t** geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    line_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_default_create", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_default_create", "*geometry_")

    ret_mem = memory_system_allocate(sizeof(line_mesh_geometry_t), MEMORY_TAG_GEOMETRY, (void**)&tmp_geometry);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("line_mesh_geometry_default_create(%s) - Failed to allocate line_mesh_geometry_t instance.", resource_rslt_to_str(ret));
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

resource_result_t line_mesh_geometry_create_from_vertices(const char* name_, size_t vertex_count_, const line_vertex_t* vertices_, line_mesh_geometry_t** geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    line_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_vertices", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_vertices", "*geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_vertices", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_vertices", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_vertices", "vertices_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_vertices", "vertex_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (vertex_count_ % 2), ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_vertices", "vertex_count_")

    ret = line_mesh_geometry_default_create(&tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_geometry_create_from_vertices(%s) - Failed to create line_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = line_mesh_geometry_initialize_from_vertices(name_, vertex_count_, vertices_, tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_geometry_create_from_vertices(%s) - Failed to initialize line_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *geometry_ = tmp_geometry;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        line_mesh_geometry_destroy(&tmp_geometry);
    }
    return ret;
}

resource_result_t line_mesh_geometry_create_from_aabbs(const char* name_, size_t aabb_count_, const aabb_3d_t* aabbs_, line_mesh_geometry_t** geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    line_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_aabbs", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_aabbs", "*geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_aabbs", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_aabbs", "name_[0]")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != aabb_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_aabbs", "aabb_count_")
    IF_ARG_NULL_GOTO_CLEANUP(aabbs_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_create_from_aabbs", "aabbs_")

    ret = line_mesh_geometry_default_create(&tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_geometry_create_from_aabbs(%s) - Failed to create line_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = line_mesh_geometry_initialize_from_aabbs(name_, aabb_count_, aabbs_, tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_geometry_create_from_aabbs(%s) - Failed to initialize line_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *geometry_ = tmp_geometry;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        line_mesh_geometry_destroy(&tmp_geometry);
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
        ERROR_MESSAGE("line_mesh_geometry_destroy(%s) - line_mesh_geometry internal state is inconsistent: vertex_count is not a multiple of 2. CPU-side vertex array was not freed because allocation size cannot be trusted.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
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
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_vertices", "name_[0]")
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

resource_result_t line_mesh_geometry_initialize_from_aabbs(const char* name_, size_t aabb_count_, const aabb_3d_t* aabbs_, line_mesh_geometry_t* geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    geometry_primitive_result_t ret_geometry = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;

    choco_string_t* tmp_name = NULL;
    line_vertex_t* tmp_vertices = NULL;

    size_t vertex_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_aabbs", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_aabbs", "name_[0]")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != aabb_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_aabbs", "aabb_count_")
    IF_ARG_NULL_GOTO_CLEANUP(aabbs_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_aabbs", "aabbs_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_initialize_from_aabbs", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_initialize_from_aabbs", "geometry_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_initialize_from_aabbs", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "line_mesh_geometry_initialize_from_aabbs", "geometry_->vertex_count")

    ret_string = choco_string_create_from_c_string(name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("line_mesh_geometry_initialize_from_aabbs(%s) - Failed to create line mesh geometry name string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    // AABB 1個につき12本の線分 -> AABB 1個につき頂点は24個
    if((SIZE_MAX / 24) < aabb_count_) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("line_mesh_geometry_initialize_from_aabbs(%s) - CPU-side vertex array size overflow. aabb_count = %zu.", resource_rslt_to_str(ret), aabb_count_);
        goto cleanup;
    }
    vertex_count = aabb_count_ * 24;
    if((SIZE_MAX / vertex_count) < sizeof(line_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("line_mesh_geometry_initialize_from_aabbs(%s) - CPU-side vertex array size overflow. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count, sizeof(line_vertex_t));
        goto cleanup;
    }
    ret_mem = memory_system_allocate(sizeof(line_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("line_mesh_geometry_initialize_from_aabbs(%s) - Failed to allocate CPU-side vertex array. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count, sizeof(line_vertex_t));
        goto cleanup;
    }

    for(size_t i = 0, ii = 0; i != aabb_count_; ++i, ii += 24) {
        vec3f_t aabb_vertices[8] = { 0 };
        ret_geometry = aabb_3d_vertices_get(&aabbs_[i], aabb_vertices);
        if(GEOMETRY_PRIMITIVE_SUCCESS != ret_geometry) {
            ret = resource_rslt_convert_geometry_primitive(ret_geometry);
            ERROR_MESSAGE("line_mesh_geometry_initialize_from_aabbs(%s) - Failed to get AABB vertices from aabbs_[%zu]. aabb_3d_vertices_get() returned %s.", resource_rslt_to_str(ret), i, geometry_primitive_rslt_to_str(ret_geometry));
            goto cleanup;
        }

        tmp_vertices[ii].position = aabb_vertices[0]; tmp_vertices[ii + 1].position = aabb_vertices[1]; // p0 - p1
        tmp_vertices[ii + 2].position = aabb_vertices[1]; tmp_vertices[ii + 3].position = aabb_vertices[2]; // p1 - p2
        tmp_vertices[ii + 4].position = aabb_vertices[2]; tmp_vertices[ii + 5].position = aabb_vertices[3]; // p2 - p3
        tmp_vertices[ii + 6].position = aabb_vertices[3]; tmp_vertices[ii + 7].position = aabb_vertices[0]; // p3 - p0

        tmp_vertices[ii + 8].position = aabb_vertices[0]; tmp_vertices[ii + 9].position = aabb_vertices[4]; // p0 - p4
        tmp_vertices[ii + 10].position = aabb_vertices[1]; tmp_vertices[ii + 11].position = aabb_vertices[5]; // p1 - p5
        tmp_vertices[ii + 12].position = aabb_vertices[2]; tmp_vertices[ii + 13].position = aabb_vertices[6]; // p2 - p6
        tmp_vertices[ii + 14].position = aabb_vertices[3]; tmp_vertices[ii + 15].position = aabb_vertices[7]; // p3 - p7

        tmp_vertices[ii + 16].position = aabb_vertices[4]; tmp_vertices[ii + 17].position = aabb_vertices[5]; // p4 - p5
        tmp_vertices[ii + 18].position = aabb_vertices[5]; tmp_vertices[ii + 19].position = aabb_vertices[6]; // p5 - p6
        tmp_vertices[ii + 20].position = aabb_vertices[6]; tmp_vertices[ii + 21].position = aabb_vertices[7]; // p6 - p7
        tmp_vertices[ii + 22].position = aabb_vertices[7]; tmp_vertices[ii + 23].position = aabb_vertices[4]; // p7 - p4
    }

    geometry_->name = tmp_name;
    geometry_->vertex_count = vertex_count;
    geometry_->vertices = tmp_vertices;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_name) {
            choco_string_destroy(&tmp_name);
        }
        if(NULL != tmp_vertices) {
            memory_system_free(tmp_vertices, sizeof(line_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
            tmp_vertices = NULL;
        }
    }
    return ret;
}

void line_mesh_geometry_deinitialize(line_mesh_geometry_t* geometry_) {
    if(NULL == geometry_) {
        return;
    }
    if(NULL != geometry_->name) {
        choco_string_destroy(&geometry_->name);
    }
    if(NULL != geometry_->vertices && 0 == geometry_->vertex_count) {
        ERROR_MESSAGE("line_mesh_geometry_deinitialize(%s) - line_mesh_geometry internal state is inconsistent: vertices is not NULL but vertex_count is 0. CPU-side vertex array was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != geometry_->vertices && 0 != (geometry_->vertex_count % 2)) {
        ERROR_MESSAGE("line_mesh_geometry_deinitialize(%s) - line_mesh_geometry internal state is inconsistent: vertex_count is not a multiple of 2.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != geometry_->vertices) {
        memory_system_free(geometry_->vertices, sizeof(line_vertex_t) * geometry_->vertex_count, MEMORY_TAG_GEOMETRY);
        geometry_->vertices = NULL;
        geometry_->vertex_count = 0;
    }
}

resource_result_t line_mesh_geometry_clone(const line_mesh_geometry_t* src_, line_mesh_geometry_t** out_geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    line_mesh_geometry_t* tmp_geometry = NULL;
    const char* tmp_name = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_clone", "src_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_clone", "out_geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "line_mesh_geometry_clone", "*out_geometry_")

    // 内部データチェック
    if(0 == src_->vertex_count && NULL != src_->vertices) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count is 0 but vertices is not NULL.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(0 != src_->vertex_count && NULL == src_->name) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count != 0, but geometry name is NULL.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(0 != src_->vertex_count && NULL == src_->vertices) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count != 0, but vertices = NULL.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(0 != (src_->vertex_count % 2)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count is not multiple of 2.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(0 == choco_string_length(src_->name) && 0 != src_->vertex_count) {    // src_->name == NULL or src_->nameが空
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("line_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count != 0, but geometry name is empty.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    // clone生成
    ret = line_mesh_geometry_default_create(&tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("line_mesh_geometry_clone(%s) - Failed to create empty clone instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 != src_->vertex_count) {
        tmp_name = choco_string_c_str(src_->name);
        ret = line_mesh_geometry_initialize_from_vertices(tmp_name, src_->vertex_count, src_->vertices, tmp_geometry);
        if(RESOURCE_OVERFLOW == ret) {
            ret = RESOURCE_DATA_CORRUPTED;
            ERROR_MESSAGE("line_mesh_geometry_clone(%s) - src_ internal state is corrupted: overflow occurred while deep-copying name or vertices.", resource_rslt_to_str(ret));
            goto cleanup;
        } else if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("line_mesh_geometry_clone(%s) - Failed to initialize clone instance from src_ geometry data.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    }

    *out_geometry_ = tmp_geometry;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        line_mesh_geometry_destroy(&tmp_geometry);
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
