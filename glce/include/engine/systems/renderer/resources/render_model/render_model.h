// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_RENDER_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct model_transform_tree model_transform_tree_t;

typedef struct render_model {
    model_transform_tree_t* transform_tree;
} render_model_t;

#ifdef __cplusplus
}
#endif
#endif
