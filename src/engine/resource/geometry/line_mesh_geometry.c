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
static test_call_control_t s_test_config_line_mesh_geometry_create;                     /**< line_mesh_geometry_create()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_destroy;                    /**< line_mesh_geometry_destroy()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_initialize_from_vertices;   /**< line_mesh_geometry_initialize_from_vertices()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_name_get;                   /**< line_mesh_geometry_name_get()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_vertices_get;               /**< line_mesh_geometry_vertices_get()テスト設定 */
static test_call_control_t s_test_config_line_mesh_geometry_vertex_count_get;           /**< line_mesh_geometry_vertex_count_get()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_line_mesh_geometry_create(void);
static void test_line_mesh_geometry_destroy(void);
static void test_line_mesh_geometry_initialize_from_vertices(void);
static void test_line_mesh_geometry_name_get(void);
static void test_line_mesh_geometry_vertices_get(void);
static void test_line_mesh_geometry_vertex_count_get(void);

// テスト用ヘルパー関数

#endif

resource_result_t line_mesh_geometry_create(line_mesh_geometry_t** geometry_) {
#ifdef TEST_BUILD
    s_test_config_line_mesh_geometry_create.call_count++;
    if(s_test_config_line_mesh_geometry_create.fail_on_call != 0) {
        if(s_test_config_line_mesh_geometry_create.call_count == s_test_config_line_mesh_geometry_create.fail_on_call) {
            return (resource_result_t)s_test_config_line_mesh_geometry_create.forced_result;
        }
    }
#endif
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

void NO_COVERAGE test_line_mesh_geometry_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_create.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_line_mesh_geometry_initialize_from_vertices_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_line_mesh_geometry_initialize_from_vertices.fail_on_call = config_->fail_on_call;
    s_test_config_line_mesh_geometry_initialize_from_vertices.forced_result = config_->forced_result;
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
    test_call_control_reset(&s_test_config_line_mesh_geometry_create);
    test_call_control_reset(&s_test_config_line_mesh_geometry_destroy);
    test_call_control_reset(&s_test_config_line_mesh_geometry_initialize_from_vertices);
    test_call_control_reset(&s_test_config_line_mesh_geometry_name_get);
    test_call_control_reset(&s_test_config_line_mesh_geometry_vertices_get);
    test_call_control_reset(&s_test_config_line_mesh_geometry_vertex_count_get);
}

void NO_COVERAGE test_line_mesh_geometry(void) {
    test_line_mesh_geometry_create();
    test_line_mesh_geometry_destroy();
    test_line_mesh_geometry_initialize_from_vertices();
    test_line_mesh_geometry_name_get();
    test_line_mesh_geometry_vertices_get();
    test_line_mesh_geometry_vertex_count_get();
}

// Generated by ChatGPT
static void NO_COVERAGE test_line_mesh_geometry_create(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // line_mesh_geometry_create() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        line_mesh_geometry_t* geometry = NULL;

        test_line_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        s_test_config_line_mesh_geometry_create.fail_on_call = 1U;
        s_test_config_line_mesh_geometry_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(NULL);
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

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(&geometry);
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

        vec3f_initialize(0.0f, 1.0f, 2.0f, &vertices[0].position);
        vec3f_initialize(3.0f, 4.0f, 5.0f, &vertices[1].position);

        ret = line_mesh_geometry_create(&geometry);
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
        vec3f_initialize(100.0f, 100.0f, 100.0f, &vertices[0].position);

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

        ret = line_mesh_geometry_create(&geometry);
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

        vec3f_initialize(0.0f, 1.0f, 2.0f, &vertices[0].position);
        vec3f_initialize(3.0f, 4.0f, 5.0f, &vertices[1].position);

        ret = line_mesh_geometry_create(&geometry);
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

        ret = line_mesh_geometry_create(&geometry);
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
