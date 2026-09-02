// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file ui_mesh_geometry.h
 * @author chocolate-pie24
 * @brief ui_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの定義
 *
 * @note ui_mesh_shader: 2D矩形領域にテクスチャを貼った描画を行う, 描画単位は矩形領域ごとに描画する
 * @note ui_mesh_geometryは矩形領域のテクスチャuv座標、矩形領域座標のみを保持する
 *
 * @date 2026-06-12
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_UI_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_UI_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct ui_mesh_geometry ui_mesh_geometry_t;   /**< ui_mesh_geometryモジュール内部状態管理構造体 */

resource_result_t ui_mesh_geometry_create_from_vertices(size_t vertex_count_, const ui_vertex_t* vertices_, ui_mesh_geometry_t** out_geometry_);

void ui_mesh_geometry_destroy(ui_mesh_geometry_t** geometry_);

resource_result_t ui_mesh_geometry_vertices_get(const ui_mesh_geometry_t* geometry_, const ui_vertex_t** out_vertices_);

resource_result_t ui_mesh_geometry_vertex_count_get(const ui_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
