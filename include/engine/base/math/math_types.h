/** @ingroup base
 *
 * @file math_types.h
 * @author chocolate-pie24
 * @brief GLCE以外のプロジェクトでも使用可能な数学型定義
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
#ifndef GLCE_ENGINE_BASE_MATH_MATH_TYPES_H
#define GLCE_ENGINE_BASE_MATH_MATH_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define CHOCO_PI 3.14159265358979f                                  /**< 円周率定義 */
#define DEG_TO_RAD(deg_) (((deg_) / 360.0f) * 2.0f * CHOCO_PI)      /**< degree -> radian */
#define RAD_TO_DEG(rad_) (((rad_) / 2.0f / CHOCO_PI) * 360.0f)      /**< radian -> degree */

/**
 * @brief float型3次元ベクトル定義
 *
 */
typedef struct vec3f {
    float elem[3];  /**< ベクトル要素格納配列 */
} vec3f_t;

/**
 * @brief float型4x4行列定義
 * @warning 配列要素は行優先で格納すること
 *
 */
typedef struct mat4x4f {
    float elem[16]; /**< 4x4行列要素格納配列 */
} mat4x4f_t;

#ifdef __cplusplus
}
#endif
#endif
