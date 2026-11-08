#include "engine/systems/renderer/resources/geometries/generators/line_grid_geometry.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/geometries/core/geometry_err_utils.h"

geometry_result_t line_grid_geometry_vertices_create(line_grid_plane_t plane_, vec4u8_t color_, vec3f_t corner1_, vec3f_t corner2_, float grid_step_, line_vertex_t** out_vertices_, size_t* out_vertex_count_) {
    geometry_result_t ret = GEOMETRY_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    vec3f_t min = { 0 };
    vec3f_t max = { 0 };

    double axis1_first_index = 0.0;
    double axis1_last_index = 0.0;

    double axis2_first_index = 0.0;
    double axis2_last_index = 0.0;

    double axis1_line_count = 0.0;
    double axis2_line_count = 0.0;

    float fixed_coordinate = 0.0;    // LINE_GRID_PLANE_XYであればZの値

    size_t axis1_count = 0;
    size_t axis2_count = 0;

    size_t line_count = 0;
    size_t vertex_count = 0;
    size_t vertex_index = 0;

    line_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, GEOMETRY_INVALID_ARGUMENT, geometry_result_to_str(GEOMETRY_INVALID_ARGUMENT), "line_grid_geometry_vertices_create", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, GEOMETRY_BAD_OPERATION, geometry_result_to_str(GEOMETRY_BAD_OPERATION), "line_grid_geometry_vertices_create", "*out_vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, GEOMETRY_INVALID_ARGUMENT, geometry_result_to_str(GEOMETRY_INVALID_ARGUMENT), "line_grid_geometry_vertices_create", "out_vertex_count_")
    if(!line_grid_plane_is_valid(plane_)) {
        ret = GEOMETRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("line_grid_geometry_vertices_create(%s) - Provided plane_ is not valid.", geometry_result_to_str(ret));
        goto cleanup;
    }
    if(!(grid_step_ > 0.0f)) {
        ret = GEOMETRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("line_grid_geometry_vertices_create(%s) - Provided grid_step_ is not valid.", geometry_result_to_str(ret));
        goto cleanup;
    }
    if(0 != *out_vertex_count_) {
        ret = GEOMETRY_BAD_OPERATION;
        ERROR_MESSAGE("line_grid_geometry_vertices_create(%s) - Provided out_vertex_count_ is not valid.", geometry_result_to_str(ret));
        goto cleanup;
    }

    min.elem[0] = (corner1_.elem[0] > corner2_.elem[0]) ? corner2_.elem[0] : corner1_.elem[0];
    min.elem[1] = (corner1_.elem[1] > corner2_.elem[1]) ? corner2_.elem[1] : corner1_.elem[1];
    min.elem[2] = (corner1_.elem[2] > corner2_.elem[2]) ? corner2_.elem[2] : corner1_.elem[2];

    max.elem[0] = (corner1_.elem[0] > corner2_.elem[0]) ? corner1_.elem[0] : corner2_.elem[0];
    max.elem[1] = (corner1_.elem[1] > corner2_.elem[1]) ? corner1_.elem[1] : corner2_.elem[1];
    max.elem[2] = (corner1_.elem[2] > corner2_.elem[2]) ? corner1_.elem[2] : corner2_.elem[2];

    // point = step * index, 原点基準なのでmin(or max) / stepでindexが求まる
    if(LINE_GRID_PLANE_XY == plane_) {
        axis1_first_index = ceil((double)min.elem[0] / (double)grid_step_);
        axis2_first_index = ceil((double)min.elem[1] / (double)grid_step_);

        axis1_last_index = floor((double)max.elem[0] / (double)grid_step_);
        axis2_last_index = floor((double)max.elem[1] / (double)grid_step_);

        fixed_coordinate = min.elem[2];
    } else if(LINE_GRID_PLANE_XZ == plane_) {
        axis1_first_index = ceil((double)min.elem[0] / (double)grid_step_);
        axis2_first_index = ceil((double)min.elem[2] / (double)grid_step_);

        axis1_last_index = floor((double)max.elem[0] / (double)grid_step_);
        axis2_last_index = floor((double)max.elem[2] / (double)grid_step_);

        fixed_coordinate = min.elem[1];
    } else {    // LINE_GRID_PLANE_YZ
        axis1_first_index = ceil((double)min.elem[1] / (double)grid_step_);
        axis2_first_index = ceil((double)min.elem[2] / (double)grid_step_);

        axis1_last_index = floor((double)max.elem[1] / (double)grid_step_);
        axis2_last_index = floor((double)max.elem[2] / (double)grid_step_);

        fixed_coordinate = min.elem[0];
    }

    // それぞれの平面の線分数計算
    // first > lastになる例(min = 1.0, max = 2.0, step = 10.0)
    if(axis1_first_index <= axis1_last_index) {
        axis1_line_count = axis1_last_index - axis1_first_index + 1.0;
    }
    if(axis2_first_index <= axis2_last_index) {
        axis2_line_count = axis2_last_index - axis2_first_index + 1.0;
    }
    if(0.0 == axis1_line_count || 0.0 == axis2_line_count) {
        ret = GEOMETRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("line_grid_geometry_vertices_create(%s) - Provided plane configuration is not valid.", geometry_result_to_str(ret));
        goto cleanup;
    }
    if(!(axis1_line_count <= LINE_GRID_MAX_LINES_PER_AXIS) || !(axis2_line_count <= LINE_GRID_MAX_LINES_PER_AXIS)) {
        ret = GEOMETRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("line_grid_geometry_vertices_create(%s) - Line count limit exceeded.", geometry_result_to_str(ret));
        goto cleanup;
    }
    axis1_count = (size_t)axis1_line_count;
    axis2_count = (size_t)axis2_line_count;

    line_count = axis1_count + axis2_count;
    vertex_count = line_count * 2;

    ret_general_allocator = general_allocator_allocate(sizeof(line_vertex_t) * vertex_count, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_vertices);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = geometry_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("line_grid_geometry_vertices_create(%s) - general_allocator_allocate failed.", geometry_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i < axis1_count; ++i) {
        const double grid_index = axis1_first_index + (double)i;
        const float u = (float)(grid_index * (double)grid_step_);

        if(LINE_GRID_PLANE_XY == plane_) {
            tmp_vertices[vertex_index].position = vec3f_initialize(u, min.elem[1], fixed_coordinate);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;

            tmp_vertices[vertex_index].position = vec3f_initialize(u, max.elem[1], fixed_coordinate);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;
        } else if(LINE_GRID_PLANE_XZ == plane_) {
            tmp_vertices[vertex_index].position = vec3f_initialize(u, fixed_coordinate, min.elem[2]);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;

            tmp_vertices[vertex_index].position = vec3f_initialize(u, fixed_coordinate, max.elem[2]);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;
        } else {    // LINE_GRID_PLANE_YZ
            tmp_vertices[vertex_index].position = vec3f_initialize(fixed_coordinate, u, min.elem[2]);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;

            tmp_vertices[vertex_index].position = vec3f_initialize(fixed_coordinate, u, max.elem[2]);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;
        }
    }
    for(size_t i = 0; i < axis2_count; ++i) {
        const double grid_index = axis2_first_index + (double)i;
        const float u = (float)(grid_index * (double)grid_step_);

        if(LINE_GRID_PLANE_XY == plane_) {
            tmp_vertices[vertex_index].position = vec3f_initialize(min.elem[0], u, fixed_coordinate);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;

            tmp_vertices[vertex_index].position = vec3f_initialize(max.elem[0], u, fixed_coordinate);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;
        } else if(LINE_GRID_PLANE_XZ == plane_) {
            tmp_vertices[vertex_index].position = vec3f_initialize(min.elem[0], fixed_coordinate, u);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;

            tmp_vertices[vertex_index].position = vec3f_initialize(max.elem[0], fixed_coordinate, u);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;
        } else {    // LINE_GRID_PLANE_YZ
            tmp_vertices[vertex_index].position = vec3f_initialize(fixed_coordinate, min.elem[1], u);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;

            tmp_vertices[vertex_index].position = vec3f_initialize(fixed_coordinate, max.elem[1], u);
            tmp_vertices[vertex_index].color = color_;
            ++vertex_index;
        }
    }

    *out_vertex_count_ = vertex_count;
    *out_vertices_ = tmp_vertices;
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

bool line_grid_plane_is_valid(line_grid_plane_t plane_) {
    switch(plane_) {
    case LINE_GRID_PLANE_XY:
        return true;
    case LINE_GRID_PLANE_XZ:
        return true;
    case LINE_GRID_PLANE_YZ:
        return true;
    default:
        return false;
    }
}
