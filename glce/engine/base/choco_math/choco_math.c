// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup base
 *
 * @file choco_math.c
 * @author chocolate-pie24
 * @brief GLCE以外のプロジェクトでも使用可能な数学演算実装
 *
 * @date 2025-09-20
 *
 */
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_macros.h"

#define DET_EPS 1.0e-6f           /**< 行列式ゼロ判定用閾値 */
#define FLOAT_EQUAL_EPS 1.0e-6f   /**< 浮動小数点イコール判定用閾値 */

float choco_tanf(float radian_) {
    return tanf(radian_);
}

bool is_equal_float(float a_, float b_) {
    const float diff = (a_ > b_) ? (a_ - b_) : (b_ - a_);
    if(diff <= FLOAT_EQUAL_EPS) {
        return true;
    } else {
        return false;
    }
}

vec2f_t vec2f_initialize(float x_, float y_) {
    vec2f_t ret = { 0.0f };

    ret.elem[0] = x_;
    ret.elem[1] = y_;

    return ret;
}

vec2f_t vec2f_add(vec2f_t vec1_, vec2f_t vec2_) {
    vec2f_t ret = { 0.0f };

    ret.elem[0] = vec1_.elem[0] + vec2_.elem[0];
    ret.elem[1] = vec1_.elem[1] + vec2_.elem[1];

    return ret;
}

bool vec2f_is_finite(vec2f_t vec_) {
    if(!isfinite(vec_.elem[0]) || !isfinite(vec_.elem[1])) {
        return false;
    }
    return true;
}

vec3f_t vec3f_initialize(float x_, float y_, float z_) {
    vec3f_t ret = { 0.0f };

    ret.elem[0] = x_;
    ret.elem[1] = y_;
    ret.elem[2] = z_;

    return ret;
}

vec3f_t vec3f_add(vec3f_t vec1_, vec3f_t vec2_) {
    vec3f_t ret = { 0.0f };

    ret.elem[0] = vec1_.elem[0] + vec2_.elem[0];
    ret.elem[1] = vec1_.elem[1] + vec2_.elem[1];
    ret.elem[2] = vec1_.elem[2] + vec2_.elem[2];

    return ret;
}

vec3f_t vec3f_scale(vec3f_t vec_, float scale_) {
    vec3f_t ret = { 0.0f };

    ret.elem[0] = vec_.elem[0] * scale_;
    ret.elem[1] = vec_.elem[1] * scale_;
    ret.elem[2] = vec_.elem[2] * scale_;

    return ret;
}

float vec3f_length_squared(vec3f_t vec_) {
    return (vec_.elem[0] * vec_.elem[0]) + (vec_.elem[1] * vec_.elem[1]) + (vec_.elem[2] * vec_.elem[2]);
}

float vec3f_length(vec3f_t vec_) {
    return sqrtf(vec3f_length_squared(vec_));
}

vec3f_t vec3f_normalize(vec3f_t vec_) {
    vec3f_t ret = vec_;

    const float length = vec3f_length(vec_);
    if(is_equal_float(length, 0.0f)) {
        WARN_MESSAGE("vec3f_normalize - Argument vec_ is a zero vector.");
        return ret;
    }

    ret.elem[0] /= length;
    ret.elem[1] /= length;
    ret.elem[2] /= length;

    return ret;
}

bool vec3f_is_finite(vec3f_t vec_) {
    if(!isfinite(vec_.elem[0]) || !isfinite(vec_.elem[1]) || !isfinite(vec_.elem[2])) {
        return false;
    }
    return true;
}

vec3f_t vec3f_component_min(vec3f_t v1_, vec3f_t v2_) {
    vec3f_t tmp_v = { 0 };

    tmp_v.elem[0] = (v1_.elem[0] > v2_.elem[0]) ? v2_.elem[0] : v1_.elem[0];
    tmp_v.elem[1] = (v1_.elem[1] > v2_.elem[1]) ? v2_.elem[1] : v1_.elem[1];
    tmp_v.elem[2] = (v1_.elem[2] > v2_.elem[2]) ? v2_.elem[2] : v1_.elem[2];

    return tmp_v;
}

