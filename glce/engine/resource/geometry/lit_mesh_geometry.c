/** @ingroup resource
 *
 * @file lit_mesh_geometry.c
 * @author chocolate-pie24
 * @brief lit_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの実装
 * 
 * @note lit_mesh_shader: 光源・法線・材質色などを使って、陰影付きでmeshを描画するためのシェーダー
 *
 * @todo カバレッジ改善
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
#include "engine/resource/geometry/lit_mesh_geometry.h"

#include <stddef.h>
#include <stdint.h>

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

#include "engine/containers/choco_string.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief lit_mesh_geometry内部状態管理構造体
 *
 */
struct lit_mesh_geometry {
    choco_string_t* name;               /**< lit_mesh_geometry CPU側リソース名称 */

    size_t vertex_count;                /**< lit_mesh_geometryが所有する頂点数 */
    point_normal_vertex_t* vertices;    /**< lit_mesh_geometryが所有する頂点配列 */
};

resource_result_t lit_mesh_geometry_default_create(lit_mesh_geometry_t** geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    lit_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_default_create", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_default_create", "*geometry_")

    ret_mem = memory_system_allocate(sizeof(lit_mesh_geometry_t), MEMORY_TAG_GEOMETRY, (void**)&tmp_geometry);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("lit_mesh_geometry_default_create(%s) - Failed to allocate lit_mesh_geometry_t instance.", resource_rslt_to_str(ret));
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
            memory_system_free(tmp_geometry, sizeof(lit_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
            tmp_geometry = NULL;
        }
    }
    return ret;
}

