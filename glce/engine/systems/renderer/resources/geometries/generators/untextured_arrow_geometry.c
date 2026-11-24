#include "engine/systems/renderer/resources/geometries/generators/untextured_arrow_geometry.h"

#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/cone.h"
#include "engine/core/geometry_primitive/cylinder.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/geometries/core/geometry_err_utils.h"
#include "engine/systems/renderer/resources/geometries/generators/untextured_cone_geometry.h"
#include "engine/systems/renderer/resources/geometries/generators/untextured_cylinder_geometry.h"

geometry_result_t untextured_arrow_geometry_vertices_create(uint16_t segment_count_, const cone_t* cone_, const cylinder_t* cylinder_, point_normal_vertex_t** out_vertices_, size_t* out_vertex_count_) {
    geometry_result_t ret = GEOMETRY_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    point_normal_vertex_t* tmp_vertices = NULL;
    point_normal_vertex_t* tmp_cylinder_vertices = NULL;
    point_normal_vertex_t* tmp_cone_vertices = NULL;

    size_t allocation_size = 0;
    size_t tmp_vertex_count = 0;
    size_t tmp_cylinder_vertex_count = 0;
    size_t tmp_cone_vertex_count = 0;
    size_t index = 0;
    mat4x4f_t model_mat = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(cone_, ret, GEOMETRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(cylinder_, ret, GEOMETRY_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, GEOMETRY_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, GEOMETRY_BAD_OPERATION);
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, GEOMETRY_INVALID_ARGUMENT);
    if(0 != *out_vertex_count_) {
        ret = GEOMETRY_BAD_OPERATION;
        ERROR_MESSAGE("untextured_arrow_geometry_vertices_create(%s) - Provided out_vertex_count_ is not valid.", geometry_result_to_str(ret));
        goto cleanup;
    }

    // Generate Geometry.
    ret = untextured_cylinder_geometry_vertices_create(cylinder_, segment_count_, &tmp_cylinder_vertices, &tmp_cylinder_vertex_count);
    if(GEOMETRY_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_arrow_geometry_vertices_create(%s) - untextured_cylinder_geometry_vertices_create failed.", geometry_result_to_str(ret));
        goto cleanup;
    }

    ret = untextured_cone_geometry_vertices_create(cone_, segment_count_, &tmp_cone_vertices, &tmp_cone_vertex_count);
    if(GEOMETRY_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_arrow_geometry_vertices_create(%s) - untextured_cone_geometry_vertices_create failed.", geometry_result_to_str(ret));
        goto cleanup;
    }

    // ConeをCylinderの上面に移動
    mat4f_translation(vec3f_initialize(0.0f, 0.0f, cylinder_->height), &model_mat);
    if(!point_normal_vertices_transform(tmp_cone_vertices, tmp_cone_vertex_count, &model_mat)) {
        ret = GEOMETRY_RUNTIME_ERROR;
        ERROR_MESSAGE("untextured_arrow_geometry_vertices_create(%s) - point_normal_vertices_transform failed.", geometry_result_to_str(ret));
        goto cleanup;
    }

    // Cylinder, Coneを1つのGeometryにするため両方の頂点情報を格納する配列リソース確保
    if(!size_add_checked(tmp_cone_vertex_count, tmp_cylinder_vertex_count, &tmp_vertex_count)) {
        ret = GEOMETRY_OVERFLOW;
        ERROR_MESSAGE("untextured_arrow_geometry_vertices_create(%s) - vertex count overflow.", geometry_result_to_str(ret));
        goto cleanup;
    }
    if(!size_mul_checked(tmp_vertex_count, sizeof(point_normal_vertex_t), &allocation_size)) {
        ret = GEOMETRY_OVERFLOW;
        ERROR_MESSAGE("untextured_arrow_geometry_vertices_create(%s) - allocation size overflow.", geometry_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = geometry_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("untextured_arrow_geometry_vertices_create(%s) - general_allocator_allocate failed.", geometry_result_to_str(ret));
        goto cleanup;
    }

    // 頂点情報コピー
    for(size_t i = 0; i != tmp_cylinder_vertex_count; ++i) {
        tmp_vertices[index] = tmp_cylinder_vertices[i];
        index++;
    }
    for(size_t i = 0; i != tmp_cone_vertex_count; ++i) {
        tmp_vertices[index] = tmp_cone_vertices[i];
        index++;
    }

    *out_vertex_count_ = tmp_vertex_count;
    *out_vertices_ = tmp_vertices;
    tmp_vertices = NULL;

    ret = GEOMETRY_SUCCESS;

cleanup:
    if(GEOMETRY_DATA_CORRUPTED == ret) {
        return ret;
    }
    if(NULL != tmp_cylinder_vertices) {
        untextured_cylinder_geometry_vertices_destroy(&tmp_cylinder_vertices);
    }
    if(NULL != tmp_cone_vertices) {
        untextured_cone_geometry_vertices_destroy(&tmp_cone_vertices);
    }
    if(NULL != tmp_vertices) {
        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
    }
    return ret;
}

void untextured_arrow_geometry_vertices_destroy(point_normal_vertex_t** vertices_) {
    if(NULL == vertices_ || NULL == *vertices_) {
        return;
    }
    general_allocator_free((void**)vertices_, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
}
