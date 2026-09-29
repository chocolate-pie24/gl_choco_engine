// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/*
 * Module Internal Contract
 *
 * Canonical state:
 * - aabb_3d_tのminおよびmaxを構成する全成分はfiniteである。
 * - x, y, zの各成分についてmin <= maxが成立する。
 * - 1つ以上の軸でmin == maxとなる退化したAABBもvalidなcanonical stateとする。
 *
 * Representation:
 * - minはAABBの各軸における最小座標を表す。
 * - maxはAABBの各軸における最大座標を表す。
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
 * - canonical validatorはaabb_3d_tのminおよびmaxについて、
 *   全成分がfiniteであること、および各軸でmin <= maxが成立することを検証する。
 * - 退化したAABBはcanonical validatorでvalidとして扱う。
 *
 * - aabb_3d_tは固定長の値のみから構成され、canonical validationで
 *   owned structureのtraversalまたはdeep validationを必要としない。
 *
 * - 現在、module内部にshallow validation depthを必要とするcall siteは存在しないため、private shallow validatorは設けない。
 * - canonical validatorはModule Internal Contractを直接検証する。
 *
 * - validatorは対象stateを変更せず、validation failure時はfalseを返す。
 * - validatorのsemanticsはBUILD_MODEによって変更しない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#include "engine/core/geometry_primitive/aabb_3d.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_macros.h"

#include "engine/core/geometry_primitive/vertex.h"

// ============================================================
// Private Constants
// ============================================================
static const char* const s_result_str_success = "SUCCESS";                        /**< 実行結果コード文字列: 正常終了 */
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";      /**< 実行結果コード文字列: 引数異常 */
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";        /**< 実行結果コード文字列: 不明なエラー */

// ============================================================
// Private Function Declarations
// ============================================================
// Utilities
static const char* result_to_str(aabb_3d_result_t result_);

// ============================================================
// Public API
// ============================================================

