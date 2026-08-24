// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup core
 *
 * @file aabb_3d.c
 * @author chocolate-pie24
 * @brief 3次元AABB幾何情報構造体, API実装
 *
 * @date 2026-06-09
 *
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/core/geometry_primitive/geometry_primitive_types.h"
#include "engine/core/geometry_primitive/geometry_primitive_err_utils.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_macros.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <math.h>

#include "test_controller.h"
#include "engine/base/choco_macros.h"
#include "engine/core/geometry_primitive/test_aabb_3d.h"

// aabb_3d用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_aabb_3d_initialize_from_min_max;               /**< aabb_3d_initialize_from_min_max()テスト設定 */
static test_call_control_t s_test_config_aabb_3d_initialize_from_point_vertices;        /**< aabb_3d_initialize_from_point_vertices()テスト設定 */
static test_call_control_t s_test_config_aabb_3d_initialize_from_line_vertices;         /**< aabb_3d_initialize_from_line_vertices()テスト設定 */
static test_call_control_t s_test_config_aabb_3d_initialize_from_point_normal_vertices; /**< aabb_3d_initialize_from_point_normal_vertices()テスト設定 */
static test_call_control_t s_test_config_aabb_3d_vertices_get;                          /**< aabb_3d_vertices_get()テスト設定 */
static test_call_control_bool_t s_test_config_aabb_3d_is_valid;                         /**< aabb_3d_is_valid()テスト設定 */

// プライベート関数テスト設定

// 全テスト関数プロトタイプ宣言
static void test_aabb_3d_initialize_from_min_max(void);
static void test_aabb_3d_initialize_from_point_vertices(void);
static void test_aabb_3d_initialize_from_line_vertices(void);
static void test_aabb_3d_initialize_from_point_normal_vertices(void);
static void test_aabb_3d_reset(void);
static void test_aabb_3d_vertices_get(void);
static void test_aabb_3d_is_valid(void);
#endif

geometry_primitive_result_t aabb_3d_initialize_from_min_max(vec3f_t min_, vec3f_t max_, aabb_3d_t* out_aabb_) {
#ifdef TEST_BUILD
    s_test_config_aabb_3d_initialize_from_min_max.call_count++;
    if(s_test_config_aabb_3d_initialize_from_min_max.fail_on_call != 0) {
        if(s_test_config_aabb_3d_initialize_from_min_max.call_count == s_test_config_aabb_3d_initialize_from_min_max.fail_on_call) {
            return (geometry_primitive_result_t)s_test_config_aabb_3d_initialize_from_min_max.forced_result;
        }
    }
#endif
    geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
    aabb_3d_t tmp_aabb = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_min_max", "out_aabb_")

    tmp_aabb.min = min_;
    tmp_aabb.max = max_;

    if(!aabb_3d_is_valid(&tmp_aabb)) {
        ret = GEOMETRY_PRIMITIVE_DATA_CORRUPTED;
        ERROR_MESSAGE("aabb_3d_initialize_from_min_max(%s) - Provided min_ and max_ do not form a valid AABB. min = [%f, %f, %f], max = [%f, %f, %f].", geometry_primitive_rslt_to_str(ret), tmp_aabb.min.elem[0], tmp_aabb.min.elem[1], tmp_aabb.min.elem[2], tmp_aabb.max.elem[0], tmp_aabb.max.elem[1], tmp_aabb.max.elem[2]);
        goto cleanup;
    }

    *out_aabb_ = tmp_aabb;

    ret = GEOMETRY_PRIMITIVE_SUCCESS;

cleanup:
    return ret;
}

geometry_primitive_result_t aabb_3d_initialize_from_point_vertices(const point_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_) {
#ifdef TEST_BUILD
    s_test_config_aabb_3d_initialize_from_point_vertices.call_count++;
    if(s_test_config_aabb_3d_initialize_from_point_vertices.fail_on_call != 0) {
        if(s_test_config_aabb_3d_initialize_from_point_vertices.call_count == s_test_config_aabb_3d_initialize_from_point_vertices.fail_on_call) {
            return (geometry_primitive_result_t)s_test_config_aabb_3d_initialize_from_point_vertices.forced_result;
        }
    }
#endif
    geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
    aabb_3d_t tmp_aabb = { 0 };
    vec3f_t min = { 0 };
    vec3f_t max = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_vertices", "out_aabb_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_vertices", "vertex_count_")

    min = vertices_[0].position;
    max = vertices_[0].position;
    for(size_t i = 0; i != vertex_count_; ++i) {
        if(!vec3f_is_finite(vertices_[i].position)) {
            ret = GEOMETRY_PRIMITIVE_DATA_CORRUPTED;
            ERROR_MESSAGE("aabb_3d_initialize_from_point_vertices(%s) - Provided vertices_[%zu].position contains NaN or Inf. position = [%f, %f, %f].", geometry_primitive_rslt_to_str(ret), i, vertices_[i].position.elem[0], vertices_[i].position.elem[1], vertices_[i].position.elem[2]);
            goto cleanup;
        }

        min = vec3f_component_min(min, vertices_[i].position);
        max = vec3f_component_max(max, vertices_[i].position);
    }
    tmp_aabb.min = min;
    tmp_aabb.max = max;

    if(!aabb_3d_is_valid(&tmp_aabb)) {
        ret = GEOMETRY_PRIMITIVE_RUNTIME_ERROR;
        ERROR_MESSAGE("aabb_3d_initialize_from_point_vertices(%s) - Internal invariant check failed after AABB calculation. min = [%f, %f, %f], max = [%f, %f, %f].", geometry_primitive_rslt_to_str(ret), tmp_aabb.min.elem[0], tmp_aabb.min.elem[1], tmp_aabb.min.elem[2], tmp_aabb.max.elem[0], tmp_aabb.max.elem[1], tmp_aabb.max.elem[2]);
        goto cleanup;
    }

    *out_aabb_ = tmp_aabb;

    ret = GEOMETRY_PRIMITIVE_SUCCESS;

cleanup:
    return ret;
}

