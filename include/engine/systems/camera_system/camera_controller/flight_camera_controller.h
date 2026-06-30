/**
 * @ingroup camera_system
 * @file flight_camera_controller.h
 * @author chocolate-pie24
 * @brief 空間内を自由に移動する飛行型カメラの制御機能を提供する
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_CONTROLLER_FLIGHT_CAMERA_CONTROLLER_H
#define GLCE_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_CONTROLLER_FLIGHT_CAMERA_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/camera_system/camera_core/camera_types.h"

typedef struct camera camera_t; /**< カメラ状態管理構造体のopaque型 */

/**
 * @brief フライトカメラを前方に動かす
 *
 * @param[in,out] camera_ 移動対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 移動速度
 * @param[in] delta_time_ 移動時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 以下のいずれか
 * - 逆行列計算に失敗
 * - カメラ位置の取得 / 更新に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_move_forward(camera_t* camera_, float speed_, float delta_time_);

/**
 * @brief フライトカメラを後方に動かす
 *
 * @param[in,out] camera_ 移動対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 移動速度
 * @param[in] delta_time_ 移動時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 以下のいずれか
 * - 逆行列計算に失敗
 * - カメラ位置の取得 / 更新に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_move_backward(camera_t* camera_, float speed_, float delta_time_);

/**
 * @brief フライトカメラを右方向に動かす
 *
 * @param[in,out] camera_ 移動対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 移動速度
 * @param[in] delta_time_ 移動時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 以下のいずれか
 * - 逆行列計算に失敗
 * - カメラ位置の取得 / 更新に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_move_right(camera_t* camera_, float speed_, float delta_time_);

/**
 * @brief フライトカメラを左方向に動かす
 *
 * @param[in,out] camera_ 移動対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 移動速度
 * @param[in] delta_time_ 移動時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 以下のいずれか
 * - 逆行列計算に失敗
 * - カメラ位置の取得 / 更新に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_move_left(camera_t* camera_, float speed_, float delta_time_);

/**
 * @brief フライトカメラを上方向に動かす
 *
 * @param[in,out] camera_ 移動対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 移動速度
 * @param[in] delta_time_ 移動時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 以下のいずれか
 * - 逆行列計算に失敗
 * - カメラ位置の取得 / 更新に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_move_up(camera_t* camera_, float speed_, float delta_time_);

/**
 * @brief フライトカメラを下方向に動かす
 *
 * @param[in,out] camera_ 移動対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 移動速度
 * @param[in] delta_time_ 移動時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_RUNTIME_ERROR 以下のいずれか
 * - 逆行列計算に失敗
 * - カメラ位置の取得 / 更新に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_move_down(camera_t* camera_, float speed_, float delta_time_);

/**
 * @brief フライトカメラをピッチ+方向に回転する
 *
 * @param[in,out] camera_ 回転対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 回転速度
 * @param[in] delta_time_ 回転時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_rot_pitch_plus(camera_t* camera_, float speed_, float delta_time_);

/**
 * @brief フライトカメラをピッチ-方向に回転する
 *
 * @param[in,out] camera_ 回転対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 回転速度
 * @param[in] delta_time_ 回転時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_rot_pitch_minus(camera_t* camera_, float speed_, float delta_time_);

/**
 * @brief フライトカメラをヨー+方向に回転する
 *
 * @param[in,out] camera_ 回転対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 回転速度
 * @param[in] delta_time_ 回転時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_rot_yaw_plus(camera_t* camera_, float speed_, float delta_time_);

/**
 * @brief フライトカメラをヨー-方向に回転する
 *
 * @param[in,out] camera_ 回転対象カメラ構造体インスタンスへのポインタ
 * @param[in] speed_ 回転速度
 * @param[in] delta_time_ 回転時間
 *
 * @retval CAMERA_INVALID_ARGUMENT camera_ == NULL
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t flight_camera_controller_rot_yaw_minus(camera_t* camera_, float speed_, float delta_time_);

#ifdef __cplusplus
}
#endif
#endif