vec3f_t vec3f_component_max(vec3f_t v1_, vec3f_t v2_) {
    vec3f_t tmp_v = { 0 };

    tmp_v.elem[0] = (v1_.elem[0] < v2_.elem[0]) ? v2_.elem[0] : v1_.elem[0];
    tmp_v.elem[1] = (v1_.elem[1] < v2_.elem[1]) ? v2_.elem[1] : v1_.elem[1];
    tmp_v.elem[2] = (v1_.elem[2] < v2_.elem[2]) ? v2_.elem[2] : v1_.elem[2];

    return tmp_v;
}

vec4f_t vec4f_initialize(float x_, float y_, float z_, float w_) {
    vec4f_t ret = { 0.0f };

    ret.elem[0] = x_;
    ret.elem[1] = y_;
    ret.elem[2] = z_;
    ret.elem[3] = w_;

    return ret;
}

vec4f_t vec4f_add(vec4f_t vec1_, vec4f_t vec2_) {
    vec4f_t ret = { 0.0f };

    ret.elem[0] = vec1_.elem[0] + vec2_.elem[0];
    ret.elem[1] = vec1_.elem[1] + vec2_.elem[1];
    ret.elem[2] = vec1_.elem[2] + vec2_.elem[2];
    ret.elem[3] = vec1_.elem[3] + vec2_.elem[3];

    return ret;
}

vec4u8_t vec4u8_initialize(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_) {
    vec4u8_t ret = { 0 };

    ret.elem[0] = r_;
    ret.elem[1] = g_;
    ret.elem[2] = b_;
    ret.elem[3] = a_;

    return ret;
}

vec4i8_t vec4i8_initialize(int8_t x_, int8_t y_, int8_t z_, int8_t w_) {
    vec4i8_t ret = { 0 };

    ret.elem[0] = x_;
    ret.elem[1] = y_;
    ret.elem[2] = z_;
    ret.elem[3] = w_;

    return ret;
}

void mat4f_zero(mat4x4f_t* out_mat_) {
    if(NULL == out_mat_) {
        ERROR_MESSAGE("mat4f_zero(INVALID_ARGUMENT) - Argument out_mat_ requires a valid pointer.");
        return;
    }
    out_mat_->elem[0] = 0.0f;
    out_mat_->elem[1] = 0.0f;
    out_mat_->elem[2] = 0.0f;
    out_mat_->elem[3] = 0.0f;
    out_mat_->elem[4] = 0.0f;
    out_mat_->elem[5] = 0.0f;
    out_mat_->elem[6] = 0.0f;
    out_mat_->elem[7] = 0.0f;
    out_mat_->elem[8] = 0.0f;
    out_mat_->elem[9] = 0.0f;
    out_mat_->elem[10] = 0.0f;
    out_mat_->elem[11] = 0.0f;
    out_mat_->elem[12] = 0.0f;
    out_mat_->elem[13] = 0.0f;
    out_mat_->elem[14] = 0.0f;
    out_mat_->elem[15] = 0.0f;
}

void mat4f_identity(mat4x4f_t* out_mat_) {
    if(NULL == out_mat_) {
        ERROR_MESSAGE("mat4f_identity(INVALID_ARGUMENT) - Argument out_mat_ requires a valid pointer.");
        return;
    }
    out_mat_->elem[0]  = 1.0f; out_mat_->elem[1]  = 0.0f; out_mat_->elem[2]  = 0.0f; out_mat_->elem[3]  = 0.0f;
    out_mat_->elem[4]  = 0.0f; out_mat_->elem[5]  = 1.0f; out_mat_->elem[6]  = 0.0f; out_mat_->elem[7]  = 0.0f;
    out_mat_->elem[8]  = 0.0f; out_mat_->elem[9]  = 0.0f; out_mat_->elem[10] = 1.0f; out_mat_->elem[11] = 0.0f;
    out_mat_->elem[12] = 0.0f; out_mat_->elem[13] = 0.0f; out_mat_->elem[14] = 0.0f; out_mat_->elem[15] = 1.0f;
}

