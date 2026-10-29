// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"
#include "engine/containers/ring_queue.h"

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

#include "application/model/core/model_types.h"

const char* model_result_to_str(model_result_t result_);

model_result_t model_result_convert_choco_string(choco_string_result_t result_);

model_result_t model_result_convert_general_allocator(general_allocator_result_t result_);

model_result_t model_result_convert_ring_queue(ring_queue_result_t result_);

model_result_t model_result_convert_renderer_frontend(renderer_frontend_result_t result_);

model_result_t model_result_convert_mesh(mesh_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
