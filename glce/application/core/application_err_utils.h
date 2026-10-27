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

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/io_utils/fs_path.h"

#include "engine/camera/core/camera_types.h"

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"

#include "engine/systems/platform_system/core/platform_system_types.h"

#include "engine/systems/event_system/core/event_system_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

/**
 * @brief アプリケーションレイヤー実行結果コードを文字列に変換する
 *
 * @param[in] result_ アプリケーションレイヤー実行結果コード
 *
 * @return const char* 変換された文字列
 */
const char* application_result_to_str(application_result_t result_);

application_result_t application_result_convert_subsystem_allocator(subsystem_allocator_result_t result_);

application_result_t application_result_convert_platform_system(platform_system_result_t result_);

/**
 * @brief Ring Queueモジュールの実行結果コードをアプリケーションレイヤー実行結果コードに変換する
 *
 * @param[in] result_ Ring Queueモジュール実行結果コード
 *
 * @return application_result_t 変換されたアプリケーションレイヤー実行結果コード
 */
application_result_t application_result_convert_ring_queue(ring_queue_result_t result_);

application_result_t application_result_convert_camera_registry(camera_registry_result_t result_);

application_result_t application_result_convert_camera(camera_result_t result_);

application_result_t application_result_convert_fs_path(fs_path_result_t result_);

application_result_t application_result_convert_event_system(event_system_result_t result_);

application_result_t application_result_convert_general_allocator(general_allocator_result_t result_);

application_result_t application_result_convert_renderer_frontend(renderer_frontend_result_t result_);

application_result_t application_result_convert_renderer_backend(renderer_backend_result_t result_);

application_result_t application_result_convert_mesh(mesh_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
