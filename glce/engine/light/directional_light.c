// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/light/directional_light.h"

#include <stdbool.h>
#include <math.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/light/core/light_types.h"
#include "engine/light/core/light_err_utils.h"

light_result_t directional_light_initialize(vec3f_t direction_, vec3f_t color_, float intensity_, directional_light_t* out_light_) {
    light_result_t ret = LIGHT_INVALID_ARGUMENT;

    directional_light_t tmp_light = { 0 };
    vec3f_t normalized_direction = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(out_light_, ret, LIGHT_INVALID_ARGUMENT, light_result_to_str(LIGHT_INVALID_ARGUMENT), "directional_light_initialize", "out_light_")
    if(is_equal_float(vec3f_length(direction_), 0.0f)) {
        ret = LIGHT_INVALID_ARGUMENT;
        ERROR_MESSAGE("directional_light_initialize(%s) - Provided direction_ is not valid.", light_result_to_str(ret));
        goto cleanup;
    }

    normalized_direction = vec3f_normalize(direction_);
    tmp_light.color = color_;
    tmp_light.direction = normalized_direction;
    tmp_light.intensity = intensity_;
    if(!directional_light_is_valid(&tmp_light)) {
        ret = LIGHT_INVALID_ARGUMENT;
        ERROR_MESSAGE("directional_light_initialize(%s) - Provided direction_, color_ or intensity is not valid.", light_result_to_str(ret));
        goto cleanup;
    }

    *out_light_ = tmp_light;

    ret = LIGHT_SUCCESS;

cleanup:
    return ret;
}

bool directional_light_is_valid(const directional_light_t* light_) {
    if(NULL == light_) {
        return false;
    }
    if(!vec3f_is_finite(light_->color) || !vec3f_is_finite(light_->direction)) {
        return false;
    }
    if(!vec3f_is_normalized(light_->direction)) {
        return false;
    }
    if(0.0f > light_->color.elem[0] || 0.0f > light_->color.elem[1] || 0.0f > light_->color.elem[2]) {
        return false;
    }
    if(1.0f < light_->color.elem[0] || 1.0f < light_->color.elem[1] || 1.0f < light_->color.elem[2]) {
        return false;
    }
    if(!isfinite(light_->intensity)) {
        return false;
    }
    if(0.0f > light_->intensity) {
        return false;
    }
    return true;
}
