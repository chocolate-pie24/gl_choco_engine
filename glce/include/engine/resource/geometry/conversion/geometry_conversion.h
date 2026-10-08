// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file geometry_conversion.h
 * @author chocolate-pie24
 * @brief 異なるgeometry representation間の変換を仲介するbridge APIを提供する
 *
 * @details
 * Geometry Conversion moduleは、異なるgeometry module間のrepresentation変換を仲介する。
 *
 * 本module自身はgeometry semanticを所有せず、
 * source geometryから変換に必要なrepresentationを取得する処理はsource側semantic ownerへ、
 * destination geometryを構築する処理はdestination側semantic ownerへ委譲する。
 *
 * Geometry Conversion moduleはpersistent stateおよびresource ownershipを持たず、
 * source resourceおよびdestination resourceのlifetimeを管理しない。
 *
 * @section geometry_conversion_boundary_contract Module Boundary Contract
 *
 * - Geometry Conversion moduleは異なるgeometry representation間を接続するbridgeとして機能する。
 * - 本module自身はsource geometryまたはdestination geometry固有の
 *   canonical stateおよびsemantic validityを定義しない。
 *
 * - source geometryから変換に必要なrepresentationを取得する処理およびvalidationは、
 *   source geometryのsemantic ownerへ委譲する。
 * - destination geometryを構築する処理およびvalidationは、
 *   destination geometryのsemantic ownerへ委譲する。
 * - Geometry Conversion moduleは各semantic ownerが行うvalidationを重複して実行しない。
 *
 * - 本moduleはexternal dataをtrusted internal representationへ昇格させる
 *   trust boundaryとして扱わない。
 * - source geometryおよびそこから取得されるrepresentationは、
 *   source moduleのcontractに従ったtrusted internal representationとして扱う。
 *
 * - source geometryのownershipおよびlifetimeはcallerまたはsource ownerに残る。
 * - Geometry Conversion moduleはsource geometryおよびそのinternal resourceの
 *   ownershipを取得しない。
 * - destinationへの変換処理中に生成するtemporary valueは本operation内で管理し、
 *   destination semantic ownerによるconstruction成功後にのみcaller outputへ反映する。
 *
 * - 本moduleはpersistent stateおよびowned resourceを保持しないため、
 *   module固有のcanonical validatorを提供しない。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_RESOURCE_GEOMETRY_CONVERSION_GEOMETRY_CONVERSION_H
#define GLCE_ENGINE_RESOURCE_GEOMETRY_CONVERSION_GEOMETRY_CONVERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/resource/core/resource_types.h"

typedef struct lit_mesh_geometry lit_mesh_geometry_t;
typedef struct aabb_3d aabb_3d_t;

resource_result_t geometry_conversion_lit_mesh_geometry_to_aabb_3d(const lit_mesh_geometry_t* src_geometry_, aabb_3d_t* dst_geometry_);

#ifdef __cplusplus
}
#endif
#endif
