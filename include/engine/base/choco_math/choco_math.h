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
#ifndef GLCE_ENGINE_BASE_CHOCO_MATH_CHOCO_MATH_H
#define GLCE_ENGINE_BASE_CHOCO_MATH_CHOCO_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

/**
 * @brief radian_のtangentを計算する
 *
 * @note <math.h>のtanfラッパー
 *
 * @param radian_ tangentを計算するradian値
 * @return float 計算結果
 */
float choco_tanf(float radian_);

/**
 * @brief 2つのfloat値が等しいかを判定する
 *
 * @param a_ 判定値1
 * @param b_ 判定値2
 * @retval true 等しい
 * @retval false 等しくない
 */
bool is_equal_float(float a_, float b_);

/**
 * @brief 3次元ベクトルxyzを初期化する
 *
 * @note out_vec3f_ == NULLの場合はエラーメッセージを出力し、何もしない
 *
 * @param[in] x_ 初期化値(x)
 * @param[in] y_ 初期化値(y)
 * @param[in] z_ 初期化値(z)
 * @param[out] out_vec3f_ 初期化対象ベクトル
 */
void vec3f_initialize(float x_, float y_, float z_, vec3f_t* out_vec3f_);

/**
 * @brief 3次元ベクトルの足し算(out_vec3f_ = vec1_ + vec2_)を実行する
 *
 * @note 以下の場合はエラーメッセージを出力し、何もしない
 * - vec1_ == NULL
 * - vec2_ == NULL
 * - out_vec3f_ == NULL
 *
 * @param[in] vec1_ vec1_ + vec2_のvec1_
 * @param[in] vec2_ vec1_ + vec2_のvec2_
 * @param[out] out_vec3f_ 加算結果ベクトル
 */
void vec3f_add(const vec3f_t* vec1_, const vec3f_t* vec2_, vec3f_t* out_vec3f_);

/**
 * @brief 4次元ベクトルxyzwを初期化する
 *
 * @note out_vec4f_ == NULLの場合はエラーメッセージを出力し、何もしない
 *
 * @param[in] x_ 初期化値(x)
 * @param[in] y_ 初期化値(y)
 * @param[in] z_ 初期化値(z)
 * @param[in] w_ 初期化値(w)
 * @param[out] out_vec4f_ 初期化対象ベクトル
 */
void vec4f_initialize(float x_, float y_, float z_, float w_, vec4f_t* out_vec4f_);

/**
 * @brief 4次元ベクトルの足し算(out_vec4f_ = vec1_ + vec2_)を実行する
 *
 * @note 以下の場合はエラーメッセージを出力し、何もしない
 * - vec1_ == NULL
 * - vec2_ == NULL
 * - out_vec4f_ == NULL
 *
 * @param[in] vec1_ vec1_ + vec2_のvec1_
 * @param[in] vec2_ vec1_ + vec2_のvec2_
 * @param[out] out_vec4f_ 加算結果ベクトル
 */
void vec4f_add(const vec4f_t* vec1_, const vec4f_t* vec2_, vec4f_t* out_vec4f_);

/**
 * @brief 4行4列の行列の全要素を0にする
 *
 * @note out_mat_ == NULLの場合はエラーメッセージを出力し、何もしない
 *
 * @param[out] out_mat_ 要素を0にする行列
 */
void mat4f_zero(mat4x4f_t* out_mat_);

/**
 * @brief 4行4列の行列を単位行列にする
 *
 * @note out_mat_ == NULLの場合はエラーメッセージを出力し、何もしない
 *
 * @param[out] out_mat_ 単位行列にする行列
 */
void mat4f_identity(mat4x4f_t* out_mat_);

/**
 * @brief 4行4列の行列の掛け算out_mat_ = mat1_ x mat2_を計算する
 *
 * @note 以下の場合はエラーメッセージを出力し、何もしない
 * - mat1_ == NULL
 * - mat2_ == NULL
 * - out_mat_ == NULL
 *
 * @note 行列の要素は全て行優先で格納されていること
 *
 * @param[in] mat1_ mat1_ x mat2_のmat1_
 * @param[in] mat2_ mat1_ x mat2_のmat2_
 * @param[out] out_mat_ 計算結果格納先
 */
