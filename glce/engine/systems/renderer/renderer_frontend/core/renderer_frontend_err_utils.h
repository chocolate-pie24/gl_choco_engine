// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_FRONTEND_CORE_RENDERER_FRONTEND_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_FRONTEND_CORE_RENDERER_FRONTEND_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"

const char* renderer_frontend_result_to_str(renderer_frontend_result_t result_);

renderer_frontend_result_t renderer_frontend_result_convert_subsystem_allocator(subsystem_allocator_result_t result_);
renderer_frontend_result_t renderer_frontend_result_convert_renderer_backend(renderer_backend_result_t result_);
renderer_frontend_result_t render_resource_result_convert_resource_registry(buffer_manager_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
