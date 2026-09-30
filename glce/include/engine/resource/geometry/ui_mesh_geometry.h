// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file ui_mesh_geometry.h
 * @author chocolate-pie24
 * @brief UI描画に使用するCPU側geometry resourceを扱うopaque objectと操作を提供する
 *
 * @details
 * UI Mesh Geometry moduleは、UI描画領域を構成する2D頂点座標およびtexture UV座標をCPU側で保持する
 * `ui_mesh_geometry_t`と、そのlifetime管理およびgeometry data参照機能を提供する。
 *
 * `ui_mesh_geometry_t`は内部表現を公開しないopaque objectであり、
 * create成功後はUI描画に使用可能な完成済みCPU geometry resourceとして扱う。
 *
 * UI Mesh Geometryでは1つのUI imageを1つの矩形領域として扱い、
 * 1つの`ui_mesh_geometry_t`は1つのUI imageに対応する。
 * 矩形領域は2枚の三角形で構成するため、1 geometryあたりのvertex countは6固定とする。
 *
 * UI Mesh GeometryはUI描画領域を構成するvertex dataを扱い、
 * texture resourceそのものやその他のrendering stateは所有しない。
 *
 * @section ui_mesh_geometry_boundary_contract Module Boundary Contract
 *
 * - `ui_mesh_geometry_t`はopaque typeとして公開し、内部表現をmodule外部へ公開しない。
 * - UI Mesh Geometry moduleは`ui_mesh_geometry_t` object自身と、
 *   そのgeometryを構成するCPU側vertex dataのownershipおよびlifetimeを管理する。
 * - callerはUI Mesh Geometry moduleが所有するinternal resourceを直接変更または解放しない。
 * - module APIへ渡す`ui_mesh_geometry_t*`は、
 *   UI Mesh Geometry moduleによって生成され、lifetime中にあるobjectを参照するものとする。
 *
 * - 1つの`ui_mesh_geometry_t`は1つのUI imageに対応する。
 * - UI imageは2枚の三角形からなる1つの矩形領域として表現し、
 *   `ui_mesh_geometry_t`が保持するvertex countは6固定とする。
 *
 * - create成功時に公開される`ui_mesh_geometry_t`は、
 *   UI mesh geometryとして通常利用可能な完成済みresourceを表す。
 * - publicなpartial initialization stateは持たない。
 * - create失敗時には`ui_mesh_geometry_t`のownershipをcallerへ移転しない。
 *
 * - geometry生成元としてcallerから渡されるvertex dataは、
 *   callerがownershipを保持する。
 * - UI Mesh Geometry moduleは生成元resourceのownershipを取得せず、
 *   完成したgeometryが必要とするCPU側vertex dataを自身のownershipとして保持する。
 *
 * - 外部sourceから取得したfloating-point dataについては、
 *   Loader等のupstream trust boundaryでGLCE内部representationとして受理するために
 *   必要なvalidationが完了していることを前提とする。
 * - UI Mesh Geometry moduleはexternal dataをtrusted internal representationへ
 *   昇格させるboundaryとしては扱わない。
 *
 * - moduleから公開されるCPU側vertex dataへの参照はborrowed viewとして扱う。
 * - borrowed vertex dataのownershipはUI Mesh Geometry moduleに残り、
 *   callerはそのstorageを変更または解放しない。
 * - borrowed viewは、そのownerである`ui_mesh_geometry_t`のlifetime終了後には使用しない。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_UI_MESH_GEOMETRY_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_UI_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/resource/core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

typedef struct ui_mesh_geometry ui_mesh_geometry_t;   /**< ui_mesh_geometryモジュール内部状態管理構造体 */

resource_result_t ui_mesh_geometry_create_from_vertices(size_t vertex_count_, const ui_vertex_t* vertices_, ui_mesh_geometry_t** out_geometry_);

void ui_mesh_geometry_destroy(ui_mesh_geometry_t** geometry_);

resource_result_t ui_mesh_geometry_vertices_get(const ui_mesh_geometry_t* geometry_, const ui_vertex_t** out_vertices_, size_t* out_vertex_count_);

bool ui_mesh_geometry_is_valid(const ui_mesh_geometry_t* geometry_);

#ifdef __cplusplus
}
#endif
#endif
