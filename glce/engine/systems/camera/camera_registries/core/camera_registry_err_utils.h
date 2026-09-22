// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_CAMERA_CAMERA_REGISTRIES_CORE_CAMERA_REGISTRY_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_CAMERA_CAMERA_REGISTRIES_CORE_CAMERA_REGISTRY_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"

const char* camera_registry_result_to_str(camera_registry_result_t result_);

camera_registry_result_t camera_registry_result_convert_linear_allocator(linear_allocator_result_t result_);

camera_registry_result_t camera_registry_result_convert_choco_string(choco_string_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
