// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_FONT_ATLAS_ASSET_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_FONT_ATLAS_ASSET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct font_atlas_asset {
    uint32_t atlas_width;
    uint32_t atlas_height;

    uint32_t cell_width;
    uint32_t cell_height;

    uint32_t atlas_columns;
    uint32_t first_character;
    uint32_t last_character;

    uint32_t advance_x;
    uint32_t line_height;

    const uint8_t* pixels;
} font_atlas_asset_t;

#ifdef __cplusplus
}
#endif
#endif
