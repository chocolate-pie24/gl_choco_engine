/** @ingroup resource
 *
 * @file line_mesh_geometry.c
 * @author chocolate-pie24
 * @brief line_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの実装
 *
 * @note line_mesh_shader: 複数の線分を描画する。色情報はuniform変数で扱い、RGB(4byte目はpadding)で指定する。このため、全ての線分が指定した色で描画される
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

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "test_controller.h"

#include "engine/resource/geometry/test_line_mesh_geometry.h"

#include "engine/core/memory/test_choco_memory.h"

#include "engine/containers/test_choco_string.h"

#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_math/math_types.h"

// line_mesh_geometry用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_line_mesh_geometry_default_create;             /**< line_mesh_geometry_default_create()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_create_from_vertices;       /**< line_mesh_geometry_create_from_vertices()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_create_from_aabbs;          /**< line_mesh_geometry_create_from_aabbs()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_initialize_from_vertices;   /**< line_mesh_geometry_initialize_from_vertices()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_initialize_from_aabbs;      /**< line_mesh_geometry_initialize_from_aabbs()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_clone;                      /**< line_mesh_geometry_clone()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_name_get;                   /**< line_mesh_geometry_name_get()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_vertices_get;               /**< line_mesh_geometry_vertices_get()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_vertex_count_get;           /**< line_mesh_geometry_vertex_count_get()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_line_mesh_geometry_default_create(void);
static void test_line_mesh_geometry_create_from_vertices(void);
static void test_line_mesh_geometry_create_from_aabbs(void);
static void test_line_mesh_geometry_destroy(void);
static void test_line_mesh_geometry_initialize_from_vertices(void);
static void test_line_mesh_geometry_initialize_from_aabbs(void);
static void test_line_mesh_geometry_deinitialize(void);
static void test_line_mesh_geometry_clone(void);
static void test_line_mesh_geometry_name_get(void);
static void test_line_mesh_geometry_vertices_get(void);
static void test_line_mesh_geometry_vertex_count_get(void);

// テスト用ヘルパー関数

#endif

resource_result_t line_mesh_geometry_default_create(line_mesh_geometry_t** geometry_) {
#ifdef TEST_BUILD
    s_test_config_line_mesh_geometry_default_create.call_count++;
    if(s_test_config_line_mesh_geometry_default_create.fail_on_call != 0) {
        if(s_test_config_line_mesh_geometry_default_create.call_count == s_test_config_line_mesh_geometry_default_create.fail_on_call) {
            return (resource_result_t)s_test_config_line_mesh_geometry_default_create.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_line_mesh_geometry_create_from_vertices.call_count++;
    if(s_test_config_line_mesh_geometry_create_from_vertices.fail_on_call != 0) {
        if(s_test_config_line_mesh_geometry_create_from_vertices.call_count == s_test_config_line_mesh_geometry_create_from_vertices.fail_on_call) {
            return (resource_result_t)s_test_config_line_mesh_geometry_create_from_vertices.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_line_mesh_geometry_create_from_aabbs.call_count++;
    if(s_test_config_line_mesh_geometry_create_from_aabbs.fail_on_call != 0) {
        if(s_test_config_line_mesh_geometry_create_from_aabbs.call_count == s_test_config_line_mesh_geometry_create_from_aabbs.fail_on_call) {
            return (resource_result_t)s_test_config_line_mesh_geometry_create_from_aabbs.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_line_mesh_geometry_initialize_from_vertices.call_count++;
    if(s_test_config_line_mesh_geometry_initialize_from_vertices.fail_on_call != 0) {
        if(s_test_config_line_mesh_geometry_initialize_from_vertices.call_count == s_test_config_line_mesh_geometry_initialize_from_vertices.fail_on_call) {
            return (resource_result_t)s_test_config_line_mesh_geometry_initialize_from_vertices.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_line_mesh_geometry_initialize_from_aabbs.call_count++;
    if(s_test_config_line_mesh_geometry_initialize_from_aabbs.fail_on_call != 0) {
        if(s_test_config_line_mesh_geometry_initialize_from_aabbs.call_count == s_test_config_line_mesh_geometry_initialize_from_aabbs.fail_on_call) {
            return (resource_result_t)s_test_config_line_mesh_geometry_initialize_from_aabbs.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_line_mesh_geometry_clone.call_count++;
    if(s_test_config_line_mesh_geometry_clone.fail_on_call != 0) {
        if(s_test_config_line_mesh_geometry_clone.call_count == s_test_config_line_mesh_geometry_clone.fail_on_call) {
            return (resource_result_t)s_test_config_line_mesh_geometry_clone.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_line_mesh_geometry_vertices_get.call_count++;
    if(s_test_config_line_mesh_geometry_vertices_get.fail_on_call != 0) {
        if(s_test_config_line_mesh_geometry_vertices_get.call_count == s_test_config_line_mesh_geometry_vertices_get.fail_on_call) {
            return (resource_result_t)s_test_config_line_mesh_geometry_vertices_get.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_line_mesh_geometry_vertex_count_get.call_count++;
    if(s_test_config_line_mesh_geometry_vertex_count_get.fail_on_call != 0) {
        if(s_test_config_line_mesh_geometry_vertex_count_get.call_count == s_test_config_line_mesh_geometry_vertex_count_get.fail_on_call) {
            return (resource_result_t)s_test_config_line_mesh_geometry_vertex_count_get.forced_result;
        }
    }
#endif
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

#ifdef TEST_BUILD

void NO_COVERAGE test_line_mesh_geometry_default_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_default_create.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_default_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_line_mesh_geometry_create_from_vertices_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_create_from_vertices.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_create_from_vertices.forced_result = config_->forced_result;
}

void NO_COVERAGE test_line_mesh_geometry_create_from_aabbs_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_create_from_aabbs.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_create_from_aabbs.forced_result = config_->forced_result;
}

void NO_COVERAGE test_line_mesh_geometry_initialize_from_vertices_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_initialize_from_vertices.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_initialize_from_vertices.forced_result = config_->forced_result;
}

void NO_COVERAGE test_line_mesh_geometry_initialize_from_aabbs_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_initialize_from_aabbs.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_initialize_from_aabbs.forced_result = config_->forced_result;
}

void NO_COVERAGE test_line_mesh_geometry_clone_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_clone.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_clone.forced_result = config_->forced_result;
}

void NO_COVERAGE test_line_mesh_geometry_vertices_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_vertices_get.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_vertices_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_line_mesh_geometry_vertex_count_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_vertex_count_get.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_vertex_count_get.forced_result = config_->forced_result;
}


void NO_COVERAGE test_line_mesh_geometry_config_reset(void) {
    test_call_control_reset(&s_test_config_line_mesh_geometry_default_create);
    test_call_control_reset(&s_test_config_line_mesh_geometry_create_from_vertices);
    test_call_control_reset(&s_test_config_line_mesh_geometry_create_from_aabbs);
    test_call_control_reset(&s_test_config_line_mesh_geometry_initialize_from_vertices);
    test_call_control_reset(&s_test_config_line_mesh_geometry_initialize_from_aabbs);
    test_call_control_reset(&s_test_config_line_mesh_geometry_clone);
    test_call_control_reset(&s_test_config_line_mesh_geometry_name_get);
    test_call_control_reset(&s_test_config_line_mesh_geometry_vertices_get);
    test_call_control_reset(&s_test_config_line_mesh_geometry_vertex_count_get);
}

void NO_COVERAGE test_line_mesh_geometry(void) {
    test_line_mesh_geometry_default_create();
    test_line_mesh_geometry_create_from_vertices();
    test_line_mesh_geometry_create_from_aabbs();
    test_line_mesh_geometry_destroy();
    test_line_mesh_geometry_initialize_from_vertices();
    test_line_mesh_geometry_initialize_from_aabbs();
    test_line_mesh_geometry_deinitialize();
    test_line_mesh_geometry_clone();
    test_line_mesh_geometry_name_get();
    test_line_mesh_geometry_vertices_get();
    test_line_mesh_geometry_vertex_count_get();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_default_create(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // line_mesh_geometry_default_create() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        s_test_config_line_mesh_geometry_default_create.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_default_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t dummy_geometry = { 0 };
        line_mesh_geometry_t* geometry = &dummy_geometry;

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_geometry == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: line_mesh_geometry_t が確保され、全フィールドが未初期化状態で初期化される
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_create_from_vertices(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // line_mesh_geometry_create_from_vertices() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        s_test_config_line_mesh_geometry_create_from_vertices.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_create_from_vertices.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t dummy_geometry = { 0 };
        line_mesh_geometry_t* geometry = &dummy_geometry;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_geometry == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // line_mesh_geometry_default_create() が失敗 -> その戻り値を返し、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        s_test_config_line_mesh_geometry_default_create.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_default_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // line_mesh_geometry_default_create() 内部のmemory_system_allocate()が失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // line_mesh_geometry_initialize_from_vertices() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        s_test_config_line_mesh_geometry_initialize_from_vertices.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_initialize_from_vertices.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = line_mesh_geometry_create_from_vertices(NULL, 2U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ == 0 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 0U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, NULL, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ が2の倍数ではない -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[3] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // choco_string_create_from_c_string() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列用memory_system_allocate() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        // 1回目のallocateはline_mesh_geometry_t本体、2回目はchoco_string_t本体、3回目は文字列バッファ、4回目がtmp_vertices用
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        config.fail_on_call = 4U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列サイズoverflow -> RESOURCE_OVERFLOW
        // SIZE_MAXは奇数のため、2の倍数チェックを通すためにSIZE_MAX - 1Uを使う
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t dummy_vertex = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        dummy_vertex.position = vec3f_initialize(0.0f, 1.0f, 2.0f);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", SIZE_MAX - 1U, &dummy_vertex, &geometry);
        assert(RESOURCE_OVERFLOW == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: line_mesh_geometry_tを生成し、vertices_をdeep copyしてgeometryが所有する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(2U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry", choco_string_c_str(geometry->name)));

        assert(vertices != geometry->vertices);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(2.0f == geometry->vertices[0].position.elem[2]);

        assert(3.0f == geometry->vertices[1].position.elem[0]);
        assert(4.0f == geometry->vertices[1].position.elem[1]);
        assert(5.0f == geometry->vertices[1].position.elem[2]);

        // 元配列を書き換えてもgeometry側には影響しない
        vertices[0].position = vec3f_initialize(100.0f, 100.0f, 100.0f);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(2.0f == geometry->vertices[0].position.elem[2]);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 複数線分の頂点配列をdeep copyする
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[4] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[3].position = vec3f_initialize(9.0f, 10.0f, 11.0f);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 4U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(4U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry", choco_string_c_str(geometry->name)));

        assert(vertices != geometry->vertices);

        for(size_t i = 0; i != 4U; ++i) {
            assert(vertices[i].position.elem[0] == geometry->vertices[i].position.elem[0]);
            assert(vertices[i].position.elem[1] == geometry->vertices[i].position.elem[1]);
            assert(vertices[i].position.elem[2] == geometry->vertices[i].position.elem[2]);
        }

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_create_from_aabbs(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // line_mesh_geometry_create_from_aabbs() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        s_test_config_line_mesh_geometry_create_from_aabbs.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_create_from_aabbs.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t dummy_geometry = { 0 };
        line_mesh_geometry_t* geometry = &dummy_geometry;
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_geometry == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // line_mesh_geometry_default_create() が失敗 -> その戻り値を返し、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        s_test_config_line_mesh_geometry_default_create.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_default_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // line_mesh_geometry_default_create() 内部のmemory_system_allocate()が失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // line_mesh_geometry_initialize_from_aabbs() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        s_test_config_line_mesh_geometry_initialize_from_aabbs.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_initialize_from_aabbs.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_create_from_aabbs(NULL, 1U, aabbs, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabb_count_ == 0 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 0U, aabbs, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabbs_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, NULL, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // choco_string_create_from_c_string() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabb_count_ * 24 の計算でoverflow -> RESOURCE_OVERFLOW
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t dummy_aabb = { 0 };
        const size_t aabb_count = (SIZE_MAX / 24U) + 1U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        dummy_aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        dummy_aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", aabb_count, &dummy_aabb, &geometry);
        assert(RESOURCE_OVERFLOW == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列確保サイズの計算でoverflow -> RESOURCE_OVERFLOW
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t dummy_aabb = { 0 };
        const size_t aabb_count = SIZE_MAX / 24U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        dummy_aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        dummy_aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", aabb_count, &dummy_aabb, &geometry);
        assert(RESOURCE_OVERFLOW == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列用memory_system_allocate() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        // 1回目のallocateはline_mesh_geometry_t本体、2回目はchoco_string_t本体、3回目は文字列バッファ、4回目がtmp_vertices用
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        config.fail_on_call = 4U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabbs_[0] が不正状態 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        aabbs[0].max = vec3f_initialize( 1.0f,  2.0f,  3.0f);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabbs_[1] が不正状態 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        aabbs[1].min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        aabbs[1].max = vec3f_initialize( 1.0f,  2.0f,  3.0f);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 2U, aabbs, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 1個のAABBから24頂点のline mesh geometryを生成する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };
        vec3f_t expected[24] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        expected[0]  = vec3f_initialize(-1.0f, -2.0f,  6.0f);
        expected[1]  = vec3f_initialize( 4.0f, -2.0f,  6.0f);
        expected[2]  = vec3f_initialize( 4.0f, -2.0f,  6.0f);
        expected[3]  = vec3f_initialize( 4.0f, -2.0f, -3.0f);
        expected[4]  = vec3f_initialize( 4.0f, -2.0f, -3.0f);
        expected[5]  = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        expected[6]  = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        expected[7]  = vec3f_initialize(-1.0f, -2.0f,  6.0f);

        expected[8]  = vec3f_initialize(-1.0f, -2.0f,  6.0f);
        expected[9]  = vec3f_initialize(-1.0f,  5.0f,  6.0f);
        expected[10] = vec3f_initialize( 4.0f, -2.0f,  6.0f);
        expected[11] = vec3f_initialize( 4.0f,  5.0f,  6.0f);
        expected[12] = vec3f_initialize( 4.0f, -2.0f, -3.0f);
        expected[13] = vec3f_initialize( 4.0f,  5.0f, -3.0f);
        expected[14] = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        expected[15] = vec3f_initialize(-1.0f,  5.0f, -3.0f);

        expected[16] = vec3f_initialize(-1.0f,  5.0f,  6.0f);
        expected[17] = vec3f_initialize( 4.0f,  5.0f,  6.0f);
        expected[18] = vec3f_initialize( 4.0f,  5.0f,  6.0f);
        expected[19] = vec3f_initialize( 4.0f,  5.0f, -3.0f);
        expected[20] = vec3f_initialize( 4.0f,  5.0f, -3.0f);
        expected[21] = vec3f_initialize(-1.0f,  5.0f, -3.0f);
        expected[22] = vec3f_initialize(-1.0f,  5.0f, -3.0f);
        expected[23] = vec3f_initialize(-1.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(24U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry", choco_string_c_str(geometry->name)));

        for(size_t i = 0; i != 24U; ++i) {
            assert(expected[i].elem[0] == geometry->vertices[i].position.elem[0]);
            assert(expected[i].elem[1] == geometry->vertices[i].position.elem[1]);
            assert(expected[i].elem[2] == geometry->vertices[i].position.elem[2]);
        }

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2個のAABBから48頂点のline mesh geometryを生成する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        aabbs[1].min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        aabbs[1].max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = line_mesh_geometry_create_from_aabbs("test_geometry_2", 2U, aabbs, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(48U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry_2", choco_string_c_str(geometry->name)));

        // 1個目AABBの先頭線分: p0 - p1
        assert(-1.0f == geometry->vertices[0].position.elem[0]);
        assert(-2.0f == geometry->vertices[0].position.elem[1]);
        assert( 6.0f == geometry->vertices[0].position.elem[2]);

        assert( 4.0f == geometry->vertices[1].position.elem[0]);
        assert(-2.0f == geometry->vertices[1].position.elem[1]);
        assert( 6.0f == geometry->vertices[1].position.elem[2]);

        // 2個目AABBの先頭線分: p0 - p1
        assert(10.0f == geometry->vertices[24].position.elem[0]);
        assert(20.0f == geometry->vertices[24].position.elem[1]);
        assert(60.0f == geometry->vertices[24].position.elem[2]);

        assert(40.0f == geometry->vertices[25].position.elem[0]);
        assert(20.0f == geometry->vertices[25].position.elem[1]);
        assert(60.0f == geometry->vertices[25].position.elem[2]);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_destroy(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // geometry_ == NULL -> 何もせずreturn
        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        line_mesh_geometry_destroy(NULL);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ == NULL -> 何もせずreturn
        line_mesh_geometry_t* geometry = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: name == NULL, vertices == NULL, vertex_count == 0 のgeometry本体だけを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);
        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: nameのみを保持するgeometryを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: nameとverticesを保持するgeometryを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t* vertices = NULL;
        const size_t vertex_count = 2U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        ret_mem = memory_system_allocate(sizeof(line_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry->vertices = vertices;
        geometry->vertex_count = vertex_count;

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count == 0 の場合、verticesはdestroy側ではfreeされない
        // テスト側で後始末する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t* leaked_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        ret_mem = memory_system_allocate(sizeof(line_vertex_t), MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry->vertices = leaked_vertices;
        geometry->vertex_count = 0U;

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        memory_system_free(leaked_vertices, sizeof(line_vertex_t), MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count が2の倍数ではない場合、verticesはdestroy側ではfreeされない
        // テスト側で後始末する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t* leaked_vertices = NULL;
        const size_t vertex_count = 3U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        ret_mem = memory_system_allocate(sizeof(line_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry->vertices = leaked_vertices;
        geometry->vertex_count = vertex_count;

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        memory_system_free(leaked_vertices, sizeof(line_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 二重destroy相当: 1回目でNULL化され、2回目は何もせずreturn
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_initialize_from_vertices(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // line_mesh_geometry_initialize_from_vertices() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_line_mesh_geometry_initialize_from_vertices.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_initialize_from_vertices.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_initialize_from_vertices(NULL, 2U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ == 0 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 0U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ が2の倍数ではない -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t vertices[3] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, NULL, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("already_initialized", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL != geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t vertices[2] = { 0 };
        line_vertex_t dummy_vertices[2] = { 0 };

        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 0U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count != 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t vertices[2] = { 0 };

        geometry.name = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 2U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(2U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // choco_string_create_from_c_string() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t vertices[2] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列サイズoverflow -> RESOURCE_OVERFLOW
        // SIZE_MAXは奇数のため、2の倍数チェックを通すためにSIZE_MAX - 1Uを使う
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t dummy_vertex = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", SIZE_MAX - 1U, &dummy_vertex, &geometry);
        assert(RESOURCE_OVERFLOW == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列用memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        // 1回目のallocateはchoco_string_t本体、2回目は文字列バッファ、3回目がtmp_vertices用
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t vertices[2] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 3U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: vertices_をdeep copyしてgeometryが所有する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(2U == geometry->vertex_count);

        assert(0 == strcmp("test_geometry", choco_string_c_str(geometry->name)));

        assert(vertices != geometry->vertices);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(2.0f == geometry->vertices[0].position.elem[2]);

        assert(3.0f == geometry->vertices[1].position.elem[0]);
        assert(4.0f == geometry->vertices[1].position.elem[1]);
        assert(5.0f == geometry->vertices[1].position.elem[2]);

        // 元配列を書き換えてもgeometry側には影響しない
        vertices[0].position = vec3f_initialize(100.0f, 100.0f, 100.0f);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(2.0f == geometry->vertices[0].position.elem[2]);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void test_line_mesh_geometry_initialize_from_aabbs(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // line_mesh_geometry_initialize_from_aabbs() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        s_test_config_line_mesh_geometry_initialize_from_aabbs.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_initialize_from_aabbs.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_initialize_from_aabbs(NULL, 1U, aabbs, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabb_count_ == 0 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 0U, aabbs, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabbs_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, NULL, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, aabbs, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret_string = choco_string_create_from_c_string("already_initialized", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL != geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t dummy_vertices[2] = { 0 };
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 0U;

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count != 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        geometry.name = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 24U;

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(24U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // choco_string_create_from_c_string() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t aabbs[1] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabb_count_ * 24 の計算でoverflow -> RESOURCE_OVERFLOW
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t dummy_aabb = { 0 };
        const size_t aabb_count = (SIZE_MAX / 24U) + 1U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        dummy_aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        dummy_aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", aabb_count, &dummy_aabb, &geometry);
        assert(RESOURCE_OVERFLOW == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列確保サイズの計算でoverflow -> RESOURCE_OVERFLOW
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t dummy_aabb = { 0 };
        const size_t aabb_count = SIZE_MAX / 24U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        dummy_aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        dummy_aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", aabb_count, &dummy_aabb, &geometry);
        assert(RESOURCE_OVERFLOW == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列用memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        // 1回目のallocateはchoco_string_t本体、2回目は文字列バッファ、3回目がtmp_vertices用
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t aabbs[1] = { 0 };
        test_call_control_t config = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        config.fail_on_call = 3U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabbs_[0] が不正状態 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t aabbs[1] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        aabbs[0].max = vec3f_initialize( 1.0f,  2.0f,  3.0f);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, aabbs, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // aabbs_[1] が不正状態 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        aabb_3d_t aabbs[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        aabbs[1].min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        aabbs[1].max = vec3f_initialize( 1.0f,  2.0f,  3.0f);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 2U, aabbs, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 1個のAABBから24頂点のline mesh geometryを生成する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[1] = { 0 };
        vec3f_t expected[24] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        expected[0] = vec3f_initialize(-1.0f, -2.0f,  6.0f);
        expected[1] = vec3f_initialize( 4.0f, -2.0f,  6.0f);
        expected[2] = vec3f_initialize( 4.0f, -2.0f,  6.0f);
        expected[3] = vec3f_initialize( 4.0f, -2.0f, -3.0f);
        expected[4] = vec3f_initialize( 4.0f, -2.0f, -3.0f);
        expected[5] = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        expected[6] = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        expected[7] = vec3f_initialize(-1.0f, -2.0f,  6.0f);

        expected[8] = vec3f_initialize(-1.0f, -2.0f,  6.0f);
        expected[9] = vec3f_initialize(-1.0f,  5.0f,  6.0f);
        expected[10] = vec3f_initialize( 4.0f, -2.0f,  6.0f);
        expected[11] = vec3f_initialize( 4.0f,  5.0f,  6.0f);
        expected[12] = vec3f_initialize( 4.0f, -2.0f, -3.0f);
        expected[13] = vec3f_initialize( 4.0f,  5.0f, -3.0f);
        expected[14] = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        expected[15] = vec3f_initialize(-1.0f,  5.0f, -3.0f);

        expected[16] = vec3f_initialize(-1.0f,  5.0f,  6.0f);
        expected[17] = vec3f_initialize( 4.0f,  5.0f,  6.0f);
        expected[18] = vec3f_initialize( 4.0f,  5.0f,  6.0f);
        expected[19] = vec3f_initialize( 4.0f,  5.0f, -3.0f);
        expected[20] = vec3f_initialize( 4.0f,  5.0f, -3.0f);
        expected[21] = vec3f_initialize(-1.0f,  5.0f, -3.0f);
        expected[22] = vec3f_initialize(-1.0f,  5.0f, -3.0f);
        expected[23] = vec3f_initialize(-1.0f,  5.0f,  6.0f);

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 1U, aabbs, geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(24U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry", choco_string_c_str(geometry->name)));

        for(size_t i = 0; i != 24U; ++i) {
            assert(true == is_equal_float(expected[i].elem[0], geometry->vertices[i].position.elem[0]));
            assert(true == is_equal_float(expected[i].elem[1], geometry->vertices[i].position.elem[1]));
            assert(true == is_equal_float(expected[i].elem[2], geometry->vertices[i].position.elem[2]));
        }

        // 元AABBを書き換えてもgeometry側には影響しない
        aabbs[0].min = vec3f_initialize(100.0f, 100.0f, 100.0f);
        aabbs[0].max = vec3f_initialize(200.0f, 200.0f, 200.0f);

        for(size_t i = 0; i != 24U; ++i) {
            assert(true == is_equal_float(expected[i].elem[0], geometry->vertices[i].position.elem[0]));
            assert(true == is_equal_float(expected[i].elem[1], geometry->vertices[i].position.elem[1]));
            assert(true == is_equal_float(expected[i].elem[2], geometry->vertices[i].position.elem[2]));
        }

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 複数AABBからAABB数 * 24頂点のline mesh geometryを生成する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        aabb_3d_t aabbs[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        aabbs[0].min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabbs[0].max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        aabbs[1].min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        aabbs[1].max = vec3f_initialize(11.0f, 22.0f, 33.0f);

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = line_mesh_geometry_initialize_from_aabbs("test_geometry", 2U, aabbs, geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(48U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry", choco_string_c_str(geometry->name)));

        // 1個目AABBの先頭線分: p0 - p1
        assert(true == is_equal_float(-1.0f, geometry->vertices[0].position.elem[0]));
        assert(true == is_equal_float(-2.0f, geometry->vertices[0].position.elem[1]));
        assert(true == is_equal_float( 6.0f, geometry->vertices[0].position.elem[2]));

        assert(true == is_equal_float( 4.0f, geometry->vertices[1].position.elem[0]));
        assert(true == is_equal_float(-2.0f, geometry->vertices[1].position.elem[1]));
        assert(true == is_equal_float( 6.0f, geometry->vertices[1].position.elem[2]));

        // 2個目AABBの先頭線分: offset 24, p0 - p1
        assert(true == is_equal_float(10.0f, geometry->vertices[24].position.elem[0]));
        assert(true == is_equal_float(20.0f, geometry->vertices[24].position.elem[1]));
        assert(true == is_equal_float(33.0f, geometry->vertices[24].position.elem[2]));

        assert(true == is_equal_float(11.0f, geometry->vertices[25].position.elem[0]));
        assert(true == is_equal_float(20.0f, geometry->vertices[25].position.elem[1]));
        assert(true == is_equal_float(33.0f, geometry->vertices[25].position.elem[2]));

        // 2個目AABBの最後の線分: p7 - p4
        assert(true == is_equal_float(10.0f, geometry->vertices[46].position.elem[0]));
        assert(true == is_equal_float(22.0f, geometry->vertices[46].position.elem[1]));
        assert(true == is_equal_float(30.0f, geometry->vertices[46].position.elem[2]));

        assert(true == is_equal_float(10.0f, geometry->vertices[47].position.elem[0]));
        assert(true == is_equal_float(22.0f, geometry->vertices[47].position.elem[1]));
        assert(true == is_equal_float(33.0f, geometry->vertices[47].position.elem[2]));

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_deinitialize(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // geometry_ == NULL -> 何もせずreturn
        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        line_mesh_geometry_deinitialize(NULL);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 未初期化状態のstack geometry -> 何も解放せず、状態はそのまま
        line_mesh_geometry_t geometry = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        line_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // nameのみを保持するgeometry -> nameを破棄し、未初期化状態に戻る
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        line_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // verticesのみを保持するgeometry -> verticesを破棄し、未初期化状態に戻る
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t* vertices = NULL;
        const size_t vertex_count = 2U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_allocate(sizeof(line_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        geometry.name = NULL;
        geometry.vertices = vertices;
        geometry.vertex_count = vertex_count;

        line_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // nameとverticesを保持するgeometry -> 両方破棄し、未初期化状態に戻る
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);
        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(2U == geometry->vertex_count);

        line_mesh_geometry_deinitialize(geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // deinitialize後に再度initializeできる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices_a[2] = { 0 };
        line_vertex_t vertices_b[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices_a[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices_a[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        vertices_b[0].position = vec3f_initialize(10.0f, 11.0f, 12.0f);
        vertices_b[1].position = vec3f_initialize(13.0f, 14.0f, 15.0f);

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry_a", 2U, vertices_a, geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(2U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry_a", choco_string_c_str(geometry->name)));

        line_mesh_geometry_deinitialize(geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry_b", 2U, vertices_b, geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(2U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry_b", choco_string_c_str(geometry->name)));

        assert(10.0f == geometry->vertices[0].position.elem[0]);
        assert(11.0f == geometry->vertices[0].position.elem[1]);
        assert(12.0f == geometry->vertices[0].position.elem[2]);

        assert(13.0f == geometry->vertices[1].position.elem[0]);
        assert(14.0f == geometry->vertices[1].position.elem[1]);
        assert(15.0f == geometry->vertices[1].position.elem[2]);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // deinitializeを2回呼んでも安全
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = line_mesh_geometry_create_from_vertices("test_geometry", 2U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        line_mesh_geometry_deinitialize(geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        line_mesh_geometry_deinitialize(geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count == 0 の場合、verticesはdeinitialize側ではfreeされない
        // nameは破棄される。verticesはテスト側で後始末する
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t* leaked_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(line_vertex_t), MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry.vertices = leaked_vertices;
        geometry.vertex_count = 0U;

        line_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(leaked_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        memory_system_free(leaked_vertices, sizeof(line_vertex_t), MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_countが2の倍数ではない場合、verticesはdeinitialize側ではfreeされない
        // nameは破棄される。verticesはテスト側で後始末する
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t* leaked_vertices = NULL;
        const size_t vertex_count = 3U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(line_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry.vertices = leaked_vertices;
        geometry.vertex_count = vertex_count;

        line_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(leaked_vertices == geometry.vertices);
        assert(vertex_count == geometry.vertex_count);

        memory_system_free(leaked_vertices, sizeof(line_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_clone(void) {

}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_name_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // geometry_ == NULL -> NULL
        const char* name = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        name = line_mesh_geometry_name_get(NULL);
        assert(NULL == name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name == NULL -> NULL
        line_mesh_geometry_t geometry = { 0 };
        const char* name = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        name = line_mesh_geometry_name_get(&geometry);
        assert(NULL == name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: geometry_が保有する名称文字列への参照を返す
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        const char* name = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        name = line_mesh_geometry_name_get(geometry);
        assert(NULL != name);
        assert(0 == strcmp("test_geometry", name));

        assert(name == choco_string_c_str(geometry->name));

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_vertices_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // line_mesh_geometry_vertices_get() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        const line_vertex_t* out_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_line_mesh_geometry_vertices_get.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_vertices_get.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = line_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == out_vertices);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        const line_vertex_t* out_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_vertices_get(NULL, &out_vertices);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == out_vertices);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_vertices_get(&geometry, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *out_vertices_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t dummy_vertex = { 0 };
        const line_vertex_t* out_vertices = &dummy_vertex;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_vertex == out_vertices);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        const line_vertex_t* out_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        const line_vertex_t* out_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        geometry.vertices = NULL;
        geometry.vertex_count = 2U;

        ret = line_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t dummy_vertices[2] = { 0 };
        const line_vertex_t* out_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 0U;

        ret = line_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count が2の倍数ではない -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t dummy_vertices[3] = { 0 };
        const line_vertex_t* out_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 3U;

        ret = line_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == out_vertices);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: geometry_が保有する頂点配列への読み取り専用参照を返す
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };
        const line_vertex_t* out_vertices = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, geometry);
        assert(RESOURCE_SUCCESS == ret);

        ret = line_mesh_geometry_vertices_get(geometry, &out_vertices);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != out_vertices);

        assert(out_vertices == geometry->vertices);
        assert(out_vertices != vertices);

        assert(0.0f == out_vertices[0].position.elem[0]);
        assert(1.0f == out_vertices[0].position.elem[1]);
        assert(2.0f == out_vertices[0].position.elem[2]);

        assert(3.0f == out_vertices[1].position.elem[0]);
        assert(4.0f == out_vertices[1].position.elem[1]);
        assert(5.0f == out_vertices[1].position.elem[2]);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_vertex_count_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // line_mesh_geometry_vertex_count_get() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        size_t out_vertex_count = 12345U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_line_mesh_geometry_vertex_count_get.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_vertex_count_get.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = line_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(12345U == out_vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 12345U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_vertex_count_get(NULL, &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(12345U == out_vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_vertex_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_vertex_count_get(&geometry, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t geometry = { 0 };
        size_t out_vertex_count = 12345U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(12345U == out_vertex_count);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        size_t out_vertex_count = 12345U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        geometry.vertices = NULL;
        geometry.vertex_count = 2U;

        ret = line_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(12345U == out_vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t dummy_vertices[2] = { 0 };
        size_t out_vertex_count = 12345U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 0U;

        ret = line_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(12345U == out_vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count が2の倍数ではない -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        line_mesh_geometry_t geometry = { 0 };
        line_vertex_t dummy_vertices[3] = { 0 };
        size_t out_vertex_count = 12345U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 3U;

        ret = line_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(12345U == out_vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: geometry_が保有する頂点数を返す
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        line_mesh_geometry_t* geometry = NULL;
        line_vertex_t vertices[2] = { 0 };
        size_t out_vertex_count = 12345U;

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = line_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = line_mesh_geometry_initialize_from_vertices("test_geometry", 2U, vertices, geometry);
        assert(RESOURCE_SUCCESS == ret);

        ret = line_mesh_geometry_vertex_count_get(geometry, &out_vertex_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(2U == out_vertex_count);

        line_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_line_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

#endif
