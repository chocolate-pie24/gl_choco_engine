// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/core/geometry_primitive/vertex.h"

#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_math/math_types.h"

bool ui_vertex_is_valid(const ui_vertex_t* vertex_) {
    if(NULL == vertex_) {
        return false;
    }
    if(!vec2f_is_finite(vertex_->position)) {
        return false;
    }
    if(!vec2f_is_finite(vertex_->tex_coord)) {
        return false;
    }
    return true;
}

bool line_vertex_is_valid(const line_vertex_t* vertex_) {
    if(NULL == vertex_) {
        return false;
    }
    if(!vec3f_is_finite(vertex_->position)) {
        return false;
    }
    return true;
}

bool point_vertex_is_valid(const point_vertex_t* vertex_) {
    if(NULL == vertex_) {
        return false;
    }
    if(!vec3f_is_finite(vertex_->position)) {
        return false;
    }
    return true;
}

bool point_normal_vertex_is_valid(const point_normal_vertex_t* vertex_) {
    if(NULL == vertex_) {
        return false;
    }
    if(!vec3f_is_finite(vertex_->position)) {
        return false;
    }
    return true;
}
