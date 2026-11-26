// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup renderer
 *
 * @file renderer_types.h
 * @author chocolate-pie24
 * @brief レンダラーレイヤー全体で使用されるデータ型を提供する
 *
 * @date 2025-12-19
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_CORE_RENDERER_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_CORE_RENDERER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 上位層で頂点情報データ種別の識別にグラフィックスAPI固有の型を使用しないで済むよう、頂点情報のデータ種別を定義
 *
 */
typedef enum {
    BUFFER_USAGE_STATIC,    /**< データ種別: 頻繁に書き換わることのないデータ */
    BUFFER_USAGE_DYNAMIC,   /**< データ種別: 頻繁に書き換わるデータ */
} buffer_usage_t;

typedef struct draw_range {
    size_t first_vertex_count;
    size_t vertex_count;
} draw_range_t;

typedef enum {
    RENDERING_PRIMITIVE_LINES,
    RENDERING_PRIMITIVE_TRIANGLES,
    RENDERING_PRIMITIVE_POINTS,
} rendering_primitive_t;

typedef enum {
  RENDERER_CLEAR_COLOR_BIT   = 1 << 0, // 0x01
  RENDERER_CLEAR_DEPTH_BIT   = 1 << 1, // 0x02
  RENDERER_CLEAR_STENCIL_BIT = 1 << 2, // 0x04
} renderer_clear_mask_t;

bool buffer_usage_is_valid(buffer_usage_t usage_);

bool draw_range_is_valid(const draw_range_t* draw_range_);

#ifdef __cplusplus
}
#endif
#endif
