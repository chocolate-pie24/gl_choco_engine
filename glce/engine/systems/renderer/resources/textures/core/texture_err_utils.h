// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_CORE_TEXTURE_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_TEXTURES_CORE_TEXTURE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/textures/core/texture_types.h"

const char* texture_result_to_str(texture_result_t result_);

texture_result_t texture_result_convert_general_allocator(general_allocator_result_t result_);

texture_result_t texture_result_convert_renderer_backend(renderer_backend_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
