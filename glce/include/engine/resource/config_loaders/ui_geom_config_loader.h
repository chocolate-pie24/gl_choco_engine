// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file ui_geom_config_loader.h
 * @author chocolate-pie24
 * @brief UI描画用ジオメトリコンフィグレーションファイルのローダーAPI定義
 *
 * @details コンフィグレーションファイルフォーマット
 * - ファイル格納場所: assets/configs/
 * - ファイル拡張子: .ui_geom
 *
 * @code{.unparsed}
 * コンフィグレーションファイルには矩形領域の幅と高さの設定が必須で、以下のように記載する
 *
 * icon_width = 1
 * icon_height = 1
 * @endcode
 *
 * @date 2026-06-30
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_CONFIG_LOADERS_UI_GEOM_CONFIG_LOADER_H
#define GLCE_ENGINE_RESOURCE_CONFIG_LOADERS_UI_GEOM_CONFIG_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/resource/core/resource_types.h"

/**
 * @brief UI描画用ジオメトリ設定値格納構造体
 *
 */
typedef struct ui_geom_config {
    uint16_t icon_width;    /**< 矩形領域形状指定(幅) */
    uint16_t icon_height;   /**< 矩形領域形状指定(高さ) */
} ui_geom_config_t;

resource_result_t ui_geom_config_loader_load(const char* config_fullpath_, ui_geom_config_t* out_config_);

#ifdef __cplusplus
}
#endif
#endif
