// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/render_model/render_model.h"

#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

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
} render_model_t;

// NOTE: 将来的にはxxx_meshの描画が不要な場合を考慮し、xxx_mesh_count_ == 0 && *xxx_meshes_ == NULLを許容しても良い
// まずはuntextured_meshだけを完成させる
render_model_result_t render_model_create(size_t untextured_mesh_count_, untextured_mesh_t** untextured_meshes_, uint16_t** untextured_mesh_coordinate_ids_, render_model_t** out_model_) {

    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    render_model_t* tmp_model = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(untextured_meshes_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_create", "untextured_meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(*untextured_meshes_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_create", "*untextured_meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(untextured_mesh_coordinate_ids_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_create", "untextured_mesh_coordinate_ids_")
    IF_ARG_NULL_GOTO_CLEANUP(*untextured_mesh_coordinate_ids_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_create", "*untextured_mesh_coordinate_ids_")
    IF_ARG_NULL_GOTO_CLEANUP(out_model_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_create", "out_model_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_model_, ret, RENDER_MODEL_BAD_OPERATION, render_model_result_to_str(RENDER_MODEL_BAD_OPERATION), "render_model_create", "*out_model_")

    ret_general_allocator = general_allocator_allocate(sizeof(render_model_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_model);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = render_model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("render_model_create(%s) - general_allocator_allocate failed.", render_model_result_to_str(ret));
        goto cleanup;
    }

    tmp_model->line_mesh_count = 0;
    tmp_model->line_meshes = NULL;
    tmp_model->point_mesh_count = 0;
    tmp_model->point_meshes = NULL;
    tmp_model->ui_mesh_count = 0;
    tmp_model->ui_meshes = NULL;
    tmp_model->untextured_mesh_coordinate_ids = *untextured_mesh_coordinate_ids_;
    tmp_model->untextured_mesh_count = untextured_mesh_count_;
    tmp_model->untextured_meshes = *untextured_meshes_;
    *out_model_ = tmp_model;
    tmp_model = NULL;
    *untextured_meshes_ = NULL;
    *untextured_mesh_coordinate_ids_ = NULL;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    return ret;
}

void render_model_destroy(render_model_t** render_model_) {
    if(NULL == render_model_ || NULL == *render_model_) {
        return;
    }
    general_allocator_free((void**)&(*render_model_)->untextured_mesh_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    general_allocator_free((void**)&(*render_model_)->untextured_meshes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    general_allocator_free((void**)render_model_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

render_model_result_t render_model_untextured_mesh_mesh_get(const render_model_t* render_model_, uint16_t untextured_mesh_index_, untextured_mesh_t* out_mesh_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_model_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_untextured_mesh_mesh_get", "render_model_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_untextured_mesh_mesh_get", "out_mesh_")
    if(untextured_mesh_index_ >= render_model_->untextured_mesh_count) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_model_untextured_mesh_mesh_get(%s) - Provided untextured_mesh_index_ is not valid.", render_model_result_to_str(ret));
        goto cleanup;
    }

    out_mesh_->geometry_id = render_model_->untextured_meshes[untextured_mesh_index_].geometry_id;
    out_mesh_->material_id = render_model_->untextured_meshes[untextured_mesh_index_].material_id;

    return RENDER_MODEL_SUCCESS;

cleanup:
    return ret;
}

render_model_result_t render_model_untextured_mesh_coordinate_id_get(const render_model_t* render_model_, uint16_t untextured_mesh_index_, uint16_t* out_coordinate_id_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_model_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_untextured_mesh_coordinate_id_get", "render_model_")
    IF_ARG_NULL_GOTO_CLEANUP(out_coordinate_id_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_untextured_mesh_coordinate_id_get", "out_coordinate_id_")
    if(untextured_mesh_index_ >= render_model_->untextured_mesh_count) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_model_untextured_mesh_mesh_get(%s) - Provided untextured_mesh_index_ is not valid.", render_model_result_to_str(ret));
        goto cleanup;
    }

    *out_coordinate_id_ = render_model_->untextured_mesh_coordinate_ids[untextured_mesh_index_];

    return RENDER_MODEL_SUCCESS;

cleanup:
    return ret;
}

render_model_result_t render_model_untextured_mesh_count_get(const render_model_t* render_model_, size_t* out_count_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_model_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_untextured_mesh_count_get", "render_model_")
    IF_ARG_NULL_GOTO_CLEANUP(out_count_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_untextured_mesh_count_get", "out_count_")

    *out_count_ = render_model_->untextured_mesh_count;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    return ret;
}
