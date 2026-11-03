// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/geometries/generators/untextured_cylinder_geometry.h"

#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/geometries/core/geometry_err_utils.h"

geometry_result_t untextured_cylinder_geometry_vertices_create(float radius_, float height_, uint16_t segment_count_, point_normal_vertex_t** out_vertices_, size_t* out_vertex_count_) {
    geometry_result_t ret = GEOMETRY_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    size_t index = 0;
    size_t triangle_count = 0;
    size_t vertex_count = 0;
    size_t allocation_size = 0;
    float step = 0.0f;

    float z_upper = 0.0f;
    float z_lower = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
    vec4i8_t normal1 = { 0 };
    vec4i8_t normal2 = { 0 };

    point_normal_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, GEOMETRY_INVALID_ARGUMENT, geometry_result_to_str(GEOMETRY_INVALID_ARGUMENT), "untextured_cylinder_geometry_vertices_create", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, GEOMETRY_BAD_OPERATION, geometry_result_to_str(GEOMETRY_BAD_OPERATION), "untextured_cylinder_geometry_vertices_create", "*out_vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, GEOMETRY_INVALID_ARGUMENT, geometry_result_to_str(GEOMETRY_INVALID_ARGUMENT), "untextured_cylinder_geometry_vertices_create", "out_vertex_count_")
    if(!isfinite(radius_) || !(radius_ > 0.0f) || !isfinite(height_) || !(height_ > 0.0f)) {
        ret = GEOMETRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_cylinder_geometry_vertices_create(%s) - Provided radius_ or height_ is not valid.", geometry_result_to_str(ret));
        goto cleanup;
    }
    if(3 > segment_count_) {
        ret = GEOMETRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_cylinder_geometry_vertices_create(%s) - Provided segment_count_ is not valid.", geometry_result_to_str(ret));
        goto cleanup;
    }
    if(0 != *out_vertex_count_) {
        ret = GEOMETRY_BAD_OPERATION;
        ERROR_MESSAGE("untextured_cylinder_geometry_vertices_create(%s) - Provided *out_vertex_count_ is not valid.", geometry_result_to_str(ret));
        goto cleanup;
    }

    // 側面でsegment_count_ x 2の三角形それに加えて上面、下面があるので4倍
    if(!size_mul_checked((size_t)segment_count_, 4, &triangle_count)) {
        ret = GEOMETRY_OVERFLOW;
        ERROR_MESSAGE("untextured_cylinder_geometry_vertices_create(%s) - Triangle count overflow.", geometry_result_to_str(ret));
        goto cleanup;
    }
    if(!size_mul_checked(triangle_count, 3, &vertex_count)) {
        ret = GEOMETRY_OVERFLOW;
        ERROR_MESSAGE("untextured_cylinder_geometry_vertices_create(%s) - Vertex count overflow.", geometry_result_to_str(ret));
        goto cleanup;
    }
    if(!size_mul_checked(vertex_count, sizeof(point_normal_vertex_t), &allocation_size)) {
        ret = GEOMETRY_OVERFLOW;
        ERROR_MESSAGE("untextured_cylinder_geometry_vertices_create(%s) - Vertex array size overflow.", geometry_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_vertices);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = geometry_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("untextured_cylinder_geometry_vertices_create(%s) - general_allocator_allocate failed.", geometry_result_to_str(ret));
        goto cleanup;
    }

    step = 360.0f / (float)segment_count_;
    z_upper = height_;
    z_lower = 0.0f;
    for(uint32_t i = 0; i < segment_count_; ++i) {
        const uint32_t next = (i + 1) % segment_count_;
        const float angle1 = step * (float)i;
        const float angle2 = step * (float)next;
        const float c1 = cosf(CHOCO_DEG_TO_RAD(angle1));
        const float s1 = sinf(CHOCO_DEG_TO_RAD(angle1));
        const float c2 = cosf(CHOCO_DEG_TO_RAD(angle2));
        const float s2 = sinf(CHOCO_DEG_TO_RAD(angle2));

        x1 = radius_ * c1;
        y1 = radius_ * s1;

        x2 = radius_ * c2;
        y2 = radius_ * s2;

        normal1 = vec4i8_initialize((int8_t)(127.0f * c1), (int8_t)(127.0f * s1), 0, 0);
        normal2 = vec4i8_initialize((int8_t)(127.0f * c2), (int8_t)(127.0f * s2), 0, 0);

        // 側面
        // Triangle1(Smooth shading)
        tmp_vertices[index].position = vec3f_initialize(x1, y1, z_lower);
        tmp_vertices[index].normal = normal1;
        index++;
        tmp_vertices[index].position = vec3f_initialize(x2, y2, z_lower);
        tmp_vertices[index].normal = normal2;
        index++;
        tmp_vertices[index].position = vec3f_initialize(x1, y1, z_upper);
        tmp_vertices[index].normal = normal1;
        index++;

        // Triangle2(Smooth shading)
        tmp_vertices[index].position = vec3f_initialize(x1, y1, z_upper);
        tmp_vertices[index].normal = normal1;
        index++;
        tmp_vertices[index].position = vec3f_initialize(x2, y2, z_lower);
        tmp_vertices[index].normal = normal2;
        index++;
        tmp_vertices[index].position = vec3f_initialize(x2, y2, z_upper);
        tmp_vertices[index].normal = normal2;
        index++;

        // 上面
        tmp_vertices[index].position = vec3f_initialize(x1, y1, z_upper);
        tmp_vertices[index].normal = vec4i8_initialize(0, 0, 127, 0);
        index++;
        tmp_vertices[index].position = vec3f_initialize(x2, y2, z_upper);
        tmp_vertices[index].normal = vec4i8_initialize(0, 0, 127, 0);
        index++;
        tmp_vertices[index].position = vec3f_initialize(0.0f, 0.0f, z_upper);
        tmp_vertices[index].normal = vec4i8_initialize(0, 0, 127, 0);
        index++;

        // 下面
        tmp_vertices[index].position = vec3f_initialize(x2, y2, z_lower);
        tmp_vertices[index].normal = vec4i8_initialize(0, 0, -127, 0);
        index++;
        tmp_vertices[index].position = vec3f_initialize(x1, y1, z_lower);
        tmp_vertices[index].normal = vec4i8_initialize(0, 0, -127, 0);
        index++;
        tmp_vertices[index].position = vec3f_initialize(0.0f, 0.0f, z_lower);
        tmp_vertices[index].normal = vec4i8_initialize(0, 0, -127, 0);
        index++;
    }

    *out_vertices_ = tmp_vertices;
    *out_vertex_count_ = vertex_count;
    tmp_vertices = NULL;

    ret = GEOMETRY_SUCCESS;

cleanup:
    if(GEOMETRY_DATA_CORRUPTED == ret || GEOMETRY_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_vertices) {
        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}