geometry_primitive_result_t aabb_3d_initialize_from_line_vertices(const line_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_) {
#ifdef TEST_BUILD
    s_test_config_aabb_3d_initialize_from_line_vertices.call_count++;
    if(s_test_config_aabb_3d_initialize_from_line_vertices.fail_on_call != 0) {
        if(s_test_config_aabb_3d_initialize_from_line_vertices.call_count == s_test_config_aabb_3d_initialize_from_line_vertices.fail_on_call) {
            return (geometry_primitive_result_t)s_test_config_aabb_3d_initialize_from_line_vertices.forced_result;
        }
    }
#endif
    geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
    aabb_3d_t tmp_aabb = { 0 };
    vec3f_t min = { 0 };
    vec3f_t max = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_line_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_line_vertices", "out_aabb_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_line_vertices", "vertex_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (vertex_count_ % 2), ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_line_vertices", "vertex_count_")

    min = vertices_[0].position;
    max = vertices_[0].position;
    for(size_t i = 0; i != vertex_count_; ++i) {
        if(!vec3f_is_finite(vertices_[i].position)) {
            ret = GEOMETRY_PRIMITIVE_DATA_CORRUPTED;
            ERROR_MESSAGE("aabb_3d_initialize_from_line_vertices(%s) - Provided vertices_[%zu].position contains NaN or Inf. position = [%f, %f, %f].", geometry_primitive_rslt_to_str(ret), i, vertices_[i].position.elem[0], vertices_[i].position.elem[1], vertices_[i].position.elem[2]);
            goto cleanup;
        }

        min = vec3f_component_min(min, vertices_[i].position);
        max = vec3f_component_max(max, vertices_[i].position);
    }
    tmp_aabb.min = min;
    tmp_aabb.max = max;

    if(!aabb_3d_is_valid(&tmp_aabb)) {
        ret = GEOMETRY_PRIMITIVE_RUNTIME_ERROR;
        ERROR_MESSAGE("aabb_3d_initialize_from_line_vertices(%s) - Internal invariant check failed after AABB calculation. min = [%f, %f, %f], max = [%f, %f, %f].", geometry_primitive_rslt_to_str(ret), tmp_aabb.min.elem[0], tmp_aabb.min.elem[1], tmp_aabb.min.elem[2], tmp_aabb.max.elem[0], tmp_aabb.max.elem[1], tmp_aabb.max.elem[2]);
        goto cleanup;
    }

    *out_aabb_ = tmp_aabb;

    ret = GEOMETRY_PRIMITIVE_SUCCESS;

cleanup:
    return ret;
}

geometry_primitive_result_t aabb_3d_initialize_from_point_normal_vertices(const point_normal_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_) {
#ifdef TEST_BUILD
    s_test_config_aabb_3d_initialize_from_point_normal_vertices.call_count++;
    if(s_test_config_aabb_3d_initialize_from_point_normal_vertices.fail_on_call != 0) {
        if(s_test_config_aabb_3d_initialize_from_point_normal_vertices.call_count == s_test_config_aabb_3d_initialize_from_point_normal_vertices.fail_on_call) {
            return (geometry_primitive_result_t)s_test_config_aabb_3d_initialize_from_point_normal_vertices.forced_result;
        }
    }
#endif
    geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
    aabb_3d_t tmp_aabb = { 0 };
    vec3f_t min = { 0 };
    vec3f_t max = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_normal_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_normal_vertices", "out_aabb_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_normal_vertices", "vertex_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (vertex_count_ % 3), ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_normal_vertices", "vertex_count_")

    min = vertices_[0].position;
    max = vertices_[0].position;
    for(size_t i = 0; i != vertex_count_; ++i) {
        if(!vec3f_is_finite(vertices_[i].position)) {
            ret = GEOMETRY_PRIMITIVE_DATA_CORRUPTED;
            ERROR_MESSAGE("aabb_3d_initialize_from_point_normal_vertices(%s) - Provided vertices_[%zu].position contains NaN or Inf. position = [%f, %f, %f].", geometry_primitive_rslt_to_str(ret), i, vertices_[i].position.elem[0], vertices_[i].position.elem[1], vertices_[i].position.elem[2]);
            goto cleanup;
        }

        min = vec3f_component_min(min, vertices_[i].position);
        max = vec3f_component_max(max, vertices_[i].position);
    }
    tmp_aabb.min = min;
    tmp_aabb.max = max;

    if(!aabb_3d_is_valid(&tmp_aabb)) {
        ret = GEOMETRY_PRIMITIVE_RUNTIME_ERROR;
        ERROR_MESSAGE("aabb_3d_initialize_from_point_normal_vertices(%s) - Internal invariant check failed after AABB calculation. min = [%f, %f, %f], max = [%f, %f, %f].", geometry_primitive_rslt_to_str(ret), tmp_aabb.min.elem[0], tmp_aabb.min.elem[1], tmp_aabb.min.elem[2], tmp_aabb.max.elem[0], tmp_aabb.max.elem[1], tmp_aabb.max.elem[2]);
        goto cleanup;
    }

    *out_aabb_ = tmp_aabb;

    ret = GEOMETRY_PRIMITIVE_SUCCESS;

cleanup:
    return ret;
}

void aabb_3d_reset(aabb_3d_t* aabb_) {
    if(NULL == aabb_) {
        return;
    }
    aabb_->min.elem[0] = 0.0f;
    aabb_->min.elem[1] = 0.0f;
    aabb_->min.elem[2] = 0.0f;

    aabb_->max.elem[0] = 0.0f;
    aabb_->max.elem[1] = 0.0f;
    aabb_->max.elem[2] = 0.0f;
}

