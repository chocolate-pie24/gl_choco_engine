#ifndef GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_AABB_3D_H
#define GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_AABB_3D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/core/geometry_primitive/geometry_primitive_types.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/base/choco_math/math_types.h"

typedef struct aabb_3d {
    vec3f_t min;
    vec3f_t max;
} aabb_3d_t;

geometry_primitive_result_t aabb_3d_initialize_from_min_max(const vec3f_t* min_, const vec3f_t* max_, aabb_3d_t* out_aabb_);

geometry_primitive_result_t aabb_3d_initialize_from_point_vertices(const point_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_);

geometry_primitive_result_t aabb_3d_initialize_from_line_vertices(const line_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_);

geometry_primitive_result_t aabb_3d_initialize_from_point_normal_vertices(const point_normal_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_);

// min, maxを全て0で初期化する(この状態のaabbはvalid)
void aabb_3d_reset(aabb_3d_t* aabb_);

// vertices_格納順(x軸: 画面右方向+, y軸: 画面上方向+, z軸: 画面奥行方向-)
// 底面
// - vertices_[0]: min_x, min_y, max_z
// - vertices_[1]: max_x, min_y, max_z
// - vertices_[2]: max_x, min_y, min_z
// - vertices_[3]: min_x, min_y, min_z
// 上面
// - vertices_[4]: min_x, max_y, max_z
// - vertices_[5]: max_x, max_y, max_z
// - vertices_[6]: max_x, max_y, min_z
// - vertices_[7]: min_x, max_y, min_z
geometry_primitive_result_t aabb_3d_vertices_get(const aabb_3d_t* aabb_, vec3f_t vertices_[8]);

// min > max -> invalid
// nan or inf -> invalid
// 厚み、体積を持たない退化したaabbはvalidとする
bool aabb_3d_is_valid(const aabb_3d_t* aabb_);

#ifdef __cplusplus
}
#endif
#endif
