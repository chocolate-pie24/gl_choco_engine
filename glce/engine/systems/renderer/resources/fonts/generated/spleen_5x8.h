// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file spleen_5x8.h
 * @brief 固定幅Font Atlasの生成済みアセット宣言。
 * @details tools/bdf_font_atlas_generator/generate.py により自動生成されたファイル。
 *          手動で編集しないこと。Atlasの寸法: 80 x 48。
 */

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_GENERATED_SPLEEN_5X8_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_GENERATED_SPLEEN_5X8_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/resources/fonts/font_atlas_asset.h"

/**
 * @brief コンパイル済みのFont Atlas Assetを取得する。
 * @details Metadataと1bit Pixel Dataは生成された.cが所有する静的領域に格納される。
 *          呼び出し側は返却ポインタをBorrowし、解放しない。
 * @return 読み取り専用Font Atlas Assetへの非NULLポインタ。
 * @note 返却先はプログラムの実行期間を通じて有効。
 *       Pixel DataはMSB-first、row-major、1bit/pixel。
 *       各行はbyte境界に揃え、Atlasの原点は左上とする。
 */
const font_atlas_asset_t* spleen_5x8_asset_get(void);

#ifdef __cplusplus
}
#endif

#endif // GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_GENERATED_SPLEEN_5X8_H
