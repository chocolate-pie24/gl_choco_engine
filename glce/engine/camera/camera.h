// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_CAMERA_CAMERA_H
#define GLCE_ENGINE_CAMERA_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/camera/core/camera_types.h"

#include "engine/base/choco_math/math_types.h"

typedef struct camera camera_t; /**< カメラ内部状態管理構造体前方宣言 */

camera_result_t camera_create(float fovy_, float aspect_, float near_clip_, float far_clip_, camera_t** out_camera_);

/**
 * @brief カメラ構造体インスタンスが管理するリソースを破棄し、自身のメモリも破棄する
 *
 * @note 2重デストロイ許可, 本APIを使用した後は*camera_ == NULLとなる
 *
 * @param[out] camera_ カメラ構造体インスタンスへのダブルポインタ
 */
void camera_destroy(camera_t** camera_);

/**
 * @brief カメラ構造体が管理する視錐台パラメータを更新(または初期化)する
 *
 * @param[in,out] camera_ カメラ構造体インスタンスへのポインタ
 * @param[in] fovy_ 画角(degree)
 * @param[in] aspect_ 画面アスペクト比
 * @param[in] near_clip_ 描画範囲(near)
 * @param[in] far_clip_ 描画範囲(far)
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - 引数で与えた視錐台パラメータが不正
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_viewing_frustum_update(camera_t* camera_, float fovy_, float aspect_, float near_clip_, float far_clip_);

/**
 * @brief カメラ姿勢情報を更新する
 *
 * @param[out] camera_ 姿勢更新対象カメラ構造体インスタンスへのポインタ
 * @param[in] euler_ カメラ姿勢ベクトル構造体インスタンス
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_euler_update(camera_t* camera_, vec3f_t euler_);

/**
 * @brief カメラ位置情報を更新する
 *
 * @param[out] camera_ 位置更新対象カメラ構造体インスタンスへのポインタ
 * @param[in] position_ カメラ位置ベクトル構造体インスタンス
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_position_update(camera_t* camera_, vec3f_t position_);

/**
 * @brief カメラ姿勢情報を取得する
 *
 * @param[in] camera_ 姿勢情報取得対象カメラ構造体インスタンスへのポインタ
 * @param[out] out_euler_ 姿勢情報格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_euler_ == NULL
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_euler_get(const camera_t* camera_, vec3f_t* out_euler_);

/**
 * @brief カメラ位置情報を取得する
 *
 * @param[in] camera_ 位置情報取得対象カメラ構造体インスタンスへのポインタ
 * @param[out] out_position_ 位置情報格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_position_ == NULL
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_position_get(const camera_t* camera_, vec3f_t* out_position_);

/**
 * @brief プロジェクション行列として、透視投影変換を行う行列を計算して取得する
 *
 * @note
 * - 座標系: 右手座標系
 * - V = (x, y, z, 1.0)に対して、得られた行列を左からかける
 * - カメラ前方はZ軸マイナス方向
 *
 * @note カメラ位置、姿勢情報とプロジェクション行列の同期が取れていない場合は同期処理を行うため、カメラ構造体のフィールドが更新される
 *
 * @param[in,out] camera_ カメラ構造体インスタンスへのポインタ
 * @param[out] out_mat_ プロジェクション行列格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_mat == NULL
 * @retval CAMERA_BAD_OPERATION camera_が保持する視錐台パラメータが異常
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_perspective_matrix_get(camera_t* camera_, mat4x4f_t* out_mat_);

/**
 * @brief ビュー行列を計算して取得する
 *
 * @note カメラ位置、姿勢情報とビュー行列の同期が取れていない場合は同期処理を行うため、カメラ構造体のフィールドが更新される
 *
 * @param[in,out] camera_ カメラ構造体インスタンスへのポインタ
 * @param[out] out_mat_ ビュー行列格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_mat_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 逆行列が求まらない
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 *
 * @todo 逆行列計算をなくすため、以下を行う
 * - 回転行列の逆行列 = 転置行列の性質を利用する
 * - 平行移動行列の逆行列は平行移動量 x (-1)の性質を利用する
 */
