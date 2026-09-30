// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file lit_mesh_geometry.c
 * @author chocolate-pie24
 * @brief lit_meshシェーダーが描画する形状データのCPU側リソースを操作するモジュールAPIの実装
 *
 * @note lit_mesh_shader: 光源・法線・材質色などを使って、陰影付きでmeshを描画するためのシェーダー
 *
 * @todo カバレッジ改善
 *
 * @date 2026-06-04
 *
 */
#include "engine/resource/geometry/lit_mesh_geometry.h"

#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

/*
 * Module Internal Contract
 *
 * Canonical state:
 * - vertex_countは0より大きい。
 * - vertex_countは3の倍数であり、連続する3頂点を1つの三角形として扱う。
 * - vertex_count * sizeof(point_normal_vertex_t)はsize_tで表現可能である。
 * - verticesはNULLではない。
 * - verticesはvertex_count個のpoint_normal_vertex_tを保持可能な連続storageを参照する。
 * - verticesに格納される各point_normal_vertex_tはcanonical stateを満たす。
 *
 * Representation / Ownership:
 * - verticesが指すstorageはGeneral Allocatorから取得したlive allocationである。
 * - Lit Mesh Geometry moduleはverticesのownershipを単独で保持し、
 *   geometry objectのlifetimeとともにそのstorageのlifetimeを管理する。
 * - lit_mesh_geometry_t object自身とverticesが指すvertex storageは、
 *   それぞれ独立したallocationとして保持する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

/*
 * Module Validation Policy
 *
 * - ValidationはModule Internal Contractで定義したcanonical stateを基準として行う。
 *
 * - private shallow validatorは、lit_mesh_geometry_t自身のroot fieldと、
 *   root field間のlocal structural relationを検証する。
 * - shallow validatorはvertex_countが0より大きい3の倍数であること、
 *   vertex array sizeがsize_tで表現可能であること、
 *   およびvertices != NULLであることを確認する。
 * - shallow validationではverticesが指すstorageをdereferenceせず、
 *   owned vertex elementのsemantic validationも行わない。
 *
 * - canonical validatorはowned verticesを走査する前に、
 *   General Allocator上のcurrent allocationであることを
 *   general_allocator_ptr_is_allocated()によって確認する。
 * - root structural conditionおよびallocation validityを確認した後、
 *   owned vertex arrayの各要素についてpoint_normal_vertex_is_valid()へvalidationを委譲し、
 *   point_normal_vertex_tとしてのcanonical validityを確認する。
 *
 * - canonical validatorは、引数として渡されたlit_mesh_geometry_t自身の
 *   allocation validityを検証しない。
 *   lit_mesh_geometry_tをowned pointerとして保持するownerが存在する場合、
 *   そのstorage validityはowner側のownership closureとして扱う。
 *
 * - 現在のcanonical validatorは、verticesがGeneral Allocator上の
 *   current allocationであることまでを確認する。
 * - verticesのactual allocation rangeと
 *   vertex_count * sizeof(point_normal_vertex_t)の整合性は現在検証しない。
 * - ownership closureおよびmemory safetyをより厳密に検証する必要が生じた場合は、
 *   General Allocatorのallocation informationを利用して、
 *   vertex array sizeとのrange relationをcanonical validationへ追加することを検討する。
 *
 * - Lit Mesh Geometry moduleはexternal data(外部ファイルやネットワーク経由から取得したデータ)を
 *   trusted internal representationへ昇格させるtrust boundaryとして扱わない。
 * - 外部source由来のfloating-point dataについては、
 *   Loader等のupstream trust boundaryで必要なfinite validationが完了していることを前提とし、
 *   本moduleではfinite性だけを目的としたsource arrayの全要素validationを重複して行わない。
 *
 * - caller-providedなarrayについて、pointer、element count、
 *   size calculation等のcontainer / arrayとしてのstructural conditionは、
 *   そのarrayを走査するLit Mesh Geometry operationが必要に応じて検証する。
 * - array element固有のsemantic validityについては、
 *   そのsemanticを実際にconsumeするoperationがvalidation responsibilityを持つ。
 * - element固有のsemantic operationを別moduleへ委譲する場合は、
 *   そのsemantic validationもsemantic ownerである委譲先moduleへ委譲し、
 *   Lit Mesh Geometry側では同一semanticを重複して検証しない。
 * - 本module内でsource elementをcopyするだけで、そのelement固有のsemanticを
 *   operation結果の判断に使用しない場合は、
 *   upstream boundaryで成立済みのsemantic contractを前提として処理する。
 *
 * - explicit canonical validatorのvalidation semanticsはBUILD_MODEによって変更しない。
 * - public API内部でcanonical validatorをautomaticに実行するかどうかは、
 *   各operationがconsumeするstate、relation、memory safetyおよび
 *   lifecycle transitionに基づいてAPIごとに決定する。
 *
 * - validatorは対象stateを変更せず、resource allocation、repair、log出力等の
 *   observable side effectを発生させない。
 * - validation failure時はfalseを返す。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
/**
 * @brief lit_mesh_geometry内部状態管理構造体
 *
 */
