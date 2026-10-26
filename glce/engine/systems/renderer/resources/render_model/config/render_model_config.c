// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/render_model/config/render_model_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"
#include "engine/systems/renderer/resources/render_model/core/render_model_err_utils.h"
#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"

render_model_result_t render_model_config_coordinate_initialize(uint16_t coordinate_id_, uint16_t parent_coordinate_id_, vec3f_t translate_, vec3f_t rotation_, render_model_coordinate_config_t* out_config_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "render_model_config_coordinate_initialize", "out_config_")
    if(0 == coordinate_id_) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_model_config_coordinate_initialize(%s) - Provided coordinate_id_ is not valid.", render_model_result_to_str(ret));
        goto cleanup;
    }
    if(coordinate_id_ == parent_coordinate_id_) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_model_config_coordinate_initialize(%s) - Provided coordinate_id_ or parent_coordinate_id_ is not valid.", render_model_result_to_str(ret));
        goto cleanup;
    }
    if(360.0f < rotation_.elem[0] || 360.0f < rotation_.elem[1] || 360.0f < rotation_.elem[2]) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_model_config_coordinate_initialize(%s) - Provided rotation_ is not valid.", render_model_result_to_str(ret));
        goto cleanup;
    }
    if(0.0f > rotation_.elem[0] || 0.0f > rotation_.elem[1] || 0.0f > rotation_.elem[2]) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_model_config_coordinate_initialize(%s) - Provided rotation_ is not valid.", render_model_result_to_str(ret));
        goto cleanup;
    }

    out_config_->coordinate_id = coordinate_id_;
    out_config_->parent_coordinate_id = parent_coordinate_id_;
    out_config_->translate = translate_;
    out_config_->rotation = rotation_;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    return ret;
}

void render_model_config_coordinate_deinitialize(render_model_coordinate_config_t* config_) {
    if(NULL == config_) {
        return;
    }
    config_->coordinate_id = 0;
    config_->parent_coordinate_id = 0;
    config_->rotation = vec3f_initialize(0.0f, 0.0f, 0.0f);
    config_->translate = vec3f_initialize(0.0f, 0.0f, 0.0f);
}

void render_model_config_deinitialize(render_model_config_t* config_) {
    if(NULL == config_) {
        return;
    }
    for(size_t i = 0; i != config_->coordinate_count; ++i) {
        render_model_config_coordinate_deinitialize(&config_->coordinates[i]);
    }
    for(size_t i = 0; i != config_->untextured_mesh_count; ++i) {
        untextured_mesh_config_deinitialize(&config_->untextured_mesh_configs[i]);
    }

    if(NULL != config_->coordinates) {
        general_allocator_free((void**)&config_->coordinates, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }

    if(NULL != config_->line_mesh_configs) {
        general_allocator_free((void**)&config_->line_mesh_configs, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != config_->point_mesh_configs) {
        general_allocator_free((void**)&config_->point_mesh_configs, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != config_->ui_mesh_configs) {
        general_allocator_free((void**)&config_->ui_mesh_configs, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != config_->untextured_mesh_configs) {
        general_allocator_free((void**)&config_->untextured_mesh_configs, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }

    if(NULL != config_->line_mesh_coordinate_ids) {
        general_allocator_free((void**)&config_->line_mesh_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != config_->point_mesh_coordinate_ids) {
        general_allocator_free((void**)&config_->point_mesh_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != config_->ui_mesh_coordinate_ids) {
        general_allocator_free((void**)&config_->ui_mesh_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != config_->untextured_mesh_coordinate_ids) {
        general_allocator_free((void**)&config_->untextured_mesh_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }

    config_->coordinate_count = 0;
    config_->line_mesh_count = 0;
    config_->point_mesh_count = 0;
    config_->ui_mesh_count = 0;
    config_->untextured_mesh_count = 0;
}

bool render_model_coordinate_config_is_valid(const render_model_coordinate_config_t* config_) {
    (void)config_;
    return true;
}

bool render_model_config_is_valid(const render_model_config_t* config_) {
    (void)config_;
    return true;
}
