// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup core
 *
 * @file vertex.h
 * @author chocolate-pie24
 * @brief 形状データを構成する基本的な幾何情報構造体定義
 *
 * @date 2026-05-14
 *
 */
#ifndef GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_VERTEX_H
#define GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_VERTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/base/choco_math/math_types.h"

/**
 * @brief ui geometry用頂点情報構造体
 *
 */
typedef struct ui_vertex {
    vec2f_t position;   /**< 頂点座標 */
    vec2f_t tex_coord;  /**< テクスチャUV座標 */
} ui_vertex_t;

/**
 * @brief 線分描画用頂点情報構造体
 *
 * @note 線分のグラデーション表示は行わないので、色情報は親構造体に持たせる
 */
typedef struct line_vertex {
    vec3f_t position;   /**< 線分の頂点座標 */
} line_vertex_t;

/**
 * @brief ポイント描画用頂点情報構造体
 *
 * @note 当初は、colorをMaterial情報として扱い、座標と色をそれぞれ別のVBOで管理する予定だった。
 * しかし、座標と色を別VBOに格納する場合でも、両者は同一の論理頂点インデックスに配置する必要がある。
 *
 * そのためcolorをMaterial Subsystemで管理すると、
 * Geometry Pipelineで確保した頂点範囲をMaterial Pipelineへ引き渡し、同じ頂点範囲へ色情報を書き込まなければならない。
 *
 * この依存関係はpoint meshに固有であり、ほかのmeshではGeometryとMaterialを独立して管理できる。
 * mesh種別間でGeometryとMaterialの責務境界を統一し、両Pipelineを不要に依存させないため、pointごとの色情報はpoint_vertex_tに含める。
 *
 */
typedef struct point_vertex {
    vec3f_t position;   /**< 頂点座標 */
    vec4u8_t color;     /**< 色情報 */
} point_vertex_t;

/**
 * @brief 頂点座標と法線を持つ頂点情報構造体
 *
 */
typedef struct point_normal_vertex {
    vec3f_t position;   /**< 頂点座標 */
    vec4i8_t normal;    /**< [Nx, Ny, Nz] + padding(4byte境界に揃えるため) */
} point_normal_vertex_t;

#ifdef __cplusplus
}
#endif
#endif
