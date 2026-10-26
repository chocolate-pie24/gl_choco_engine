// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/render_model/render_model.h"

#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"
#include "engine/systems/renderer/resources/render_model/core/render_model_err_utils.h"
#include "engine/systems/renderer/resources/render_model/config/render_model_config.h"
#include "engine/systems/renderer/resources/render_model/model_transform_tree.h"

typedef struct render_model {
    size_t line_mesh_count;
    size_t point_mesh_count;
    size_t ui_mesh_count;
    size_t untextured_mesh_count;

    line_mesh_t* line_meshes;
    point_mesh_t* point_meshes;
    ui_mesh_t* ui_meshes;
    untextured_mesh_t* untextured_meshes;

    uint16_t* untextured_mesh_coordinate_ids;

    model_transform_tree_t* transform_tree;
} render_model_t;

static render_model_result_t transform_tree_create_from_config(const render_model_config_t* config_, model_transform_tree_t** out_transform_tree_);

// NOTE: 将来的にはxxx_meshの描画が不要な場合を考慮し、xxx_mesh_count_ == 0 && *xxx_meshes_ == NULLを許容しても良い
// まずはuntextured_meshだけを完成させる
render_model_result_t render_model_create(
    size_t untextured_mesh_count_,
    untextured_mesh_t** untextured_meshes_,
    uint16_t** untextured_mesh_coordinate_ids_,
    model_transform_tree_t** transform_tree_,
    render_model_t** out_model_) {

    (void)untextured_mesh_count_;
    (void)untextured_meshes_;
    (void)untextured_mesh_coordinate_ids_;
    (void)transform_tree_;
    (void)out_model_;

    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

cleanup:
    return ret;
}

void render_model_destroy(render_model_t** render_model_) {
    (void)render_model_;
    return;
}