void mat4f_mul(const mat4x4f_t* mat1_, const mat4x4f_t* mat2_, mat4x4f_t* out_mat_);

/**
 * @brief 4行4列の行列を転置する
 *
 * @note mat_ == NULLの場合はエラーメッセージを出力し、何もしない
 *
 * @param[in,out] mat_ 転置行列化する行列
 */
void mat4f_transpose(mat4x4f_t* mat_);

/**
 * @brief 4行4列の行列をコピーする
 *
 * @note 以下の場合はエラーメッセージを出力し、何もしない
 * - src_ == NULL
 * - dst_ == NULL
 *
 * @param[in] src_ コピー元行列
 * @param[out] dst_ コピー先行列
 */
void mat4f_copy(const mat4x4f_t* src_, mat4x4f_t* dst_);

/**
 * @brief 行列を逆行列に変換する
 *
 * @note 計算失敗時にはmat_の値は不変
 *
 * @param[in,out] mat_ 逆行列化する行列
 *
 * @retval true 逆行列計算成功
 * @retval false 以下のいずれか
 * - mat_ == NULL
 * - 逆行列が求まらない
 */
bool mat4f_inverse(mat4x4f_t* mat_);

/**
 * @brief out_vec_ = mat_ x vec_を計算する
 *
 * @note 行列の要素は全て行優先で格納されていること
 *
 * @note 以下の場合はエラーメッセージを出力し、何もしない
 * - mat_ == NULL
 * - vec_ == NULL
 * - out_vec_ == NULL
 *
 * @param[in] mat_ 4行4列の行列
 * @param[in] vec_ 4次元ベクトル
 * @param[out] out_vec_ 計算結果格納先
 */
void mat4f_vec4f_mul(const mat4x4f_t* mat_, const vec4f_t* vec_, vec4f_t* out_vec_);

/**
 * @brief 平行移動行列を取得する
 *
 * @note 以下の場合は何もしない
 * - position_ == NULL
 * - mat_ == NULL
 *
 * @param[in] position_ 平行移動量
 * @param[out] mat_ 平行移動行列格納先
 */
void mat4f_translation(const vec3f_t* position_, mat4x4f_t* mat_);

/**
 * @brief X軸周りにradian_回転させる回転行列を取得する
 *
 * @note mat_ == NULLの場合は何もしない
 *
 * @param[in] radian_ X軸周りの回転角度(radian)
 * @param[out] mat_ 回転行列格納先
 */
void mat4f_rot_x(float radian_, mat4x4f_t* mat_);

/**
 * @brief Y軸周りにradian_回転させる回転行列を取得する
 *
 * @note mat_ == NULLの場合は何もしない
 *
 * @param[in] radian_ Y軸周りの回転角度(radian)
 * @param[out] mat_ 回転行列格納先
 */
void mat4f_rot_y(float radian_, mat4x4f_t* mat_);

/**
 * @brief Z軸周りにradian_回転させる回転行列を取得する
 *
 * @note mat_ == NULLの場合は何もしない
 *
 * @param[in] radian_ Z軸周りの回転角度(radian)
 * @param[out] mat_ 回転行列格納先
 */
void mat4f_rot_z(float radian_, mat4x4f_t* mat_);

/**
 * @brief XYZ軸それぞれを回転させる回転行列を取得する
 *
 * @note mat_ == NULLの場合は何もしない
 *
 * @note X, Y, Zの順で回転する
 *
 * @param[in] x_radian_ X軸周りの回転角度(radian)
 * @param[in] y_radian_ Y軸周りの回転角度(radian)
 * @param[in] z_radian_ Z軸周りの回転角度(radian)
 * @param[out] mat_ 回転行列格納先
 */
void mat4f_rot_xyz(float x_radian_, float y_radian_, float z_radian_, mat4x4f_t* mat_);

#ifdef __cplusplus
}
#endif
#endif
