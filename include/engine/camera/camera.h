/**
 * @file camera.h
 * @author chocolate-pie24
 * @brief カメラモジュールで、以下を提供する
 * - カメラ姿勢変更
 * - カメラ視錐台パラメータ更新
 * - ビュー / プロジェクション行列の取得
 *
 * @version 0.1
 * @date 2026-03-09
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_CAMERA_CAMERA_H
#define GLCE_ENGINE_CAMERA_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/base/choco_math/math_types.h"

typedef struct camera camera_t; /**< カメラ内部状態管理構造体前方宣言 */

/**
 * @brief カメラモジュール実行結果コード定義
 *
 */
typedef enum {
    CAMERA_SUCCESS = 0,         /**< カメラ実行結果コード: 成功 */
    CAMERA_INVALID_ARGUMENT,    /**< カメラ実行結果コード: 無効な引数 */
    CAMERA_RUNTIME_ERROR,       /**< カメラ実行結果コード: 実行時エラー */
    CAMERA_BAD_OPERATION,       /**< カメラ実行結果コード: API誤用 */
    CAMERA_NO_MEMORY,           /**< カメラ実行結果コード: メモリ不足 */
    CAMERA_LIMIT_EXCEEDED,      /**< カメラ実行結果コード: メモリシステム使用可能範囲超過 */
    CAMERA_DATA_CORRUPTED,      /**< カメラ実行結果コード: 内部データ破損 */
    CAMERA_UNDEFINED_ERROR,     /**< カメラ実行結果コード: 不明なエラー */
} camera_result_t;

/**
 * @brief カメラ構造体インスタンスのメモリを確保し、構造体フィールドを0で初期化する
 *
 * @param[in] name_ カメラ名称文字列
 * @param[out] out_camera_ カメラ構造体インスタンスへのダブルポインタ
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - name_ == NULL
 * - out_camera_ == NULL
 * - *out_camera_ != NULL
 * @retval CAMERA_LIMIT_EXCEEDED メモリシステム使用範囲上限超過
 * @retval CAMERA_NO_MEMORY メモリ割り当て失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_create(const char* name_, camera_t** out_camera_);

/**
 * @brief カメラ構造体インスタンスが管理するリソースを破棄し、自身のメモリも破棄する
 *
 * @note 2重デストロイ許可, 本APIを使用した後は*camera_ == NULLとなる
 *
 * @param[out] camera_ カメラ構造体インスタンスへのダブルポインタ
 */
void camera_destroy(camera_t** camera_);

/**
 * @brief カメラ構造体が管理するカメラ名称文字列を取得する
 *
 * @warning 取得した文字列のメモリを解放してはいけない。メモリ解放は必ずcamera_destroyを介して行うこと
 *
 * @note 以下の場合はエラーメッセージを出力し、NULLを返す
 * - camera_ == NULL
 * - camera_->name == NULL
 *
 * @param camera_ カメラ構造体インスタンスへのポインタ
 *
 * @return const char* カメラ名称文字列
 */
const char* camera_name_get(const camera_t* camera_);

/**
 * @brief カメラ構造体が管理する視錐台パラメータを更新(または初期化)する
 *
 * @param[in] fovy_ 画角(degree)
 * @param[in] aspect_ 画面アスペクト比
 * @param[in] near_clip_ 描画範囲(near)
 * @param[in] far_clip_ 描画範囲(far)
 * @param[in,out] camera_ カメラ構造体インスタンスへのポインタ
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - 引数で与えた視錐台パラメータが不正
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_viewing_frustum_update(float fovy_, float aspect_, float near_clip_, float far_clip_, camera_t* camera_);

/**
 * @brief プロジェクション行列として、透視投影変換を行う行列を計算して取得する
 *
 * @note
 * - 座標系: 右手座標系
 * - V = (x, y, z, 1.0)に対して、得られた行列を左からかける
 * - カメラ前方はZ軸マイナス方向
 *
 * @param[in] camera_ カメラ構造体インスタンスへのポインタ
 * @param[out] out_mat_ プロジェクション行列格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_ == NULL
 * - out_mat == NULL
 * @retval CAMERA_BAD_OPERATION camera_が保持する視錐台パラメータが異常
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_perspective_matrix_get(const camera_t* camera_, mat4x4f_t* out_mat_);

/**
 * @brief ビュー行列を計算して取得する
 *
 * @param[in] camera_ カメラ構造体インスタンスへのポインタ
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
camera_result_t camera_view_matrix_get(const camera_t* camera_, mat4x4f_t* out_mat_);

#ifdef __cplusplus
}
#endif
#endif