geometry_primitive_result_t aabb_3d_vertices_get(const aabb_3d_t* aabb_, vec3f_t vertices_[8]) {
#ifdef TEST_BUILD
    s_test_config_aabb_3d_vertices_get.call_count++;
    if(s_test_config_aabb_3d_vertices_get.fail_on_call != 0) {
        if(s_test_config_aabb_3d_vertices_get.call_count == s_test_config_aabb_3d_vertices_get.fail_on_call) {
            return (geometry_primitive_result_t)s_test_config_aabb_3d_vertices_get.forced_result;
        }
    }
#endif
    geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_vertices_get", "aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_vertices_get", "vertices_")

    if(!aabb_3d_is_valid(aabb_)) {
        ret = GEOMETRY_PRIMITIVE_BAD_OPERATION;
        ERROR_MESSAGE("aabb_3d_vertices_get(%s) - Cannot generate vertices from invalid aabb_. min = [%f, %f, %f], max = [%f, %f, %f].", geometry_primitive_rslt_to_str(ret), aabb_->min.elem[0], aabb_->min.elem[1], aabb_->min.elem[2], aabb_->max.elem[0], aabb_->max.elem[1], aabb_->max.elem[2]);
        goto cleanup;
    }

    vertices_[0] = vec3f_initialize(aabb_->min.elem[0], aabb_->min.elem[1], aabb_->max.elem[2]);    // min_x, min_y, max_z
    vertices_[1] = vec3f_initialize(aabb_->max.elem[0], aabb_->min.elem[1], aabb_->max.elem[2]);    // max_x, min_y, max_z
    vertices_[2] = vec3f_initialize(aabb_->max.elem[0], aabb_->min.elem[1], aabb_->min.elem[2]);    // max_x, min_y, min_z
    vertices_[3] = vec3f_initialize(aabb_->min.elem[0], aabb_->min.elem[1], aabb_->min.elem[2]);    // min_x, min_y, min_z

    vertices_[4] = vec3f_initialize(aabb_->min.elem[0], aabb_->max.elem[1], aabb_->max.elem[2]);    // min_x, max_y, max_z
    vertices_[5] = vec3f_initialize(aabb_->max.elem[0], aabb_->max.elem[1], aabb_->max.elem[2]);    // max_x, max_y, max_z
    vertices_[6] = vec3f_initialize(aabb_->max.elem[0], aabb_->max.elem[1], aabb_->min.elem[2]);    // max_x, max_y, min_z
    vertices_[7] = vec3f_initialize(aabb_->min.elem[0], aabb_->max.elem[1], aabb_->min.elem[2]);    // min_x, max_y, min_z

    ret = GEOMETRY_PRIMITIVE_SUCCESS;

cleanup:
    return ret;
}

bool aabb_3d_is_valid(const aabb_3d_t* aabb_) {
#ifdef TEST_BUILD
    s_test_config_aabb_3d_is_valid.call_count++;
    if(s_test_config_aabb_3d_is_valid.fail_on_call != 0) {
        if(s_test_config_aabb_3d_is_valid.call_count == s_test_config_aabb_3d_is_valid.fail_on_call) {
            return s_test_config_aabb_3d_is_valid.forced_result;
        }
    }
#endif
    if(NULL == aabb_) {
        return false;
    }
    for(uint8_t i = 0; i != 3; ++i) {
        if(aabb_->min.elem[i] > aabb_->max.elem[i]) {
            return false;
        }
    }
    if(!vec3f_is_finite(aabb_->min) || !vec3f_is_finite(aabb_->max)) {
        return false;
    }
    return true;
}

#ifdef TEST_BUILD
void NO_COVERAGE test_aabb_3d_initialize_from_min_max_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_aabb_3d_initialize_from_min_max.fail_on_call = config_->fail_on_call;
    s_test_config_aabb_3d_initialize_from_min_max.forced_result = config_->forced_result;
}

void NO_COVERAGE test_aabb_3d_initialize_from_point_vertices_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_aabb_3d_initialize_from_point_vertices.fail_on_call = config_->fail_on_call;
    s_test_config_aabb_3d_initialize_from_point_vertices.forced_result = config_->forced_result;
}

void NO_COVERAGE test_aabb_3d_initialize_from_line_vertices_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_aabb_3d_initialize_from_line_vertices.fail_on_call = config_->fail_on_call;
    s_test_config_aabb_3d_initialize_from_line_vertices.forced_result = config_->forced_result;
}

void NO_COVERAGE test_aabb_3d_initialize_from_point_normal_vertices_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_aabb_3d_initialize_from_point_normal_vertices.fail_on_call = config_->fail_on_call;
    s_test_config_aabb_3d_initialize_from_point_normal_vertices.forced_result = config_->forced_result;
}

void NO_COVERAGE test_aabb_3d_vertices_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_aabb_3d_vertices_get.fail_on_call = config_->fail_on_call;
    s_test_config_aabb_3d_vertices_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_aabb_3d_is_valid_config_set(const test_call_control_bool_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_aabb_3d_is_valid.fail_on_call = config_->fail_on_call;
    s_test_config_aabb_3d_is_valid.forced_result = config_->forced_result;
}

void NO_COVERAGE test_aabb_3d_config_reset(void) {
    test_call_control_reset(&s_test_config_aabb_3d_initialize_from_min_max);
    test_call_control_reset(&s_test_config_aabb_3d_initialize_from_point_vertices);
    test_call_control_reset(&s_test_config_aabb_3d_initialize_from_line_vertices);
    test_call_control_reset(&s_test_config_aabb_3d_initialize_from_point_normal_vertices);
    test_call_control_reset(&s_test_config_aabb_3d_vertices_get);
    test_call_control_bool_reset(&s_test_config_aabb_3d_is_valid);
}

void NO_COVERAGE test_aabb_3d(void) {
    test_aabb_3d_initialize_from_min_max();
    test_aabb_3d_initialize_from_point_vertices();
    test_aabb_3d_initialize_from_line_vertices();
    test_aabb_3d_initialize_from_point_normal_vertices();
    test_aabb_3d_reset();
    test_aabb_3d_vertices_get();
    test_aabb_3d_is_valid();
}

