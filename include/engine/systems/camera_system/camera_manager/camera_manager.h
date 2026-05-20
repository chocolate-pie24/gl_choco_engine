/** @ingroup camera_system
 *
 * @file camera_manager.h
 * @author chocolate-pie24
 * @brief カメラ管理システムで、カメラ構造体インスタンスの追加 / 削除 / 取得APIを提供する
 *
 * @version 0.1
 * @date 2026-03-25
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_MANAGER_CAMERA_MANAGER_H
#define GLCE_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_MANAGER_CAMERA_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/core/memory/linear_allocator.h"

#include "engine/systems/camera_system/camera_core/camera_types.h"

typedef struct camera_manager camera_manager_t; /**< カメラ管理システム構造体前方宣言 */
typedef struct camera camera_t;                 /**< カメラ構造体前方宣言 */

#define INVALID_CAMERA_ID (-1)  /**< 無効なカメラ識別子定義 */

/**
 * @brief カメラ管理システムのメモリを確保し、初期化する
 *
 * @warning カメラ管理システムはリニアアロケータを使用したメモリ確保を行うため、本APIの失敗時を含め、リソースの破棄はリニアアロケータの破棄によって行う
 *
 * @param[in] max_camera_count_ カメラ管理システムに登録可能なカメラ数上限値
 * @param[in,out] allocator_ リニアアロケータ
 * @param[out] out_camera_manager_ カメラ管理システム格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - allocator_ == NULL
 * - out_camera_manager_ == NULL
 * - *out_camera_manager_ != NULL
 * - max_camera_count_ <= 0
 * - メモリ割り当てアドレス異常
 * @retval CAMERA_NO_MEMORY メモリ確保に失敗
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_manager_initialize(int16_t max_camera_count_, linear_alloc_t* allocator_, camera_manager_t** out_camera_manager_);

/**
 * @brief カメラ管理システムが管理するカメラ構造体インスタンスを全て削除する
 *
 * @warning カメラ管理システム自身はリニアアロケータによるメモリ確保であるため、自身のリソース破棄は行わない
 *
 * @param[in,out] camera_manager_ 処理対象カメラ管理システム構造体インスタンスへのポインタ
 */
void camera_manager_deinitialize(camera_manager_t* camera_manager_);

/**
 * @brief カメラをカメラ管理システムに追加する
 *
 * @param[in] camera_name_ 追加するカメラの名称
 * @param[in,out] camera_manager_ 追加対象カメラ管理システム
 * @param[out] out_camera_id_ 追加したカメラに付与されるカメラ識別子
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_name_ == NULL
 * - camera_manager_ == NULL
 * - out_camera_id_ == NULL
 * @retval CAMERA_BAD_OPERATION 以下のいずれか
 * - camera_manager_->max_camera_count <= 0
 * - camera_manager_->camera_array == NULL
 * - 追加するカメラ名称が既にカメラ管理システムに登録されている
 * @retval CAMERA_LIMIT_EXCEEDED 以下のいずれか
 * - カメラ管理システムに空き領域がない
 * - メモリ管理システムがメモリ使用量の上限を超過した
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_manager_register(const char* camera_name_, camera_manager_t* camera_manager_, int16_t* out_camera_id_);

/**
 * @brief カメラIDを使用して対応するカメラをカメラ管理システムから削除する
 *
 * @param[in] camera_id_ 削除対象カメラID
 * @param[in,out] camera_manager_ 対象カメラ管理システム構造体インスタンスへのポインタ
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_manager_ == NULL
 * - camera_id_ < 0
 * - camera_manager_->max_camera_count <= camera_id_
 * @retval CAMERA_BAD_OPERATION 以下のいずれか
 * - camera_manager_->max_camera_count <= 0
 * - camera_manager_->camera_array == NULL
 * - カメラIDに対応するカメラが管理システム内に見つからない
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_manager_unregister(int16_t camera_id_, camera_manager_t* camera_manager_);

/**
 * @brief カメラ名称を使用して対応するカメラをカメラ管理システムから削除する
 *
 * @param[in] name_ 削除対象カメラ名称
 * @param[in,out] camera_manager_ 対象カメラ管理システム構造体インスタンスへのポインタ
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_manager_ == NULL
 * - name_ == NULL
 * @retval CAMERA_BAD_OPERATION 以下のいずれか
 * - camera_manager_->max_camera_count <= 0
 * - camera_manager_->camera_array == NULL
 * - カメラ名称に対応するカメラが管理システム内に見つからない
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_manager_unregister_by_name(const char* name_, camera_manager_t* camera_manager_);

/**
 * @brief カメラ名称に対応するカメラ識別子をカメラ管理システムから取得する
 *
 * @param[in] name_ 識別子を取得するカメラの名称
 * @param[in] camera_manager_ 取得元カメラ管理システム構造体インスタンスへのポインタ
 *
 * @retval INVALID_CAMERA_ID以外: カメラ識別子
 * @retval INVALID_CAMERA_ID 以下のいずれか
 * - camera_manager_ == NULL
 * - camera_manager_->max_camera_count <= 0
 * - camera_manager_->camera_array == NULL
 * - name_ == NULL
 * - カメラ名称が管理システム内に見つからない
 */
int16_t camera_manager_camera_id_get(const char* name_, const camera_manager_t* camera_manager_);

/**
 * @brief カメラIDを使用して対応するカメラ管理システムからカメラ構造体インスタンスへのポインタを取得する
 *
 * @param[in] camera_id_ 取得対象カメラ識別子
 * @param[in] camera_manager_ カメラ管理システム構造体インスタンスへのポインタ
 * @param[out] out_camera_ カメラ構造体インスタンスへのポインタ格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_manager_ == NULL
 * - camera_manager_->max_camera_count <= camera_id_
 * - out_camera_ == NULL
 * - camera_id_ < 0
 * @retval CAMERA_BAD_OPERATION 以下のいずれか
 * - camera_manager_->max_camera_count <= 0
 * - camera_manager_->camera_array == NULL
 * - camera_id_に対応するカメラが管理システム内に見つからない
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_manager_camera_get(int16_t camera_id_, const camera_manager_t* camera_manager_, camera_t** out_camera_);

/**
 * @brief カメラ管理システムからカメラ名称をもとにカメラ構造体インスタンスへのポインタを取得する
 *
 * @param[in] name_ カメラ名称文字列
 * @param[in] camera_manager_ カメラ管理システム構造体インスタンスへのポインタ
 * @param[out] out_camera_ カメラ構造体インスタンスへのポインタ格納先
 *
 * @retval CAMERA_INVALID_ARGUMENT 以下のいずれか
 * - camera_manager_ == NULL
 * - name_ == NULL
 * - out_camera_ == NULL
 * @retval CAMERA_BAD_OPERATION
 * - camera_manager_->max_camera_count <= 0
 * - camera_manager_->camera_array == NULL
 * - name_に対応するカメラが管理システム内に見つからない
 * @retval CAMERA_SUCCESS 処理に成功し、正常終了
 */
camera_result_t camera_manager_camera_get_by_name(const char* name_, const camera_manager_t* camera_manager_, camera_t** out_camera_);

#ifdef __cplusplus
}
#endif
#endif
