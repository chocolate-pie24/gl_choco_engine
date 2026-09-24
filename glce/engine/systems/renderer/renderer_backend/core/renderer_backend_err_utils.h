// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_CORE_RENDERER_BACKEND_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_CORE_RENDERER_BACKEND_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

const char* renderer_backend_result_to_str(renderer_backend_result_t result_);

renderer_backend_result_t renderer_backend_result_convert_linear_allocator(linear_allocator_result_t result_);

renderer_backend_result_t renderer_backend_result_convert_general_allocator(general_allocator_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
