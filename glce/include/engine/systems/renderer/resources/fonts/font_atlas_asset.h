// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_FONT_ATLAS_ASSET_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_FONT_ATLAS_ASSET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/fonts/core/font_types.h"

typedef struct font_atlas_asset {
    int32_t atlas_width;
    int32_t atlas_height;

    int32_t cell_width;
    int32_t cell_height;

    uint32_t atlas_columns;
    uint32_t first_character;
    uint32_t last_character;

    int32_t advance_x;
    int32_t line_height;

    const uint8_t* pixels;
} font_atlas_asset_t;

// Font Atlasは8 Pixel分の情報を1 Byteに格納している。
// 例えば、80 x 48 PixelのFont Atlasは、
// 1行あたり10 Byte、48行のPixel Dataとして保持される。
// 本APIは、Font Atlasの論理的なWidth、Heightから、
// このPacked Dataの1行あたりのByte数と行数を取得する。
font_result_t font_atlas_asset_packed_dimensions_get(const font_atlas_asset_t* asset_, int32_t* out_width_, int32_t* out_height_);

#ifdef __cplusplus
}
#endif
#endif
