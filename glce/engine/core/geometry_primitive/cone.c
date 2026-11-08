// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/core/geometry_primitive/cone.h"

#include <stdbool.h>
#include <stddef.h>
#include <math.h>

cone_t cone_initialize(float height_, float radius_) {
    cone_t cone = { 0 };
    cone.height = height_;
    cone.radius = radius_;
    return cone;
}

bool cone_is_valid(const cone_t* cone_) {
    if(NULL == cone_) {
        return false;
    }
    if(!isfinite(cone_->radius) || !(cone_->radius > 0.0f)) {
        return false;
    }
    if(!isfinite(cone_->height) || !(cone_->height > 0.0f)) {
        return false;
    }
    return true;
}
