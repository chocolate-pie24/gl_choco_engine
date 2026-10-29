// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/model/model_assembly_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/point_mesh.h"
#include "engine/systems/renderer/resources/meshes/ui_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

#include "application/model/core/model_types.h"
#include "application/model/core/model_err_utils.h"

model_result_t model_coordinate_config_initialize(uint16_t coordinate_id_, uint16_t parent_coordinate_id_, vec3f_t translate_, vec3f_t rotation_, model_coordinate_config_t* out_config_) {
    model_result_t ret = MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_coordinate_config_initialize", "out_config_")
    if(0 == coordinate_id_) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_coordinate_config_initialize(%s) - Provided coordinate_id_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(coordinate_id_ == parent_coordinate_id_) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_coordinate_config_initialize(%s) - Provided coordinate_id_ or parent_coordinate_id_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(360.0f < rotation_.elem[0] || 360.0f < rotation_.elem[1] || 360.0f < rotation_.elem[2]) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_coordinate_config_initialize(%s) - Provided rotation_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0.0f > rotation_.elem[0] || 0.0f > rotation_.elem[1] || 0.0f > rotation_.elem[2]) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_coordinate_config_initialize(%s) - Provided rotation_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }

    out_config_->coordinate_id = coordinate_id_;
    out_config_->parent_coordinate_id = parent_coordinate_id_;
    out_config_->translate = translate_;
    out_config_->rotation = rotation_;

    ret = MODEL_SUCCESS;

cleanup:
    return ret;
}

void model_coordinate_config_deinitialize(model_coordinate_config_t* config_) {
    if(NULL == config_) {
        return;
    }
    config_->coordinate_id = 0;
    config_->parent_coordinate_id = 0;
    config_->rotation = vec3f_initialize(0.0f, 0.0f, 0.0f);
    config_->translate = vec3f_initialize(0.0f, 0.0f, 0.0f);
}

