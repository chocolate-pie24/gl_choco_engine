// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_CORE_FONT_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_FONTS_CORE_FONT_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/resources/textures/core/texture_types.h"

#include "engine/systems/renderer/resources/fonts/core/font_types.h"

const char* font_result_to_str(font_result_t result_);

font_result_t font_result_convert_texture(texture_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
