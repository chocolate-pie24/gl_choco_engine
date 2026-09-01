// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file point_mesh_geometry.h
 * @author chocolate-pie24
 * @brief point_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの定義
 *
 * @note point_mesh_shader: 複数の点を描画する
 * @note point_mesh_geometryは点群の幾何情報のみを保持し、色情報はpoint_mesh_geometryを保持する親構造体で扱う
 *
 * @todo TODO: pcdファイル等の点群ファイルからの初期化はそのうちやる
 *
 * @date 2026-06-06
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_POINT_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_POINT_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct point_mesh_geometry point_mesh_geometry_t;   /**< point_mesh_geometryモジュール内部状態管理構造体 */

resource_result_t point_mesh_geometry_default_create(point_mesh_geometry_t** geometry_);

resource_result_t point_mesh_geometry_create_from_vertices(size_t vertex_count_, const point_vertex_t* vertices_, point_mesh_geometry_t** geometry_);

/**
 * @brief point_mesh_geometry_tが保有するリソースと自身のメモリを解放する
 *
 * @warning 内部データの不整合が発生していた場合はpoint_mesh_geometry_tが保有する頂点配列のメモリは解放されず、リーク状態となる。この場合、geometry_自身のメモリは解放し、エラーメッセージを出力する
 * @note geometry_ == NULL または *geometry_ == NULL の場合は何も行わない
 * @note 本API実行後、*geometry_はNULLとなる
 *
 * @param[in,out] geometry_ point_mesh_geometry_t構造体インスタンスへのダブルポインタ
 */
void point_mesh_geometry_destroy(point_mesh_geometry_t** geometry_);

resource_result_t point_mesh_geometry_initialize_from_vertices(size_t vertex_count_, const point_vertex_t* vertices_, point_mesh_geometry_t* geometry_);

void point_mesh_geometry_deinitialize(point_mesh_geometry_t* geometry_);

resource_result_t point_mesh_geometry_clone(const point_mesh_geometry_t* src_, point_mesh_geometry_t** out_geometry_);

resource_result_t point_mesh_geometry_vertices_get(const point_mesh_geometry_t* geometry_, const point_vertex_t** out_vertices_);

resource_result_t point_mesh_geometry_vertex_count_get(const point_mesh_geometry_t* geometry_, size_t* out_vertex_count_);

#ifdef __cplusplus
}
#endif
#endif
