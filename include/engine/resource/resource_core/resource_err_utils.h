/** @ingroup resource
 *
 * @file resource_err_utils.h
 * @author chocolate-pie24
 * @brief Resourceレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能を提供する
 *
 * @version 0.1
 * @date 2026-05-05
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_RESOURCE_CORE_RESOURCE_ERR_UTILS_H
#define GLCE_ENGINE_RESOURCE_RESOURCE_CORE_RESOURCE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/resource/resource_core/resource_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/filesystem/filesystem.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

/**
 * @brief Resourceレイヤー実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ Resourceレイヤー実行結果コード
 *
 * @return const char* 変換された文字列
 */
const char* resource_rslt_to_str(resource_result_t rslt_);

/**
 * @brief choco_memoryモジュールの実行結果コードをResourceレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ choco_memoryモジュール実行結果コード
 *
 * @return resource_result_t 変換されたResourceレイヤー実行結果コード
 */
resource_result_t resource_rslt_convert_choco_memory(memory_system_result_t rslt_);

/**
 * @brief filesystemモジュールの実行結果コードをResourceレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ filesystemモジュール実行結果コード
 *
 * @return resource_result_t 変換されたResourceレイヤー実行結果コード
 */
resource_result_t resource_rslt_convert_filesystem(filesystem_result_t rslt_);

/**
 * @brief fs_utilsモジュールの実行結果コードをResourceレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ fs_utilsモジュール実行結果コード
 *
 * @return resource_result_t 変換されたResourceレイヤー実行結果コード
 */
resource_result_t resource_rslt_convert_fs_utils(fs_utils_result_t rslt_);

/**
 * @brief choco_stringモジュールの実行結果コードをResourceレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ choco_stringモジュール実行結果コード
 *
 * @return resource_result_t 変換されたResourceレイヤー実行結果コード
 */
resource_result_t resource_rslt_convert_choco_string(choco_string_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