void mat4f_mul(const mat4x4f_t* mat1_, const mat4x4f_t* mat2_, mat4x4f_t* out_mat_) {
    if(NULL == mat1_) {
        ERROR_MESSAGE("mat4f_mul(INVALID_ARGUMENT) - Argument mat1_ requires a valid pointer.");
        return;
    }
    if(NULL == mat2_) {
        ERROR_MESSAGE("mat4f_mul(INVALID_ARGUMENT) - Argument mat2_ requires a valid pointer.");
        return;
    }
    if(NULL == out_mat_) {
        ERROR_MESSAGE("mat4f_mul(INVALID_ARGUMENT) - Argument out_mat_ requires a valid pointer.");
        return;
    }
    const float mat2_11 = mat2_->elem[0];
    const float mat2_12 = mat2_->elem[1];
    const float mat2_13 = mat2_->elem[2];
    const float mat2_14 = mat2_->elem[3];
    const float mat2_21 = mat2_->elem[4];
    const float mat2_22 = mat2_->elem[5];
    const float mat2_23 = mat2_->elem[6];
    const float mat2_24 = mat2_->elem[7];
    const float mat2_31 = mat2_->elem[8];
    const float mat2_32 = mat2_->elem[9];
    const float mat2_33 = mat2_->elem[10];
    const float mat2_34 = mat2_->elem[11];
    const float mat2_41 = mat2_->elem[12];
    const float mat2_42 = mat2_->elem[13];
    const float mat2_43 = mat2_->elem[14];
    const float mat2_44 = mat2_->elem[15];
    {
        const float mat1_11 = mat1_->elem[0];
        const float mat1_12 = mat1_->elem[1];
        const float mat1_13 = mat1_->elem[2];
        const float mat1_14 = mat1_->elem[3];
        out_mat_->elem[0]  = mat1_11 * mat2_11 + mat1_12 * mat2_21 + mat1_13 * mat2_31 + mat1_14 * mat2_41;
        out_mat_->elem[1]  = mat1_11 * mat2_12 + mat1_12 * mat2_22 + mat1_13 * mat2_32 + mat1_14 * mat2_42;
        out_mat_->elem[2]  = mat1_11 * mat2_13 + mat1_12 * mat2_23 + mat1_13 * mat2_33 + mat1_14 * mat2_43;
        out_mat_->elem[3]  = mat1_11 * mat2_14 + mat1_12 * mat2_24 + mat1_13 * mat2_34 + mat1_14 * mat2_44;
    }
    {
        const float mat1_21 = mat1_->elem[4];
        const float mat1_22 = mat1_->elem[5];
        const float mat1_23 = mat1_->elem[6];
        const float mat1_24 = mat1_->elem[7];
        out_mat_->elem[4]  = mat1_21 * mat2_11 + mat1_22 * mat2_21 + mat1_23 * mat2_31 + mat1_24 * mat2_41;
        out_mat_->elem[5]  = mat1_21 * mat2_12 + mat1_22 * mat2_22 + mat1_23 * mat2_32 + mat1_24 * mat2_42;
        out_mat_->elem[6]  = mat1_21 * mat2_13 + mat1_22 * mat2_23 + mat1_23 * mat2_33 + mat1_24 * mat2_43;
        out_mat_->elem[7]  = mat1_21 * mat2_14 + mat1_22 * mat2_24 + mat1_23 * mat2_34 + mat1_24 * mat2_44;
    }
    {
        const float mat1_31 = mat1_->elem[8];
        const float mat1_32 = mat1_->elem[9];
        const float mat1_33 = mat1_->elem[10];
        const float mat1_34 = mat1_->elem[11];
        out_mat_->elem[8]  = mat1_31 * mat2_11 + mat1_32 * mat2_21 + mat1_33 * mat2_31 + mat1_34 * mat2_41;
        out_mat_->elem[9]  = mat1_31 * mat2_12 + mat1_32 * mat2_22 + mat1_33 * mat2_32 + mat1_34 * mat2_42;
        out_mat_->elem[10] = mat1_31 * mat2_13 + mat1_32 * mat2_23 + mat1_33 * mat2_33 + mat1_34 * mat2_43;
        out_mat_->elem[11] = mat1_31 * mat2_14 + mat1_32 * mat2_24 + mat1_33 * mat2_34 + mat1_34 * mat2_44;
    }
    {
        const float mat1_41 = mat1_->elem[12];
        const float mat1_42 = mat1_->elem[13];
        const float mat1_43 = mat1_->elem[14];
        const float mat1_44 = mat1_->elem[15];
        out_mat_->elem[12] = mat1_41 * mat2_11 + mat1_42 * mat2_21 + mat1_43 * mat2_31 + mat1_44 * mat2_41;
        out_mat_->elem[13] = mat1_41 * mat2_12 + mat1_42 * mat2_22 + mat1_43 * mat2_32 + mat1_44 * mat2_42;
        out_mat_->elem[14] = mat1_41 * mat2_13 + mat1_42 * mat2_23 + mat1_43 * mat2_33 + mat1_44 * mat2_43;
        out_mat_->elem[15] = mat1_41 * mat2_14 + mat1_42 * mat2_24 + mat1_43 * mat2_34 + mat1_44 * mat2_44;
    }
}

