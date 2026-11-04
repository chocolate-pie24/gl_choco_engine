// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/core/geometry_primitive/vertex.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_math/math_types.h"

static int8_t clamp_to_i8(float value_);

// NOTE: rotationとtranslateのみのtransform_に対応、scale, せん断変形には対応していない(将来的に変更するかも)
bool line_vertices_transform(line_vertex_t* vertices_, size_t vertex_count_, const mat4x4f_t* transform_) {
    if(NULL == vertices_ || NULL == transform_) {
        return false;
    }
    if(0 == vertex_count_) {
        return false;
    }
    for(size_t i = 0; i != vertex_count_; ++i) {
        const vec4f_t tmp_point = vec4f_initialize(vertices_[i].position.elem[0], vertices_[i].position.elem[1], vertices_[i].position.elem[2], 1.0f);

        const vec4f_t point_new = mat4f_vec4f_mul(transform_, tmp_point);

        vertices_[i].position = vec3f_initialize(point_new.elem[0], point_new.elem[1], point_new.elem[2]);
    }
    return true;
}

// NOTE: rotationとtranslateのみのtransform_に対応、scale, せん断変形には対応していない(将来的に変更するかも)
bool point_vertices_transform(point_vertex_t* vertices_, size_t vertex_count_, const mat4x4f_t* transform_) {
    if(NULL == vertices_ || NULL == transform_) {
        return false;
    }
    if(0 == vertex_count_) {
        return false;
    }
    for(size_t i = 0; i != vertex_count_; ++i) {
        const vec4f_t tmp_point = vec4f_initialize(vertices_[i].position.elem[0], vertices_[i].position.elem[1], vertices_[i].position.elem[2], 1.0f);

        const vec4f_t point_new = mat4f_vec4f_mul(transform_, tmp_point);

        vertices_[i].position = vec3f_initialize(point_new.elem[0], point_new.elem[1], point_new.elem[2]);
    }
    return true;
}

// NOTE: rotationとtranslateのみのtransform_に対応、scale, せん断変形には対応していない(将来的に変更するかも)
bool ui_vertices_transform(ui_vertex_t* vertices_, size_t vertex_count_, const mat4x4f_t* transform_) {
    if(NULL == vertices_ || NULL == transform_) {
        return false;
    }
    if(0 == vertex_count_) {
        return false;
    }
    for(size_t i = 0; i != vertex_count_; ++i) {
        const vec4f_t tmp_point = vec4f_initialize(vertices_[i].position.elem[0], vertices_[i].position.elem[1], 0.0f, 1.0f);
        const vec4f_t point_new = mat4f_vec4f_mul(transform_, tmp_point);
        vertices_[i].position = vec2f_initialize(point_new.elem[0], point_new.elem[1]);
    }
    return true;
}

// NOTE: rotationとtranslateのみのtransform_に対応、scale, せん断変形には対応していない(将来的に変更するかも)
// NOTE: vertices_内の要素はfiniteであること(point_normal_vertex_tを直接applicationレイヤーが触ることはなく、engine内のみで使用されるためfinite性を保証する)。
bool point_normal_vertices_transform(point_normal_vertex_t* vertices_, size_t vertex_count_, const mat4x4f_t* transform_) {
    if(NULL == vertices_ || NULL == transform_) {
        return false;
    }
    if(0 == vertex_count_) {
        return false;
    }
    for(size_t i = 0; i != vertex_count_; ++i) {
        const vec4f_t tmp_point = vec4f_initialize(vertices_[i].position.elem[0], vertices_[i].position.elem[1], vertices_[i].position.elem[2], 1.0f);
        const vec4f_t tmp_normal = vec4f_initialize((float)vertices_[i].normal.elem[0] / 127.0f, (float)vertices_[i].normal.elem[1] / 127.0f, (float)vertices_[i].normal.elem[2] / 127.0f, 0.0f);

        const vec4f_t point_new = mat4f_vec4f_mul(transform_, tmp_point);
        const vec4f_t normal_new = mat4f_vec4f_mul(transform_, tmp_normal);

        const int8_t normal_x = clamp_to_i8(normal_new.elem[0] * 127.0f);
        const int8_t normal_y = clamp_to_i8(normal_new.elem[1] * 127.0f);
        const int8_t normal_z = clamp_to_i8(normal_new.elem[2] * 127.0f);

        vertices_[i].position = vec3f_initialize(point_new.elem[0], point_new.elem[1], point_new.elem[2]);
        vertices_[i].normal = vec4i8_initialize(normal_x, normal_y, normal_z, 0);
    }
    return true;
}

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

static int8_t clamp_to_i8(float value_) {
    float scaled = value_;
    if(scaled > 127.0f) {
        scaled = 127.0f;
    }
    if(scaled < -127.0f) {
        scaled = -127.0f;
    }

    return (int8_t)scaled;
}