struct lit_mesh_geometry {
    size_t vertex_count;                /**< lit_mesh_geometryが所有する頂点数 */
    point_normal_vertex_t* vertices;    /**< lit_mesh_geometryが所有する頂点配列 */
};

// ============================================================
// Private Function Declarations
// ============================================================
// Validators
static bool is_valid_shallow(const lit_mesh_geometry_t* geometry_);

// ============================================================
// Public API
// ============================================================

// lit_mesh_geometry_create_from_vertices Validation Policy
//
// - create前には有効なlit_mesh_geometry_tが存在しないため、
//   PreconditionsではLit Mesh Geometry validatorを使用しない。
//
// - vertices_およびout_geometry_のpointer existenceは、
//   container accessおよびpublic output contractに必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
// - *out_geometry_ != NULLは既存pointerを上書きするAPI misuseであるため、BAD_OPERATIONとして扱う。
// - vertex_count_は0より大きい3の倍数であることを要求する。
// - vertex_count_からvertex array sizeを算出する処理が
//   size_tの表現可能範囲内であることを検証する。
// - vertices_が実際にvertex_count_個以上のpoint_normal_vertex_tを読み取り可能な
//   storageを参照していることはcaller contractとする。
//
// - source verticesの各point_normal_vertex_tは、upstream trust boundaryで
//   validなinternal representationとして受理済みであることを前提とする。
// - 本operationはsource vertexをgeometry-owned storageへcopyするだけであり、
//   point_normal_vertex_t固有のsemanticをoperation判断に使用しないため、
//   source arrayに対するpoint_normal_vertex_is_valid()の全要素Precondition validationは行わない。
//
// - lit_mesh_geometry_t storageおよびowned vertex storageのallocationは
//   General Allocatorへ委譲する。
// - General Allocatorから返されたresultはResource resultへ変換して伝播する。
// - 下位moduleからDATA_CORRUPTEDを受け取った場合は、
//   fail-stop ruleに従って通常cleanupを行わない。
//
// - DEBUG_BUILD / TEST_BUILDではtemporary geometryのconstruction完了後、
//   callerへ公開する前のStable boundaryでlit_mesh_geometry_is_valid()を実行し、
//   canonical Postcondition validationを行う。
// - このPostcondition validationはsource arrayをtrust boundaryとして
//   再認証するためのものではなく、construction後のlit_mesh_geometry_tが
//   Module Internal Contractを満たすことをdiagnosticとして確認するために行う。
// - Postcondition validationに成功した場合のみ、geometry objectおよび
//   owned vertex storageのownershipをcallerへcommitする。
// - RELEASE_BUILDではautomatic canonical Postcondition validationを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
resource_result_t lit_mesh_geometry_create_from_vertices(size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t** out_geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    lit_mesh_geometry_t* tmp_geometry = NULL;
    point_normal_vertex_t* tmp_vertices = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create_from_vertices", "out_geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_geometry_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_create_from_vertices", "*out_geometry_")
    if(0 == vertex_count_ || 0 != (vertex_count_ % 3)) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_geometry_create_from_vertices(%s) - Provided vertex_count_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX / vertex_count_) < sizeof(point_normal_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("lit_mesh_geometry_create_from_vertices(%s) - CPU-side vertex array size overflow. vertex_count = %zu, vertex_size = %zu.", resource_result_to_str(ret), vertex_count_, sizeof(point_normal_vertex_t));
        goto cleanup;
    }

    // Prepare.
    // 一時リソース確保
    ret_general_allocator = general_allocator_allocate(sizeof(lit_mesh_geometry_t), GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY, (void**)&tmp_geometry);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("lit_mesh_geometry_create_from_vertices(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(point_normal_vertex_t) * vertex_count_, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("lit_mesh_geometry_create_from_vertices(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    // 頂点情報コピー
    for(size_t i = 0; i != vertex_count_; ++i) {
        tmp_vertices[i] = vertices_[i];
    }

    // geometry初期化
    tmp_geometry->vertices = tmp_vertices;
    tmp_geometry->vertex_count = vertex_count_;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!lit_mesh_geometry_is_valid(tmp_geometry)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_create_from_vertices(%s) - Postcondition validation failed for 'tmp_geometry'.", resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_geometry_ = tmp_geometry;
    tmp_geometry = NULL;
    tmp_vertices = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != tmp_vertices) {
            general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
        }
        if(NULL != tmp_geometry) {
            general_allocator_free((void**)&tmp_geometry, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
        }
    }
    return ret;
}

