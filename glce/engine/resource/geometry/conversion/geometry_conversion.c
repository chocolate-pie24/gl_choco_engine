// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/resource/geometry/conversion/geometry_conversion.h"

#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

#include "engine/resource/geometry/lit_mesh_geometry.h"

resource_result_t geometry_conversion_lit_mesh_geometry_to_aabb_3d(const lit_mesh_geometry_t* src_geometry_, aabb_3d_t* dst_geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    geometry_primitive_result_t ret_geometry_primitive = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;

    size_t vertex_count = 0;
    aabb_3d_t tmp_aabb_3d = { 0 };
    const point_normal_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(src_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "geometry_conversion_lit_mesh_geometry_to_aabb_3d", "src_geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(dst_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "geometry_conversion_lit_mesh_geometry_to_aabb_3d", "dst_geometry_")

    ret = lit_mesh_geometry_vertex_count_get(src_geometry_, &vertex_count);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("geometry_conversion_lit_mesh_geometry_to_aabb_3d(%s) - lit_mesh_geometry_vertex_count_get failed.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = lit_mesh_geometry_vertices_get(src_geometry_, &tmp_vertices);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("geometry_conversion_lit_mesh_geometry_to_aabb_3d(%s) - lit_mesh_geometry_vertices_get failed.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_geometry_primitive = aabb_3d_initialize_from_point_normal_vertices(tmp_vertices, vertex_count, &tmp_aabb_3d);
    if(GEOMETRY_PRIMITIVE_SUCCESS != ret_geometry_primitive) {
        ret = resource_rslt_convert_geometry_primitive(ret_geometry_primitive);
        ERROR_MESSAGE("geometry_conversion_lit_mesh_geometry_to_aabb_3d(%s) - aabb_3d_initialize_from_point_normal_vertices failed.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    // TODO: aabb_3d_tのcanonical validator追加後にpostcondition validationを追加

    *dst_geometry_ = tmp_aabb_3d;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}
