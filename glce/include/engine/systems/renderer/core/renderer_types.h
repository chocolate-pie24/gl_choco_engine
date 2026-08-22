/** @ingroup renderer
 *
 * @file renderer_types.h
 * @author chocolate-pie24
 * @brief レンダラーレイヤー全体で使用されるデータ型を提供する
 *
 * @version 0.1
 * @date 2025-12-19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_CORE_RENDERER_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_CORE_RENDERER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief GraphicsAPI種別リスト
 *
 */
typedef enum {
    GRAPHICS_API_GL33,  /**< OpenGL 3.3 */
} target_graphics_api_t;

/**
 * @brief 上位層で頂点情報データ種別の識別にグラフィックスAPI固有の型を使用しないで済むよう、頂点情報のデータ種別を定義
 *
 */
typedef enum {
    BUFFER_USAGE_STATIC,    /**< データ種別: 頻繁に書き換わることのないデータ */
    BUFFER_USAGE_DYNAMIC,   /**< データ種別: 頻繁に書き換わるデータ */
} buffer_usage_t;

/**
 * @brief 上位層でグラフィックスAPI固有の変数型を使用しないで済むよう、グラフィックスAPI固有型を定義
 *
 */
typedef enum {
    RENDERER_TYPE_FLOAT,            /**< データ型: GLfloat */
    RENDERER_TYPE_UNSIGNED_BYTE,    /**< データ型: GL_UNSIGNED_BYTE */
    RENDERER_TYPE_BYTE,             /**< データ型: GL_BYTE */
} renderer_type_t;

typedef enum {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
} shader_type_t;

/**
 * @brief 画像を拡大表示した際の表示設定
 *
 */
typedef enum {
    TEXTURE_MAG_FILTER_CONFIG_NEAREST,      /**< GL_NEAREST相当: 一番近いtexelをそのまま使う。ドットがくっきり見える */
    TEXTURE_MAG_FILTER_CONFIG_LINEAR,       /**< GL_LINEAR相当: 近くのtexelを線形補間する。なめらかになる */
} texture_mag_filter_config_t;

/**
 * @brief 画像を縮小表示した際の表示設定
 * @note mipmapは当面は使用しない
 *
 */
typedef enum {
    TEXTURE_MIN_FILTER_CONFIG_NEAREST,                 /**< GL_NEAREST相当: mipmapなし。近いtexelを1個選ぶ */
    TEXTURE_MIN_FILTER_CONFIG_LINEAR,                  /**< GL_LINEAR相当: mipmapなし。近いtexel群を補間する */
    // TEXTURE_MIN_FILTER_CONFIG_NEAREST_MIPMAP_NEAREST,  /**< GL_NEAREST_MIPMAP_NEAREST相当: mipmapあり。選ばれたmipレベル内ではnearest */
    // TEXTURE_MIN_FILTER_CONFIG_LINEAR_MIPMAP_NEAREST,   /**< GL_LINEAR_MIPMAP_NEAREST相当: mipmapあり。選ばれたmipレベル内では linear */
    // TEXTURE_MIN_FILTER_CONFIG_NEAREST_MIPMAP_LINEAR,   /**< GL_NEAREST_MIPMAP_LINEAR相当: 2つのmipレベルをまたいで補間するが、各mip内ではnearest */
    // TEXTURE_MIN_FILTER_CONFIG_LINEAR_MIPMAP_LINEAR,    /**< GL_LINEAR_MIPMAP_LINEAR相当: 2つのmipレベルをまたいで補間し、各mip内でも linear */
} texture_min_filter_config_t;

/**
 * @brief uv座標が0...1の外に出た時の表示設定
 *
 */
typedef enum {
    TEXTURE_WRAP_CONFIG_REPEAT,             /**< GL_REPEAT相当: 小数部分だけを使って繰り返す。タイル状に敷き詰めたいときに使用 */
    TEXTURE_WRAP_CONFIG_MIRRORED_REPEAT,    /**< GL_MIRRORED_REPEAT相当: 繰り返しつつ、1枚おきに反転する */
    TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE,      /**< GL_CLAMP_TO_EDGE相当: 端のtexelを引き延ばすように扱う。境界のにじみを避けたいときによく使用する */
    TEXTURE_WRAP_CONFIG_CLAMP_TO_BORDER,    /**< GL_CLAMP_TO_BORDER相当: 範囲外をborder colorで読む方式 */
} texture_wrap_config_t;

bool buffer_usage_is_valid(buffer_usage_t usage_);

#ifdef __cplusplus
}
#endif
#endif
