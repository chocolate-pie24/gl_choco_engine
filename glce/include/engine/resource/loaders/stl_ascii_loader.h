// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file stl_ascii_loader.h
 * @author chocolate-pie24
 * @brief ASCII STLファイルをGLCE内部geometry representationへloadするAPIを提供する
 *
 * @details
 * STL ASCII Loader moduleはexternal ASCII STL fileを読み込み、
 * GLCE内部で使用可能なvertex dataへ変換する。
 *
 * @todo Binary STL supportおよびASCII / Binary format selectionは別途整理する。
 * - binary_loadの追加(別モジュール)
 * - stl_loaderモジュールを追加し、format_detectの追加(ascii, binary判定+stl_ascii_loader, stl_binary_loader)
 *
 * @section stl_ascii_loader_boundary_contract Module Boundary Contract
 *
 * - STL ASCII Loader moduleはexternal STL resourceとGLCE内部representationの間にある
 *   resource trust boundaryとして動作する。
 * - load成功時に公開されるvertex dataは、
 *   GLCE内部で使用可能なtrusted representationとして扱う。
 *
 * - 本moduleはASCII STLのtriangle facet representationを対象とする。
 * - 各triangleは1つのfacet normalと3つのvertex positionから構成される。
 * - geometryには1つ以上のtriangleが存在する。
 *
 * - load成功時に返されるvertex dataはpoint_normal_vertex_tの配列である。
 * - 各triangleは3つのpoint_normal_vertex_tへ変換される。
 * - 各vertex positionはvalidなGLCE vertex representationである。
 * - facet normalはGLCE内部で使用するpoint_normal_vertex_tのnormal representationへ
 *   normalizeされ、対応する3頂点へ設定される。
 *
 * - load成功時に返されるvertex storageはGeneral Allocatorから
 *   GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRYで取得されたallocationである。
 * - output vertex storageのownershipはcallerへ移転する。
 * - callerは取得したvertex storageのlifetime管理およびreleaseに責任を持つ。
 *
 * - load失敗時にはoutputとして有効なgeometry representationを公開しない。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_RESOURCE_LOADERS_STL_ASCII_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_STL_ASCII_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/resource/core/resource_types.h"

#include "engine/core/geometry_primitive/vertex.h"

resource_result_t stl_ascii_loader_load(const char* fullpath_, size_t* out_vertex_count_, point_normal_vertex_t** out_vertices_);

#ifdef __cplusplus
}
#endif
#endif
