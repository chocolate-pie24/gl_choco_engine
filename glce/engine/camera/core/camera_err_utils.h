// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_CAMERA_CORE_CAMERA_ERR_UTILS_H
#define GLCE_ENGINE_CAMERA_CORE_CAMERA_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/camera/core/camera_types.h"

#include "engine/core/memory/choco_memory.h"

const char* camera_rslt_to_str(camera_result_t rslt_);

camera_result_t camera_rslt_convert_choco_memory(memory_system_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