camera_result_t camera_view_matrix_get(camera_t* camera_, mat4x4f_t* out_mat_);

/**
 * @brief カメラ前方の正規化されたベクトルを返す
 *
 * @note カメラ前方: Z軸マイナス側
 *
 * @note カメラ位置、姿勢情報とビュー行列の同期が取れていない場合は同期処理を行うため、カメラ構造体のフィールドが更新される
 *
 * @param[in,out] camera_ 取得対象カメラ構造体インスタンスのポインタ
 * @param[out] out_vec_ ベクトル格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_vec_ == NULL
 * @retval CAMERA_RUNTIME_ERROR ビュー行列更新の際の逆行列計算に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_forward_vector_get(camera_t* camera_, vec3f_t* out_vec_);

/**
 * @brief カメラ後方の正規化されたベクトルを返す
 *
 * @note カメラ後方: Z軸プラス側
 *
 * @note カメラ位置、姿勢情報とビュー行列の同期が取れていない場合は同期処理を行うため、カメラ構造体のフィールドが更新される
 *
 * @param[in,out] camera_ 取得対象カメラ構造体インスタンスのポインタ
 * @param[out] out_vec_ ベクトル格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_vec_ == NULL
 * @retval CAMERA_RUNTIME_ERROR ビュー行列更新の際の逆行列計算に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_backward_vector_get(camera_t* camera_, vec3f_t* out_vec_);

/**
 * @brief カメラ右方向の正規化されたベクトルを返す
 *
 * @note カメラ右方向: X軸プラス側
 *
 * @note カメラ位置、姿勢情報とビュー行列の同期が取れていない場合は同期処理を行うため、カメラ構造体のフィールドが更新される
 *
 * @param[in,out] camera_ 取得対象カメラ構造体インスタンスのポインタ
 * @param[out] out_vec_ ベクトル格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_vec_ == NULL
 * @retval CAMERA_RUNTIME_ERROR ビュー行列更新の際の逆行列計算に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_right_vector_get(camera_t* camera_, vec3f_t* out_vec_);

/**
 * @brief カメラ左方向の正規化されたベクトルを返す
 *
 * @note カメラ左方向: X軸マイナス側
 *
 * @note カメラ位置、姿勢情報とビュー行列の同期が取れていない場合は同期処理を行うため、カメラ構造体のフィールドが更新される
 *
 * @param[in,out] camera_ 取得対象カメラ構造体インスタンスのポインタ
 * @param[out] out_vec_ ベクトル格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_vec_ == NULL
 * @retval CAMERA_RUNTIME_ERROR ビュー行列更新の際の逆行列計算に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_left_vector_get(camera_t* camera_, vec3f_t* out_vec_);

/**
 * @brief カメラ上方向の正規化されたベクトルを返す
 *
 * @note カメラ上方向: Y軸プラス側
 *
 * @note カメラ位置、姿勢情報とビュー行列の同期が取れていない場合は同期処理を行うため、カメラ構造体のフィールドが更新される
 *
 * @param[in,out] camera_ 取得対象カメラ構造体インスタンスのポインタ
 * @param[out] out_vec_ ベクトル格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_vec_ == NULL
 * @retval CAMERA_RUNTIME_ERROR ビュー行列更新の際の逆行列計算に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_up_vector_get(camera_t* camera_, vec3f_t* out_vec_);

/**
 * @brief カメラ下方向の正規化されたベクトルを返す
 *
 * @note カメラ下方向: Y軸マイナス方向
 *
 * @note カメラ位置、姿勢情報とビュー行列の同期が取れていない場合は同期処理を行うため、カメラ構造体のフィールドが更新される
 *
 * @param[in,out] camera_ 取得対象カメラ構造体インスタンスのポインタ
 * @param[out] out_vec_ ベクトル格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_vec_ == NULL
 * @retval CAMERA_RUNTIME_ERROR ビュー行列更新の際の逆行列計算に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_down_vector_get(camera_t* camera_, vec3f_t* out_vec_);

bool camera_is_valid(const camera_t* camera_);

#ifdef __cplusplus
}
#endif
#endif
