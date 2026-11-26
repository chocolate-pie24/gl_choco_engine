// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/fonts/font_catalog.h"

#include <stdint.h>
#include <stddef.h>

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/fonts/font_atlas_asset.h"

#include "engine/systems/renderer/resources/fonts/generated/spleen_12x24.h"
#include "engine/systems/renderer/resources/fonts/generated/spleen_16x32.h"
#include "engine/systems/renderer/resources/fonts/generated/spleen_32x64.h"
#include "engine/systems/renderer/resources/fonts/generated/spleen_5x8.h"
#include "engine/systems/renderer/resources/fonts/generated/spleen_6x12.h"
#include "engine/systems/renderer/resources/fonts/generated/spleen_8x16.h"

#define MAX_FONT_ASSET_ENTRIES 6

typedef struct font_asset_entry {
    const char* name;
    uint32_t width;
    uint32_t height;
    const font_atlas_asset_t* (*asset_get)(void);
} font_asset_entry_t;

static const font_asset_entry_t s_font_assets[] = {
    { "spleen",  5,  8, spleen_5x8_asset_get },
    { "spleen",  6, 12, spleen_6x12_asset_get },
    { "spleen",  8, 16, spleen_8x16_asset_get },
    { "spleen", 12, 24, spleen_12x24_asset_get },
    { "spleen", 16, 32, spleen_16x32_asset_get },
    { "spleen", 32, 64, spleen_32x64_asset_get },
};

const font_atlas_asset_t* font_asset_catalog_find(const char* font_name_, int32_t width_, int32_t height_) {
    const font_atlas_asset_t* out_asset = NULL;

    for(size_t i = 0; i != MAX_FONT_ASSET_ENTRIES; ++i) {
        if(s_font_assets[i].width == width_ && s_font_assets[i].height == height_) {
            if(choco_string_is_equal(s_font_assets[i].name, font_name_)) {
                out_asset = s_font_assets[i].asset_get();
            }
        }
    }

    return out_asset;
}
