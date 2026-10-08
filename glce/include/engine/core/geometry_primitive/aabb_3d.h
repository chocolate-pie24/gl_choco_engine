// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup core
 *
 * @file aabb_3d.h
 * @author chocolate-pie24
 * @brief 3次元Axis-Aligned Bounding Boxを表す値型と、その基本操作を提供する
 *
 * @details
 * AABB 3D moduleは、3次元空間上のAxis-Aligned Bounding Box（AABB）を
 * `aabb_3d_t`として表現し、AABBの生成、変換および参照に関する機能を提供する。
 *
 * `aabb_3d_t`はresource ownershipや独立したlifecycleを持たない値型であり、
 * caller側でstorageを保持して使用する。
 *
 * @section aabb_3d_boundary_contract Module Boundary Contract
 *
 * - `aabb_3d_t`はpublic complete typeとして公開し、caller側でstorageを保持する。
 * - AABB 3D moduleは`aabb_3d_t`自身のstorageを確保または解放しない。
 * - `aabb_3d_t`は外部resourceを所有せず、resourceの取得または解放を行わない。
 * - AABBの生成元として渡されたvertex arrayその他のsource objectを所有しない。
 * - `aabb_3d_t`は生成元となったsource objectへのpointerまたはreferenceを内部に保持しない。
 * - AABBを生成するために受け取るsource valueおよびsource objectは、
 *   そのsource typeが定義するcontractを満たしたtrusted representationとして扱う。
 * - AABB生成に使用するfloating-point coordinateは、
 *   finiteなtrusted representationとして扱う。
 * - AABB 3D moduleは、AABB生成のsource valueまたはsource objectを受け取ったことだけを理由として、
 *   source type自身が所有するsemantic validityを再認証しない。
 */
#ifndef GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_AABB_3D_H
#define GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_AABB_3D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/base/choco_math/math_types.h"

typedef enum {
    AABB_3D_SUCCESS = 0,         /**< 実行結果正常 */
    AABB_3D_INVALID_ARGUMENT,    /**< 実行結果: 引数異常 */
    AABB_3D_UNDEFINED_ERROR,     /**< 実行結果: 不明なエラー */
} aabb_3d_result_t;


/**
 * @brief AABB内部データ格納構造体
 *
 */
typedef struct aabb_3d {
    vec3f_t min;    /**< AABBを構成する8頂点のうち、x, y, zが全て最小の点の座標 */
    vec3f_t max;    /**< AABBを構成する8頂点のうち、x, y, zが全て最大の点の座標 */
} aabb_3d_t;

aabb_3d_result_t aabb_3d_initialize_from_min_max(vec3f_t min_, vec3f_t max_, aabb_3d_t* out_aabb_);

aabb_3d_result_t aabb_3d_initialize_from_point_vertices(const point_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_);

aabb_3d_result_t aabb_3d_initialize_from_line_vertices(const line_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_);

aabb_3d_result_t aabb_3d_initialize_from_point_normal_vertices(const point_normal_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_);

/**
 * @brief aabb_のmin, maxを全て0で初期化する
 *
 * @note min, maxが全て0の状態のaabb_は退化したAABBであり、有効な状態とするため、aabb_3d_is_valid()はtrueとなる
 * @note aabb_ == NULLの場合は何もしない
 *
 * @param[out] aabb_ 初期化対象aabb_3d_t構造体インスタンスへのポインタ
 */
void aabb_3d_reset(aabb_3d_t* aabb_);

aabb_3d_result_t aabb_3d_vertices_get(const aabb_3d_t* aabb_, vec3f_t vertices_[8]);

/**
 * @brief aabb_の内部データの整合性を検証する
 *
 * @note 以下の場合は不整合とする
 * - min, maxの値にNaN, Infが含まれる
 * - x, y, z のいずれかの成分でmin_がmax_を上回る
 * @note 厚み、体積を持たない退化したaabb_3d_tは有効(整合性が取れている)とする
 *
 * @param[in] aabb_ 判定対象aabb_3d_t構造体インスタンスへのポインタ
 *
 * @return true 内部データ有効
 * @return false 内部データ不整合もしくはaabb_ == NULL
 */
bool aabb_3d_is_valid(const aabb_3d_t* aabb_);

#ifdef __cplusplus
}
#endif
#endif