void mat4f_transpose(mat4x4f_t* mat_) {
    if(NULL == mat_) {
        ERROR_MESSAGE("mat4f_transpose(INVALID_ARGUMENT) - Argument mat_ requires a valid pointer.");
        return;
    }
    {
        // (1, 2) <-> (2, 1)
        const float tmp = mat_->elem[1];
        mat_->elem[1] = mat_->elem[4];
        mat_->elem[4] = tmp;
    }
    {
        // (1, 3) <-> (3, 1)
        const float tmp = mat_->elem[2];
        mat_->elem[2] = mat_->elem[8];
        mat_->elem[8] = tmp;
    }
    {
        // (1, 4) <-> (4, 1)
        const float tmp = mat_->elem[3];
        mat_->elem[3] = mat_->elem[12];
        mat_->elem[12] = tmp;
    }
    {
        // (2, 3) <-> (3, 2)
        const float tmp = mat_->elem[6];
        mat_->elem[6] = mat_->elem[9];
        mat_->elem[9] = tmp;
    }
    {
        // (2, 4) <-> (4, 2)
        const float tmp = mat_->elem[7];
        mat_->elem[7] = mat_->elem[13];
        mat_->elem[13] = tmp;
    }
    {
        // (3, 4) <-> (4, 3)
        const float tmp = mat_->elem[11];
        mat_->elem[11] = mat_->elem[14];
        mat_->elem[14] = tmp;
    }
}

void mat4f_copy(const mat4x4f_t* src_, mat4x4f_t* dst_) {
    if(NULL == src_) {
        ERROR_MESSAGE("mat4f_copy(INVALID_ARGUMENT) - Argument src_ requires a valid pointer.");
        return;
    }
    if(NULL == dst_) {
        ERROR_MESSAGE("mat4f_copy(INVALID_ARGUMENT) - Argument dst_ requires a valid pointer.");
        return;
    }
    dst_->elem[0] = src_->elem[0];
    dst_->elem[1] = src_->elem[1];
    dst_->elem[2] = src_->elem[2];
    dst_->elem[3] = src_->elem[3];
    dst_->elem[4] = src_->elem[4];
    dst_->elem[5] = src_->elem[5];
    dst_->elem[6] = src_->elem[6];
    dst_->elem[7] = src_->elem[7];
    dst_->elem[8] = src_->elem[8];
    dst_->elem[9] = src_->elem[9];
    dst_->elem[10] = src_->elem[10];
    dst_->elem[11] = src_->elem[11];
    dst_->elem[12] = src_->elem[12];
    dst_->elem[13] = src_->elem[13];
    dst_->elem[14] = src_->elem[14];
    dst_->elem[15] = src_->elem[15];
}

