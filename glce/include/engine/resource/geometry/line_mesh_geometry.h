// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file line_mesh_geometry.h
 * @author chocolate-pie24
 * @brief line_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの定義
 *
 * @note line_mesh_shader: 複数の線分を描画する。色情報はuniform変数で扱い、RGB(4byte目はpadding)で指定する。このため、全ての線分が指定した色で描画される
 * @note line_mesh_geometryは線分の幾何情報のみを保持し、色情報はline_mesh_geometryを保持する親構造体で扱う
 *
 * @date 2026-06-04
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_LINE_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_LINE_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/aabb_3d.h"

typedef struct line_mesh_geometry line_mesh_geometry_t;   /**< line_mesh_geometryモジュール内部状態管理構造体 */

resource_result_t line_mesh_geometry_create_from_vertices(size_t vertex_count_, const line_vertex_t* vertices_, line_mesh_geometry_t** out_geometry_);

resource_result_t line_mesh_geometry_create_from_aabbs(size_t aabb_count_, const aabb_3d_t* aabbs_, line_mesh_geometry_t** out_geometry_);

void line_mesh_geometry_destroy(line_mesh_geometry_t** geometry_);

resource_result_t line_mesh_geometry_vertices_get(const line_mesh_geometry_t* geometry_, const line_vertex_t** out_vertices_);

resource_result_t line_mesh_geometry_vertex_count_get(const line_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
