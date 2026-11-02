// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_CORE_MODEL_FRAMEWORK_ERR_UTILS_H
#define GLCE_APPLICATION_MODEL_FRAMEWORK_ASSEMBLY_CORE_MODEL_FRAMEWORK_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"
#include "engine/containers/ring_queue.h"

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

#include "application/model_framework/assembly/core/model_framework_types.h"

const char* model_framework_result_to_str(model_framework_result_t result_);

model_framework_result_t model_framework_result_convert_choco_string(choco_string_result_t result_);

model_framework_result_t model_framework_result_convert_general_allocator(general_allocator_result_t result_);

model_framework_result_t model_framework_result_convert_ring_queue(ring_queue_result_t result_);

model_framework_result_t model_framework_result_convert_renderer_frontend(renderer_frontend_result_t result_);

model_framework_result_t model_framework_result_convert_mesh(mesh_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