// Generated by ChatGPT
static void NO_COVERAGE test_aabb_3d_initialize_from_min_max(void) {
    {
        // aabb_3d_initialize_from_min_max() 冒頭で強制的に GEOMETRY_PRIMITIVE_RUNTIME_ERROR を返させる
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        vec3f_t min = { 0 };
        vec3f_t max = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        max = vec3f_initialize( 1.0f,  2.0f,  3.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        s_test_config_aabb_3d_initialize_from_min_max.fail_on_call = 1U;
        s_test_config_aabb_3d_initialize_from_min_max.forced_result = (int)GEOMETRY_PRIMITIVE_RUNTIME_ERROR;

        ret = aabb_3d_initialize_from_min_max(min, max, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_RUNTIME_ERROR == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // out_aabb_ == NULL -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        vec3f_t min = { 0 };
        vec3f_t max = { 0 };

        test_aabb_3d_config_reset();

        min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        max = vec3f_initialize( 1.0f,  2.0f,  3.0f);

        ret = aabb_3d_initialize_from_min_max(min, max, NULL);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        test_aabb_3d_config_reset();
    }
    {
        // min_.x > max_.x -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        vec3f_t min = { 0 };
        vec3f_t max = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        min = vec3f_initialize(2.0f, 0.0f, 0.0f);
        max = vec3f_initialize(1.0f, 1.0f, 1.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_min_max(min, max, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // min_.y > max_.y -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        vec3f_t min = { 0 };
        vec3f_t max = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        min = vec3f_initialize(0.0f, 2.0f, 0.0f);
        max = vec3f_initialize(1.0f, 1.0f, 1.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_min_max(min, max, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // min_.z > max_.z -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        vec3f_t min = { 0 };
        vec3f_t max = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        min = vec3f_initialize(0.0f, 0.0f, 2.0f);
        max = vec3f_initialize(1.0f, 1.0f, 1.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_min_max(min, max, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // min_ に NaN が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        vec3f_t min = { 0 };
        vec3f_t max = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        min = vec3f_initialize(0.0f, 0.0f, 0.0f);
        max = vec3f_initialize(1.0f, 1.0f, 1.0f);
        min.elem[0] = NAN;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_min_max(min, max, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // max_ に Inf が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        vec3f_t min = { 0 };
        vec3f_t max = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        min = vec3f_initialize(0.0f, 0.0f, 0.0f);
        max = vec3f_initialize(1.0f, 1.0f, 1.0f);
        max.elem[2] = INFINITY;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_min_max(min, max, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: min_ == max_ の退化AABBはvalid
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        vec3f_t min = { 0 };
        vec3f_t max = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        min = vec3f_initialize(1.0f, 2.0f, 3.0f);
        max = vec3f_initialize(1.0f, 2.0f, 3.0f);

        ret = aabb_3d_initialize_from_min_max(min, max, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        assert(1.0f == out_aabb.min.elem[0]);
        assert(2.0f == out_aabb.min.elem[1]);
        assert(3.0f == out_aabb.min.elem[2]);
        assert(1.0f == out_aabb.max.elem[0]);
        assert(2.0f == out_aabb.max.elem[1]);
        assert(3.0f == out_aabb.max.elem[2]);

        assert(true == aabb_3d_is_valid(&out_aabb));

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: min_ / max_ からAABBを初期化する
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        vec3f_t min = { 0 };
        vec3f_t max = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = aabb_3d_initialize_from_min_max(min, max, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        assert(is_equal_float(-1.0f, out_aabb.min.elem[0]));
        assert(is_equal_float(-2.0f, out_aabb.min.elem[1]));
        assert(is_equal_float(-3.0f, out_aabb.min.elem[2]));
        assert(is_equal_float(4.0f, out_aabb.max.elem[0]));
        assert(is_equal_float(5.0f, out_aabb.max.elem[1]));
        assert(is_equal_float(6.0f, out_aabb.max.elem[2]));

        assert(true == aabb_3d_is_valid(&out_aabb));

        test_aabb_3d_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_aabb_3d_initialize_from_point_vertices(void) {
    {
        // aabb_3d_initialize_from_point_vertices() 冒頭で強制的に GEOMETRY_PRIMITIVE_RUNTIME_ERROR を返させる
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_vertex_t vertices[2] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        s_test_config_aabb_3d_initialize_from_point_vertices.fail_on_call = 1U;
        s_test_config_aabb_3d_initialize_from_point_vertices.forced_result = (int)GEOMETRY_PRIMITIVE_RUNTIME_ERROR;

        ret = aabb_3d_initialize_from_point_vertices(vertices, 2U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_RUNTIME_ERROR == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_ == NULL -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_vertices(NULL, 1U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // out_aabb_ == NULL -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_vertex_t vertices[1] = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);

        ret = aabb_3d_initialize_from_point_vertices(vertices, 1U, NULL);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        test_aabb_3d_config_reset();
    }
    {
        // vertex_count_ == 0 -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_vertex_t vertices[1] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_vertices(vertices, 0U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_[0].position に NaN が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_vertex_t vertices[2] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[0].position.elem[0] = NAN;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_vertices(vertices, 2U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_[1].position に NaN が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_vertex_t vertices[3] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[1].position.elem[1] = NAN;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_vertices(vertices, 3U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_[2].position に Inf が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_vertex_t vertices[3] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[2].position.elem[2] = INFINITY;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_vertices(vertices, 3U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // aabb_3d_is_valid() が false を返す -> GEOMETRY_PRIMITIVE_RUNTIME_ERROR
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_vertex_t vertices[2] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        s_test_config_aabb_3d_is_valid.fail_on_call = 1U;
        s_test_config_aabb_3d_is_valid.forced_result = false;

        ret = aabb_3d_initialize_from_point_vertices(vertices, 2U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_RUNTIME_ERROR == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: 1頂点から退化AABBを生成する
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        point_vertex_t vertices[1] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(1.0f, 2.0f, 3.0f);

        ret = aabb_3d_initialize_from_point_vertices(vertices, 1U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        assert(1.0f == out_aabb.min.elem[0]);
        assert(2.0f == out_aabb.min.elem[1]);
        assert(3.0f == out_aabb.min.elem[2]);
        assert(1.0f == out_aabb.max.elem[0]);
        assert(2.0f == out_aabb.max.elem[1]);
        assert(3.0f == out_aabb.max.elem[2]);

        assert(true == aabb_3d_is_valid(&out_aabb));

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: 複数頂点からmin/maxを計算してAABBを生成する
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        point_vertex_t vertices[4] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize( 2.0f,  3.0f,  4.0f);
        vertices[1].position = vec3f_initialize(-1.0f,  8.0f,  0.5f);
        vertices[2].position = vec3f_initialize( 6.0f, -2.0f,  7.0f);
        vertices[3].position = vec3f_initialize( 0.0f,  1.0f, -9.0f);

        ret = aabb_3d_initialize_from_point_vertices(vertices, 4U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        assert(is_equal_float(-1.0f, out_aabb.min.elem[0]));
        assert(is_equal_float(-2.0f, out_aabb.min.elem[1]));
        assert(is_equal_float(-9.0f, out_aabb.min.elem[2]));
        assert(is_equal_float(6.0f, out_aabb.max.elem[0]));
        assert(is_equal_float(8.0f, out_aabb.max.elem[1]));
        assert(is_equal_float(7.0f, out_aabb.max.elem[2]));

        assert(true == aabb_3d_is_valid(&out_aabb));

        test_aabb_3d_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_aabb_3d_initialize_from_line_vertices(void) {
    {
        // aabb_3d_initialize_from_line_vertices() 冒頭で強制的に GEOMETRY_PRIMITIVE_RUNTIME_ERROR を返させる
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        line_vertex_t vertices[2] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        s_test_config_aabb_3d_initialize_from_line_vertices.fail_on_call = 1U;
        s_test_config_aabb_3d_initialize_from_line_vertices.forced_result = (int)GEOMETRY_PRIMITIVE_RUNTIME_ERROR;

        ret = aabb_3d_initialize_from_line_vertices(vertices, 2U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_RUNTIME_ERROR == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_ == NULL -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_line_vertices(NULL, 2U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // out_aabb_ == NULL -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        line_vertex_t vertices[2] = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        ret = aabb_3d_initialize_from_line_vertices(vertices, 2U, NULL);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        test_aabb_3d_config_reset();
    }
    {
        // vertex_count_ == 0 -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        line_vertex_t vertices[2] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_line_vertices(vertices, 0U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertex_count_ が2の倍数ではない -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        line_vertex_t vertices[3] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_line_vertices(vertices, 3U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_[0].position に NaN が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        line_vertex_t vertices[2] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[0].position.elem[0] = NAN;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_line_vertices(vertices, 2U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_[1].position に NaN が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        line_vertex_t vertices[2] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[1].position.elem[1] = NAN;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_line_vertices(vertices, 2U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_[3].position に Inf が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        line_vertex_t vertices[4] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[3].position = vec3f_initialize(9.0f, 10.0f, 11.0f);
        vertices[3].position.elem[2] = INFINITY;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_line_vertices(vertices, 4U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // aabb_3d_is_valid() が false を返す -> GEOMETRY_PRIMITIVE_RUNTIME_ERROR
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        line_vertex_t vertices[2] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        s_test_config_aabb_3d_is_valid.fail_on_call = 1U;
        s_test_config_aabb_3d_is_valid.forced_result = false;

        ret = aabb_3d_initialize_from_line_vertices(vertices, 2U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_RUNTIME_ERROR == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: 1線分、2頂点からAABBを生成する
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        line_vertex_t vertices[2] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(-1.0f, 2.0f, -3.0f);
        vertices[1].position = vec3f_initialize( 4.0f, 5.0f,  6.0f);

        ret = aabb_3d_initialize_from_line_vertices(vertices, 2U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        assert(is_equal_float(-1.0f, out_aabb.min.elem[0]));
        assert(is_equal_float(2.0f, out_aabb.min.elem[1]));
        assert(is_equal_float(-3.0f, out_aabb.min.elem[2]));
        assert(is_equal_float(4.0f, out_aabb.max.elem[0]));
        assert(is_equal_float(5.0f, out_aabb.max.elem[1]));
        assert(is_equal_float(6.0f, out_aabb.max.elem[2]));

        assert(true == aabb_3d_is_valid(&out_aabb));

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: 複数線分、4頂点からmin/maxを計算してAABBを生成する
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        line_vertex_t vertices[4] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize( 2.0f,  3.0f,  4.0f);
        vertices[1].position = vec3f_initialize(-1.0f,  8.0f,  0.5f);
        vertices[2].position = vec3f_initialize( 6.0f, -2.0f,  7.0f);
        vertices[3].position = vec3f_initialize( 0.0f,  1.0f, -9.0f);

        ret = aabb_3d_initialize_from_line_vertices(vertices, 4U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        assert(is_equal_float(-1.0f, out_aabb.min.elem[0]));
        assert(is_equal_float(-2.0f, out_aabb.min.elem[1]));
        assert(is_equal_float(-9.0f, out_aabb.min.elem[2]));
        assert(is_equal_float(6.0f, out_aabb.max.elem[0]));
        assert(is_equal_float(8.0f, out_aabb.max.elem[1]));
        assert(is_equal_float(7.0f, out_aabb.max.elem[2]));

        assert(true == aabb_3d_is_valid(&out_aabb));

        test_aabb_3d_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_aabb_3d_initialize_from_point_normal_vertices(void) {
    {
        // aabb_3d_initialize_from_point_normal_vertices() 冒頭で強制的に GEOMETRY_PRIMITIVE_RUNTIME_ERROR を返させる
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_normal_vertex_t vertices[3] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        s_test_config_aabb_3d_initialize_from_point_normal_vertices.fail_on_call = 1U;
        s_test_config_aabb_3d_initialize_from_point_normal_vertices.forced_result = (int)GEOMETRY_PRIMITIVE_RUNTIME_ERROR;

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 3U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_RUNTIME_ERROR == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_ == NULL -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_normal_vertices(NULL, 3U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // out_aabb_ == NULL -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_normal_vertex_t vertices[3] = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 3U, NULL);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        test_aabb_3d_config_reset();
    }
    {
        // vertex_count_ == 0 -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_normal_vertex_t vertices[3] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 0U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertex_count_ が3の倍数ではない -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_normal_vertex_t vertices[4] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[3].position = vec3f_initialize(9.0f, 10.0f, 11.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 4U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_[0].position に NaN が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_normal_vertex_t vertices[3] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[0].position.elem[0] = NAN;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 3U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_[1].position に NaN が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_normal_vertex_t vertices[3] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[1].position.elem[1] = NAN;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 3U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // vertices_[5].position に Inf が含まれる -> GEOMETRY_PRIMITIVE_DATA_CORRUPTED
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_normal_vertex_t vertices[6] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);
        vertices[3].position = vec3f_initialize(9.0f, 10.0f, 11.0f);
        vertices[4].position = vec3f_initialize(12.0f, 13.0f, 14.0f);
        vertices[5].position = vec3f_initialize(15.0f, 16.0f, 17.0f);
        vertices[5].position.elem[2] = INFINITY;

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 6U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_DATA_CORRUPTED == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // aabb_3d_is_valid() が false を返す -> GEOMETRY_PRIMITIVE_RUNTIME_ERROR
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        point_normal_vertex_t vertices[3] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(0.0f, 1.0f, 2.0f);
        vertices[1].position = vec3f_initialize(3.0f, 4.0f, 5.0f);
        vertices[2].position = vec3f_initialize(6.0f, 7.0f, 8.0f);

        out_aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        out_aabb.max = vec3f_initialize(40.0f, 50.0f, 60.0f);

        s_test_config_aabb_3d_is_valid.fail_on_call = 1U;
        s_test_config_aabb_3d_is_valid.forced_result = false;

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 3U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_RUNTIME_ERROR == ret);

        assert(10.0f == out_aabb.min.elem[0]);
        assert(20.0f == out_aabb.min.elem[1]);
        assert(30.0f == out_aabb.min.elem[2]);
        assert(40.0f == out_aabb.max.elem[0]);
        assert(50.0f == out_aabb.max.elem[1]);
        assert(60.0f == out_aabb.max.elem[2]);

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: 1三角形、3頂点からAABBを生成する
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        point_normal_vertex_t vertices[3] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize(-1.0f, 2.0f, -3.0f);
        vertices[1].position = vec3f_initialize( 4.0f, 5.0f,  6.0f);
        vertices[2].position = vec3f_initialize( 0.5f, 8.0f,  1.0f);

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 3U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        assert(is_equal_float(-1.0f, out_aabb.min.elem[0]));
        assert(is_equal_float(2.0f, out_aabb.min.elem[1]));
        assert(is_equal_float(-3.0f, out_aabb.min.elem[2]));
        assert(is_equal_float(4.0f, out_aabb.max.elem[0]));
        assert(is_equal_float(8.0f, out_aabb.max.elem[1]));
        assert(is_equal_float(6.0f, out_aabb.max.elem[2]));

        assert(true == aabb_3d_is_valid(&out_aabb));

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: 複数三角形、6頂点からmin/maxを計算してAABBを生成する
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        point_normal_vertex_t vertices[6] = { 0 };
        aabb_3d_t out_aabb = { 0 };

        test_aabb_3d_config_reset();

        vertices[0].position = vec3f_initialize( 2.0f,  3.0f,  4.0f);
        vertices[1].position = vec3f_initialize(-1.0f,  8.0f,  0.5f);
        vertices[2].position = vec3f_initialize( 6.0f, -2.0f,  7.0f);
        vertices[3].position = vec3f_initialize( 0.0f,  1.0f, -9.0f);
        vertices[4].position = vec3f_initialize( 9.0f,  4.0f,  3.0f);
        vertices[5].position = vec3f_initialize(-4.0f, 10.0f, 12.0f);

        ret = aabb_3d_initialize_from_point_normal_vertices(vertices, 6U, &out_aabb);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        assert(is_equal_float(-4.0f, out_aabb.min.elem[0]));
        assert(is_equal_float(-2.0f, out_aabb.min.elem[1]));
        assert(is_equal_float(-9.0f, out_aabb.min.elem[2]));
        assert(is_equal_float(9.0f, out_aabb.max.elem[0]));
        assert(is_equal_float(10.0f, out_aabb.max.elem[1]));
        assert(is_equal_float(12.0f, out_aabb.max.elem[2]));

        assert(true == aabb_3d_is_valid(&out_aabb));

        test_aabb_3d_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_aabb_3d_reset(void) {
    {
        // aabb_ == NULL -> 何もせずreturn
        test_aabb_3d_config_reset();

        aabb_3d_reset(NULL);

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: 通常AABBをresetするとmin/maxが全て0になる
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        aabb_3d_reset(&aabb);

        assert(0.0f == aabb.min.elem[0]);
        assert(0.0f == aabb.min.elem[1]);
        assert(0.0f == aabb.min.elem[2]);
        assert(0.0f == aabb.max.elem[0]);
        assert(0.0f == aabb.max.elem[1]);
        assert(0.0f == aabb.max.elem[2]);

        assert(true == aabb_3d_is_valid(&aabb));

        test_aabb_3d_config_reset();
    }
    {
        // 不正状態: min > max のAABBもresetするとvalidな退化AABBになる
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        aabb.max = vec3f_initialize( 1.0f,  2.0f,  3.0f);

        assert(false == aabb_3d_is_valid(&aabb));

        aabb_3d_reset(&aabb);

        assert(0.0f == aabb.min.elem[0]);
        assert(0.0f == aabb.min.elem[1]);
        assert(0.0f == aabb.min.elem[2]);
        assert(0.0f == aabb.max.elem[0]);
        assert(0.0f == aabb.max.elem[1]);
        assert(0.0f == aabb.max.elem[2]);

        assert(true == aabb_3d_is_valid(&aabb));

        test_aabb_3d_config_reset();
    }
    {
        // 不正状態: NaN / Infを含むAABBもresetするとvalidな退化AABBになる
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(0.0f, 1.0f, 2.0f);
        aabb.max = vec3f_initialize(3.0f, 4.0f, 5.0f);
        aabb.min.elem[0] = NAN;
        aabb.max.elem[2] = INFINITY;

        assert(false == aabb_3d_is_valid(&aabb));

        aabb_3d_reset(&aabb);

        assert(0.0f == aabb.min.elem[0]);
        assert(0.0f == aabb.min.elem[1]);
        assert(0.0f == aabb.min.elem[2]);
        assert(0.0f == aabb.max.elem[0]);
        assert(0.0f == aabb.max.elem[1]);
        assert(0.0f == aabb.max.elem[2]);

        assert(true == aabb_3d_is_valid(&aabb));

        test_aabb_3d_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_aabb_3d_vertices_get(void) {
    {
        // aabb_3d_vertices_get() 冒頭で強制的に GEOMETRY_PRIMITIVE_RUNTIME_ERROR を返させる
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        aabb_3d_t aabb = { 0 };
        vec3f_t vertices[8] = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        for(size_t i = 0; i != 8U; ++i) {
            vertices[i] = vec3f_initialize(10.0f, 20.0f, 30.0f);
        }

        s_test_config_aabb_3d_vertices_get.fail_on_call = 1U;
        s_test_config_aabb_3d_vertices_get.forced_result = (int)GEOMETRY_PRIMITIVE_RUNTIME_ERROR;

        ret = aabb_3d_vertices_get(&aabb, vertices);
        assert(GEOMETRY_PRIMITIVE_RUNTIME_ERROR == ret);

        for(size_t i = 0; i != 8U; ++i) {
            assert(true == is_equal_float(10.0f, vertices[i].elem[0]));
            assert(true == is_equal_float(20.0f, vertices[i].elem[1]));
            assert(true == is_equal_float(30.0f, vertices[i].elem[2]));
        }

        test_aabb_3d_config_reset();
    }
    {
        // aabb_ == NULL -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        vec3f_t vertices[8] = { 0 };

        test_aabb_3d_config_reset();

        for(size_t i = 0; i != 8U; ++i) {
            vertices[i] = vec3f_initialize(10.0f, 20.0f, 30.0f);
        }

        ret = aabb_3d_vertices_get(NULL, vertices);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        for(size_t i = 0; i != 8U; ++i) {
            assert(true == is_equal_float(10.0f, vertices[i].elem[0]));
            assert(true == is_equal_float(20.0f, vertices[i].elem[1]));
            assert(true == is_equal_float(30.0f, vertices[i].elem[2]));
        }

        test_aabb_3d_config_reset();
    }
    {
        // vertices_ == NULL -> GEOMETRY_PRIMITIVE_INVALID_ARGUMENT
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = aabb_3d_vertices_get(&aabb, NULL);
        assert(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT == ret);

        test_aabb_3d_config_reset();
    }
    {
        // aabb_ が不正状態: min > max -> GEOMETRY_PRIMITIVE_BAD_OPERATION
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        aabb_3d_t aabb = { 0 };
        vec3f_t vertices[8] = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        aabb.max = vec3f_initialize( 1.0f,  2.0f,  3.0f);

        for(size_t i = 0; i != 8U; ++i) {
            vertices[i] = vec3f_initialize(40.0f, 50.0f, 60.0f);
        }

        ret = aabb_3d_vertices_get(&aabb, vertices);
        assert(GEOMETRY_PRIMITIVE_BAD_OPERATION == ret);

        for(size_t i = 0; i != 8U; ++i) {
            assert(true == is_equal_float(40.0f, vertices[i].elem[0]));
            assert(true == is_equal_float(50.0f, vertices[i].elem[1]));
            assert(true == is_equal_float(60.0f, vertices[i].elem[2]));
        }

        test_aabb_3d_config_reset();
    }
    {
        // aabb_3d_is_valid() が false を返す -> GEOMETRY_PRIMITIVE_BAD_OPERATION
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_SUCCESS;
        aabb_3d_t aabb = { 0 };
        vec3f_t vertices[8] = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        for(size_t i = 0; i != 8U; ++i) {
            vertices[i] = vec3f_initialize(40.0f, 50.0f, 60.0f);
        }

        s_test_config_aabb_3d_is_valid.fail_on_call = 1U;
        s_test_config_aabb_3d_is_valid.forced_result = false;

        ret = aabb_3d_vertices_get(&aabb, vertices);
        assert(GEOMETRY_PRIMITIVE_BAD_OPERATION == ret);

        for(size_t i = 0; i != 8U; ++i) {
            assert(true == is_equal_float(40.0f, vertices[i].elem[0]));
            assert(true == is_equal_float(50.0f, vertices[i].elem[1]));
            assert(true == is_equal_float(60.0f, vertices[i].elem[2]));
        }

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: AABBを構成する8頂点を仕様順で取得する
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        aabb_3d_t aabb = { 0 };
        vec3f_t vertices[8] = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = aabb_3d_vertices_get(&aabb, vertices);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        // vertices_[0]: min_x, min_y, max_z
        assert(true == is_equal_float(-1.0f, vertices[0].elem[0]));
        assert(true == is_equal_float(-2.0f, vertices[0].elem[1]));
        assert(true == is_equal_float( 6.0f, vertices[0].elem[2]));

        // vertices_[1]: max_x, min_y, max_z
        assert(true == is_equal_float( 4.0f, vertices[1].elem[0]));
        assert(true == is_equal_float(-2.0f, vertices[1].elem[1]));
        assert(true == is_equal_float( 6.0f, vertices[1].elem[2]));

        // vertices_[2]: max_x, min_y, min_z
        assert(true == is_equal_float( 4.0f, vertices[2].elem[0]));
        assert(true == is_equal_float(-2.0f, vertices[2].elem[1]));
        assert(true == is_equal_float(-3.0f, vertices[2].elem[2]));

        // vertices_[3]: min_x, min_y, min_z
        assert(true == is_equal_float(-1.0f, vertices[3].elem[0]));
        assert(true == is_equal_float(-2.0f, vertices[3].elem[1]));
        assert(true == is_equal_float(-3.0f, vertices[3].elem[2]));

        // vertices_[4]: min_x, max_y, max_z
        assert(true == is_equal_float(-1.0f, vertices[4].elem[0]));
        assert(true == is_equal_float( 5.0f, vertices[4].elem[1]));
        assert(true == is_equal_float( 6.0f, vertices[4].elem[2]));

        // vertices_[5]: max_x, max_y, max_z
        assert(true == is_equal_float( 4.0f, vertices[5].elem[0]));
        assert(true == is_equal_float( 5.0f, vertices[5].elem[1]));
        assert(true == is_equal_float( 6.0f, vertices[5].elem[2]));

        // vertices_[6]: max_x, max_y, min_z
        assert(true == is_equal_float( 4.0f, vertices[6].elem[0]));
        assert(true == is_equal_float( 5.0f, vertices[6].elem[1]));
        assert(true == is_equal_float(-3.0f, vertices[6].elem[2]));

        // vertices_[7]: min_x, max_y, min_z
        assert(true == is_equal_float(-1.0f, vertices[7].elem[0]));
        assert(true == is_equal_float( 5.0f, vertices[7].elem[1]));
        assert(true == is_equal_float(-3.0f, vertices[7].elem[2]));

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: 退化AABBでも8頂点を取得できる
        geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
        aabb_3d_t aabb = { 0 };
        vec3f_t vertices[8] = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(1.0f, 2.0f, 3.0f);
        aabb.max = vec3f_initialize(1.0f, 2.0f, 3.0f);

        ret = aabb_3d_vertices_get(&aabb, vertices);
        assert(GEOMETRY_PRIMITIVE_SUCCESS == ret);

        for(size_t i = 0; i != 8U; ++i) {
            assert(true == is_equal_float(1.0f, vertices[i].elem[0]));
            assert(true == is_equal_float(2.0f, vertices[i].elem[1]));
            assert(true == is_equal_float(3.0f, vertices[i].elem[2]));
        }

        test_aabb_3d_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_aabb_3d_is_valid(void) {
    {
        // aabb_3d_is_valid() 冒頭で強制的に false を返させる
        bool ret = true;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        s_test_config_aabb_3d_is_valid.fail_on_call = 1U;
        s_test_config_aabb_3d_is_valid.forced_result = false;

        ret = aabb_3d_is_valid(&aabb);
        assert(false == ret);

        test_aabb_3d_config_reset();
    }
    {
        // aabb_3d_is_valid() 冒頭で強制的に true を返させる
        bool ret = false;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(10.0f, 20.0f, 30.0f);
        aabb.max = vec3f_initialize( 1.0f,  2.0f,  3.0f);

        s_test_config_aabb_3d_is_valid.fail_on_call = 1U;
        s_test_config_aabb_3d_is_valid.forced_result = true;

        ret = aabb_3d_is_valid(&aabb);
        assert(true == ret);

        test_aabb_3d_config_reset();
    }
    {
        // aabb_ == NULL -> false
        bool ret = true;

        test_aabb_3d_config_reset();

        ret = aabb_3d_is_valid(NULL);
        assert(false == ret);

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: min < max の通常AABBはvalid
        bool ret = false;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(-1.0f, -2.0f, -3.0f);
        aabb.max = vec3f_initialize( 4.0f,  5.0f,  6.0f);

        ret = aabb_3d_is_valid(&aabb);
        assert(true == ret);

        test_aabb_3d_config_reset();
    }
    {
        // 正常系: min == max の退化AABBはvalid
        bool ret = false;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(1.0f, 2.0f, 3.0f);
        aabb.max = vec3f_initialize(1.0f, 2.0f, 3.0f);

        ret = aabb_3d_is_valid(&aabb);
        assert(true == ret);

        test_aabb_3d_config_reset();
    }
    {
        // min.x > max.x -> false
        bool ret = true;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(2.0f, 0.0f, 0.0f);
        aabb.max = vec3f_initialize(1.0f, 1.0f, 1.0f);

        ret = aabb_3d_is_valid(&aabb);
        assert(false == ret);

        test_aabb_3d_config_reset();
    }
    {
        // min.y > max.y -> false
        bool ret = true;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(0.0f, 2.0f, 0.0f);
        aabb.max = vec3f_initialize(1.0f, 1.0f, 1.0f);

        ret = aabb_3d_is_valid(&aabb);
        assert(false == ret);

        test_aabb_3d_config_reset();
    }
    {
        // min.z > max.z -> false
        bool ret = true;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(0.0f, 0.0f, 2.0f);
        aabb.max = vec3f_initialize(1.0f, 1.0f, 1.0f);

        ret = aabb_3d_is_valid(&aabb);
        assert(false == ret);

        test_aabb_3d_config_reset();
    }
    {
        // min に NaN が含まれる -> false
        bool ret = true;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(0.0f, 0.0f, 0.0f);
        aabb.max = vec3f_initialize(1.0f, 1.0f, 1.0f);
        aabb.min.elem[0] = NAN;

        ret = aabb_3d_is_valid(&aabb);
        assert(false == ret);

        test_aabb_3d_config_reset();
    }
    {
        // max に NaN が含まれる -> false
        bool ret = true;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(0.0f, 0.0f, 0.0f);
        aabb.max = vec3f_initialize(1.0f, 1.0f, 1.0f);
        aabb.max.elem[1] = NAN;

        ret = aabb_3d_is_valid(&aabb);
        assert(false == ret);

        test_aabb_3d_config_reset();
    }
    {
        // min に Inf が含まれる -> false
        bool ret = true;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(0.0f, 0.0f, 0.0f);
        aabb.max = vec3f_initialize(1.0f, 1.0f, 1.0f);
        aabb.min.elem[2] = INFINITY;

        ret = aabb_3d_is_valid(&aabb);
        assert(false == ret);

        test_aabb_3d_config_reset();
    }
    {
        // max に Inf が含まれる -> false
        bool ret = true;
        aabb_3d_t aabb = { 0 };

        test_aabb_3d_config_reset();

        aabb.min = vec3f_initialize(0.0f, 0.0f, 0.0f);
        aabb.max = vec3f_initialize(1.0f, 1.0f, 1.0f);
        aabb.max.elem[2] = INFINITY;

        ret = aabb_3d_is_valid(&aabb);
        assert(false == ret);

        test_aabb_3d_config_reset();
    }
}
#endif
