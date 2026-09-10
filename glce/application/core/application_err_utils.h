// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup application
 *
 * @file application_err_utils.h
 * @author chocolate-pie24
 * @brief アプリケーションレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能を提供する
 *
 * @date 2026-03-25
 *
 */
#ifndef GLCE_APPLICATION_CORE_APPLICATION_ERR_UTILS_H
#define GLCE_APPLICATION_CORE_APPLICATION_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "application/core/application_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"
#include "engine/core/geometry_primitive/geometry_primitive_types.h"

#include "engine/containers/ring_queue.h"

#include "engine/io_utils/fs_path.h"

#include "engine/camera/core/camera_types.h"

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"

#include "engine/resource/core/resource_types.h"

#include "engine/systems/platform/core/platform_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"

// NOTE: engine/systems/renderer/resources/shaders/core/shader_resource_types.hのincludeについて
// Applicationからengine内部headerを直接includeするのは本来layering違反であるが暫定的に許可する。
// 将来Renderer Frontendを導入し、Shaderおよびその内部型をApplicationから隠した時点で削除する予定。
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

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
 * @param[in] rslt_ Choco Memoryモジュール実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_mem_sys(memory_system_result_t rslt_);

/**
 * @brief Linear Allocatorモジュールの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ Linear Allocatorモジュール実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

/**
 * @brief Platformレイヤーの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ Platformレイヤー実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_platform(platform_result_t rslt_);

/**
 * @brief Ring Queueモジュールの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ Ring Queueモジュール実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_ring_queue(ring_queue_result_t rslt_);

application_result_t app_rslt_convert_renderer_backend(renderer_backend_result_t rslt_);

/**
 * @brief Resourceレイヤーの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ Resourceレイヤー実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_resource(resource_result_t rslt_);

/**
 * @brief geometry_primitive保有モジュールの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ geometry_primitive保有モジュール実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t app_rslt_convert_geometry_primitive(geometry_primitive_result_t rslt_);

application_result_t app_rslt_convert_shader(shader_result_t rslt_);

application_result_t app_rslt_convert_camera_registry(camera_registry_result_t rslt_);

application_result_t app_rslt_convert_camera(camera_result_t rslt_);

application_result_t app_rslt_convert_fs_path(fs_path_result_t rslt_);

application_result_t app_rslt_convert_render_resource(render_resource_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
