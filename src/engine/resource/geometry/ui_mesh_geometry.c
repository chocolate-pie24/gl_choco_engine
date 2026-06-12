/** @ingroup resource
 *
 * @file ui_mesh_geometry.c
 * @author chocolate-pie24
 * @brief ui_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの実装
 * 
 * @note ui_mesh_shader: 2D矩形領域にテクスチャを貼った描画を行う, 描画単位は矩形領域ごとに描画する
 * @note ui_mesh_geometryは矩形領域のテクスチャuv座標、矩形領域座標のみを保持する
 *
 * @version 0.1
 * @date 2026-06-12
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/resource/geometry/ui_mesh_geometry.h"

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

#include "engine/containers/choco_string.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/geometry_primitive_err_utils.h"
#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief ui_mesh_geometry内部状態管理構造体
 *
 */
struct ui_mesh_geometry {
    choco_string_t* name;       /**< ui_mesh_geometry CPU側リソース名称 */

    size_t vertex_count;        /**< ui_mesh_geometryが所有する頂点数(当面は1矩形領域のみなので、三角形2枚分で頂点数は6固定) */
    ui_vertex_t* vertices;      /**< ui_mesh_geometryが所有する頂点配列(三角形1 p1, p2, p3, 三角形2 p1, p2, p3) */
};

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "test_controller.h"

#include "engine/resource/geometry/test_ui_mesh_geometry.h"

#include "engine/core/memory/test_choco_memory.h"

#include "engine/containers/test_choco_string.h"

#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_math/math_types.h"