resource_result_t lit_mesh_geometry_create(const char* name_, size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t** geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    lit_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "*geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "vertices_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "vertex_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (vertex_count_ % 3), ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "name_[0]")

    ret = lit_mesh_geometry_default_create(&tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_geometry_create(%s) - Failed to create lit_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = lit_mesh_geometry_initialize(name_, vertex_count_, vertices_, tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_geometry_create(%s) - Failed to initialize lit_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *geometry_ = tmp_geometry;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        lit_mesh_geometry_destroy(&tmp_geometry);
    }
    return ret;
}

void lit_mesh_geometry_destroy(lit_mesh_geometry_t** geometry_) {
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
        ERROR_MESSAGE("lit_mesh_geometry_destroy(%s) - lit_mesh_geometry internal state is inconsistent: vertices is not NULL but vertex_count is 0. CPU-side vertex array was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != (*geometry_)->vertices && 0 != ((*geometry_)->vertex_count % 3)) {
        ERROR_MESSAGE("lit_mesh_geometry_destroy(%s) - lit_mesh_geometry internal state is inconsistent: vertex_count is not a multiple of 3.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != (*geometry_)->vertices) {
        memory_system_free((*geometry_)->vertices, sizeof(point_normal_vertex_t) * (*geometry_)->vertex_count, MEMORY_TAG_GEOMETRY);
        (*geometry_)->vertices = NULL;
        (*geometry_)->vertex_count = 0;
    }

    memory_system_free(*geometry_, sizeof(lit_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
    *geometry_ = NULL;
}

resource_result_t lit_mesh_geometry_initialize(const char* name_, size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t* geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    choco_string_t* tmp_name = NULL;
    point_normal_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize", "name_[0]")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize", "vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize", "geometry_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize", "geometry_->vertex_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (vertex_count_ % 3), ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize", "vertex_count_")

    ret_string = choco_string_create_from_c_string(name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("lit_mesh_geometry_initialize(%s) - Failed to create lit mesh geometry name string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX / vertex_count_) < sizeof(point_normal_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("lit_mesh_geometry_initialize(%s) - CPU-side vertex array size overflow. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(point_normal_vertex_t));
        goto cleanup;
    }
    ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("lit_mesh_geometry_initialize(%s) - Failed to allocate CPU-side vertex array. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(point_normal_vertex_t));
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
            memory_system_free(tmp_vertices, sizeof(point_normal_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY);
            tmp_vertices = NULL;
        }
    }
    return ret;
}

void lit_mesh_geometry_deinitialize(lit_mesh_geometry_t* geometry_) {
    if(NULL == geometry_) {
        return;
    }
    if(NULL != geometry_->name) {
        choco_string_destroy(&geometry_->name);
    }
    if(NULL != geometry_->vertices && 0 == geometry_->vertex_count) {
        ERROR_MESSAGE("lit_mesh_geometry_deinitialize(%s) - lit_mesh_geometry internal state is inconsistent: vertices is not NULL but vertex_count is 0. CPU-side vertex array was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != geometry_->vertices && 0 != (geometry_->vertex_count % 3)) {
        ERROR_MESSAGE("lit_mesh_geometry_deinitialize(%s) - lit_mesh_geometry internal state is inconsistent: vertex_count is not a multiple of 3. CPU-side vertex array was not freed because allocation size cannot be trusted.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != geometry_->vertices) {
        memory_system_free(geometry_->vertices, sizeof(point_normal_vertex_t) * geometry_->vertex_count, MEMORY_TAG_GEOMETRY);
        geometry_->vertices = NULL;
        geometry_->vertex_count = 0;
    }
}

resource_result_t lit_mesh_geometry_clone(const lit_mesh_geometry_t* src_, lit_mesh_geometry_t** out_geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    lit_mesh_geometry_t* tmp_geometry = NULL;
    const char* tmp_name = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_clone", "src_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_clone", "out_geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_clone", "*out_geometry_")

    // 内部データチェック
    if(0 == src_->vertex_count && NULL != src_->vertices) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count is 0 but vertices is not NULL.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(0 != src_->vertex_count && NULL == src_->name) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count != 0, but geometry name is NULL.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(0 != src_->vertex_count && NULL == src_->vertices) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count != 0, but vertices = NULL.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(0 != (src_->vertex_count % 3)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count is not multiple of 3.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(0 == choco_string_length(src_->name) && 0 != src_->vertex_count) {    // src_->name == NULL or src_->nameが空
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_clone(%s) - src_ internal state is corrupted: vertex count != 0, but geometry name is empty.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    // clone生成
    ret = lit_mesh_geometry_default_create(&tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_geometry_clone(%s) - Failed to create empty clone instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 != src_->vertex_count) {
        tmp_name = choco_string_c_str(src_->name);
        ret = lit_mesh_geometry_initialize(tmp_name, src_->vertex_count, src_->vertices, tmp_geometry);
        if(RESOURCE_OVERFLOW == ret) {
            ret = RESOURCE_DATA_CORRUPTED;
            ERROR_MESSAGE("lit_mesh_geometry_clone(%s) - src_ internal state is corrupted: overflow occurred while deep-copying name or vertices.", resource_rslt_to_str(ret));
            goto cleanup;
        } else if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("lit_mesh_geometry_clone(%s) - Failed to initialize clone instance from src_ geometry data.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    }

    *out_geometry_ = tmp_geometry;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        lit_mesh_geometry_destroy(&tmp_geometry);
    }
    return ret;
}

const char* lit_mesh_geometry_name_get(const lit_mesh_geometry_t* geometry_) {
    if(NULL == geometry_) {
        return NULL;
    }
    if(NULL == geometry_->name) {
        return NULL;
    }
    return choco_string_c_str(geometry_->name);
}

resource_result_t lit_mesh_geometry_vertices_get(const lit_mesh_geometry_t* geometry_, const point_normal_vertex_t** out_vertices_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertices_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertices_get", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertices_get", "*out_vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertices_get", "geometry_->name")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertices_get", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertices_get", "geometry_->vertex_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (geometry_->vertex_count % 3), ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "lit_mesh_geometry_vertices_get", "geometry_->vertex_count")

    *out_vertices_ = geometry_->vertices;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t lit_mesh_geometry_vertex_count_get(const lit_mesh_geometry_t* geometry_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertex_count_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertex_count_get", "out_vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertex_count_get", "geometry_->name")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertex_count_get", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertex_count_get", "geometry_->vertex_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (geometry_->vertex_count % 3), ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "lit_mesh_geometry_vertex_count_get", "geometry_->vertex_count")

    *out_vertex_count_ = geometry_->vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}
