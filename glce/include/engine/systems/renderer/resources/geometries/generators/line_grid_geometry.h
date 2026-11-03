// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_GEOMETRIES_GENERATORS_LINE_GRID_GEOMETRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_GEOMETRIES_GENERATORS_LINE_GRID_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"

#define LINE_GRID_MAX_LINES_PER_AXIS 1000

typedef enum {
    LINE_GRID_PLANE_XY = 0,
    LINE_GRID_PLANE_YZ,
    LINE_GRID_PLANE_XZ
} line_grid_plane_t;

// 原点を基準にしてGrid lineを生成する。grid_step_がcorner1_, corner2_ちょうどの位置に来ない場合はcorner1_, corner2_の直前まででgrid lineを生成する
// TODO: 線分による座標軸描画を検討する(使っていくうちに必要性を感じなければ不要)
geometry_result_t line_grid_geometry_vertices_create(line_grid_plane_t plane_, vec4u8_t color_, vec3f_t corner1_, vec3f_t corner2_, float grid_step_, line_vertex_t** out_vertices_, size_t* out_vertex_count_);

bool line_grid_plane_is_valid(line_grid_plane_t plane_);

#ifdef __cplusplus
}
#endif
#endif
