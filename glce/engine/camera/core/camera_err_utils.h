// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_CAMERA_CORE_CAMERA_ERR_UTILS_H
#define GLCE_ENGINE_CAMERA_CORE_CAMERA_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/camera/core/camera_types.h"

#include "engine/memory/general_allocator/general_allocator.h"

const char* camera_result_to_str(camera_result_t result_);

camera_result_t camera_result_convert_general_allocator(general_allocator_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
