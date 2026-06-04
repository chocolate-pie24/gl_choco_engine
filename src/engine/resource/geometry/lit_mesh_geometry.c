#include "engine/resource/geometry/lit_mesh_geometry.h"

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

#include "engine/resource/loaders/stl_loader.h"

#include "engine/containers/choco_string.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_math/math_types.h"

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

// texture用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_lit_mesh_geometry_create;                      /**< lit_mesh_geometry_create()テスト設定 */
static test_call_control_t s_test_config_lit_mesh_geometry_initialize_from_vertices;    /**< lit_mesh_geometry_initialize_from_vertices()テスト設定 */
static test_call_control_t s_test_config_lit_mesh_geometry_initialize_from_file;        /**< lit_mesh_geometry_initialize_from_file()テスト設定 */
static test_call_control_t s_test_config_lit_mesh_geometry_vertices_get;                /**< lit_mesh_geometry_vertices_get()テスト設定 */
static test_call_control_t s_test_config_lit_mesh_geometry_vertex_count_get;            /**< lit_mesh_geometry_vertex_count_get()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_lit_mesh_geometry_create(void);
static void test_lit_mesh_geometry_destroy(void);
static void test_lit_mesh_geometry_initialize_from_vertices(void);
static void test_lit_mesh_geometry_initialize_from_file(void);
static void test_lit_mesh_geometry_name_get(void);
static void test_lit_mesh_geometry_vertices_get(void);
static void test_lit_mesh_geometry_vertex_count_get(void);

// テスト用ヘルパー関数

#endif

resource_result_t lit_mesh_geometry_create(lit_mesh_geometry_t** geometry_) {
#ifdef TEST_BUILD
    s_test_config_lit_mesh_geometry_create.call_count++;
    if(s_test_config_lit_mesh_geometry_create.fail_on_call != 0) {
        if(s_test_config_lit_mesh_geometry_create.call_count == s_test_config_lit_mesh_geometry_create.fail_on_call) {
            return (resource_result_t)s_test_config_lit_mesh_geometry_create.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    lit_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "*geometry_")

    ret_mem = memory_system_allocate(sizeof(lit_mesh_geometry_t), MEMORY_TAG_GEOMETRY, (void**)&tmp_geometry);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("lit_mesh_geometry_create(%s) - Failed to allocate lit_mesh_geometry_t instance.", resource_rslt_to_str(ret));
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
    if(NULL != (*geometry_)->vertices && 0 != (*geometry_)->vertex_count) {
        memory_system_free((*geometry_)->vertices, sizeof(point_normal_vertex_t) * (*geometry_)->vertex_count, MEMORY_TAG_GEOMETRY);
        (*geometry_)->vertices = NULL;
        (*geometry_)->vertex_count = 0;
    } else if(NULL != (*geometry_)->vertices && 0 == (*geometry_)->vertex_count) {
        ERROR_MESSAGE("lit_mesh_geometry_destroy(%s) - lit_mesh_geometry internal state is inconsistent: vertices is not NULL but vertex_count is 0. CPU-side vertex array was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    }

    memory_system_free(*geometry_, sizeof(lit_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
    *geometry_ = NULL;
}

// lit_mesh_geometry_tの内部リソースはlit_mesh_geometryが所有するため、一度初期化したあと、destroyをせずに再初期化するのは禁止
// 失敗時はgeometry_は不変
resource_result_t lit_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t* geometry_) {
#ifdef TEST_BUILD
    s_test_config_lit_mesh_geometry_initialize_from_vertices.call_count++;
    if(s_test_config_lit_mesh_geometry_initialize_from_vertices.fail_on_call != 0) {
        if(s_test_config_lit_mesh_geometry_initialize_from_vertices.call_count == s_test_config_lit_mesh_geometry_initialize_from_vertices.fail_on_call) {
            return (resource_result_t)s_test_config_lit_mesh_geometry_initialize_from_vertices.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    choco_string_t* tmp_name = NULL;
    point_normal_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_vertices", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_vertices", "vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_vertices", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_vertices", "geometry_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_vertices", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_vertices", "geometry_->vertex_count")

    ret_string = choco_string_create_from_c_string(name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("lit_mesh_geometry_initialize_from_vertices(%s) - Failed to create lit mesh geometry name string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX / vertex_count_) < sizeof(point_normal_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("lit_mesh_geometry_initialize_from_vertices(%s) - CPU-side vertex array size overflow. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(point_normal_vertex_t));
        goto cleanup;
    }
    ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("lit_mesh_geometry_initialize_from_vertices(%s) - Failed to allocate CPU-side vertex array. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(point_normal_vertex_t));
        goto cleanup;
    }

    for(size_t i = 0; i != vertex_count_; ++i) {
        vec4i8_initialize(vertices_[i].normal.elem[0], vertices_[i].normal.elem[1], vertices_[i].normal.elem[2], vertices_[i].normal.elem[3], &tmp_vertices[i].normal);
        vec3f_initialize(vertices_[i].position.elem[0], vertices_[i].position.elem[1], vertices_[i].position.elem[2], &tmp_vertices[i].position);
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

// lit_mesh_geometry_tの内部リソースはlit_mesh_geometryが所有するため、一度初期化したあと、destroyをせずに再初期化するのは禁止
// 失敗時にはgeometry_は不変
resource_result_t lit_mesh_geometry_initialize_from_file(const char* path_, const char* name_, const char* extension_, lit_mesh_geometry_t* geometry_) {
#ifdef TEST_BUILD
    s_test_config_lit_mesh_geometry_initialize_from_file.call_count++;
    if(s_test_config_lit_mesh_geometry_initialize_from_file.fail_on_call != 0) {
        if(s_test_config_lit_mesh_geometry_initialize_from_file.call_count == s_test_config_lit_mesh_geometry_initialize_from_file.fail_on_call) {
            return (resource_result_t)s_test_config_lit_mesh_geometry_initialize_from_file.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    stl_loader_t* stl_loader = NULL;
    point_normal_vertex_t* tmp_vertices = NULL;
    choco_string_t* tmp_name = NULL;
    size_t tmp_vertex_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_file", "path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_file", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_file", "extension_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_file", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_file", "geometry_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_file", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_file", "geometry_->vertex_count")

    if(choco_string_equal(".stl", extension_)) {
        ret = stl_loader_create(&stl_loader);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Failed to create stl_loader_t instance for lit mesh geometry loading.", resource_rslt_to_str(ret));
            goto cleanup;
        }
        ret = stl_loader_ascii_load(path_, name_, extension_, stl_loader);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Failed to load ASCII STL file for lit mesh geometry. name = '%s', extension = '%s'.", resource_rslt_to_str(ret), name_, extension_);
            goto cleanup;
        }
        ret = stl_loader_vertices_move(stl_loader, &tmp_vertices, &tmp_vertex_count);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Failed to move vertices from STL loader.", resource_rslt_to_str(ret));
            goto cleanup;
        }
        ret_string = choco_string_create_from_c_string(name_, &tmp_name);
        if(CHOCO_STRING_SUCCESS != ret_string) {
            ret = resource_rslt_convert_choco_string(ret_string);
            ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Failed to create lit mesh geometry name string.", resource_rslt_to_str(ret));
            goto cleanup;
        }

        geometry_->name = tmp_name;
        geometry_->vertex_count = tmp_vertex_count;
        geometry_->vertices = tmp_vertices;
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Unsupported lit mesh geometry file extension '%s'. Currently supported: '.stl' ASCII STL.", resource_rslt_to_str(ret), extension_);
        goto cleanup;
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_name) {
            choco_string_destroy(&tmp_name);
        }
        if(NULL != tmp_vertices) {
            memory_system_free(tmp_vertices, sizeof(point_normal_vertex_t) * tmp_vertex_count, MEMORY_TAG_GEOMETRY);
            tmp_vertices = NULL;
        }
    }
    if(NULL != stl_loader) {
        stl_loader_destroy(&stl_loader);
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

    *out_vertex_count_ = geometry_->vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

#ifdef TEST_BUILD

void NO_COVERAGE test_lit_mesh_geometry_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_lit_mesh_geometry_create.fail_on_call = config_->fail_on_call;
    s_test_config_lit_mesh_geometry_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_lit_mesh_geometry_initialize_from_vertices_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_lit_mesh_geometry_initialize_from_vertices.fail_on_call = config_->fail_on_call;
    s_test_config_lit_mesh_geometry_initialize_from_vertices.forced_result = config_->forced_result;
}

void NO_COVERAGE test_lit_mesh_geometry_initialize_from_file_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_lit_mesh_geometry_initialize_from_file.fail_on_call = config_->fail_on_call;
    s_test_config_lit_mesh_geometry_initialize_from_file.forced_result = config_->forced_result;
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
    test_call_control_reset(&s_test_config_lit_mesh_geometry_create);
    test_call_control_reset(&s_test_config_lit_mesh_geometry_initialize_from_vertices);
    test_call_control_reset(&s_test_config_lit_mesh_geometry_initialize_from_file);
    test_call_control_reset(&s_test_config_lit_mesh_geometry_vertices_get);
    test_call_control_reset(&s_test_config_lit_mesh_geometry_vertex_count_get);
}

void NO_COVERAGE test_lit_mesh_geometry(void) {
    test_lit_mesh_geometry_create();
    test_lit_mesh_geometry_destroy();
    test_lit_mesh_geometry_initialize_from_vertices();
    test_lit_mesh_geometry_initialize_from_file();
    test_lit_mesh_geometry_name_get();
    test_lit_mesh_geometry_vertices_get();
    test_lit_mesh_geometry_vertex_count_get();
}

// Generated by ChatGPT
static void test_lit_mesh_geometry_create(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());
    {
        // lit_mesh_geometry_create() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t* geometry = NULL;

        test_lit_mesh_geometry_config_reset();
        test_choco_memory_config_reset();

        s_test_config_lit_mesh_geometry_create.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = lit_mesh_geometry_create(&geometry);
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

        ret = lit_mesh_geometry_create(NULL);
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

        ret = lit_mesh_geometry_create(&geometry);
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

        ret = lit_mesh_geometry_create(&geometry);
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

        ret = lit_mesh_geometry_create(&geometry);
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
static void test_lit_mesh_geometry_destroy(void) {
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

        ret = lit_mesh_geometry_create(&geometry);
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

        ret = lit_mesh_geometry_create(&geometry);
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

        ret = lit_mesh_geometry_create(&geometry);
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

        ret = lit_mesh_geometry_create(&geometry);
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

        ret = lit_mesh_geometry_create(&geometry);
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
static void test_lit_mesh_geometry_initialize_from_vertices(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // lit_mesh_geometry_initialize_from_vertices() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t vertices[3] = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_lit_mesh_geometry_initialize_from_vertices.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_initialize_from_vertices.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, &geometry);
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

        ret = lit_mesh_geometry_initialize_from_vertices(NULL, 3U, vertices, &geometry);
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

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 0U, vertices, &geometry);
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

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, NULL, &geometry);
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

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, NULL);
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

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, &geometry);
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

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, &geometry);
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

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, &geometry);
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

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, &geometry);
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

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", SIZE_MAX, &dummy_vertex, &geometry);
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

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, &geometry);
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

        vec3f_initialize(0.0f, 1.0f, 2.0f, &vertices[0].position);
        vec4i8_initialize(0, 0, 127, 0, &vertices[0].normal);

        vec3f_initialize(3.0f, 4.0f, 5.0f, &vertices[1].position);
        vec4i8_initialize(0, 127, 0, 0, &vertices[1].normal);

        vec3f_initialize(6.0f, 7.0f, 8.0f, &vertices[2].position);
        vec4i8_initialize(127, 0, 0, 0, &vertices[2].normal);

        ret = lit_mesh_geometry_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, geometry);
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
        vec3f_initialize(100.0f, 100.0f, 100.0f, &vertices[0].position);
        vec4i8_initialize(-1, -1, -1, -1, &vertices[0].normal);

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
static void test_lit_mesh_geometry_initialize_from_file(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // テスト用ASCII STLファイル生成
        FILE* fp = NULL;

        fp = fopen("assets/test/filesystem/test_lit_mesh_geometry_from_file_valid_1_triangle.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_lit_mesh_geometry_from_file_invalid_stl.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));
    }
    {
        // lit_mesh_geometry_initialize_from_file() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_lit_mesh_geometry_initialize_from_file.fail_on_call = 1U;
        s_test_config_lit_mesh_geometry_initialize_from_file.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            &geometry
        );
        assert(RESOURCE_RUNTIME_ERROR == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // path_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize_from_file(
            NULL,
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            &geometry
        );
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            NULL,
            ".stl",
            &geometry
        );
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // extension_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            NULL,
            &geometry
        );
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            NULL
        );
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->name != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_string = choco_string_create_from_c_string("already_initialized", &geometry.name);
        assert(CHOCO_STRING_SUCCESS == ret_string);
        assert(NULL != geometry.name);

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            &geometry
        );
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL != geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        choco_string_destroy(&geometry.name);
        assert(NULL == geometry.name);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertices != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };

        geometry.vertices = dummy_vertices;
        geometry.vertex_count = 0U;

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            &geometry
        );
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(dummy_vertices == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // geometry_->vertex_count != 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };

        geometry.name = NULL;
        geometry.vertices = NULL;
        geometry.vertex_count = 3U;

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            &geometry
        );
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(3U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 未対応拡張子 -> RESOURCE_UNSUPPORTED_FILE
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".obj",
            &geometry
        );
        assert(RESOURCE_UNSUPPORTED_FILE == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stl_loader_create() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_NO_MEMORY;
        test_stl_loader_create_config_set(&config);

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            &geometry
        );
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stl_loader_ascii_load() 失敗 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_DATA_CORRUPTED;
        test_stl_loader_ascii_load_config_set(&config);

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            &geometry
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stl_loader_vertices_move() 失敗 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_BAD_OPERATION;
        test_stl_loader_vertices_move_config_set(&config);

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            &geometry
        );
        assert(RESOURCE_BAD_OPERATION == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stl_loader_vertices_move()成功後のname生成失敗 -> RESOURCE_NO_MEMORY
        // tmp_verticesはcleanupでfreeされ、geometry_は不変のまま
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };
        test_call_control_t config = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 7U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            &geometry
        );
        assert(RESOURCE_NO_MEMORY == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 実STL構造不正 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        lit_mesh_geometry_t geometry = { 0 };

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_invalid_stl",
            ".stl",
            &geometry
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);

        assert(NULL == geometry.name);
        assert(NULL == geometry.vertices);
        assert(0U == geometry.vertex_count);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: ASCII STLからlit_mesh_geometryを初期化する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        lit_mesh_geometry_t* geometry = NULL;

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = lit_mesh_geometry_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize_from_file(
            "assets/test/filesystem/",
            "test_lit_mesh_geometry_from_file_valid_1_triangle",
            ".stl",
            geometry
        );
        assert(RESOURCE_SUCCESS == ret);

        assert(NULL != geometry->name);
        assert(NULL != geometry->vertices);
        assert(3U == geometry->vertex_count);

        assert(0 == strcmp("test_lit_mesh_geometry_from_file_valid_1_triangle", choco_string_c_str(geometry->name)));

        assert(0.0f == geometry->vertices[0].position.elem[0]);
        assert(0.0f == geometry->vertices[0].position.elem[1]);
        assert(0.0f == geometry->vertices[0].position.elem[2]);
        assert(0 == geometry->vertices[0].normal.elem[0]);
        assert(0 == geometry->vertices[0].normal.elem[1]);
        assert(127 == geometry->vertices[0].normal.elem[2]);

        assert(1.0f == geometry->vertices[1].position.elem[0]);
        assert(0.0f == geometry->vertices[1].position.elem[1]);
        assert(0.0f == geometry->vertices[1].position.elem[2]);
        assert(0 == geometry->vertices[1].normal.elem[0]);
        assert(0 == geometry->vertices[1].normal.elem[1]);
        assert(127 == geometry->vertices[1].normal.elem[2]);

        assert(0.0f == geometry->vertices[2].position.elem[0]);
        assert(1.0f == geometry->vertices[2].position.elem[1]);
        assert(0.0f == geometry->vertices[2].position.elem[2]);
        assert(0 == geometry->vertices[2].normal.elem[0]);
        assert(0 == geometry->vertices[2].normal.elem[1]);
        assert(127 == geometry->vertices[2].normal.elem[2]);

        lit_mesh_geometry_destroy(&geometry);
        assert(NULL == geometry);

        test_lit_mesh_geometry_config_reset();
        test_stl_loader_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // テスト用ASCII STLファイル削除
        assert(0 == remove("assets/test/filesystem/test_lit_mesh_geometry_from_file_valid_1_triangle.stl"));
        assert(0 == remove("assets/test/filesystem/test_lit_mesh_geometry_from_file_invalid_stl.stl"));
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void test_lit_mesh_geometry_name_get(void) {
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

        vec3f_initialize(0.0f, 0.0f, 0.0f, &vertices[0].position);
        vec4i8_initialize(0, 0, 127, 0, &vertices[0].normal);

        vec3f_initialize(1.0f, 0.0f, 0.0f, &vertices[1].position);
        vec4i8_initialize(0, 0, 127, 0, &vertices[1].normal);

        vec3f_initialize(0.0f, 1.0f, 0.0f, &vertices[2].position);
        vec4i8_initialize(0, 0, 127, 0, &vertices[2].normal);

        ret = lit_mesh_geometry_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, geometry);
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
static void test_lit_mesh_geometry_vertices_get(void) {
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

        vec3f_initialize(0.0f, 1.0f, 2.0f, &vertices[0].position);
        vec4i8_initialize(0, 0, 127, 0, &vertices[0].normal);

        vec3f_initialize(3.0f, 4.0f, 5.0f, &vertices[1].position);
        vec4i8_initialize(0, 127, 0, 0, &vertices[1].normal);

        vec3f_initialize(6.0f, 7.0f, 8.0f, &vertices[2].position);
        vec4i8_initialize(127, 0, 0, 0, &vertices[2].normal);

        ret = lit_mesh_geometry_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, geometry);
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
static void test_lit_mesh_geometry_vertex_count_get(void) {
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

        vec3f_initialize(0.0f, 1.0f, 2.0f, &vertices[0].position);
        vec4i8_initialize(0, 0, 127, 0, &vertices[0].normal);

        vec3f_initialize(3.0f, 4.0f, 5.0f, &vertices[1].position);
        vec4i8_initialize(0, 127, 0, 0, &vertices[1].normal);

        vec3f_initialize(6.0f, 7.0f, 8.0f, &vertices[2].position);
        vec4i8_initialize(127, 0, 0, 0, &vertices[2].normal);

        ret = lit_mesh_geometry_create(&geometry);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != geometry);

        ret = lit_mesh_geometry_initialize_from_vertices("test_geometry", 3U, vertices, geometry);
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