bool mat4f_inverse(mat4x4f_t* mat_) {
    if(NULL == mat_) {
        ERROR_MESSAGE("mat4f_inverse(INVALID_ARGUMENT) - Argument mat_ requires a valid pointer.");
        return false;
    }

    const float m11 = mat_->elem[0];  const float m12 = mat_->elem[1];  const float m13 = mat_->elem[2];  const float m14 = mat_->elem[3];
    const float m21 = mat_->elem[4];  const float m22 = mat_->elem[5];  const float m23 = mat_->elem[6];  const float m24 = mat_->elem[7];
    const float m31 = mat_->elem[8];  const float m32 = mat_->elem[9];  const float m33 = mat_->elem[10]; const float m34 = mat_->elem[11];
    const float m41 = mat_->elem[12]; const float m42 = mat_->elem[13]; const float m43 = mat_->elem[14]; const float m44 = mat_->elem[15];

    // 行列式の計算
    float det = m11 * m22 * m33 * m44; det += m11 * m23 * m34 * m42; det += m11 * m24 * m32 * m43;
    det -= m11 * m24 * m33 * m42; det -= m11 * m23 * m32 * m44; det -= m11 * m22 * m34 * m43;
    det -= m12 * m21 * m33 * m44; det -= m13 * m21 * m34 * m42; det -= m14 * m21 * m32 * m43;
    det += m14 * m21 * m33 * m42; det += m13 * m21 * m32 * m44; det += m12 * m21 * m34 * m43;
    det += m12 * m23 * m31 * m44; det += m13 * m24 * m31 * m42; det += m14 * m22 * m31 * m43;
    det -= m14 * m23 * m31 * m42; det -= m13 * m22 * m31 * m44; det -= m12 * m24 * m31 * m43;
    det -= m12 * m23 * m34 * m41; det -= m13 * m24 * m32 * m41; det -= m14 * m22 * m33 * m41;
    det += m14 * m23 * m32 * m41; det += m13 * m22 * m34 * m41; det += m12 * m24 * m33 * m41;
    const float tmp = (det > 0.0f) ? det : (-1.0f * det);
    if(tmp <= DET_EPS) {
        DEBUG_MESSAGE("mat4f_inverse: Matrix inversion failed because the determinant is zero or near zero.");
        return false;
    }

    // 逆行列の計算
    mat_->elem[0]  = (m22 * m33 * m44 + m23 * m34 * m42 + m24 * m32 * m43 - m24 * m33 * m42 - m23 * m32 * m44 - m22 * m34 * m43) / det;
    mat_->elem[1]  = (-1.0f * m12 * m33 * m44 - m13 * m34 * m42 - m14 * m32 * m43 + m14 * m33 * m42 + m13 * m32 * m44 + m12 * m34 * m43) / det;
    mat_->elem[2]  = (m12 * m23 * m44 + m13 * m24 * m42 + m14 * m22 * m43 - m14 * m23 * m42 - m13 * m22 * m44 - m12 * m24 * m43) / det;
    mat_->elem[3]  = (-1.0f * m12 * m23 * m34 - m13 * m24 * m32 - m14 * m22 * m33 + m14 * m23 * m32 + m13 * m22 * m34 + m12 * m24 * m33) / det;

    mat_->elem[4]  = (-1.0f * m21 * m33 * m44 - m23 * m34 * m41 - m24 * m31 * m43 + m24 * m33 * m41 + m23 * m31 * m44 + m21 * m34 * m43) / det;
    mat_->elem[5]  = (m11 * m33 * m44 + m13 * m34 * m41 + m14 * m31 * m43 - m14 * m33 * m41 - m13 * m31 * m44 - m11 * m34 * m43) / det;
    mat_->elem[6]  = (-1.0f * m11 * m23 * m44 - m13 * m24 * m41 - m14 * m21 * m43 + m14 * m23 * m41 + m13 * m21 * m44 + m11 * m24 * m43) / det;
    mat_->elem[7]  = (m11 * m23 * m34 + m13 * m24 * m31 + m14 * m21 * m33 - m14 * m23 * m31 - m13 * m21 * m34 - m11 * m24 * m33) / det;

    mat_->elem[8]  = (m21 * m32 * m44 + m22 * m34 * m41 + m24 * m31 * m42 - m24 * m32 * m41 - m22 * m31 * m44 - m21 * m34 * m42) / det;
    mat_->elem[9]  = (-1.0f * m11 * m32 * m44 - m12 * m34 * m41 - m14 * m31 * m42 + m14 * m32 * m41 + m12 * m31 * m44 + m11 * m34 * m42) / det;
    mat_->elem[10] = (m11 * m22 * m44 + m12 * m24 * m41 + m14 * m21 * m42 - m14 * m22 * m41 - m12 * m21 * m44 - m11 * m24 * m42) / det;
    mat_->elem[11] = (-1.0f * m11 * m22 * m34 - m12 * m24 * m31 - m14 * m21 * m32 + m14 * m22 * m31 + m12 * m21 * m34 + m11 * m24 * m32) / det;

    mat_->elem[12] = (-1.0f * m21 * m32 * m43 - m22 * m33 * m41 - m23 * m31 * m42 + m23 * m32 * m41 + m22 * m31 * m43 + m21 * m33 * m42) / det;
    mat_->elem[13] = (m11 * m32 * m43 + m12 * m33 * m41 + m13 * m31 * m42 - m13 * m32 * m41 - m12 * m31 * m43 - m11 * m33 * m42) / det;
    mat_->elem[14] = (-1.0f * m11 * m22 * m43 - m12 * m23 * m41 - m13 * m21 * m42 + m13 * m22 * m41 + m12 * m21 * m43 + m11 * m23 * m42) / det;
    mat_->elem[15] = (m11 * m22 * m33 + m12 * m23 * m31 + m13 * m21 * m32 - m13 * m22 * m31 - m12 * m21 * m33 - m11 * m23 * m32) / det;

    return true;
}

