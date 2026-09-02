// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file lit_mesh_geometry.h
 * @author chocolate-pie24
 * @brief lit_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの定義
 *
 * @note lit_mesh_shader: 光源・法線・材質色などを使って、陰影付きでmeshを描画するためのシェーダー
 *
 * @date 2026-06-04
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_LIT_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_LIT_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/resource/core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct lit_mesh_geometry lit_mesh_geometry_t;   /**< lit_mesh_geometryモジュール内部状態管理構造体 */

resource_result_t lit_mesh_geometry_create_from_vertices(size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t** out_geometry_);

void lit_mesh_geometry_destroy(lit_mesh_geometry_t** geometry_);

resource_result_t lit_mesh_geometry_vertices_get(const lit_mesh_geometry_t* geometry_, const point_normal_vertex_t** out_vertices_);

resource_result_t lit_mesh_geometry_vertex_count_get(const lit_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

bool lit_mesh_geometry_is_valid(const lit_mesh_geometry_t* geometry_);

#ifdef __cplusplus
}
#endif
#endif
