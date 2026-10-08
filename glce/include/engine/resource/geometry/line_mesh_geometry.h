// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file line_mesh_geometry.h
 * @author chocolate-pie24
 * @brief line mesh描画に使用するCPU側geometry resourceを扱うopaque objectと操作を提供する
 *
 * @details
 * Line Mesh Geometry moduleは、複数の線分を構成する幾何情報をCPU側で保持する
 * `line_mesh_geometry_t`と、そのlifetime管理およびgeometry data参照機能を提供する。
 *
 * `line_mesh_geometry_t`は内部表現を公開しないopaque objectであり、
 * create成功後はline mesh描画に使用可能な完成済みCPU geometry resourceとして扱う。
 *
 * Line Mesh Geometryは線分の幾何情報のみを扱い、
 * 描画色などのmaterial / rendering stateは所有しない。
 *
 * @section line_mesh_geometry_boundary_contract Module Boundary Contract
 *
 * - `line_mesh_geometry_t`はopaque typeとして公開し、内部表現をmodule外部へ公開しない。
 * - Line Mesh Geometry moduleは`line_mesh_geometry_t` object自身と、
 *   そのgeometryを構成するCPU側vertex dataのownershipおよびlifetimeを管理する。
 * - callerはLine Mesh Geometry moduleが所有するinternal resourceを直接変更または解放しない。
 * - module APIへ渡す`line_mesh_geometry_t*`は、
 *   Line Mesh Geometry moduleによって生成され、lifetime中にあるobjectを参照するものとする。
 *
 * - create成功時に公開される`line_mesh_geometry_t`は、
 *   line mesh geometryとして通常利用可能な完成済みresourceを表す。
 * - publicなpartial initialization stateは持たない。
 * - create失敗時には`line_mesh_geometry_t`のownershipをcallerへ移転しない。
 *
 * - geometry生成元としてcallerから渡されるvertex dataおよびgeometry primitiveは、
 *   callerがownershipを保持する。
 * - Line Mesh Geometry moduleは生成元resourceのownershipを取得せず、
 *   完成したgeometryが必要とするCPU側vertex dataを自身のownershipとして保持する。
 *
 * - 外部sourceから取得したfloating-point dataについては、
 *   Loader等のupstream trust boundaryでGLCE内部representationとして受理するために
 *   必要なvalidationが完了していることを前提とする。
 * - Line Mesh Geometry moduleはexternal dataをtrusted internal representationへ
 *   昇格させるboundaryとしては扱わない。
 *
 * - moduleから公開されるCPU側vertex dataへの参照はborrowed viewとして扱う。
 * - borrowed vertex dataのownershipはLine Mesh Geometry moduleに残り、
 *   callerはそのstorageを変更または解放しない。
 * - borrowed viewは、そのownerである`line_mesh_geometry_t`のlifetime終了後には使用しない。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_LINE_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_LINE_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/resource/core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/aabb_3d.h"

typedef struct line_mesh_geometry line_mesh_geometry_t;   /**< line_mesh_geometryモジュール内部状態管理構造体 */

resource_result_t line_mesh_geometry_create_from_vertices(size_t vertex_count_, const line_vertex_t* vertices_, line_mesh_geometry_t** out_geometry_);

resource_result_t line_mesh_geometry_create_from_aabbs(size_t aabb_count_, const aabb_3d_t* aabbs_, line_mesh_geometry_t** out_geometry_);

void line_mesh_geometry_destroy(line_mesh_geometry_t** geometry_);

resource_result_t line_mesh_geometry_vertices_get(const line_mesh_geometry_t* geometry_, const line_vertex_t** out_vertices_, size_t* out_vertex_count_);

bool line_mesh_geometry_is_valid(const line_mesh_geometry_t* geometry_);

#ifdef __cplusplus
}
#endif
#endif