// aabb_3d_initialize_from_min_max Validation Policy
//
// - out_aabb_のpointer contractは、処理結果を書き込むために必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - min_およびmax_はModule Boundary Contractで定義された
//   trusted representationとして扱い、それぞれの値について
//   finite性を再認証しない。
//
// - min_およびmax_をAABBの境界として組み合わせることで成立する
//   x, y, z各成分のmin <= maxというrelationは、本operationがconsumeする
//   AABB固有のsemanticであるため、operation-specific checked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
//
// - 本operationは既存のaabb_3d_t stateをconsumeしないため、
//   PreconditionsでAABB validatorを使用しない。
//
// - API contractを満たす入力から生成されるaabb_3d_tのcanonical validityは
//   operation implementationによって保証し、automatic Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
aabb_3d_result_t aabb_3d_initialize_from_min_max(vec3f_t min_, vec3f_t max_, aabb_3d_t* out_aabb_) {
    aabb_3d_result_t ret = AABB_3D_INVALID_ARGUMENT;

    aabb_3d_t tmp_aabb = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, AABB_3D_INVALID_ARGUMENT, result_to_str(AABB_3D_INVALID_ARGUMENT), "aabb_3d_initialize_from_min_max", "out_aabb_")
    if(min_.elem[0] > max_.elem[0] || min_.elem[1] > max_.elem[1] || min_.elem[2] > max_.elem[2]) {
        ret = AABB_3D_INVALID_ARGUMENT;
        ERROR_MESSAGE("aabb_3d_initialize_from_min_max(%s) - Provided min_ or max_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    tmp_aabb.min = min_;
    tmp_aabb.max = max_;

    *out_aabb_ = tmp_aabb;

    ret = AABB_3D_SUCCESS;

cleanup:
    return ret;
}

// aabb_3d_initialize_from_point_vertices Validation Policy
//
// - vertices_およびout_aabb_のpointer contractは、operationの実行に必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
// - vertex_count_ > 0はvertices_[0]を参照して変換を開始するために必要な
//   operation-specific checked preconditionとして全BUILDで検証する。
//
// - vertices_が指すpoint_vertex_t arrayはModule Boundary Contractで定義された
//   trusted source representationとして扱い、各elementのcanonical validityを
//   AABB 3D module側で再認証しない。
//
// - 本operationは既存のaabb_3d_t stateをconsumeしないため、
//   PreconditionsでAABB validatorを使用しない。
//
// - API contractを満たす入力から生成されるaabb_3d_tのcanonical validityは
//   operation implementationによって保証し、automatic Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
aabb_3d_result_t aabb_3d_initialize_from_point_vertices(const point_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_) {
    aabb_3d_result_t ret = AABB_3D_INVALID_ARGUMENT;

    aabb_3d_t tmp_aabb = { 0 };
    vec3f_t min = { 0 };
    vec3f_t max = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, AABB_3D_INVALID_ARGUMENT, result_to_str(AABB_3D_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, AABB_3D_INVALID_ARGUMENT, result_to_str(AABB_3D_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_vertices", "out_aabb_")
    if(0 == vertex_count_) {
        ret = AABB_3D_INVALID_ARGUMENT;
        ERROR_MESSAGE("aabb_3d_initialize_from_point_vertices(%s) - Provided vertex_count_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    min = vertices_[0].position;
    max = vertices_[0].position;
    for(size_t i = 0; i != vertex_count_; ++i) {
        min = vec3f_component_min(min, vertices_[i].position);
        max = vec3f_component_max(max, vertices_[i].position);
    }
    tmp_aabb.min = min;
    tmp_aabb.max = max;

    // Output.
    *out_aabb_ = tmp_aabb;

    ret = AABB_3D_SUCCESS;

cleanup:
    return ret;
}

// aabb_3d_initialize_from_line_vertices Validation Policy
//
// - vertices_およびout_aabb_のpointer contractは、operationの実行に必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
// - vertex_count_ > 0はvertices_[0]を参照して変換を開始するために必要な
//   operation-specific checked preconditionとして全BUILDで検証する。
//
// - vertices_が指すline_vertex_t arrayはModule Boundary Contractで定義された
//   trusted source representationとして扱い、各elementのcanonical validityを
//   AABB 3D module側で再認証しない。
// - AABB生成は各vertexのpositionを独立してconsumeするため、
//   line topologyに固有のvertex count relationはvalidationしない。
//
// - 本operationは既存のaabb_3d_t stateをconsumeしないため、
//   PreconditionsでAABB validatorを使用しない。
//
// - API contractを満たす入力から生成されるaabb_3d_tのcanonical validityは
//   operation implementationによって保証し、automatic Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
aabb_3d_result_t aabb_3d_initialize_from_line_vertices(const line_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_) {
    aabb_3d_result_t ret = AABB_3D_INVALID_ARGUMENT;

    aabb_3d_t tmp_aabb = { 0 };
    vec3f_t min = { 0 };
    vec3f_t max = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, AABB_3D_INVALID_ARGUMENT, result_to_str(AABB_3D_INVALID_ARGUMENT), "aabb_3d_initialize_from_line_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, AABB_3D_INVALID_ARGUMENT, result_to_str(AABB_3D_INVALID_ARGUMENT), "aabb_3d_initialize_from_line_vertices", "out_aabb_")
    if(0 == vertex_count_) {
        ret = AABB_3D_INVALID_ARGUMENT;
        ERROR_MESSAGE("aabb_3d_initialize_from_line_vertices(%s) - Provided vertex_count_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    min = vertices_[0].position;
    max = vertices_[0].position;
    for(size_t i = 0; i != vertex_count_; ++i) {
        min = vec3f_component_min(min, vertices_[i].position);
        max = vec3f_component_max(max, vertices_[i].position);
    }
    tmp_aabb.min = min;
    tmp_aabb.max = max;

    // Output.
    *out_aabb_ = tmp_aabb;

    ret = AABB_3D_SUCCESS;

cleanup:
    return ret;
}

// aabb_3d_initialize_from_point_normal_vertices Validation Policy
//
// - vertices_およびout_aabb_のpointer contractは、operationの実行に必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
// - vertex_count_ > 0はvertices_[0]を参照して変換を開始するために必要な
//   operation-specific checked preconditionとして全BUILDで検証する。
//
// - vertices_が指すpoint_normal_vertex_t arrayはModule Boundary Contractで定義された
//   trusted source representationとして扱い、各elementのcanonical validityを
//   AABB 3D module側で再認証しない。
// - AABB生成は各vertexのpositionを独立してconsumeするため、
//   mesh topologyに固有のvertex count relationはvalidationしない。
//
// - 本operationは既存のaabb_3d_t stateをconsumeしないため、
//   PreconditionsでAABB validatorを使用しない。
//
// - API contractを満たす入力から生成されるaabb_3d_tのcanonical validityは
//   operation implementationによって保証し、automatic Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
aabb_3d_result_t aabb_3d_initialize_from_point_normal_vertices(const point_normal_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_) {
    aabb_3d_result_t ret = AABB_3D_INVALID_ARGUMENT;

    aabb_3d_t tmp_aabb = { 0 };
    vec3f_t min = { 0 };
    vec3f_t max = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, AABB_3D_INVALID_ARGUMENT, result_to_str(AABB_3D_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_normal_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_, ret, AABB_3D_INVALID_ARGUMENT, result_to_str(AABB_3D_INVALID_ARGUMENT), "aabb_3d_initialize_from_point_normal_vertices", "out_aabb_")
    if(0 == vertex_count_) {
        ret = AABB_3D_INVALID_ARGUMENT;
        ERROR_MESSAGE("aabb_3d_initialize_from_point_normal_vertices(%s) - Provided vertex_count_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    min = vertices_[0].position;
    max = vertices_[0].position;
    for(size_t i = 0; i != vertex_count_; ++i) {
        min = vec3f_component_min(min, vertices_[i].position);
        max = vec3f_component_max(max, vertices_[i].position);
    }
    tmp_aabb.min = min;
    tmp_aabb.max = max;

    // Output.
    *out_aabb_ = tmp_aabb;

    ret = AABB_3D_SUCCESS;

cleanup:
    return ret;
}

// aabb_3d_reset Validation Policy
//
// - aabb_ == NULLは本APIの定義されたno-opとして扱い、その場合は即座にreturnする。
//
// - 本operationはaabb_の既存semantic stateを参照せず、全fieldを既知のstateへ
//   上書きするため、PreconditionsでAABB validatorを使用しない。
//
// - 書き込む値からcanonical-validなstateが直接決定されるため、
//   automatic Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
void aabb_3d_reset(aabb_3d_t* aabb_) {
    if(NULL == aabb_) {
        return;
    }
    aabb_->min.elem[0] = 0.0f;
    aabb_->min.elem[1] = 0.0f;
    aabb_->min.elem[2] = 0.0f;

    aabb_->max.elem[0] = 0.0f;
    aabb_->max.elem[1] = 0.0f;
    aabb_->max.elem[2] = 0.0f;
}

// aabb_3d_vertices_get Validation Policy
//
// - aabb_およびout_vertices_のpointer contractは、operationの実行に必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - 本operationはaabb_をAABBとしてsemanticにconsumeして8頂点へ変換するため、
//   aabb_のcanonical validityをoperation-specific checked preconditionとして
//   全BUILDでaabb_3d_is_valid()により検証する。
//
// - 本operationはaabb_自身を変更しないため、AABBに対するPostcondition validationは行わない。
// - 出力vertexはvalidated AABBから直接構成されるため、
//   element validatorによるautomatic Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
aabb_3d_result_t aabb_3d_vertices_get(const aabb_3d_t* aabb_, vec3f_t out_vertices_[8]) {
    aabb_3d_result_t ret = AABB_3D_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, AABB_3D_INVALID_ARGUMENT, result_to_str(AABB_3D_INVALID_ARGUMENT), "aabb_3d_vertices_get", "aabb_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, AABB_3D_INVALID_ARGUMENT, result_to_str(AABB_3D_INVALID_ARGUMENT), "aabb_3d_vertices_get", "out_vertices_")
    if(!aabb_3d_is_valid(aabb_)) {
        ret = AABB_3D_INVALID_ARGUMENT;
        ERROR_MESSAGE("aabb_3d_vertices_get(%s) - Provided aabb_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Output.
    out_vertices_[0] = vec3f_initialize(aabb_->min.elem[0], aabb_->min.elem[1], aabb_->max.elem[2]);    // min_x, min_y, max_z
    out_vertices_[1] = vec3f_initialize(aabb_->max.elem[0], aabb_->min.elem[1], aabb_->max.elem[2]);    // max_x, min_y, max_z
    out_vertices_[2] = vec3f_initialize(aabb_->max.elem[0], aabb_->min.elem[1], aabb_->min.elem[2]);    // max_x, min_y, min_z
    out_vertices_[3] = vec3f_initialize(aabb_->min.elem[0], aabb_->min.elem[1], aabb_->min.elem[2]);    // min_x, min_y, min_z

    out_vertices_[4] = vec3f_initialize(aabb_->min.elem[0], aabb_->max.elem[1], aabb_->max.elem[2]);    // min_x, max_y, max_z
    out_vertices_[5] = vec3f_initialize(aabb_->max.elem[0], aabb_->max.elem[1], aabb_->max.elem[2]);    // max_x, max_y, max_z
    out_vertices_[6] = vec3f_initialize(aabb_->max.elem[0], aabb_->max.elem[1], aabb_->min.elem[2]);    // max_x, max_y, min_z
    out_vertices_[7] = vec3f_initialize(aabb_->min.elem[0], aabb_->max.elem[1], aabb_->min.elem[2]);    // min_x, max_y, min_z

    ret = AABB_3D_SUCCESS;

cleanup:
    return ret;
}

// aabb_3d_is_valid Validation Policy
//
// - 本APIはaabb_3d_tのpublic canonical validatorである。
// - aabb_ == NULLの場合はfalseを返す。
// - Module Internal Contractで定義されたcanonical state全体を検証する。
//
// - explicit validatorであるため、BUILD_MODEによってvalidation semanticsを変更しない。
// - 現在のaabb_3d_tにはshallow validatorを必要とするstructureが存在しないため、
//   Module Internal Contractを直接検証する。
//
// - validation中に対象stateを変更しない。
// - validation failure時はfalseを返すのみとし、error messageは出力しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool aabb_3d_is_valid(const aabb_3d_t* aabb_) {
    if(NULL == aabb_) {
        return false;
    }
    for(uint8_t i = 0; i != 3; ++i) {
        if(aabb_->min.elem[i] > aabb_->max.elem[i]) {
            return false;
        }
    }
    if(!vec3f_is_finite(aabb_->min) || !vec3f_is_finite(aabb_->max)) {
        return false;
    }
    return true;
}

// ============================================================
// Utilities
// ============================================================
static const char* result_to_str(aabb_3d_result_t result_) {
    switch(result_) {
    case AABB_3D_SUCCESS:
        return s_result_str_success;
    case AABB_3D_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case AABB_3D_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}
