/** @ingroup application
 *
 * @file application_err_utils.h
 * @author chocolate-pie24
 * @brief アプリケーションレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能を提供する
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
#ifndef GLCE_APPLICATION_APPLICATION_CORE_APPLICATION_ERR_UTILS_H
#define GLCE_APPLICATION_APPLICATION_CORE_APPLICATION_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "application/application_core/application_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/camera_system/camera_core/camera_types.h"

#include "engine/platform/platform_core/platform_types.h"

#include "engine/renderer/renderer_core/renderer_types.h"

/**
 * @brief アプリケーションレイヤー実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ アプリケーションレイヤー実行結果コード
 *
 * @return const char* 変換された文字列
 */
const char* app_rslt_to_str(application_result_t rslt_);

/**
 * @brief Choco Memoryモジュールの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param rslt_ Choco Memoryモジュール実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_mem_sys(memory_system_result_t rslt_);

/**
 * @brief Linear Allocatorモジュールの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param rslt_ Linear Allocatorモジュール実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

/**
 * @brief Platformレイヤーの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param rslt_ Platformレイヤー実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_platform(platform_result_t rslt_);

/**
 * @brief Ring Queueモジュールの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param rslt_ Ring Queueモジュール実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_ring_queue(ring_queue_result_t rslt_);

/**
 * @brief Rendererレイヤーの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ camera_systemレイヤー実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_renderer(renderer_result_t rslt_);

/**
 * @brief Camera Systemレイヤーの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ Camera Systemレイヤー実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_camera(camera_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