// lit_mesh_geometry_destroy Validation Policy
//
// - geometry_ == NULLまたは*geometry_ == NULLはno-opとして扱う。
//
// - DEBUG_BUILD / TEST_BUILDではresource release開始前に
//   lit_mesh_geometry_is_valid()を実行し、
//   canonical Precondition validationを行う。
// - canonical validationに失敗した場合はsuspectなowned vertex storageを辿らず、
//   vertex storageおよびlit_mesh_geometry_t storageのfreeを行わない。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてdestroyを実行する。
//
// - owned vertex storageおよびlit_mesh_geometry_t自身のstorage releaseは
//   General Allocatorへ委譲する。
// - destroyによってobject lifetimeが終了するため、
//   Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
void lit_mesh_geometry_destroy(lit_mesh_geometry_t** geometry_) {
    if(NULL == geometry_) {
        return;
    }
    if(NULL == *geometry_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!lit_mesh_geometry_is_valid(*geometry_)) {
        ERROR_MESSAGE("lit_mesh_geometry_destroy(%s) - Provided geometry_ is corrupted.", resource_result_to_str(RESOURCE_DATA_CORRUPTED));
        return;
    }
#endif

    general_allocator_free((void**)&(*geometry_)->vertices, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
    general_allocator_free((void**)geometry_, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
}

// lit_mesh_geometry_vertices_get Validation Policy
//
// - geometry_、out_vertices_およびout_vertex_count_のpointer existenceは
//   public API contractとしてRELEASE_BUILDを含む全BUILDで検証する。
// - *out_vertices_ != NULLは既存pointerを上書きするAPI misuseであるため、
//   BAD_OPERATIONとして扱う。
//
// - 本operationはowned vertex storageへのborrowed pointerとvertex countを
//   一組のvertex array viewとしてcallerへ公開する。
// - borrowed pointer単体ではなく、verticesとvertex_countのrelationを
//   operation結果としてconsumeする。
//
// - DEBUG_BUILD / TEST_BUILDではoutput公開前に
//   lit_mesh_geometry_is_valid()を実行し、
//   geometry_にcanonical Precondition validationを行う。
// - canonical validationに失敗した場合はRESOURCE_DATA_CORRUPTEDを返し、
//   borrowed vertex array viewをcallerへ公開しない。
// - RELEASE_BUILDではautomatic canonical validationを行わず、
//   Module Internal Contractが成立していることを前提としてviewを公開する。
//
// - 本operationはlit_mesh_geometry_tおよびowned vertex storageを変更せず、
//   ownership relationも変更しないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
resource_result_t lit_mesh_geometry_vertices_get(const lit_mesh_geometry_t* geometry_, const point_normal_vertex_t** out_vertices_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertices_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertices_get", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertices_get", "*out_vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertices_get", "out_vertex_count_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!lit_mesh_geometry_is_valid(geometry_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_geometry_vertices_get(%s) - Precondition validation failed for 'geometry_'.", resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_vertices_ = geometry_->vertices;
    *out_vertex_count_ = geometry_->vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// lit_mesh_geometry_is_valid Validation Policy
//
// - 本APIはlit_mesh_geometry_tのpublic canonical validatorである。
// - geometry_ == NULLの場合はfalseを返す。
// - explicit validatorであるため、BUILD_MODEによってvalidation semanticsを変更しない。
//
// - private shallow validatorによってvertex_count、vertex array sizeのrepresentability、
//   およびverticesのpointer existenceに関するlocal structural invariantを検証する。
// - owned verticesを要素走査する前に、
//   general_allocator_ptr_is_allocated()によってverticesがGeneral Allocator上の
//   current allocationであることを確認する。
// - allocation validityおよびlocal structural invariant確認後、
//   owned vertex arrayを走査し、各point_normal_vertex_tのcanonical validationを
//   point_normal_vertex_is_valid()へ委譲する。
//
// - geometry_自身のallocation validityは本validatorでは検証しない。
//   geometry_をowned pointerとして保持するownerが存在する場合、
//   そのstorage validityはowner側のownership closureとして扱う。
//
// - 現在はverticesがGeneral Allocator上のcurrent allocationであることまでを確認し、
//   actual allocation rangeとvertex_count * sizeof(point_normal_vertex_t)のrelationは検証しない。
// - ownership closureおよびmemory safetyをより厳密に検証する必要が生じた場合は、
//   General Allocatorのallocation informationを利用したrange validationの追加を検討する。
//
// - validatorは対象stateを変更せず、resource allocation、repair、log出力等の
//   observable side effectを発生させない。
// - validation failure時はfalseを返す。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool lit_mesh_geometry_is_valid(const lit_mesh_geometry_t* geometry_) {
    if(NULL == geometry_) {
        return false;
    }
    if(!is_valid_shallow(geometry_)) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated((const void*)geometry_->vertices)) {
        return false;
    }
    for(size_t i = 0; i != geometry_->vertex_count; ++i) {
        if(!point_normal_vertex_is_valid(&geometry_->vertices[i])) {
            return false;
        }
    }
    return true;
}

// ============================================================
// Validators
// ============================================================
static bool is_valid_shallow(const lit_mesh_geometry_t* geometry_) {
    if(NULL == geometry_) {
        return false;
    }
    if(0 == geometry_->vertex_count) {
        return false;
    }
    if(0 != (geometry_->vertex_count % 3)) {
        return false;
    }
    if((SIZE_MAX / geometry_->vertex_count) < sizeof(point_normal_vertex_t)) {
        return false;
    }
    if(NULL == geometry_->vertices) {
        return false;
    }
    return true;
}
