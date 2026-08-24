// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_CORE_RENDERER_BACKEND_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_CORE_RENDERER_BACKEND_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

const char* renderer_backend_rslt_to_str(renderer_backend_result_t rslt_);

renderer_backend_result_t renderer_backend_rslt_convert_choco_memory(memory_system_result_t rslt_);

renderer_backend_result_t renderer_backend_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
