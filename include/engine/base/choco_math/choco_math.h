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
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

/**
 * @brief radian_のtangentを計算する
 *
 * @note <math.h>のtanfラッパー
 *
 * @param[in] radian_ tangentを計算するradian値
 * @return float 計算結果
 */
float choco_tanf(float radian_);

/**
 * @brief 2つのfloat値が等しいかを判定する
 *
 * @param[in] a_ 判定値1
 * @param[in] b_ 判定値2
 * @retval true 等しい
 * @retval false 等しくない
 */
bool is_equal_float(float a_, float b_);

/**
 * @brief 2次元ベクトルxyを初期化する
 *
 * @param[in] x_ 初期化値(x)
 * @param[in] y_ 初期化値(y)
 *
 * @return 初期化されたvec2f_t構造体インスタンス
 */
vec2f_t vec2f_initialize(float x_, float y_);

/**
 * @brief 2次元ベクトルの足し算vec1_ + vec2_を実行し、返り値として返す
 *
 * @param[in] vec1_ vec2f_t構造体インスタンス
 * @param[in] vec2_ vec2f_t構造体インスタンス
 *
 * @return 加算結果vec2f_t構造体インスタンス
 */
vec2f_t vec2f_add(vec2f_t vec1_, vec2f_t vec2_);

/**
 * @brief 3次元ベクトルxyzを初期化する
 *
 * @param[in] x_ 初期化値(x)
 * @param[in] y_ 初期化値(y)
 * @param[in] z_ 初期化値(z)
 *
 * @return 初期化されたvec3f_t構造体インスタンス
 */
vec3f_t vec3f_initialize(float x_, float y_, float z_);

/**
 * @brief 3次元ベクトルの足し算vec1_ + vec2_を実行し、返り値として返す
 *
 * @param[in] vec1_ vec3f_t構造体インスタンス
 * @param[in] vec2_ vec3f_t構造体インスタンス
 *
 * @return 加算結果のvec3f_t構造体インスタンス
 */
vec3f_t vec3f_add(vec3f_t vec1_, vec3f_t vec2_);

/**
 * @brief 3次元ベクトルの長さの2乗を返す
 *
 * @param[in] vec_ 計算対象ベクトル
 *
 * @return float 長さの2乗値
 */
float vec3f_length_squared(vec3f_t vec_);

/**
 * @brief 3次元ベクトルの長さを返す
 *
 * @param[in] vec_ 計算対象ベクトル
 *
 * @return float 計算されたベクトルの長さ
 */
float vec3f_length(vec3f_t vec_);

/**
 * @brief 3次元ベクトルを正規化する
 *
 * @note 与えられたベクトルの長さが0の場合はワーニングメッセージを出し何もしない
 *
 * @param[in] vec_ 正規化対象vec3f_t構造体インスタンス
 *
 * @return 正規化されたvec3f_t構造体インスタンス
 */
vec3f_t vec3f_normalize(vec3f_t vec_);

/**
 * @brief 3次元ベクトルの全要素が有限の値かをチェックする
 * 
 * @param[in] vec_ 判定対象vec3f_t構造体インスタンス
 *
 * @return true 全要素が正常
 * @return false 要素にNaN, Infが含まれる
 */
bool vec3f_is_finite(vec3f_t vec_);

/**
 * @brief v1_とv2_の各要素の小さい方を格納したvec3f_tを返す
 * 
 * @param v1_ vec3f_t構造体インスタンス1
 * @param v2_ vec3f_t構造体インスタンス2
 *
 * @return vec3f_t v1_, v2_の各要素の小さい方を格納したvec3f_t
 */
vec3f_t vec3f_component_min(vec3f_t v1_, vec3f_t v2_);

/**
 * @brief v1_とv2_の各要素の大きい方を格納したvec3f_tを返す
 * 
 * @param v1_ vec3f_t構造体インスタンス1
 * @param v2_ vec3f_t構造体インスタンス2
 *
 * @return vec3f_t v1_, v2_の各要素の大きい方を格納したvec3f_t
 */
vec3f_t vec3f_component_max(vec3f_t v1_, vec3f_t v2_);

/**
 * @brief 4次元ベクトルxyzwを初期化する
 *
 * @param[in] x_ 初期化値(x)
 * @param[in] y_ 初期化値(y)
 * @param[in] z_ 初期化値(z)
 * @param[in] w_ 初期化値(w)
 *
 * @return 初期化されたvec4f_t構造体インスタンス
 */
vec4f_t vec4f_initialize(float x_, float y_, float z_, float w_);

/**
 * @brief 4次元ベクトルの足し算vec1_ + vec2_を実行し、返り値として返す
 *
 * @param[in] vec1_ vec4f_t構造体インスタンス
 * @param[in] vec2_ vec4f_t構造体インスタンス
 *
 * @return 加算されたvec4f_t構造体インスタンス
 */
vec4f_t vec4f_add(vec4f_t vec1_, vec4f_t vec2_);

/**
 * @brief 4次元ベクトルrgbaを初期化する
 *
 * @param[in] r_ 初期化値(r)
 * @param[in] g_ 初期化値(g)
 * @param[in] b_ 初期化値(b)
 * @param[in] a_ 初期化値(a)
 *
 * @return 初期化されたvec4u8_t構造体インスタンス
 */
vec4u8_t vec4u8_initialize(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_);

/**
 * @brief 4次元ベクトルxyzwを初期化する
 *
 * @param[in] x_ 初期化値(x)
 * @param[in] y_ 初期化値(y)
 * @param[in] z_ 初期化値(z)
 * @param[in] w_ 初期化値(w)
 *
 * @return 初期化されたvec4i8_t構造体インスタンス
 */
vec4i8_t vec4i8_initialize(int8_t x_, int8_t y_, int8_t z_, int8_t w_);

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
 * @warning mat_ == NULLの場合は0ベクトルを返す
 *
 * @param[in] mat_ 4行4列の行列
 * @param[in] vec_ 4次元ベクトル
 *
 * @return 計算されたvec4f_t構造体インスタンス
 */
vec4f_t mat4f_vec4f_mul(const mat4x4f_t* mat_, vec4f_t vec_);

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
