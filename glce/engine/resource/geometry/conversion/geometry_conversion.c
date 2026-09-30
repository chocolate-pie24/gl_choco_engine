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

    aabb_3d_result_t ret_aabb_3d = AABB_3D_INVALID_ARGUMENT;

    size_t vertex_count = 0;
    aabb_3d_t tmp_aabb_3d = { 0 };
    const point_normal_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(src_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "geometry_conversion_lit_mesh_geometry_to_aabb_3d", "src_geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(dst_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "geometry_conversion_lit_mesh_geometry_to_aabb_3d", "dst_geometry_")

    ret = lit_mesh_geometry_vertices_get(src_geometry_, &tmp_vertices, &vertex_count);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("geometry_conversion_lit_mesh_geometry_to_aabb_3d(%s) - lit_mesh_geometry_vertices_get failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_aabb_3d = aabb_3d_initialize_from_point_normal_vertices(tmp_vertices, vertex_count, &tmp_aabb_3d);
    if(AABB_3D_SUCCESS != ret_aabb_3d) {
        ret = resource_result_convert_aabb_3d(ret_aabb_3d);
        ERROR_MESSAGE("geometry_conversion_lit_mesh_geometry_to_aabb_3d(%s) - aabb_3d_initialize_from_point_normal_vertices failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    *dst_geometry_ = tmp_aabb_3d;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}
