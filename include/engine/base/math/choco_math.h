/** @ingroup base
 *
 * @file choco_math.h
 * @author chocolate-pie24
 * @brief GLCE以外のプロジェクトでも使用可能な数学演算定義
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
#ifndef GLCE_ENGINE_BASE_MATH_CHOCO_MATH_H
#define GLCE_ENGINE_BASE_MATH_CHOCO_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/base/math/math_types.h"

void vec3f_initialize(float x_, float y_, float z_, vec3f_t* out_vec3f_);

void mat4f_zero(mat4x4f_t* out_mat_);

void mat4f_identity(mat4x4f_t* out_mat_);

void mat4f_mul(const mat4x4f_t* mat1_, const mat4x4f_t* mat2_, mat4x4f_t* out_mat_);

void mat4f_transpose(mat4x4f_t* mat_);

#ifdef __cplusplus
}
#endif
#endif
