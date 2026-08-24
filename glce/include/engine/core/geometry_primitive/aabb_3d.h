// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup core
 *
 * @file aabb_3d.h
 * @author chocolate-pie24
 * @brief 3次元AABB幾何情報構造体, API定義
 *
 * @date 2026-06-09
 *
 */
#ifndef GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_AABB_3D_H
#define GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_AABB_3D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/core/geometry_primitive/geometry_primitive_types.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/base/choco_math/math_types.h"

/**
 * @brief AABB内部データ格納構造体
 *
 */
typedef struct aabb_3d {
    vec3f_t min;    /**< AABBを構成する8頂点のうち、x, y, zが全て最小の点の座標 */
    vec3f_t max;    /**< AABBを構成する8頂点のうち、x, y, zが全て最大の点の座標 */
} aabb_3d_t;

/**
 * @brief AABBの最小点・最大点を使ってout_aabb_を初期化する
 *
 * @param[in] min_ AABBの最小点
 * @param[in] max_ AABBの最大点
 * @param[out] out_aabb_ 初期化対象aabb_3d_t構造体インスタンスへのポインタ
 *
 * @retval GEOMETRY_PRIMITIVE_INVALID_ARGUMENT 以下のいずれか
 * - out_aabb_ == NULL
 * @retval GEOMETRY_PRIMITIVE_DATA_CORRUPTED 以下のいずれか
 * - min_, max_の関係がmin_ > max_になっている
 * - min_もしくはmax_にNaN or Infが含まれる
 * @retval GEOMETRY_PRIMITIVE_SUCCESS 処理に成功し、正常終了
 */
geometry_primitive_result_t aabb_3d_initialize_from_min_max(vec3f_t min_, vec3f_t max_, aabb_3d_t* out_aabb_);

/**
 * @brief ポイント用頂点配列を使用してout_aabb_を初期化する
 *
 * @param[in] vertices_ ポイント用頂点配列
 * @param[in] vertex_count_ ポイント頂点数
 * @param[out] out_aabb_ 初期化対象aabb_3d_t構造体インスタンスへのポインタ
 *
 * @retval GEOMETRY_PRIMITIVE_INVALID_ARGUMENT 以下のいずれか
 * - vertices_ == NULL
 * - out_aabb_ == NULL
 * - vertex_count_ == 0
 * @retval GEOMETRY_PRIMITIVE_DATA_CORRUPTED 与えられた頂点配列にNaN or Infが含まれる
 * @retval GEOMETRY_PRIMITIVE_RUNTIME_ERROR 計算の結果、AABB形状不整合が発生(通常は起こり得ない)
 * @retval GEOMETRY_PRIMITIVE_SUCCESS 処理に成功し、正常終了
 */
geometry_primitive_result_t aabb_3d_initialize_from_point_vertices(const point_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_);

/**
 * @brief 線分用頂点配列を使用してout_aabb_を初期化する
 *
 * @param[in] vertices_ 線分用頂点配列
 * @param[in] vertex_count_ 線分用頂点配列に含まれる頂点数(線分の数 x 2)
 * @param[out] out_aabb_ 初期化対象aabb_3d_t構造体インスタンスへのポインタ
 *
 * @retval GEOMETRY_PRIMITIVE_INVALID_ARGUMENT 以下のいずれか
 * - vertices_ == NULL
 * - out_aabb_ == NULL
 * - vertex_count_ == 0
 * - vertex_count_が2の倍数ではない
 * @retval GEOMETRY_PRIMITIVE_DATA_CORRUPTED 与えられた頂点配列にNaN or Infが含まれる
 * @retval GEOMETRY_PRIMITIVE_RUNTIME_ERROR 計算の結果、AABB形状不整合が発生(通常は起こり得ない)
 * @retval GEOMETRY_PRIMITIVE_SUCCESS 処理に成功し、正常終了
 */
geometry_primitive_result_t aabb_3d_initialize_from_line_vertices(const line_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_);

/**
 * @brief 頂点座標と法線を持つ頂点情報配列を用いてout_aabb_を初期化する
 *
 * @param[in] vertices_ 頂点座標と法線を持つ頂点情報配列
 * @param[in] vertex_count_ 頂点数(三角形で描画するので必ず3の倍数)
 * @param[out] out_aabb_ 初期化対象aabb_3d_t構造体インスタンスへのポインタ
 *
 * @retval GEOMETRY_PRIMITIVE_INVALID_ARGUMENT 以下のいずれか
 * - vertices_ == NULL
 * - out_aabb_ == NULL
 * - vertex_count_ == 0
 * - vertex_count_が3の倍数ではない
 * @retval GEOMETRY_PRIMITIVE_DATA_CORRUPTED 与えられた頂点配列にNaN or Infが含まれる
 * @retval GEOMETRY_PRIMITIVE_RUNTIME_ERROR 計算の結果、AABB形状不整合が発生(通常は起こり得ない)
 * @retval GEOMETRY_PRIMITIVE_SUCCESS 処理に成功し、正常終了
 */
geometry_primitive_result_t aabb_3d_initialize_from_point_normal_vertices(const point_normal_vertex_t* vertices_, size_t vertex_count_, aabb_3d_t* out_aabb_);

/**
 * @brief aabb_のmin, maxを全て0で初期化する
 *
 * @note min, maxが全て0の状態のaabb_は退化したAABBであり、有効な状態とするため、aabb_3d_is_valid()はtrueとなる
 * @note aabb_ == NULLの場合は何もしない
 *
 * @param[out] aabb_ 初期化対象aabb_3d_t構造体インスタンスへのポインタ
 */
void aabb_3d_reset(aabb_3d_t* aabb_);

/**
 * @brief aabb_を構成する8頂点の座標を取得し、vertices_に格納する
 *
 * @note vertices_には以下の順で格納する
 * vertices_格納順(x軸: 画面右方向+, y軸: 画面上方向+, z軸: 画面奥行方向-)
 * - 底面
 * - vertices_[0]: min_x, min_y, max_z
 * - vertices_[1]: max_x, min_y, max_z
 * - vertices_[2]: max_x, min_y, min_z
 * - vertices_[3]: min_x, min_y, min_z
 * - 上面
 * - vertices_[4]: min_x, max_y, max_z
 * - vertices_[5]: max_x, max_y, max_z
 * - vertices_[6]: max_x, max_y, min_z
 * - vertices_[7]: min_x, max_y, min_z
 *
 * @param[in] aabb_ 座標を取得するaabb_3d_t構造体インスタンスへのポインタ
 * @param[out] vertices_ 頂点座標格納先
 *
 * @retval GEOMETRY_PRIMITIVE_INVALID_ARGUMENT 以下のいずれか
 * - aabb_ == NULL
 * - vertices_ == NULL
 * @retval GEOMETRY_PRIMITIVE_BAD_OPERATION 与えられたaabb_の内部データが不整合
 * @retval GEOMETRY_PRIMITIVE_SUCCESS 処理に成功し、正常終了
 */
geometry_primitive_result_t aabb_3d_vertices_get(const aabb_3d_t* aabb_, vec3f_t vertices_[8]);

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
