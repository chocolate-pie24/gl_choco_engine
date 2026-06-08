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

geometry_primitive_result_t aabb_3d_initialize_from_min_max(const vec3f_t* min_, const vec3f_t* max_, aabb_3d_t* out_aabb_) {
    geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
    aabb_3d_t tmp_aabb = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(min_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_min_max", "min_")
    IF_ARG_NULL_GOTO_CLEANUP(max_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_min_max", "max_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_min_max", "out_aabb_")

    tmp_aabb.min = *min_;
    tmp_aabb.max = *max_;

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
        if(!vec3f_is_finite(&vertices_[i].position)) {
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
    geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
    aabb_3d_t tmp_aabb = { 0 };
    vec3f_t min = { 0 };
    vec3f_t max = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_line_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_line_vertices", "out_aabb_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_line_vertices", "vertex_count_")

    min = vertices_[0].position;
    max = vertices_[0].position;
    for(size_t i = 0; i != vertex_count_; ++i) {
        if(!vec3f_is_finite(&vertices_[i].position)) {
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
    geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
    aabb_3d_t tmp_aabb = { 0 };
    vec3f_t min = { 0 };
    vec3f_t max = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_normal_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_normal_vertices", "out_aabb_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_normal_vertices", "vertex_count_")

    min = vertices_[0].position;
    max = vertices_[0].position;
    for(size_t i = 0; i != vertex_count_; ++i) {
        if(!vec3f_is_finite(&vertices_[i].position)) {
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
    geometry_primitive_result_t ret = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_vertices_get", "aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, GEOMETRY_PRIMITIVE_INVALID_ARGUMENT, geometry_primitive_rslt_to_str(GEOMETRY_PRIMITIVE_INVALID_ARGUMENT), "aabb_3d_vertices_get", "vertices_")

    if(!aabb_3d_is_valid(aabb_)) {
        ret = GEOMETRY_PRIMITIVE_BAD_OPERATION;
        ERROR_MESSAGE("aabb_3d_vertices_get(%s) - Cannot generate vertices from invalid aabb_. min = [%f, %f, %f], max = [%f, %f, %f].", geometry_primitive_rslt_to_str(ret), aabb_->min.elem[0], aabb_->min.elem[1], aabb_->min.elem[2], aabb_->max.elem[0], aabb_->max.elem[1], aabb_->max.elem[2]);
        goto cleanup;
    }

    vec3f_initialize(aabb_->min.elem[0], aabb_->min.elem[1], aabb_->max.elem[2], &vertices_[0]);    // min_x, min_y, max_z
    vec3f_initialize(aabb_->max.elem[0], aabb_->min.elem[1], aabb_->max.elem[2], &vertices_[1]);    // max_x, min_y, max_z
    vec3f_initialize(aabb_->max.elem[0], aabb_->min.elem[1], aabb_->min.elem[2], &vertices_[2]);    // max_x, min_y, min_z
    vec3f_initialize(aabb_->min.elem[0], aabb_->min.elem[1], aabb_->min.elem[2], &vertices_[3]);    // min_x, min_y, min_z

    vec3f_initialize(aabb_->min.elem[0], aabb_->max.elem[1], aabb_->max.elem[2], &vertices_[4]);    // min_x, max_y, max_z
    vec3f_initialize(aabb_->max.elem[0], aabb_->max.elem[1], aabb_->max.elem[2], &vertices_[5]);    // max_x, max_y, max_z
    vec3f_initialize(aabb_->max.elem[0], aabb_->max.elem[1], aabb_->min.elem[2], &vertices_[6]);    // max_x, max_y, min_z
    vec3f_initialize(aabb_->min.elem[0], aabb_->max.elem[1], aabb_->min.elem[2], &vertices_[7]);    // min_x, max_y, min_z

    ret = GEOMETRY_PRIMITIVE_SUCCESS;

cleanup:
    return ret;
}

/*
// min > max -> invalid
// nan or inf -> invalid
// 厚み、体積を持たない退化したaabbはvalidとする
*/
bool aabb_3d_is_valid(const aabb_3d_t* aabb_) {
    if(NULL == aabb_) {
        return false;
    }
    for(uint8_t i = 0; i != 3; ++i) {
        if(aabb_->min.elem[i] > aabb_->max.elem[i]) {
            return false;
        }
    }
    if(!vec3f_is_finite(&aabb_->min) || !vec3f_is_finite(&aabb_->max)) {
        return false;
    }
    return true;
}