model_result_t model_assembly_config_initialize(
    size_t coordinate_count_,
    model_coordinate_config_t** coordinates_,
    size_t line_mesh_count_,
    uint16_t** line_mesh_coordinate_ids_,
    line_mesh_config_t** line_mesh_configs_,
    size_t point_mesh_count_,
    uint16_t** point_mesh_coordinate_ids_,
    point_mesh_config_t** point_mesh_configs_,
    size_t ui_mesh_count_,
    uint16_t** ui_mesh_coordinate_ids_,
    ui_mesh_config_t** ui_mesh_configs_,
    size_t untextured_mesh_count_,
    uint16_t** untextured_mesh_coordinate_ids_,
    untextured_mesh_config_t** untextured_mesh_configs_,
    model_assembly_config_t* out_config_) {

    model_result_t ret = MODEL_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_assembly_config_initialize", "out_config_")
    IF_ARG_NULL_GOTO_CLEANUP(coordinates_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_assembly_config_initialize", "coordinates_")
    IF_ARG_NULL_GOTO_CLEANUP(*coordinates_, ret, MODEL_INVALID_ARGUMENT, model_result_to_str(MODEL_INVALID_ARGUMENT), "model_assembly_config_initialize", "*coordinates_")
    if(0 == coordinate_count_) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided coordinate_count_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }

    if(0 != line_mesh_count_ && (NULL == line_mesh_coordinate_ids_ || NULL == line_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided line_mesh_count_, line_mesh_coordinate_ids_ or line_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 != line_mesh_count_ && (NULL == *line_mesh_coordinate_ids_ || NULL == *line_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided line_mesh_count_, line_mesh_coordinate_ids_ or line_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 == line_mesh_count_ && (NULL != line_mesh_coordinate_ids_ || NULL != line_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided line_mesh_count_, line_mesh_coordinate_ids_ or line_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 == line_mesh_count_ && (NULL != *line_mesh_coordinate_ids_ || NULL != *line_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided line_mesh_count_, line_mesh_coordinate_ids_ or line_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }

    if(0 != point_mesh_count_ && (NULL == point_mesh_coordinate_ids_ || NULL == point_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided point_mesh_count_, point_mesh_coordinate_ids_ or point_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 != point_mesh_count_ && (NULL == *point_mesh_coordinate_ids_ || NULL == *point_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided point_mesh_count_, point_mesh_coordinate_ids_ or point_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 == point_mesh_count_ && (NULL != point_mesh_coordinate_ids_ || NULL != point_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided point_mesh_count_, point_mesh_coordinate_ids_ or point_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 == point_mesh_count_ && (NULL != *point_mesh_coordinate_ids_ || NULL != *point_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided point_mesh_count_, point_mesh_coordinate_ids_ or point_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }

    if(0 != ui_mesh_count_ && (NULL == ui_mesh_coordinate_ids_ || NULL == ui_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided ui_mesh_count_, ui_mesh_coordinate_ids_ or ui_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 != ui_mesh_count_ && (NULL == *ui_mesh_coordinate_ids_ || NULL == *ui_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided ui_mesh_count_, ui_mesh_coordinate_ids_ or ui_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 == ui_mesh_count_ && (NULL != ui_mesh_coordinate_ids_ || NULL != ui_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided ui_mesh_count_, ui_mesh_coordinate_ids_ or ui_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 == ui_mesh_count_ && (NULL != *ui_mesh_coordinate_ids_ || NULL != *ui_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided ui_mesh_count_, ui_mesh_coordinate_ids_ or ui_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }

    if(0 != untextured_mesh_count_ && (NULL == untextured_mesh_coordinate_ids_ || NULL == untextured_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided untextured_mesh_count_, untextured_mesh_coordinate_ids_ or untextured_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 != untextured_mesh_count_ && (NULL == *untextured_mesh_coordinate_ids_ || NULL == *untextured_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided untextured_mesh_count_, untextured_mesh_coordinate_ids_ or untextured_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 == untextured_mesh_count_ && (NULL != untextured_mesh_coordinate_ids_ || NULL != untextured_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided untextured_mesh_count_, untextured_mesh_coordinate_ids_ or untextured_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }
    if(0 == untextured_mesh_count_ && (NULL != *untextured_mesh_coordinate_ids_ || NULL != *untextured_mesh_configs_)) {
        ret = MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_assembly_config_initialize(%s) - Provided untextured_mesh_count_, untextured_mesh_coordinate_ids_ or untextured_mesh_configs_ is not valid.", model_result_to_str(ret));
        goto cleanup;
    }

    out_config_->coordinate_count = coordinate_count_;
    out_config_->coordinates = *coordinates_;

    out_config_->line_mesh_count = line_mesh_count_;
    out_config_->line_mesh_coordinate_ids = *line_mesh_coordinate_ids_;
    out_config_->line_mesh_configs = *line_mesh_configs_;

    out_config_->point_mesh_count = point_mesh_count_;
    out_config_->point_mesh_coordinate_ids = *point_mesh_coordinate_ids_;
    out_config_->point_mesh_configs = *point_mesh_configs_;

    out_config_->ui_mesh_count = ui_mesh_count_;
    out_config_->ui_mesh_coordinate_ids = *ui_mesh_coordinate_ids_;
    out_config_->ui_mesh_configs = *ui_mesh_configs_;

    out_config_->untextured_mesh_count = untextured_mesh_count_;
    out_config_->untextured_mesh_coordinate_ids = *untextured_mesh_coordinate_ids_;
    out_config_->untextured_mesh_configs = *untextured_mesh_configs_;

    *line_mesh_coordinate_ids_ = NULL;
    *line_mesh_configs_ = NULL;
    *point_mesh_coordinate_ids_ = NULL;
    *point_mesh_configs_ = NULL;
    *ui_mesh_coordinate_ids_ = NULL;
    *ui_mesh_configs_ = NULL;
    *untextured_mesh_coordinate_ids_ = NULL;
    *untextured_mesh_configs_ = NULL;

    ret = MODEL_SUCCESS;

cleanup:
    return ret;
}

void model_assembly_config_deinitialize(model_assembly_config_t* config_) {
    if(NULL == config_) {
        return;
    }
    if(!model_assembly_config_is_valid(config_)) {
        ERROR_MESSAGE("model_assembly_config_deinitialize(%s) - Provided config_ is corrupted.", model_result_to_str(MODEL_DATA_CORRUPTED));
        return;
    }
    for(size_t i = 0; i != config_->coordinate_count; ++i) {
        model_coordinate_config_deinitialize(&config_->coordinates[i]);
    }

    // TODO: xxx_mesh_config_deinitialize実装
    // for(size_t i = 0; i != config_->line_mesh_count; ++i) {
    //     line_mesh_config_deinitialize(&config_->line_mesh_configs[i]);
    // }
    // for(size_t i = 0; i != config_->point_mesh_count; ++i) {
    //     point_mesh_config_deinitialize(&config_->point_mesh_configs[i]);
    // }
    // for(size_t i = 0; i != config_->ui_mesh_count; ++i) {
    //     ui_mesh_config_deinitialize(&config_->ui_mesh_configs[i]);
    // }
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

bool model_coordinate_config_is_valid(const model_coordinate_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->coordinate_id) {
        return false;
    }
    if(config_->coordinate_id == config_->parent_coordinate_id) {
        return false;
    }
    if(!vec3f_is_finite(config_->rotation)) {
        return false;
    }
    if(!vec3f_is_finite(config_->translate)) {
        return false;
    }
    return true;
}

bool model_assembly_config_is_valid(const model_assembly_config_t* config_) {
    if(NULL == config_) {
        return false;
    }

    // coordinate
    if(!general_allocator_ptr_is_allocated(config_->coordinates)) {
        return false;
    }
    if(0 == config_->coordinate_count) {
        return false;
    }
    if(NULL == config_->coordinates) {
        return false;
    }

    // line mesh
    if(0 != config_->line_mesh_count && (NULL == config_->line_mesh_coordinate_ids || NULL == config_->line_mesh_configs)) {
        return false;
    }
    if(0 == config_->line_mesh_count && (NULL != config_->line_mesh_coordinate_ids || NULL != config_->line_mesh_configs)) {
        return false;
    }
    if(0 != config_->line_mesh_count) {
        if(!general_allocator_ptr_is_allocated(config_->line_mesh_coordinate_ids)) {
            return false;
        }
        if(!general_allocator_ptr_is_allocated(config_->line_mesh_configs)) {
            return false;
        }
    }

    // point mesh
    if(0 != config_->point_mesh_count && (NULL == config_->point_mesh_coordinate_ids || NULL == config_->point_mesh_configs)) {
        return false;
    }
    if(0 == config_->point_mesh_count && (NULL != config_->point_mesh_coordinate_ids || NULL != config_->point_mesh_configs)) {
        return false;
    }
    if(0 != config_->point_mesh_count) {
        if(!general_allocator_ptr_is_allocated(config_->point_mesh_coordinate_ids)) {
            return false;
        }
        if(!general_allocator_ptr_is_allocated(config_->point_mesh_configs)) {
            return false;
        }
    }

    // ui mesh
    if(0 != config_->ui_mesh_count && (NULL == config_->ui_mesh_coordinate_ids || NULL == config_->ui_mesh_configs)) {
        return false;
    }
    if(0 == config_->ui_mesh_count && (NULL != config_->ui_mesh_coordinate_ids || NULL != config_->ui_mesh_configs)) {
        return false;
    }
    if(0 != config_->ui_mesh_count) {
        if(!general_allocator_ptr_is_allocated(config_->ui_mesh_coordinate_ids)) {
            return false;
        }
        if(!general_allocator_ptr_is_allocated(config_->ui_mesh_configs)) {
            return false;
        }
    }

    // untextured mesh
    if(0 != config_->untextured_mesh_count && (NULL == config_->untextured_mesh_coordinate_ids || NULL == config_->untextured_mesh_configs)) {
        return false;
    }
    if(0 == config_->untextured_mesh_count && (NULL != config_->untextured_mesh_coordinate_ids || NULL != config_->untextured_mesh_configs)) {
        return false;
    }
    if(0 != config_->untextured_mesh_count) {
        if(!general_allocator_ptr_is_allocated(config_->untextured_mesh_coordinate_ids)) {
            return false;
        }
        if(!general_allocator_ptr_is_allocated(config_->untextured_mesh_configs)) {
            return false;
        }
    }

    return true;
}
