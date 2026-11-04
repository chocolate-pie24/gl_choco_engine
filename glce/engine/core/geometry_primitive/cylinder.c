// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/core/geometry_primitive/cylinder.h"

#include <stdbool.h>
#include <stddef.h>
#include <math.h>

cylinder_t cylinder_initialize(float height_, float radius_) {
    cylinder_t cylinder = { 0 };
    cylinder.height = height_;
    cylinder.radius = radius_;
    return cylinder;
}

bool cylinder_is_valid(const cylinder_t* cylinder_) {
    if(NULL == cylinder_) {
        return false;
    }
    if(!isfinite(cylinder_->radius) || !(cylinder_->radius > 0.0f)) {
        return false;
    }
    if(!isfinite(cylinder_->height) || !(cylinder_->height > 0.0f)) {
        return false;
    }
    return true;
}
