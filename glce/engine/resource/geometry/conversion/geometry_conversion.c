// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/resource/geometry/conversion/geometry_conversion.h"

#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

#include "engine/resource/geometry/lit_mesh_geometry.h"

// geometry_conversion_lit_mesh_geometry_to_aabb_3d Validation Policy
//
// - src_geometry_およびdst_geometry_のpointer existenceは、
//   source accessおよびpublic output contractに必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
//
// - 本operationはLit Mesh GeometryからAABB 3Dへのrepresentation変換を仲介するbridgeであり、
//   Lit Mesh GeometryおよびAABB 3D固有のsemantic validityを自身では定義しない。
//
// - source geometryから変換に必要なvertex array viewを取得する処理は、
//   lit_mesh_geometry_vertices_get()へ委譲する。
// - src_geometry_のcanonical validityおよびverticesとvertex_countのrelationに関する
//   validationはLit Mesh Geometry moduleの責務とし、
//   Geometry Conversion側ではlit_mesh_geometry_is_valid()を重複して実行しない。
// - lit_mesh_geometry_vertices_get()から返されたResource resultはそのまま伝播する。
//
// - Lit Mesh Geometryから取得したpoint_normal_vertex_t arrayを
//   AABB 3D representationへ変換する処理は、
//   aabb_3d_initialize_from_point_normal_vertices()へ委譲する。
// - vertex elementおよびvertex arrayから有効なAABBを構築できるかというsemantic validationは
//   AABB 3D moduleの責務とし、Geometry Conversion側では
//   point_normal_vertex_tまたはAABB固有のsemantic validationを重複して実行しない。
// - aabb_3d_initialize_from_point_normal_vertices()から返されたresultは
//   Resource resultへ変換して伝播する。
//
// - 本operationはexternal dataをtrusted internal representationへ昇格させる
//   trust boundaryとして扱わない。
// - lit_mesh_geometry_vertices_get()から取得したvertex array viewは、
//   Lit Mesh Geometry moduleのcontractに従ったtrusted internal representationとして扱う。
//
// - destination AABBはまずoperation-localなtmp_aabb_3dへ構築する。
// - aabb_3d_initialize_from_point_normal_vertices()が成功した場合のみ、
//   tmp_aabb_3dをdst_geometry_へcopyしてoutputをcommitする。
// - conversion途中で失敗した場合はdst_geometry_を変更しない。
//
// - 本operationはsource geometryおよびそのowned resourceを変更せず、
//   ownership relationも変更しない。
// - destination semantic ownerによるconstruction結果をそのままcommitするため、
//   Geometry Conversion module独自のPostcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
resource_result_t geometry_conversion_lit_mesh_geometry_to_aabb_3d(const lit_mesh_geometry_t* src_geometry_, aabb_3d_t* dst_geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    aabb_3d_result_t ret_aabb_3d = AABB_3D_INVALID_ARGUMENT;

    size_t vertex_count = 0;
    aabb_3d_t tmp_aabb_3d = { 0 };
    const point_normal_vertex_t* tmp_vertices = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(src_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "geometry_conversion_lit_mesh_geometry_to_aabb_3d", "src_geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(dst_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "geometry_conversion_lit_mesh_geometry_to_aabb_3d", "dst_geometry_")

    // Prepare.
    ret = lit_mesh_geometry_vertices_get(src_geometry_, &tmp_vertices, &vertex_count);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("geometry_conversion_lit_mesh_geometry_to_aabb_3d(%s) - lit_mesh_geometry_vertices_get failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_aabb_3d = aabb_3d_initialize_from_point_normal_vertices(tmp_vertices, vertex_count, &tmp_aabb_3d);
    if(AABB_3D_SUCCESS != ret_aabb_3d) {
        ret = resource_result_convert_aabb_3d(ret_aabb_3d);
        ERROR_MESSAGE("geometry_conversion_lit_mesh_geometry_to_aabb_3d(%s) - aabb_3d_initialize_from_point_normal_vertices failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *dst_geometry_ = tmp_aabb_3d;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}
