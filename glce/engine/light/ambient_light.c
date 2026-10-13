// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/light/ambient_light.h"

#include <stdbool.h>
#include <math.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/light/core/light_types.h"
#include "engine/light/core/light_err_utils.h"

light_result_t ambient_light_initialize(vec3f_t color_, float intensity_, ambient_light_t* out_light_) {
    light_result_t ret = LIGHT_INVALID_ARGUMENT;

    ambient_light_t tmp_light = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(out_light_, ret, LIGHT_INVALID_ARGUMENT, light_result_to_str(LIGHT_INVALID_ARGUMENT), "ambient_light_initialize", "out_light_")

    tmp_light.color = color_;
    tmp_light.intensity = intensity_;
    if(!ambient_light_is_valid(&tmp_light)) {
        ret = LIGHT_INVALID_ARGUMENT;
        ERROR_MESSAGE("ambient_light_initialize(%s) - Provided color_ or intensity is not valid.", light_result_to_str(ret));
        goto cleanup;
    }

    *out_light_ = tmp_light;

    ret = LIGHT_SUCCESS;

cleanup:
    return ret;
}

bool ambient_light_is_valid(const ambient_light_t* light_) {
    if(NULL == light_) {
        return false;
    }
    if(!vec3f_is_finite(light_->color)) {
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
