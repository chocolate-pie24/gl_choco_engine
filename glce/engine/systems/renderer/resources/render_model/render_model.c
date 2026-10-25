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

render_model_result_t render_model_create(const render_model_config_t* config_, render_model_t** out_render_model_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    render_model_t* tmp_render_model = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(out_render_model_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_create", "out_render_model_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_render_model_, ret, RENDER_MODEL_BAD_OPERATION, render_model_result_to_str(RENDER_MODEL_BAD_OPERATION), "render_model_create", "*out_render_model_")

    ret = transform_tree_create_from_config(config_, &tmp_transform_tree);
    if(RENDER_MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("render_model_create(%s) - transform_tree_create_from_config failed.", render_model_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(render_model_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_render_model);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = render_model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("render_model_create(%s) - general_allocator_allocate failed.", render_model_result_to_str(ret));
        goto cleanup;
    }

    tmp_render_model->line_mesh_count = config_->line_mesh_count;
    tmp_render_model->point_mesh_count = config_->point_mesh_count;
    tmp_render_model->ui_mesh_count = config_->ui_mesh_count;
    tmp_render_model->untextured_mesh_count = config_->untextured_mesh_count;

cleanup:
    return ret;
}

void render_model_destroy(render_model_t** render_model_) {
}

static render_model_result_t transform_tree_create_from_config(const render_model_config_t* config_, model_transform_tree_t** out_transform_tree_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    ring_queue_t* parent_queue = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;

    size_t queue_capacity = 0;
    size_t added_count = 0;
    uint16_t current_parent = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "transform_tree_create_from_config", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(out_transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "transform_tree_create_from_config", "out_transform_tree_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_transform_tree_, ret, RENDER_MODEL_BAD_OPERATION, render_model_result_to_str(RENDER_MODEL_BAD_OPERATION), "transform_tree_create_from_config", "*out_transform_tree_")
    if(config_->coordinate_count > 0 && NULL == config_->coordinates) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Provided config_ is not valid.", render_model_result_to_str(ret));
        goto cleanup;
    }

    if(!size_add_checked(config_->coordinate_count, 1, &queue_capacity)) {  // RootのID分を追加したQueue容量
        ret = RENDER_MODEL_OVERFLOW;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Queue capacity overflow.", render_model_result_to_str(ret));
        goto cleanup;
    }
    ret_ring_queue = ring_queue_create(queue_capacity, sizeof(uint16_t), alignof(uint16_t), &parent_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = render_model_result_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_create failed.", render_model_result_to_str(ret));
        goto cleanup;
    }

    ret = model_transform_tree_create(0, &tmp_transform_tree);  // Root Coordinate
    if(RENDER_MODEL_SUCCESS != ret) {
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - model_transform_tree_create failed.", render_model_result_to_str(ret));
        goto cleanup;
    }

    // Rootを探索Queueに追加
    current_parent = 0;
    ret_ring_queue = ring_queue_push(&current_parent, sizeof(uint16_t), alignof(uint16_t), parent_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = render_model_result_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_push failed.", render_model_result_to_str(ret));
        goto cleanup;
    }
    while(!ring_queue_is_empty(parent_queue)) {
        ret_ring_queue = ring_queue_pop(sizeof(uint16_t), alignof(uint16_t), parent_queue, &current_parent);
        if(RING_QUEUE_SUCCESS != ret_ring_queue) {
            ret = render_model_result_convert_ring_queue(ret_ring_queue);
            ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_pop failed.", render_model_result_to_str(ret));
            goto cleanup;
        }

        // 現在のParentを持つCoordinateを探索
        for(size_t i = 0; i != config_->coordinate_count; ++i) {
            const render_model_coordinate_config_t* coordinate = &config_->coordinates[i];
            if(current_parent != coordinate->parent_coordinate_id) {
                continue;
            }

            ret = model_transform_tree_add_child(tmp_transform_tree, current_parent, coordinate->coordinate_id, coordinate->translate, coordinate->rotation);
            if(RENDER_MODEL_SUCCESS != ret) {
                ERROR_MESSAGE("transform_tree_create_from_config(%s) - model_transform_tree_add_child failed.", render_model_result_to_str(ret));
                goto cleanup;
            }

            ++added_count;

            // 追加したChildを次のParent候補としてQueueへpush
            ret_ring_queue = ring_queue_push(&coordinate->coordinate_id, sizeof(uint16_t), alignof(uint16_t), parent_queue);
            if(RING_QUEUE_SUCCESS != ret_ring_queue) {
                ret = render_model_result_convert_ring_queue(ret_ring_queue);
                ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_push failed.", render_model_result_to_str(ret));
                goto cleanup;
            }
        }
    }

    // 全CoordinateがRootから到達可能か検査
    if(added_count != config_->coordinate_count) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Unreachable coordinates detected.", render_model_result_to_str(ret));
        goto cleanup;
    }

    // Ownership Commit.
    *out_transform_tree_ = tmp_transform_tree;
    tmp_transform_tree = NULL;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    if(RENDER_MODEL_DATA_CORRUPTED != ret) {
        ring_queue_destroy(&parent_queue);
        model_transform_tree_destroy(&tmp_transform_tree);
    }

    return ret;
}
