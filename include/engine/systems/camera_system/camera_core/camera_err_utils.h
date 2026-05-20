/** @ingroup camera_system
 *
 * @file camera_err_utils.h
 * @author chocolate-pie24
 * @brief カメラシステムレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能を提供する
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
#ifndef GLCE_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_CORE_CAMERA_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_CAMERA_SYSTEM_CAMERA_CORE_CAMERA_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/camera_system/camera_core/camera_types.h"

#include "engine/containers/choco_string.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

/**
 * @brief カメラシステムレイヤー実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ カメラシステムレイヤー実行結果コード
 *
 * @return const char* 変換された文字列
 */
const char* camera_rslt_to_str(camera_result_t rslt_);

/**
 * @brief choco_memoryモジュールの実行結果コードをカメラシステムレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ choco_memoryモジュール実行結果コード
 *
 * @return camera_result_t 変換されたカメラシステムレイヤー実行結果コード
 */
camera_result_t camera_rslt_convert_choco_memory(memory_system_result_t rslt_);

/**
 * @brief choco_stringモジュールの実行結果コードをカメラシステムレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ choco_stringモジュール実行結果コード
 *
 * @return camera_result_t 変換されたカメラシステムレイヤー実行結果コード
 */
camera_result_t camera_rslt_convert_choco_string(choco_string_result_t rslt_);

/**
 * @brief linear_allocatorモジュールの実行結果コードをカメラシステムレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ linear_allocatorモジュール実行結果コード
 *
 * @return camera_result_t 変換されたカメラシステムレイヤー実行結果コード
 */
camera_result_t camera_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
