// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_FONT_CATALOG_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_FONT_CATALOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/fonts/font_atlas_asset.h"

const font_atlas_asset_t* font_asset_catalog_find(const char* font_name_, uint32_t width_, uint32_t height_);

#ifdef __cplusplus
}
#endif

#endif