// ui_mesh_geometry用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_ui_mesh_geometry_default_create;               /**< ui_mesh_geometry_default_create()テスト設定 */
static test_call_control_t s_test_config_ui_mesh_geometry_create_from_vertices;         /**< ui_mesh_geometry_create_from_vertices()テスト設定 */
static test_call_control_t s_test_config_ui_mesh_geometry_initialize_from_vertices;     /**< ui_mesh_geometry_initialize_from_vertices()テスト設定 */
static test_call_control_t s_test_config_ui_mesh_geometry_vertices_get;                 /**< ui_mesh_geometry_vertices_get()テスト設定 */
static test_call_control_t s_test_config_ui_mesh_geometry_vertex_count_get;             /**< ui_mesh_geometry_vertex_count_get()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_ui_mesh_geometry_default_create(void);
static void test_ui_mesh_geometry_create_from_vertices(void);
static void test_ui_mesh_geometry_destroy(void);
static void test_ui_mesh_geometry_initialize_from_vertices(void);
static void test_ui_mesh_geometry_deinitialize(void);
static void test_ui_mesh_geometry_name_get(void);
static void test_ui_mesh_geometry_vertices_get(void);
static void test_ui_mesh_geometry_vertex_count_get(void);

// テスト用ヘルパー関数

#endif

resource_result_t ui_mesh_geometry_default_create(ui_mesh_geometry_t** geometry_) {
#ifdef TEST_BUILD
    s_test_config_ui_mesh_geometry_default_create.call_count++;
    if(s_test_config_ui_mesh_geometry_default_create.fail_on_call != 0) {
        if(s_test_config_ui_mesh_geometry_default_create.call_count == s_test_config_ui_mesh_geometry_default_create.fail_on_call) {
            return (resource_result_t)s_test_config_ui_mesh_geometry_default_create.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    ui_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_default_create", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_default_create", "*geometry_")

    ret_mem = memory_system_allocate(sizeof(ui_mesh_geometry_t), MEMORY_TAG_GEOMETRY, (void**)&tmp_geometry);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("ui_mesh_geometry_default_create(%s) - Failed to allocate ui_mesh_geometry_t instance.", resource_rslt_to_str(ret));
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
            memory_system_free(tmp_geometry, sizeof(ui_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
            tmp_geometry = NULL;
        }
    }
    return ret;
}

resource_result_t ui_mesh_geometry_create_from_vertices(const char* name_, size_t vertex_count_, const ui_vertex_t* vertices_, ui_mesh_geometry_t** geometry_) {
#ifdef TEST_BUILD
    s_test_config_ui_mesh_geometry_create_from_vertices.call_count++;
    if(s_test_config_ui_mesh_geometry_create_from_vertices.fail_on_call != 0) {
        if(s_test_config_ui_mesh_geometry_create_from_vertices.call_count == s_test_config_ui_mesh_geometry_create_from_vertices.fail_on_call) {
            return (resource_result_t)s_test_config_ui_mesh_geometry_create_from_vertices.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    ui_mesh_geometry_t* tmp_geometry = NULL;

    // NOTE: vertex_count_ == 6のチェックはui_mesh_geometry_initialize_from_verticesで行う
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_create_from_vertices", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_create_from_vertices", "*geometry_")

    ret = ui_mesh_geometry_default_create(&tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_geometry_create_from_vertices(%s) - Failed to create ui_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = ui_mesh_geometry_initialize_from_vertices(name_, vertex_count_, vertices_, tmp_geometry);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_geometry_create_from_vertices(%s) - Failed to initialize ui_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *geometry_ = tmp_geometry;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        ui_mesh_geometry_destroy(&tmp_geometry);
    }
    return ret;
}

void ui_mesh_geometry_destroy(ui_mesh_geometry_t** geometry_) {
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
        ERROR_MESSAGE("ui_mesh_geometry_destroy(%s) - ui_mesh_geometry internal state is inconsistent: vertices is not NULL but vertex_count is 0. CPU-side vertex array was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != (*geometry_)->vertices && 6 != (*geometry_)->vertex_count) {
        ERROR_MESSAGE("ui_mesh_geometry_destroy(%s) - ui_mesh_geometry internal state is inconsistent: vertex_count is not 6. CPU-side vertex array was not freed because allocation size cannot be trusted.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != (*geometry_)->vertices) {
        memory_system_free((*geometry_)->vertices, sizeof(ui_vertex_t) * (*geometry_)->vertex_count, MEMORY_TAG_GEOMETRY);
        (*geometry_)->vertices = NULL;
        (*geometry_)->vertex_count = 0;
    }

    memory_system_free(*geometry_, sizeof(ui_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
    *geometry_ = NULL;
}

resource_result_t ui_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const ui_vertex_t* vertices_, ui_mesh_geometry_t* geometry_) {
#ifdef TEST_BUILD
    s_test_config_ui_mesh_geometry_initialize_from_vertices.call_count++;
    if(s_test_config_ui_mesh_geometry_initialize_from_vertices.fail_on_call != 0) {
        if(s_test_config_ui_mesh_geometry_initialize_from_vertices.call_count == s_test_config_ui_mesh_geometry_initialize_from_vertices.fail_on_call) {
            return (resource_result_t)s_test_config_ui_mesh_geometry_initialize_from_vertices.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    choco_string_t* tmp_name = NULL;
    ui_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_initialize_from_vertices", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP(6 == vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_initialize_from_vertices", "vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_initialize_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_initialize_from_vertices", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "ui_mesh_geometry_initialize_from_vertices", "geometry_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "ui_mesh_geometry_initialize_from_vertices", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "ui_mesh_geometry_initialize_from_vertices", "geometry_->vertex_count")

    ret_string = choco_string_create_from_c_string(name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("ui_mesh_geometry_initialize_from_vertices(%s) - Failed to create ui mesh geometry name string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX / vertex_count_) < sizeof(ui_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("ui_mesh_geometry_initialize_from_vertices(%s) - CPU-side vertex array size overflow. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(ui_vertex_t));
        goto cleanup;
    }
    ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("ui_mesh_geometry_initialize_from_vertices(%s) - Failed to allocate CPU-side vertex array. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(ui_vertex_t));
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
            memory_system_free(tmp_vertices, sizeof(ui_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY);
            tmp_vertices = NULL;
        }
    }
    return ret;
}

void ui_mesh_geometry_deinitialize(ui_mesh_geometry_t* geometry_) {
    if(NULL == geometry_) {
        return;
    }
    if(NULL != geometry_->name) {
        choco_string_destroy(&geometry_->name);
    }
    if(NULL != geometry_->vertices && 0 == geometry_->vertex_count) {
        ERROR_MESSAGE("ui_mesh_geometry_deinitialize(%s) - ui_mesh_geometry internal state is inconsistent: vertices is not NULL but vertex_count is 0. CPU-side vertex array was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != geometry_->vertices && 6 != geometry_->vertex_count) {
        ERROR_MESSAGE("ui_mesh_geometry_deinitialize(%s) - ui_mesh_geometry internal state is inconsistent: vertex_count is not 6.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    } else if(NULL != geometry_->vertices) {
        memory_system_free(geometry_->vertices, sizeof(ui_vertex_t) * geometry_->vertex_count, MEMORY_TAG_GEOMETRY);
        geometry_->vertices = NULL;
        geometry_->vertex_count = 0;
    }
}

const char* ui_mesh_geometry_name_get(const ui_mesh_geometry_t* geometry_) {
    if(NULL == geometry_) {
        return NULL;
    }
    if(NULL == geometry_->name) {
        return NULL;
    }
    return choco_string_c_str(geometry_->name);
}

resource_result_t ui_mesh_geometry_vertices_get(const ui_mesh_geometry_t* geometry_, const ui_vertex_t** out_vertices_) {
#ifdef TEST_BUILD
    s_test_config_ui_mesh_geometry_vertices_get.call_count++;
    if(s_test_config_ui_mesh_geometry_vertices_get.fail_on_call != 0) {
        if(s_test_config_ui_mesh_geometry_vertices_get.call_count == s_test_config_ui_mesh_geometry_vertices_get.fail_on_call) {
            return (resource_result_t)s_test_config_ui_mesh_geometry_vertices_get.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_vertices_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_vertices_get", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_vertices_get", "*out_vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "ui_mesh_geometry_vertices_get", "geometry_->name")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "ui_mesh_geometry_vertices_get", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(6 == geometry_->vertex_count, ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "ui_mesh_geometry_vertices_get", "geometry_->vertex_count")

    *out_vertices_ = geometry_->vertices;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t ui_mesh_geometry_vertex_count_get(const ui_mesh_geometry_t* geometry_, size_t* out_vertex_count_) {
#ifdef TEST_BUILD
    s_test_config_ui_mesh_geometry_vertex_count_get.call_count++;
    if(s_test_config_ui_mesh_geometry_vertex_count_get.fail_on_call != 0) {
        if(s_test_config_ui_mesh_geometry_vertex_count_get.call_count == s_test_config_ui_mesh_geometry_vertex_count_get.fail_on_call) {
            return (resource_result_t)s_test_config_ui_mesh_geometry_vertex_count_get.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_vertex_count_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_mesh_geometry_vertex_count_get", "out_vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "ui_mesh_geometry_vertex_count_get", "geometry_->name")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "ui_mesh_geometry_vertex_count_get", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(6 == geometry_->vertex_count, ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "ui_mesh_geometry_vertex_count_get", "geometry_->vertex_count")

    *out_vertex_count_ = geometry_->vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

#ifdef TEST_BUILD

void NO_COVERAGE test_ui_mesh_geometry_default_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_ui_mesh_geometry_default_create.fail_on_call = config_->fail_on_call;
    s_test_config_ui_mesh_geometry_default_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_ui_mesh_geometry_create_from_vertices_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_ui_mesh_geometry_create_from_vertices.fail_on_call = config_->fail_on_call;
    s_test_config_ui_mesh_geometry_create_from_vertices.forced_result = config_->forced_result;
}

void NO_COVERAGE test_ui_mesh_geometry_initialize_from_vertices_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_ui_mesh_geometry_initialize_from_vertices.fail_on_call = config_->fail_on_call;
    s_test_config_ui_mesh_geometry_initialize_from_vertices.forced_result = config_->forced_result;
}

void NO_COVERAGE test_ui_mesh_geometry_vertices_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_ui_mesh_geometry_vertices_get.fail_on_call = config_->fail_on_call;
    s_test_config_ui_mesh_geometry_vertices_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_ui_mesh_geometry_vertex_count_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_ui_mesh_geometry_vertex_count_get.fail_on_call = config_->fail_on_call;
    s_test_config_ui_mesh_geometry_vertex_count_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_ui_mesh_geometry_config_reset(void) {
    test_call_control_reset(&s_test_config_ui_mesh_geometry_default_create);
    test_call_control_reset(&s_test_config_ui_mesh_geometry_create_from_vertices);
    test_call_control_reset(&s_test_config_ui_mesh_geometry_initialize_from_vertices);
    test_call_control_reset(&s_test_config_ui_mesh_geometry_vertices_get);
    test_call_control_reset(&s_test_config_ui_mesh_geometry_vertex_count_get);
}

void NO_COVERAGE test_ui_mesh_geometry(void) {
    test_ui_mesh_geometry_default_create();
    test_ui_mesh_geometry_create_from_vertices();
    test_ui_mesh_geometry_destroy();
    test_ui_mesh_geometry_initialize_from_vertices();
    test_ui_mesh_geometry_deinitialize();
    test_ui_mesh_geometry_name_get();
    test_ui_mesh_geometry_vertices_get();
    test_ui_mesh_geometry_vertex_count_get();
}

// Generated by ChatGPT
static void NO_COVERAGE test_ui_mesh_geometry_default_create(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // ui_mesh_geometry_default_create() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        s_test_config_ui_mesh_geometry_default_create.fail_on_call = 1U;
        s_test_config_ui_mesh_geometry_default_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t dummy_geometry = { 0 };
        ui_mesh_geometry_t* geometry = &dummy_geometry;

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_geometry == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        test_call_control_t config = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: ui_mesh_geometry_t が確保され、全フィールドが未初期化状態で初期化される
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_ui_mesh_geometry_create_from_vertices(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // ui_mesh_geometry_create_from_vertices() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        s_test_config_ui_mesh_geometry_create_from_vertices.fail_on_call = 1U;
        s_test_config_ui_mesh_geometry_create_from_vertices.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t dummy_geometry = { 0 };
        ui_mesh_geometry_t* geometry = &dummy_geometry;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_geometry == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // ui_mesh_geometry_default_create() が失敗 -> その戻り値を返し、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_ui_mesh_geometry_default_create.fail_on_call = 1U;
        s_test_config_ui_mesh_geometry_default_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // ui_mesh_geometry_default_create() 内部のmemory_system_allocate()が失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };
        test_call_control_t config = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // ui_mesh_geometry_initialize_from_vertices() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_ui_mesh_geometry_initialize_from_vertices.fail_on_call = 1U;
        s_test_config_ui_mesh_geometry_initialize_from_vertices.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_create_from_vertices(NULL, 6U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ == 0 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 0U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ != 6 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[5] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 5U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ が6の倍数でも6でなければ無効 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[12] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 12U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, NULL, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // choco_string_create_from_c_string() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };
        test_call_control_t config = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列用memory_system_allocate() が失敗 -> tmp_geometryはcleanupされ、geometryは変更されない
        // 1回目のallocateはui_mesh_geometry_t本体、2回目はchoco_string_t本体、3回目は文字列バッファ、4回目がtmp_vertices用
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };
        test_call_control_t config = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 4U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: ui_mesh_geometry_tを生成し、vertices_をdeep copyしてgeometryが所有する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(6U == geometry->vertex_count);
        assert(0 == strcmp("test_ui_geometry", choco_string_c_str(geometry->name)));

        assert(vertices != geometry->vertices);

        for(size_t i = 0; i != 6U; ++i) {
            assert(vertices[i].position.elem[0] == geometry->vertices[i].position.elem[0]);
            assert(vertices[i].position.elem[1] == geometry->vertices[i].position.elem[1]);
            assert(vertices[i].tex_coord.elem[0] == geometry->vertices[i].tex_coord.elem[0]);
            assert(vertices[i].tex_coord.elem[1] == geometry->vertices[i].tex_coord.elem[1]);
        }

        // 元配列を書き換えてもgeometry側には影響しない
        vertices[0].position = vec2f_initialize(100.0f, 100.0f);
        vertices[0].tex_coord = vec2f_initialize(100.0f, 100.0f);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(0.0f == geometry->vertices[0].tex_coord.elem[0]);
        assert(0.0f == geometry->vertices[0].tex_coord.elem[1]);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_ui_mesh_geometry_destroy(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // geometry_ == NULL -> 何もせずreturn
        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ui_mesh_geometry_destroy(NULL);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *geometry_ == NULL -> 何もせずreturn
        ui_mesh_geometry_t* geometry = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: name == NULL, vertices == NULL, vertex_count == 0 のgeometry本体だけを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);
        assert(NULL == geometry->name);
        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: nameのみを保持するgeometryを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        assert(NULL == geometry->vertices);
        assert(0U == geometry->vertex_count);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: verticesのみを保持するgeometryを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t* vertices = NULL;
        const size_t vertex_count = 6U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry->vertices = vertices;
        geometry->vertex_count = vertex_count;

        assert(NULL == geometry->name);
        assert(NULL != geometry->vertices);
        assert(6U == geometry->vertex_count);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: nameとverticesを保持するgeometryを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t* vertices = NULL;
        const size_t vertex_count = 6U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry->vertices = vertices;
        geometry->vertex_count = vertex_count;

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count == 0 の場合、verticesはdestroy側ではfreeされない
        // テスト側で後始末する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t* leaked_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t), MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry->vertices = leaked_vertices;
        geometry->vertex_count = 0U;

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        memory_system_free(leaked_vertices, sizeof(ui_vertex_t), MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count != 6 の場合、verticesはdestroy側ではfreeされない
        // テスト側で後始末する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t* leaked_vertices = NULL;
        const size_t vertex_count = 5U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry->vertices = leaked_vertices;
        geometry->vertex_count = vertex_count;

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        memory_system_free(leaked_vertices, sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertex_countが6の倍数でも6でなければ不正。verticesはdestroy側ではfreeされない
        // テスト側で後始末する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t* leaked_vertices = NULL;
        const size_t vertex_count = 12U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry->name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry->name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry->vertices = leaked_vertices;
        geometry->vertex_count = vertex_count;

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        memory_system_free(leaked_vertices, sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 二重destroy相当: 1回目でNULL化され、2回目は何もせずreturn
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_ui_mesh_geometry_initialize_from_vertices(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // ui_mesh_geometry_initialize_from_vertices() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_ui_mesh_geometry_initialize_from_vertices.fail_on_call = 1U;
        s_test_config_ui_mesh_geometry_initialize_from_vertices.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_RUNTIME_ERROR == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices(NULL, 6U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ == 0 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 0U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_ != 6 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[5] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 5U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count_が6の倍数でも6でなければ無効 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[12] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 12U, vertices, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, NULL, &geometry);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("already_initialized", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL != geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };
        ui_vertex_t dummy_vertices[6] = { 0 };

        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 0U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count != 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };

        geometry.name = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 6U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(6U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // choco_string_create_from_c_string() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };
        test_call_control_t config = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 頂点配列用memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        // 1回目のallocateはchoco_string_t本体、2回目は文字列バッファ、3回目がtmp_vertices用
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };
        test_call_control_t config = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 3U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: vertices_をdeep copyしてgeometryが所有する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(6U == geometry->vertex_count);

        assert(0 == strcmp("test_ui_geometry", choco_string_c_str(geometry->name)));

        assert(vertices != geometry->vertices);

        for(size_t i = 0; i != 6U; ++i) {
            assert(vertices[i].position.elem[0] == geometry->vertices[i].position.elem[0]);
            assert(vertices[i].position.elem[1] == geometry->vertices[i].position.elem[1]);
            assert(vertices[i].tex_coord.elem[0] == geometry->vertices[i].tex_coord.elem[0]);
            assert(vertices[i].tex_coord.elem[1] == geometry->vertices[i].tex_coord.elem[1]);
        }

        // 元配列を書き換えてもgeometry側には影響しない
        vertices[0].position = vec2f_initialize(100.0f, 100.0f);
        vertices[0].tex_coord = vec2f_initialize(100.0f, 100.0f);

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(1.0f == geometry->vertices[0].position.elem[1]);
        assert(0.0f == geometry->vertices[0].tex_coord.elem[0]);
        assert(0.0f == geometry->vertices[0].tex_coord.elem[1]);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_ui_mesh_geometry_deinitialize(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // geometry_ == NULL -> 何もせずreturn
        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ui_mesh_geometry_deinitialize(NULL);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 初期状態のgeometry -> 何も解放せず、初期状態のまま
        ui_mesh_geometry_t geometry = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: nameのみを保持するgeometryをdeinitialize
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: verticesのみを保持するgeometryをdeinitialize
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* vertices = NULL;
        const size_t vertex_count = 6U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry.name = NULL;
        geometry.vertices = vertices;
        geometry.vertex_count = vertex_count;

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: nameとverticesを保持するgeometryをdeinitialize
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* vertices = NULL;
        const size_t vertex_count = 6U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry.vertices = vertices;
        geometry.vertex_count = vertex_count;

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: initialize_from_vertices後にdeinitializeし、初期状態へ戻る
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry.name);
        assert(NULL != geometry.vertices);
        assert(6U == geometry.vertex_count);

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // deinitialize後に再初期化できる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry_0", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry_1", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry.name);
        assert(NULL != geometry.vertices);
        assert(6U == geometry.vertex_count);
        assert(0 == strcmp("test_ui_geometry_1", choco_string_c_str(geometry.name)));

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 二重deinitialize相当: 2回目は初期状態に対するdeinitializeとして何もせずreturn
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count == 0 の場合、verticesはdeinitialize側ではfreeされない
        // nameは解放される。verticesはテスト側で後始末する
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* leaked_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t), MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry.vertices = leaked_vertices;
        geometry.vertex_count = 0U;

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(leaked_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        memory_system_free(leaked_vertices, sizeof(ui_vertex_t), MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;
        geometry.vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count != 6 の場合、verticesはdeinitialize側ではfreeされない
        // nameは解放される。verticesはテスト側で後始末する
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* leaked_vertices = NULL;
        const size_t vertex_count = 5U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry.vertices = leaked_vertices;
        geometry.vertex_count = vertex_count;

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(leaked_vertices == geometry.vertices);
        assert(vertex_count == geometry.vertex_count);

        memory_system_free(leaked_vertices, sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertex_countが6の倍数でも6でなければ不正。verticesはdeinitialize側ではfreeされない
        // nameは解放される。verticesはテスト側で後始末する
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* leaked_vertices = NULL;
        const size_t vertex_count = 12U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        geometry.vertices = leaked_vertices;
        geometry.vertex_count = vertex_count;

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(leaked_vertices == geometry.vertices);
        assert(vertex_count == geometry.vertex_count);

        memory_system_free(leaked_vertices, sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_ui_mesh_geometry_name_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // geometry_ == NULL -> NULL
        const char* name = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        name = ui_mesh_geometry_name_get(NULL);
        assert(NULL == name);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name == NULL -> NULL
        ui_mesh_geometry_t geometry = { 0 };
        const char* name = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        name = ui_mesh_geometry_name_get(&geometry);
        assert(NULL == name);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // default_create直後はname未設定なのでNULL
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        const char* name = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_default_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);
        assert(NULL == geometry->name);

        name = ui_mesh_geometry_name_get(geometry);
        assert(NULL == name);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // nameのみを保持するgeometryから名称文字列を取得する
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        const char* name = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        name = ui_mesh_geometry_name_get(&geometry);
        assert(NULL != name);
        assert(0 == strcmp("test_ui_geometry", name));

        // 戻り値は内部文字列への参照
        assert(choco_string_c_str(geometry.name) == name);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // initialize_from_vertices後に名称文字列を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };
        const char* name = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);

        name = ui_mesh_geometry_name_get(&geometry);
        assert(NULL != name);
        assert(0 == strcmp("test_ui_geometry", name));
        assert(choco_string_c_str(geometry.name) == name);

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        name = ui_mesh_geometry_name_get(&geometry);
        assert(NULL == name);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // create_from_vertices後に名称文字列を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };
        const char* name = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        name = ui_mesh_geometry_name_get(geometry);
        assert(NULL != name);
        assert(0 == strcmp("test_ui_geometry", name));
        assert(choco_string_c_str(geometry->name) == name);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_ui_mesh_geometry_vertices_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // ui_mesh_geometry_vertices_get() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        const ui_vertex_t* out_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_ui_mesh_geometry_vertices_get.fail_on_call = 1U;
        s_test_config_ui_mesh_geometry_vertices_get.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = ui_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == out_vertices);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        const ui_vertex_t* out_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_vertices_get(NULL, &out_vertices);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == out_vertices);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_vertices_get(&geometry, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *out_vertices_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t dummy_vertices[6] = { 0 };
        const ui_vertex_t* out_vertices = dummy_vertices;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(dummy_vertices == out_vertices);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        const ui_vertex_t* out_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        geometry.name = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        ret = ui_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        const ui_vertex_t* out_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        ret = ui_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count == 0 かつ vertices != NULL -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* vertices = NULL;
        const ui_vertex_t* out_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t), MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry.vertices = vertices;
        geometry.vertex_count = 0U;

        ret = ui_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == out_vertices);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        memory_system_free(vertices, sizeof(ui_vertex_t), MEMORY_TAG_GEOMETRY);
        vertices = NULL;
        geometry.vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count != 6 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* vertices = NULL;
        const ui_vertex_t* out_vertices = NULL;
        const size_t vertex_count = 5U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry.vertices = vertices;
        geometry.vertex_count = vertex_count;

        ret = ui_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == out_vertices);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        memory_system_free(vertices, sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_countが6の倍数でも6でなければ不正 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* vertices = NULL;
        const ui_vertex_t* out_vertices = NULL;
        const size_t vertex_count = 12U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry.vertices = vertices;
        geometry.vertex_count = vertex_count;

        ret = ui_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == out_vertices);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        memory_system_free(vertices, sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // deinitialize後は未初期化扱い -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };
        const ui_vertex_t* out_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);

        ui_mesh_geometry_deinitialize(&geometry);

        ret = ui_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: initialize_from_vertices後、内部頂点配列への読み取り専用参照を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };
        const ui_vertex_t* out_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);

        ret = ui_mesh_geometry_vertices_get(&geometry, &out_vertices);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != out_vertices);

        // 返却値はコピーではなく、geometry内部配列への参照
        assert(geometry.vertices == out_vertices);
        assert(vertices != out_vertices);

        for(size_t i = 0; i != 6U; ++i) {
            assert(vertices[i].position.elem[0] == out_vertices[i].position.elem[0]);
            assert(vertices[i].position.elem[1] == out_vertices[i].position.elem[1]);
            assert(vertices[i].tex_coord.elem[0] == out_vertices[i].tex_coord.elem[0]);
            assert(vertices[i].tex_coord.elem[1] == out_vertices[i].tex_coord.elem[1]);
        }

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: create_from_vertices後、内部頂点配列への読み取り専用参照を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };
        const ui_vertex_t* out_vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = ui_mesh_geometry_vertices_get(geometry, &out_vertices);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != out_vertices);

        // 返却値はコピーではなく、geometry内部配列への参照
        assert(geometry->vertices == out_vertices);
        assert(vertices != out_vertices);

        for(size_t i = 0; i != 6U; ++i) {
            assert(vertices[i].position.elem[0] == out_vertices[i].position.elem[0]);
            assert(vertices[i].position.elem[1] == out_vertices[i].position.elem[1]);
            assert(vertices[i].tex_coord.elem[0] == out_vertices[i].tex_coord.elem[0]);
            assert(vertices[i].tex_coord.elem[1] == out_vertices[i].tex_coord.elem[1]);
        }

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_ui_mesh_geometry_vertex_count_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // ui_mesh_geometry_vertex_count_get() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        size_t out_vertex_count = 999U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_ui_mesh_geometry_vertex_count_get.fail_on_call = 1U;
        s_test_config_ui_mesh_geometry_vertex_count_get.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = ui_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(999U == out_vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_vertex_count_get(NULL, &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(999U == out_vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_vertex_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_vertex_count_get(&geometry, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        ui_mesh_geometry_t geometry = { 0 };
        size_t out_vertex_count = 999U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        geometry.name = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        ret = ui_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(999U == out_vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        size_t out_vertex_count = 999U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        geometry.vertices = NULL;
        geometry.vertex_count = 6U;

        ret = ui_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(999U == out_vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count == 0 かつ vertices != NULL -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* vertices = NULL;
        size_t out_vertex_count = 999U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t), MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry.vertices = vertices;
        geometry.vertex_count = 0U;

        ret = ui_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(999U == out_vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        memory_system_free(vertices, sizeof(ui_vertex_t), MEMORY_TAG_GEOMETRY);
        vertices = NULL;
        geometry.vertices = NULL;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count != 6 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* vertices = NULL;
        const size_t vertex_count = 5U;
        size_t out_vertex_count = 999U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry.vertices = vertices;
        geometry.vertex_count = vertex_count;

        ret = ui_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(999U == out_vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        memory_system_free(vertices, sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_countが6の倍数でも6でなければ不正 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t* vertices = NULL;
        const size_t vertex_count = 12U;
        size_t out_vertex_count = 999U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("test_ui_geometry", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret_mem = memory_system_allocate(sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        geometry.vertices = vertices;
        geometry.vertex_count = vertex_count;

        ret = ui_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(999U == out_vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        memory_system_free(vertices, sizeof(ui_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        vertices = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 0U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // deinitialize後は未初期化扱い -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };
        size_t out_vertex_count = 999U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);

        ui_mesh_geometry_deinitialize(&geometry);

        ret = ui_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(999U == out_vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: initialize_from_vertices後、頂点数6を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t geometry = { 0 };
        ui_vertex_t vertices[6] = { 0 };
        size_t out_vertex_count = 0U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_initialize_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);

        ret = ui_mesh_geometry_vertex_count_get(&geometry, &out_vertex_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(6U == out_vertex_count);
        assert(geometry.vertex_count == out_vertex_count);

        ui_mesh_geometry_deinitialize(&geometry);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: create_from_vertices後、頂点数6を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        ui_mesh_geometry_t* geometry = NULL;
        ui_vertex_t vertices[6] = { 0 };
        size_t out_vertex_count = 0U;

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        vertices[0].position = vec2f_initialize(0.0f, 1.0f);
        vertices[0].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[1].position = vec2f_initialize(2.0f, 3.0f);
        vertices[1].tex_coord = vec2f_initialize(0.0f, 1.0f);
        vertices[2].position = vec2f_initialize(4.0f, 5.0f);
        vertices[2].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[3].position = vec2f_initialize(6.0f, 7.0f);
        vertices[3].tex_coord = vec2f_initialize(0.0f, 0.0f);
        vertices[4].position = vec2f_initialize(8.0f, 9.0f);
        vertices[4].tex_coord = vec2f_initialize(1.0f, 1.0f);
        vertices[5].position = vec2f_initialize(10.0f, 11.0f);
        vertices[5].tex_coord = vec2f_initialize(1.0f, 0.0f);

        ret = ui_mesh_geometry_create_from_vertices("test_ui_geometry", 6U, vertices, &geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = ui_mesh_geometry_vertex_count_get(geometry, &out_vertex_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(6U == out_vertex_count);
        assert(geometry->vertex_count == out_vertex_count);

        ui_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_ui_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

#endif
