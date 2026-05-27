/** @ingroup core
 *
 * @file vertex.h
 * @author chocolate-pie24
 * @brief 形状データを構成する基本的な幾何情報構造体定義
 *
 * @version 0.1
 * @date 2026-05-14
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
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

#ifdef __cplusplus
}
#endif
#endif
