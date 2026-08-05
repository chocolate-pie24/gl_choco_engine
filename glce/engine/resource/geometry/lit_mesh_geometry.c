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

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "test_controller.h"

#include "engine/resource/geometry/test_lit_mesh_geometry.h"
#include "engine/resource/loaders/test_stl_loader.h"

#include "engine/core/memory/test_choco_memory.h"

#include "engine/containers/test_choco_string.h"

#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_math/math_types.h"

// lit_mesh_geometry用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_lit_mesh_geometry_default_create;              /**< lit_mesh_geometry_default_create()テスト設定 */
static test_call_control_t s_test_config_lit_mesh_geometry_create;                      /**< lit_mesh_geometry_create()テスト設定 */
static test_call_control_t s_test_config_lit_mesh_geometry_initialize;                  /**< lit_mesh_geometry_initialize()テスト設定 */
static test_call_control_t s_test_config_lit_mesh_geometry_clone;                       /**< lit_mesh_geometry_clone()テスト設定 */
static test_call_control_t s_test_config_lit_mesh_geometry_vertices_get;                /**< lit_mesh_geometry_vertices_get()テスト設定 */
static test_call_control_t s_test_config_lit_mesh_geometry_vertex_count_get;            /**< lit_mesh_geometry_vertex_count_get()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_lit_mesh_geometry_default_create(void);
static void test_lit_mesh_geometry_create(void);
static void test_lit_mesh_geometry_destroy(void);
static void test_lit_mesh_geometry_initialize(void);
static void test_lit_mesh_geometry_deinitialize(void);
static void test_lit_mesh_geometry_clone(void);
static void test_lit_mesh_geometry_name_get(void);
static void test_lit_mesh_geometry_vertices_get(void);
static void test_lit_mesh_geometry_vertex_count_get(void);

// テスト用ヘルパー関数

#endif

resource_result_t lit_mesh_geometry_default_create(lit_mesh_geometry_t** geometry_) {
#ifdef TEST_BUILD
    s_test_config_lit_mesh_geometry_default_create.call_count++;
    if(s_test_config_lit_mesh_geometry_default_create.fail_on_call != 0) {
        if(s_test_config_lit_mesh_geometry_default_create.call_count == s_test_config_lit_mesh_geometry_default_create.fail_on_call) {
            return (resource_result_t)s_test_config_lit_mesh_geometry_default_create.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_lit_mesh_geometry_create.call_count++;
    if(s_test_config_lit_mesh_geometry_create.fail_on_call != 0) {
        if(s_test_config_lit_mesh_geometry_create.call_count == s_test_config_lit_mesh_geometry_create.fail_on_call) {
            return (resource_result_t)s_test_config_lit_mesh_geometry_create.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_lit_mesh_geometry_initialize.call_count++;
    if(s_test_config_lit_mesh_geometry_initialize.fail_on_call != 0) {
        if(s_test_config_lit_mesh_geometry_initialize.call_count == s_test_config_lit_mesh_geometry_initialize.fail_on_call) {
            return (resource_result_t)s_test_config_lit_mesh_geometry_initialize.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_lit_mesh_geometry_clone.call_count++;
    if(s_test_config_lit_mesh_geometry_clone.fail_on_call != 0) {
        if(s_test_config_lit_mesh_geometry_clone.call_count == s_test_config_lit_mesh_geometry_clone.fail_on_call) {
            return (resource_result_t)s_test_config_lit_mesh_geometry_clone.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_lit_mesh_geometry_vertices_get.call_count++;
    if(s_test_config_lit_mesh_geometry_vertices_get.fail_on_call != 0) {
        if(s_test_config_lit_mesh_geometry_vertices_get.call_count == s_test_config_lit_mesh_geometry_vertices_get.fail_on_call) {
            return (resource_result_t)s_test_config_lit_mesh_geometry_vertices_get.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_lit_mesh_geometry_vertex_count_get.call_count++;
    if(s_test_config_lit_mesh_geometry_vertex_count_get.fail_on_call != 0) {
        if(s_test_config_lit_mesh_geometry_vertex_count_get.call_count == s_test_config_lit_mesh_geometry_vertex_count_get.fail_on_call) {
            return (resource_result_t)s_test_config_lit_mesh_geometry_vertex_count_get.forced_result;
        }
    }
#endif
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

#ifdef TEST_BUILD

void NO_COVERAGE test_lit_mesh_geometry_default_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_lit_mesh_geometry_default_create.fail_on_call = config_->fail_on_call;
    s_test_config_lit_mesh_geometry_default_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_lit_mesh_geometry_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_lit_mesh_geometry_create.fail_on_call = config_->fail_on_call;
    s_test_config_lit_mesh_geometry_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_lit_mesh_geometry_initialize_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_lit_mesh_geometry_initialize.fail_on_call = config_->fail_on_call;
    s_test_config_lit_mesh_geometry_initialize.forced_result = config_->forced_result;
}

void NO_COVERAGE test_lit_mesh_geometry_clone_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_lit_mesh_geometry_clone.fail_on_call = config_->fail_on_call;
    s_test_config_lit_mesh_geometry_clone.forced_result = config_->forced_result;
}

void NO_COVERAGE test_lit_mesh_geometry_vertices_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_lit_mesh_geometry_vertices_get.fail_on_call = config_->fail_on_call;
    s_test_config_lit_mesh_geometry_vertices_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_lit_mesh_geometry_vertex_count_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_lit_mesh_geometry_vertex_count_get.fail_on_call = config_->fail_on_call;
    s_test_config_lit_mesh_geometry_vertex_count_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_lit_mesh_geometry_config_reset(void) {
    test_call_control_reset(&s_test_config_lit_mesh_geometry_default_create);
    test_call_control_reset(&s_test_config_lit_mesh_geometry_create);
    test_call_control_reset(&s_test_config_lit_mesh_geometry_initialize);
    test_call_control_reset(&s_test_config_lit_mesh_geometry_clone);
    test_call_control_reset(&s_test_config_lit_mesh_geometry_vertices_get);
    test_call_control_reset(&s_test_config_lit_mesh_geometry_vertex_count_get);
}

void NO_COVERAGE test_lit_mesh_geometry(void) {
    test_lit_mesh_geometry_default_create();
    test_lit_mesh_geometry_create();
    test_lit_mesh_geometry_destroy();
    test_lit_mesh_geometry_initialize();
    test_lit_mesh_geometry_deinitialize();
    test_lit_mesh_geometry_clone();
    test_lit_mesh_geometry_name_get();
    test_lit_mesh_geometry_vertices_get();
    test_lit_mesh_geometry_vertex_count_get();
}

// Generated by ChatGPT
static void NO_COVERAGE test_lit_mesh_geometry_default_create(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
    {
        // lit_mesh_geometry_default_create() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        s_test_config_lit_mesh_geometry_default_create.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_default_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_default_create(NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t dummy_geometry = { 0 };
        lit_mesh_geometry_t* geometry = &dummy_geometry;

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_geometry == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: lit_mesh_geometry_t が確保され、全フィールドが未初期化状態で初期化される
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_lit_mesh_geometry_create(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // lit_mesh_geometry_create() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].normal = vec4i8_initialize(127, 0, 0, 0);

        s_test_config_lit_mesh_geometry_create.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_create.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t dummy_geometry = { 0 };
        lit_mesh_geometry_t* geometry = &dummy_geometry;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_geometry == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // lit_mesh_geometry_default_create() が失敗 -> その戻り値を返し、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_lit_mesh_geometry_default_create.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_default_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // lit_mesh_geometry_default_create() 内部のmemory_system_allocate()が失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // lit_mesh_geometry_initialize() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_lit_mesh_geometry_initialize.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_initialize.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_create(NULL, 3U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ == 0 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_create("test_geometry", 0U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_create("test_geometry", 3U, NULL, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ が3の倍数ではない -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[4] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_create("test_geometry", 4U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ == 1 もlit meshでは無効
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[1] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_create("test_geometry", 1U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // choco_string_create_from_c_string() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列用memory_system_allocate() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        // 1回目のallocateはlit_mesh_geometry_t本体、2回目はchoco_string_t本体、3回目は文字列バッファ、4回目がtmp_vertices用
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 4U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列サイズoverflow -> RESOURCE_OVERFLOW
        // SIZE_MAXは3の倍数なので、lit meshの3倍数チェックを通過してoverflowチェックに到達する
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t dummy_vertex = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        dummy_vertex.position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        dummy_vertex.normal = vec4i8_initialize(0, 0, 127, 0);

        ret = lit_mesh_geometry_create("test_geometry", SIZE_MAX, &dummy_vertex, &geometry);
        assert(RESOURCE_OVERFLOW == ret);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: lit_mesh_geometry_tを生成し、vertices_をdeep copyしてgeometryが所有する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].normal = vec4i8_initialize(127, 0, 0, 0);

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(3U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry", choco_string_c_str(geometry->name)));

        assert(vertices != geometry->vertices);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(2.0f == geometry->vertices[0].position.elem[2]);
        assert(0 == geometry->vertices[0].normal.elem[0]);
        assert(0 == geometry->vertices[0].normal.elem[1]);
        assert(127 == geometry->vertices[0].normal.elem[2]);
        assert(0 == geometry->vertices[0].normal.elem[3]);

        assert(3.0f == geometry->vertices[1].position.elem[0]);
        assert(4.0f == geometry->vertices[1].position.elem[1]);
        assert(5.0f == geometry->vertices[1].position.elem[2]);
        assert(0 == geometry->vertices[1].normal.elem[0]);
        assert(127 == geometry->vertices[1].normal.elem[1]);
        assert(0 == geometry->vertices[1].normal.elem[2]);
        assert(0 == geometry->vertices[1].normal.elem[3]);

        assert(6.0f == geometry->vertices[2].position.elem[0]);
        assert(7.0f == geometry->vertices[2].position.elem[1]);
        assert(8.0f == geometry->vertices[2].position.elem[2]);
        assert(127 == geometry->vertices[2].normal.elem[0]);
        assert(0 == geometry->vertices[2].normal.elem[1]);
        assert(0 == geometry->vertices[2].normal.elem[2]);
        assert(0 == geometry->vertices[2].normal.elem[3]);

        // 元配列を書き換えてもgeometry側には影響しない
        vertices[0].position = vec3f_initialize(100.0f, 100.0f, 100.0f);
        vertices[0].normal = vec4i8_initialize(-1, -1, -1, -1);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(2.0f == geometry->vertices[0].position.elem[2]);
        assert(0 == geometry->vertices[0].normal.elem[0]);
        assert(0 == geometry->vertices[0].normal.elem[1]);
        assert(127 == geometry->vertices[0].normal.elem[2]);
        assert(0 == geometry->vertices[0].normal.elem[3]);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2三角形ぶんの頂点配列をdeep copyする
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[6] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].normal = vec4i8_initialize(127, 0, 0, 0);

        vertices[3].position = vec3f_initialize(9.0f, 10.0f, 11.0f);
        vertices[3].normal = vec4i8_initialize(10, 20, 30, 40);

        vertices[4].position = vec3f_initialize(12.0f, 13.0f, 14.0f);
        vertices[4].normal = vec4i8_initialize(50, 60, 70, 80);

        vertices[5].position = vec3f_initialize(15.0f, 16.0f, 17.0f);
        vertices[5].normal = vec4i8_initialize(90, 100, 110, 120);

        ret = lit_mesh_geometry_create("test_geometry_two_triangles", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(6U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry_two_triangles", choco_string_c_str(geometry->name)));

        assert(vertices != geometry->vertices);

        for(size_t i = 0; i != 6U; ++i) {
            assert(vertices[i].position.elem[0] == geometry->vertices[i].position.elem[0]);
            assert(vertices[i].position.elem[1] == geometry->vertices[i].position.elem[1]);
            assert(vertices[i].position.elem[2] == geometry->vertices[i].position.elem[2]);

            assert(vertices[i].normal.elem[0] == geometry->vertices[i].normal.elem[0]);
            assert(vertices[i].normal.elem[1] == geometry->vertices[i].normal.elem[1]);
            assert(vertices[i].normal.elem[2] == geometry->vertices[i].normal.elem[2]);
            assert(vertices[i].normal.elem[3] == geometry->vertices[i].normal.elem[3]);
        }

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_lit_mesh_geometry_destroy(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // geometry_ == NULL -> 何もせずreturn
        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        lit_mesh_geometry_destroy(NULL);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ == NULL -> 何もせずreturn
        lit_mesh_geometry_t* geometry = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: name == NULL, vertices == NULL, vertex_count == 0 のgeometry本体だけを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);
        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: nameのみを保持するgeometryを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: nameとverticesを保持するgeometryを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t* vertices = NULL;
        const size_t vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry->vertices = vertices;
        geometry->vertex_count = vertex_count;

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count == 0 の場合、verticesはdestroy側ではfreeされない
        // テスト側で後始末する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t* leaked_vertices = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t), MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry->vertices = leaked_vertices;
        geometry->vertex_count = 0U;

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        memory_system_free(leaked_vertices, sizeof(point_normal_vertex_t), MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 二重destroy相当: 1回目でNULL化され、2回目は何もせずreturn
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_lit_mesh_geometry_initialize(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // lit_mesh_geometry_initialize() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_lit_mesh_geometry_initialize.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_initialize.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize(NULL, 3U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ == 0 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize("test_geometry", 0U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, NULL, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("already_initialized", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL != geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t vertices[3] = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };

        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 0U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count != 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t vertices[3] = { 0 };

        geometry.name = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(3U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // choco_string_create_from_c_string() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t vertices[3] = { 0 };
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列サイズoverflow -> RESOURCE_OVERFLOW
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertex = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize("test_geometry", SIZE_MAX, &dummy_vertex, &geometry);
        assert(RESOURCE_OVERFLOW == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列用memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        // 1回目のallocateはchoco_string内部、2回目がtmp_vertices用
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t vertices[3] = { 0 };
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 3U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: vertices_をdeep copyしてgeometryが所有する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].normal = vec4i8_initialize(127, 0, 0, 0);

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(3U == geometry->vertex_count);

        assert(0 == strcmp("test_geometry", choco_string_c_str(geometry->name)));

        assert(vertices != geometry->vertices);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(2.0f == geometry->vertices[0].position.elem[2]);
        assert(0 == geometry->vertices[0].normal.elem[0]);
        assert(0 == geometry->vertices[0].normal.elem[1]);
        assert(127 == geometry->vertices[0].normal.elem[2]);
        assert(0 == geometry->vertices[0].normal.elem[3]);

        assert(3.0f == geometry->vertices[1].position.elem[0]);
        assert(4.0f == geometry->vertices[1].position.elem[1]);
        assert(5.0f == geometry->vertices[1].position.elem[2]);
        assert(0 == geometry->vertices[1].normal.elem[0]);
        assert(127 == geometry->vertices[1].normal.elem[1]);
        assert(0 == geometry->vertices[1].normal.elem[2]);
        assert(0 == geometry->vertices[1].normal.elem[3]);

        assert(6.0f == geometry->vertices[2].position.elem[0]);
        assert(7.0f == geometry->vertices[2].position.elem[1]);
        assert(8.0f == geometry->vertices[2].position.elem[2]);
        assert(127 == geometry->vertices[2].normal.elem[0]);
        assert(0 == geometry->vertices[2].normal.elem[1]);
        assert(0 == geometry->vertices[2].normal.elem[2]);
        assert(0 == geometry->vertices[2].normal.elem[3]);

        // 元配列を書き換えてもgeometry側には影響しない
        vertices[0].position = vec3f_initialize(100.0f, 100.0f, 100.0f);
        vertices[0].normal = vec4i8_initialize(-1, -1, -1, -1);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(2.0f == geometry->vertices[0].position.elem[2]);
        assert(0 == geometry->vertices[0].normal.elem[0]);
        assert(0 == geometry->vertices[0].normal.elem[1]);
        assert(127 == geometry->vertices[0].normal.elem[2]);
        assert(0 == geometry->vertices[0].normal.elem[3]);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_lit_mesh_geometry_deinitialize(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // geometry_ == NULL -> 何もせずreturn
        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        lit_mesh_geometry_deinitialize(NULL);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 未初期化状態のstack geometry -> 何も解放せず、状態はそのまま
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        lit_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // nameのみを保持するgeometry -> nameを破棄し、未初期化状態に戻る
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        lit_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // verticesのみを保持するgeometry -> verticesを破棄し、未初期化状態に戻る
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t* vertices = NULL;
        const size_t vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].normal = vec4i8_initialize(127, 0, 0, 0);

        geometry.name = NULL;
        geometry.vertices = vertices;
        geometry.vertex_count = vertex_count;

        lit_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // nameとverticesを保持するgeometry -> 両方破棄し、未初期化状態に戻る
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].normal = vec4i8_initialize(127, 0, 0, 0);

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);
        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(3U == geometry->vertex_count);

        lit_mesh_geometry_deinitialize(geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 2三角形ぶんのgeometryも正常にdeinitializeできる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[6] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        for(size_t i = 0; i != 6U; ++i) {
            vertices[i].position = vec3f_initialize((float)i, (float)(i + 1U), (float)(i + 2U));
            vertices[i].normal = vec4i8_initialize(0, 0, 127, 0);
        }

        ret = lit_mesh_geometry_create("two_triangle_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);
        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(6U == geometry->vertex_count);

        lit_mesh_geometry_deinitialize(geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // deinitialize後に再度initializeできる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices_a[3] = { 0 };
        point_normal_vertex_t vertices_b[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices_a[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices_a[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices_a[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices_a[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices_a[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices_a[2].normal = vec4i8_initialize(127, 0, 0, 0);

        vertices_b[0].position = vec3f_initialize(10.0f, 11.0f, 12.0f);
        vertices_b[0].normal = vec4i8_initialize(10, 20, 30, 40);

        vertices_b[1].position = vec3f_initialize(13.0f, 14.0f, 15.0f);
        vertices_b[1].normal = vec4i8_initialize(50, 60, 70, 80);

        vertices_b[2].position = vec3f_initialize(16.0f, 17.0f, 18.0f);
        vertices_b[2].normal = vec4i8_initialize(90, 100, 110, 120);

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize("test_geometry_a", 3U, vertices_a, geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(3U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry_a", choco_string_c_str(geometry->name)));

        lit_mesh_geometry_deinitialize(geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        ret = lit_mesh_geometry_initialize("test_geometry_b", 3U, vertices_b, geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(3U == geometry->vertex_count);
        assert(0 == strcmp("test_geometry_b", choco_string_c_str(geometry->name)));

        assert(10.0f == geometry->vertices[0].position.elem[0]);
        assert(11.0f == geometry->vertices[0].position.elem[1]);
        assert(12.0f == geometry->vertices[0].position.elem[2]);
        assert(10 == geometry->vertices[0].normal.elem[0]);
        assert(20 == geometry->vertices[0].normal.elem[1]);
        assert(30 == geometry->vertices[0].normal.elem[2]);
        assert(40 == geometry->vertices[0].normal.elem[3]);

        assert(13.0f == geometry->vertices[1].position.elem[0]);
        assert(14.0f == geometry->vertices[1].position.elem[1]);
        assert(15.0f == geometry->vertices[1].position.elem[2]);
        assert(50 == geometry->vertices[1].normal.elem[0]);
        assert(60 == geometry->vertices[1].normal.elem[1]);
        assert(70 == geometry->vertices[1].normal.elem[2]);
        assert(80 == geometry->vertices[1].normal.elem[3]);

        assert(16.0f == geometry->vertices[2].position.elem[0]);
        assert(17.0f == geometry->vertices[2].position.elem[1]);
        assert(18.0f == geometry->vertices[2].position.elem[2]);
        assert(90 == geometry->vertices[2].normal.elem[0]);
        assert(100 == geometry->vertices[2].normal.elem[1]);
        assert(110 == geometry->vertices[2].normal.elem[2]);
        assert(120 == geometry->vertices[2].normal.elem[3]);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // deinitializeを2回呼んでも安全
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].normal = vec4i8_initialize(127, 0, 0, 0);

        ret = lit_mesh_geometry_create("test_geometry", 3U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        lit_mesh_geometry_deinitialize(geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        lit_mesh_geometry_deinitialize(geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count == 0 の場合、verticesはdeinitialize側ではfreeされない
        // nameは破棄される。verticesはテスト側で後始末する
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t* leaked_vertices = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t), MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry.vertices = leaked_vertices;
        geometry.vertex_count = 0U;

        lit_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(leaked_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        memory_system_free(leaked_vertices, sizeof(point_normal_vertex_t), MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_countが3の倍数ではない場合、verticesはdeinitialize側ではfreeされない
        // nameは破棄される。verticesはテスト側で後始末する
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t* leaked_vertices = NULL;
        const size_t vertex_count = 4U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry.vertices = leaked_vertices;
        geometry.vertex_count = vertex_count;

        lit_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(leaked_vertices == geometry.vertices);
        assert(vertex_count == geometry.vertex_count);

        memory_system_free(leaked_vertices, sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_lit_mesh_geometry_clone(void) {

}

// Generated by ChatGPT
static void NO_COVERAGE test_lit_mesh_geometry_name_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // geometry_ == NULL -> NULL
        const char* name = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        name = lit_mesh_geometry_name_get(NULL);
        assert(NULL == name);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name == NULL -> NULL
        lit_mesh_geometry_t geometry = { 0 };
        const char* name = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        name = lit_mesh_geometry_name_get(&geometry);
        assert(NULL == name);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: geometryが保持するnameへの参照を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };
        const char* name = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 0.0f, 0.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(1.0f, 0.0f, 0.0f);
        vertices[1].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[2].position = vec3f_initialize(0.0f, 1.0f, 0.0f);
        vertices[2].normal = vec4i8_initialize(0, 0, 127, 0);

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, geometry);
        assert(RESOURCE_SUCCESS == ret);

        name = lit_mesh_geometry_name_get(geometry);
        assert(NULL != name);
        assert(0 == strcmp("test_geometry", name));

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_lit_mesh_geometry_vertices_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // lit_mesh_geometry_vertices_get() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        const point_normal_vertex_t* out_vertices = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        geometry.name = (choco_string_t*)0x1;
        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 3U;

        s_test_config_lit_mesh_geometry_vertices_get.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_vertices_get.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = lit_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == out_vertices);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        const point_normal_vertex_t* out_vertices = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertices_get(NULL, &out_vertices);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == out_vertices);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };

        geometry.name = (choco_string_t*)0x1;
        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertices_get(&geometry, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert((choco_string_t*)0x1 == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(3U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *out_vertices_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        const point_normal_vertex_t* out_vertices = dummy_vertices;

        geometry.name = (choco_string_t*)0x1;
        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(dummy_vertices == out_vertices);

        assert((choco_string_t*)0x1 == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(3U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        const point_normal_vertex_t* out_vertices = NULL;

        geometry.name = NULL;
        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);

        assert(NULL == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(3U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        const point_normal_vertex_t* out_vertices = NULL;

        geometry.name = (choco_string_t*)0x1;
        geometry.vertices = NULL;
        geometry.vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);

        assert((choco_string_t*)0x1 == geometry.name);
        assert(NULL == geometry.vertices);
        assert(3U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        const point_normal_vertex_t* out_vertices = NULL;

        geometry.name = (choco_string_t*)0x1;
        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 0U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);

        assert((choco_string_t*)0x1 == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: geometryが保持するverticesへのconst参照を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };
        const point_normal_vertex_t* out_vertices = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].normal = vec4i8_initialize(127, 0, 0, 0);

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, geometry);
        assert(RESOURCE_SUCCESS == ret);

        ret = lit_mesh_geometry_vertices_get(geometry, &out_vertices);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != out_vertices);
        assert(geometry->vertices == out_vertices);
        assert(vertices != out_vertices);

        assert(0.0f == out_vertices[0].position.elem[0]);
        assert(1.0f == out_vertices[0].position.elem[1]);
        assert(2.0f == out_vertices[0].position.elem[2]);
        assert(0 == out_vertices[0].normal.elem[0]);
        assert(0 == out_vertices[0].normal.elem[1]);
        assert(127 == out_vertices[0].normal.elem[2]);
        assert(0 == out_vertices[0].normal.elem[3]);

        assert(3.0f == out_vertices[1].position.elem[0]);
        assert(4.0f == out_vertices[1].position.elem[1]);
        assert(5.0f == out_vertices[1].position.elem[2]);
        assert(0 == out_vertices[1].normal.elem[0]);
        assert(127 == out_vertices[1].normal.elem[1]);
        assert(0 == out_vertices[1].normal.elem[2]);
        assert(0 == out_vertices[1].normal.elem[3]);

        assert(6.0f == out_vertices[2].position.elem[0]);
        assert(7.0f == out_vertices[2].position.elem[1]);
        assert(8.0f == out_vertices[2].position.elem[2]);
        assert(127 == out_vertices[2].normal.elem[0]);
        assert(0 == out_vertices[2].normal.elem[1]);
        assert(0 == out_vertices[2].normal.elem[2]);
        assert(0 == out_vertices[2].normal.elem[3]);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_lit_mesh_geometry_vertex_count_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // lit_mesh_geometry_vertex_count_get() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        size_t out_vertex_count = 999U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        geometry.name = (choco_string_t*)0x1;
        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 3U;

        s_test_config_lit_mesh_geometry_vertex_count_get.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_vertex_count_get.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = lit_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(999U == out_vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertex_count_get(NULL, &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(999U == out_vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_vertex_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };

        geometry.name = (choco_string_t*)0x1;
        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertex_count_get(&geometry, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert((choco_string_t*)0x1 == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(3U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        size_t out_vertex_count = 999U;

        geometry.name = NULL;
        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(999U == out_vertex_count);

        assert(NULL == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(3U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        size_t out_vertex_count = 999U;

        geometry.name = (choco_string_t*)0x1;
        geometry.vertices = NULL;
        geometry.vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(999U == out_vertex_count);

        assert((choco_string_t*)0x1 == geometry.name);
        assert(NULL == geometry.vertices);
        assert(3U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        size_t out_vertex_count = 999U;

        geometry.name = (choco_string_t*)0x1;
        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 0U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(999U == out_vertex_count);

        assert((choco_string_t*)0x1 == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: geometryが保持するvertex_countを取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;
        point_normal_vertex_t vertices[3] = { 0 };
        size_t out_vertex_count = 0U;

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[0].normal = vec4i8_initialize(0, 0, 127, 0);

        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].normal = vec4i8_initialize(0, 127, 0, 0);

        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].normal = vec4i8_initialize(127, 0, 0, 0);

        ret = lit_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize("test_geometry", 3U, vertices, geometry);
        assert(RESOURCE_SUCCESS == ret);

        ret = lit_mesh_geometry_vertex_count_get(geometry, &out_vertex_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(3U == out_vertex_count);

        assert(3U == geometry->vertex_count);
        assert(NULL != geometry->vertices);
        assert(NULL != geometry->name);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

#endif
