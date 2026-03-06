/** @ingroup base
 *
 * @file choco_math.c
 * @author chocolate-pie24
 * @brief GLCE以外のプロジェクトでも使用可能な数学演算実装
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stddef.h>

#include "engine/base/math/choco_math.h"

#ifdef TEST_BUILD
#include <stdbool.h>
#include <assert.h>
#endif

void vec3f_initialize(float x_, float y_, float z_, vec3f_t* out_vec3f_) {
    if(NULL == out_vec3f_) {
#ifdef TEST_BUILD
        assert(false);
#endif
        return;
    }
    out_vec3f_->elem[0] = x_;
    out_vec3f_->elem[1] = y_;
    out_vec3f_->elem[2] = z_;
}

void mat4f_zero(mat4x4f_t* out_mat_) {
    if(NULL == out_mat_) {
#ifdef TEST_BUILD
        assert(false);
#endif
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
#ifdef TEST_BUILD
        assert(false);
#endif
        return;
    }
    out_mat_->elem[0]  = 1.0f; out_mat_->elem[1]  = 0.0f; out_mat_->elem[2]  = 0.0f; out_mat_->elem[3]  = 0.0f;
    out_mat_->elem[4]  = 0.0f; out_mat_->elem[5]  = 1.0f; out_mat_->elem[6]  = 0.0f; out_mat_->elem[7]  = 0.0f;
    out_mat_->elem[8]  = 0.0f; out_mat_->elem[9]  = 0.0f; out_mat_->elem[10] = 1.0f; out_mat_->elem[11] = 0.0f;
    out_mat_->elem[12] = 0.0f; out_mat_->elem[13] = 0.0f; out_mat_->elem[14] = 0.0f; out_mat_->elem[15] = 1.0f;
}

void mat4f_mul(const mat4x4f_t* mat1_, const mat4x4f_t* mat2_, mat4x4f_t* out_mat_) {
    if(NULL == out_mat_) {
#ifdef TEST_BUILD
        assert(false);
#endif
        return;
    }
    if(NULL == mat1_ || NULL == mat2_) {
#ifdef TEST_BUILD
        assert(false);
#endif
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
#ifdef TEST_BUILD
        assert(false);
#endif
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
