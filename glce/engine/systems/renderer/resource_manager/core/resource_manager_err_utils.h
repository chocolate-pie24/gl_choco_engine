// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_MANAGER_CORE_RESOURCE_MANAGER_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_MANAGER_CORE_RESOURCE_MANAGER_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/resource_manager/core/resource_manager_types.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

const char* resource_manager_result_to_str(resource_manager_result_t result_);

resource_manager_result_t resource_manager_result_convert_subsystem_allocator(subsystem_allocator_result_t result_);
resource_manager_result_t resource_manager_result_convert_renderer_backend(renderer_backend_result_t result_);
resource_manager_result_t resource_manager_result_convert_resource_registry(resource_registry_result_t result_);
resource_manager_result_t resource_manager_result_convert_resource_pipeline(resource_pipeline_result_t result_);
resource_manager_result_t resource_manager_result_convert_buffer_manager(buffer_manager_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