vec4f_t mat4f_vec4f_mul(const mat4x4f_t* mat_, vec4f_t vec_) {
    vec4f_t ret = { 0.0f };

    if(NULL == mat_) {
        ERROR_MESSAGE("mat4f_vec4f_mul(INVALID_ARGUMENT) - Argument mat_ requires a valid pointer.");
        return ret;
    }
    const float x = vec_.elem[0];
    const float y = vec_.elem[1];
    const float z = vec_.elem[2];
    const float w = vec_.elem[3];

    ret.elem[0] = mat_->elem[0]  * x + mat_->elem[1]  * y + mat_->elem[2]  * z + mat_->elem[3]  * w;
    ret.elem[1] = mat_->elem[4]  * x + mat_->elem[5]  * y + mat_->elem[6]  * z + mat_->elem[7]  * w;
    ret.elem[2] = mat_->elem[8]  * x + mat_->elem[9]  * y + mat_->elem[10] * z + mat_->elem[11] * w;
    ret.elem[3] = mat_->elem[12] * x + mat_->elem[13] * y + mat_->elem[14] * z + mat_->elem[15] * w;

    return ret;
}

void mat4f_translation(vec3f_t position_, mat4x4f_t* mat_) {
    if(NULL == mat_) {
        ERROR_MESSAGE("mat4f_translation(INVALID_ARGUMENT) - Argument mat_ requires a valid pointer.");
        return;
    }

    mat4f_identity(mat_);
    mat_->elem[3] = position_.elem[0];
    mat_->elem[7] = position_.elem[1];
    mat_->elem[11] = position_.elem[2];
}

void mat4f_rot_x(float radian_, mat4x4f_t* mat_) {
    if(NULL == mat_) {
        ERROR_MESSAGE("mat4f_rot_x(INVALID_ARGUMENT) - Argument mat_ requires a valid pointer.");
        return;
    }

    mat4f_identity(mat_);
    mat_->elem[5] = cosf(radian_);
    mat_->elem[6] = -1.0f * sinf(radian_);
    mat_->elem[9] = sinf(radian_);
    mat_->elem[10] = cosf(radian_);
}

void mat4f_rot_y(float radian_, mat4x4f_t* mat_) {
    if(NULL == mat_) {
        ERROR_MESSAGE("mat4f_rot_y(INVALID_ARGUMENT) - Argument mat_ requires a valid pointer.");
        return;
    }

    mat4f_identity(mat_);
    mat_->elem[0] = cosf(radian_);
    mat_->elem[2] = sinf(radian_);
    mat_->elem[8] = -1.0f * sinf(radian_);
    mat_->elem[10] = cosf(radian_);
}

void mat4f_rot_z(float radian_, mat4x4f_t* mat_) {
    if(NULL == mat_) {
        ERROR_MESSAGE("mat4f_rot_z(INVALID_ARGUMENT) - Argument mat_ requires a valid pointer.");
        return;
    }

    mat4f_identity(mat_);
    mat_->elem[0] = cosf(radian_);
    mat_->elem[1] = -1.0f * sinf(radian_);
    mat_->elem[4] = sinf(radian_);
    mat_->elem[5] = cosf(radian_);
}

void mat4f_rot_xyz(float x_radian_, float y_radian_, float z_radian_, mat4x4f_t* mat_) {
    if(NULL == mat_) {
        ERROR_MESSAGE("mat4f_rot_xyz(INVALID_ARGUMENT) - Argument mat_ requires a valid pointer.");
        return;
    }

    mat4x4f_t x_rot = { 0 };
    mat4x4f_t y_rot = { 0 };
    mat4x4f_t z_rot = { 0 };
    mat4x4f_t tmp = { 0 };

    mat4f_rot_x(x_radian_, &x_rot);
    mat4f_rot_y(y_radian_, &y_rot);
    mat4f_rot_z(z_radian_, &z_rot);
    mat4f_mul(&x_rot, &y_rot, &tmp);
    mat4f_mul(&tmp, &z_rot, mat_);
}
