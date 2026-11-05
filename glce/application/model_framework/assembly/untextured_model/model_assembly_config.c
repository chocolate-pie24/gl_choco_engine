// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/model_framework/assembly/untextured_model/model_assembly_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "application/model_framework/core/model_framework_types.h"
#include "application/model_framework/core/model_framework_err_utils.h"
#include "application/model_framework/coordinates/model_coordinate_config.h"

model_framework_result_t untextured_model_assembly_config_create(
    size_t coordinate_count_,
    model_coordinate_config_t** coordinate_configs_,
    size_t mesh_count_,
    uint16_t** mesh_coordinate_ids_,
    untextured_mesh_config_t** mesh_configs_,
    untextured_model_assembly_config_t** out_config_) {

    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    untextured_model_assembly_config_t* tmp_config = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(coordinate_configs_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_assembly_config_create", "coordinate_configs_")
    IF_ARG_NULL_GOTO_CLEANUP(*coordinate_configs_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_assembly_config_create", "*coordinate_configs_")
    IF_ARG_NULL_GOTO_CLEANUP(mesh_coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_assembly_config_create", "mesh_coordinate_ids_")
    IF_ARG_NULL_GOTO_CLEANUP(*mesh_coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_assembly_config_create", "*mesh_coordinate_ids_")
    IF_ARG_NULL_GOTO_CLEANUP(mesh_configs_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_assembly_config_create", "mesh_configs_")
    IF_ARG_NULL_GOTO_CLEANUP(*mesh_configs_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_assembly_config_create", "*mesh_configs_")
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_assembly_config_create", "out_config_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_config_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "untextured_model_assembly_config_create", "*out_config_")
    if(0 == coordinate_count_) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_model_assembly_config_create(%s) - Provided coordinate_count_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    if(0 == mesh_count_) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_model_assembly_config_create(%s) - Provided mesh_count_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(untextured_model_assembly_config_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_config);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_framework_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("untextured_model_assembly_config_create(%s) - general_allocator_allocate failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    tmp_config->coordinate_count = coordinate_count_;
    tmp_config->coordinate_configs = *coordinate_configs_;

    tmp_config->mesh_count = mesh_count_;
    tmp_config->mesh_coordinate_ids = *mesh_coordinate_ids_;
    tmp_config->mesh_configs = *mesh_configs_;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_model_assembly_config_is_valid(tmp_config)) {
        ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_model_assembly_config_create(%s) - Result validation failed for 'tmp_config'.", model_framework_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_config_ = tmp_config;
    tmp_config = NULL;

    *mesh_coordinate_ids_ = NULL;
    *mesh_configs_ = NULL;
    *coordinate_configs_ = NULL;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

void untextured_model_assembly_config_destroy(untextured_model_assembly_config_t** config_) {
    if(NULL == config_) {
        return;
    }
    if(!untextured_model_assembly_config_is_valid(*config_)) {
        ERROR_MESSAGE("untextured_model_assembly_config_destroy(%s) - Provided config_ is corrupted.", model_framework_result_to_str(MODEL_FRAMEWORK_DATA_CORRUPTED));
        return;
    }
    for(size_t i = 0; i != (*config_)->coordinate_count; ++i) {
        model_coordinate_config_deinitialize(&(*config_)->coordinate_configs[i]);
    }

    for(size_t i = 0; i != (*config_)->mesh_count; ++i) {
        untextured_mesh_config_deinitialize(&(*config_)->mesh_configs[i]);
    }

    if(NULL != (*config_)->coordinate_configs) {
        general_allocator_free((void**)&(*config_)->coordinate_configs, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }

    if(NULL != (*config_)->mesh_configs) {
        general_allocator_free((void**)&(*config_)->mesh_configs, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != (*config_)->mesh_coordinate_ids) {
        general_allocator_free((void**)&(*config_)->mesh_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }

    general_allocator_free((void**)config_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

bool untextured_model_assembly_config_is_valid(const untextured_model_assembly_config_t* config_) {
    if(NULL == config_) {
        return false;
    }

    bool found_parent = false;

    // coordinate
    if(!general_allocator_ptr_is_allocated(config_->coordinate_configs)) {
        return false;
    }
    if(0 == config_->coordinate_count) {
        return false;
    }
    if(NULL == config_->coordinate_configs) {
        return false;
    }
    for(size_t i = 0; i != config_->coordinate_count; ++i) {
        if(!model_coordinate_config_is_valid(&config_->coordinate_configs[i])) {
            return false;
        }
    }
    for(size_t i = 0; i != config_->coordinate_count; ++i) {
        found_parent = false;
        for(size_t j = i + 1; j != config_->coordinate_count; ++j) {
            if(config_->coordinate_configs[i].coordinate_id == config_->coordinate_configs[j].coordinate_id) {
                return false;
            }
        }
    }
    for(size_t i = 0; i < config_->coordinate_count; ++i) {
        if(0 == config_->coordinate_configs[i].parent_coordinate_id) {
            continue;
        }
        found_parent = false;
        for(size_t j = 0; j < config_->coordinate_count; ++j) {
            if(config_->coordinate_configs[i].parent_coordinate_id == config_->coordinate_configs[j].coordinate_id) {
                found_parent = true;
                break;
            }
        }
        if(!found_parent) {
            return false;
        }
    }

    if(0 == config_->mesh_count) {
        return false;
    }
    if(NULL == config_->mesh_coordinate_ids || NULL == config_->mesh_configs) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(config_->mesh_coordinate_ids)) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(config_->mesh_configs)) {
        return false;
    }

    return true;
}
