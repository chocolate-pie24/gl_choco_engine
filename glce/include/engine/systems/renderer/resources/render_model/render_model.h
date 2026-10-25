// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"

typedef struct render_model_config render_model_config_t;
typedef struct model_transform_tree model_transform_tree_t;
typedef struct render_model render_model_t;

render_model_result_t render_model_create(const render_model_config_t* config_, render_model_t** out_render_model_);

void render_model_destroy(render_model_t** render_model_);

#ifdef __cplusplus
}
#endif
#endif
