// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_CORE_RENDERER_BACKEND_TEXTURE_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_CORE_RENDERER_BACKEND_TEXTURE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

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

/**
 * @brief TextureのGPU上でのデータ表現を定義する
 */
typedef enum texture_format {
    TEXTURE_FORMAT_RGB8,  /**< 各Channelが8bitの正規化値であるRGB形式。Alphaを持たないカラー画像等に使用する。 */
    TEXTURE_FORMAT_RGBA8, /**< 各Channelが8bitの正規化値であるRGBA形式。Alphaを持つカラー画像等に使用する。 */
    TEXTURE_FORMAT_R8UI,  /**< 1 Texelを8bit符号なし整数として扱う形式。Bit Packed Font Atlas等に使用する。 */
} texture_format_t;

typedef struct texture_gpu_config {
    texture_format_t format;
    texture_min_filter_config_t min_filter_config;
    texture_mag_filter_config_t mag_filter_config;
    texture_wrap_config_t wrap_config_s_axis;
    texture_wrap_config_t wrap_config_t_axis;
} texture_gpu_config_t;

bool texture_mag_filter_config_is_valid(texture_mag_filter_config_t config_);

bool texture_min_filter_config_is_valid(texture_min_filter_config_t config_);

bool texture_wrap_config_is_valid(texture_wrap_config_t config_);

bool texture_format_is_valid(texture_format_t format_);

bool texture_gpu_config_is_valid(const texture_gpu_config_t* config_);

#ifdef __cplusplus
}
#endif
#endif
