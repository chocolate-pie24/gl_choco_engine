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

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"

const char* render_model_result_to_str(render_model_result_t result_);

render_model_result_t render_model_result_convert_choco_string(choco_string_result_t result_);

render_model_result_t render_model_result_convert_general_allocator(general_allocator_result_t result_);

render_model_result_t render_model_result_convert_ring_queue(ring_queue_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
