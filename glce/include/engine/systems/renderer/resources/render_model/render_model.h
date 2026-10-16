// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/resources/render_model/model_transform_tree.h"

typedef struct render_model {
    model_transform_tree_t* root;
} render_model_t;

#ifdef __cplusplus
}
#endif
#endif
