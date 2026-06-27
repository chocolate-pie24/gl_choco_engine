/** @ingroup renderer
 *
 * @file resource_registry_err_utils.h
 * @author chocolate-pie24
 *
 * @brief resource_registry_err_utilsは、resource_registries内でのエラー処理仕様を統一するため、実行結果コード変換機能を提供する
 *
 * @version 0.1
 * @date 2026-06-20
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_CORE_RESOURCE_REGISTRY_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_CORE_RESOURCE_REGISTRY_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

#include "engine/core/memory/linear_allocator.h"
#include "engine/resource/resource_core/resource_types.h"

/**
 * @brief ログ出力用にresource_registriesの実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ 実行結果コード
 *
 * @return const char* 文字列化された実行結果コード
 */
const char* resource_registry_rslt_to_str(resource_registry_result_t rslt_);

/**
 * @brief 下位モジュールであるlinear_allocatorが出力する実行結果コードをresource_registriesの実行結果コードに変換する
 *
 * @param[in] rslt_ linear_allocatorモジュールが出力する実行結果コード
 *
 * @return resource_registry_result_t 変換されたresource_registriesの実行結果コード
 */
resource_registry_result_t resource_registry_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

/**
 * @brief resourceレイヤーが出力する実行結果コードをresource_registriesの実行結果コードに変換する
 *
 * @param[in] rslt_ resourceレイヤーが出力する実行結果コード
 *
 * @return resource_registry_result_t 変換されたresource_registriesの実行結果コード
 */
resource_registry_result_t resource_registry_rslt_convert_resource(resource_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
