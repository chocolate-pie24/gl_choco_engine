// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_MODEL_TRANSFORM_TREE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_RENDER_MODEL_MODEL_TRANSFORM_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"

// TODO: 将来的にはopaque化する
typedef struct model_transform_node {
    struct model_transform_node* parent;
    struct model_transform_node* child;
    struct model_transform_node* next;

    uint16_t coordinate_id; // Coordinate IDはModel内でのみ一意
    vec3f_t translate;
    vec3f_t rotation;
} model_transform_node_t;

typedef struct model_transform_tree {
    model_transform_node_t* root;
} model_transform_tree_t;

render_model_result_t model_transform_tree_initialize(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_);

void model_transform_tree_deinitialize(model_transform_tree_t* transform_tree_);

render_model_result_t model_transform_tree_add_child(model_transform_tree_t* transform_tree_, uint16_t parent_coordinate_id_, uint16_t coordinate_id_, vec3f_t translate_, vec3f_t rotation_);

render_model_result_t model_transform_tree_rotation_set(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, vec3f_t rotation_);

render_model_result_t model_transform_tree_translate_set(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, vec3f_t translate_);

render_model_result_t model_transform_tree_world_matrix_get(const model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, mat4x4f_t* out_matrix_);

#ifdef __cplusplus
}
#endif
#endif
