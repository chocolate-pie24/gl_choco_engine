// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/model_framework/coordinates/model_coordinate_config.h"

#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "application/model_framework/core/model_framework_types.h"
#include "application/model_framework/core/model_framework_err_utils.h"

model_framework_result_t model_coordinate_config_initialize(uint16_t coordinate_id_, uint16_t parent_coordinate_id_, vec3f_t translate_, vec3f_t rotation_, model_coordinate_config_t* out_config_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "model_coordinate_config_initialize", "out_config_")
    if(0 == coordinate_id_) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_coordinate_config_initialize(%s) - Provided coordinate_id_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    if(coordinate_id_ == parent_coordinate_id_) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_coordinate_config_initialize(%s) - Provided coordinate_id_ or parent_coordinate_id_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    out_config_->coordinate_id = coordinate_id_;
    out_config_->parent_coordinate_id = parent_coordinate_id_;
    out_config_->translate = translate_;
    out_config_->rotation = rotation_;

    ret = MODEL_FRAMEWORK_SUCCESS;

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

bool model_coordinate_config_is_valid(const model_coordinate_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->coordinate_id) {   // 0はroot node専用
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
