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
 * @retval "SUCCESS" 実行結果コード:RESOURCE_SUCCESS(正常終了)
 * @retval "NO_MEMORY" 実行結果コード:RESOURCE_NO_MEMORY(メモリ不足)
 * @retval "RUNTIME_ERROR" 実行結果コード:RESOURCE_RUNTIME_ERROR(実行時エラー)
 * @retval "INVALID_ARGUMENT" 実行結果コード:RESOURCE_INVALID_ARGUMENT(無効な引数)
 * @retval "DATA_CORRUPTED" 実行結果コード:RESOURCE_DATA_CORRUPTED(データ破損)
 * @retval "BAD_OPERATION" 実行結果コード:RESOURCE_BAD_OPERATION(API誤用)
 * @retval "OVERFLOW" 実行結果コード:RESOURCE_OVERFLOW(計算過程でオーバーフロー)
 * @retval "LIMIT_EXCEEDED" 実行結果コード:RESOURCE_LIMIT_EXCEEDED(システム使用可能範囲上限超過)
 * @retval "FILE_OPEN_ERROR" 実行結果コード:RESOURCE_FILE_OPEN_ERROR(ファイルオープンエラー)
 * @retval "FILE_READ_ERROR" 実行結果コード:RESOURCE_FILE_READ_ERROR(ファイル読み込みエラー)
 * @retval "FILE_CLOSE_ERROR" 実行結果コード:RESOURCE_FILE_CLOSE_ERROR(ファイルクローズエラー)
 * @retval "UNSUPPORTED_FILE" 実行結果コード:RESOURCE_UNSUPPORTED_FILE(サポート対象外のリソースファイル)
 * @retval "UNDEFINED_ERROR" 実行結果コード:RESOURCE_UNDEFINED_ERROR(未定義の実行結果コード)
 */
const char* resource_rslt_to_str(resource_result_t rslt_);

/**
 * @brief choco_memoryモジュールの実行結果コードをResourceレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ choco_memoryモジュール実行結果コード
 *
 * @retval RESOURCE_SUCCESS choco_memory実行結果コード:MEMORY_SYSTEM_SUCCESS
 * @retval RESOURCE_INVALID_ARGUMENT choco_memory実行結果コード:MEMORY_SYSTEM_INVALID_ARGUMENT
 * @retval RESOURCE_RUNTIME_ERROR choco_memory実行結果コード:MEMORY_SYSTEM_RUNTIME_ERROR
 * @retval RESOURCE_LIMIT_EXCEEDED choco_memory実行結果コード:MEMORY_SYSTEM_LIMIT_EXCEEDED
 * @retval RESOURCE_BAD_OPERATION choco_memory実行結果コード:MEMORY_SYSTEM_BAD_OPERATION
 * @retval RESOURCE_NO_MEMORY choco_memory実行結果コード:MEMORY_SYSTEM_NO_MEMORY
 * @retval RESOURCE_UNDEFINED_ERROR 未定義のchoco_memory実行結果コード
 */
resource_result_t resource_rslt_convert_choco_memory(memory_system_result_t rslt_);

/**
 * @brief filesystemモジュールの実行結果コードをResourceレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ filesystemモジュール実行結果コード
 *
 * @retval RESOURCE_SUCCESS file_system実行結果コード:FILESYSTEM_SUCCESS
 * @retval RESOURCE_INVALID_ARGUMENT file_system実行結果コード:FILESYSTEM_INVALID_ARGUMENT
 * @retval RESOURCE_RUNTIME_ERROR file_system実行結果コード:FILESYSTEM_RUNTIME_ERROR
 * @retval RESOURCE_NO_MEMORY file_system実行結果コード:FILESYSTEM_NO_MEMORY
 * @retval RESOURCE_FILE_OPEN_ERROR file_system実行結果コード:FILESYSTEM_FILE_OPEN_ERROR
 * @retval RESOURCE_FILE_CLOSE_ERROR file_system実行結果コード:FILESYSTEM_FILE_CLOSE_ERROR
 * @retval RESOURCE_UNDEFINED_ERROR file_system実行結果コード:FILESYSTEM_UNDEFINED_ERROR
 * @retval RESOURCE_LIMIT_EXCEEDED file_system実行結果コード:FILESYSTEM_LIMIT_EXCEEDED
 * @retval RESOURCE_BAD_OPERATION file_system実行結果コード:FILESYSTEM_BAD_OPERATION
 * @retval RESOURCE_FILE_READ_ERROR file_system実行結果コード:FILESYSTEM_EOF
 * @retval RESOURCE_UNDEFINED_ERROR 未定義のfile_system実行結果コード
 */
resource_result_t resource_rslt_convert_filesystem(filesystem_result_t rslt_);

/**
 * @brief fs_utilsモジュールの実行結果コードをResourceレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ fs_utilsモジュール実行結果コード
 *
 * @retval RESOURCE_SUCCESS fs_utils実行結果コード:FS_UTILS_SUCCESS
 * @retval RESOURCE_INVALID_ARGUMENT fs_utils実行結果コード:FS_UTILS_INVALID_ARGUMENT
 * @retval RESOURCE_BAD_OPERATION fs_utils実行結果コード:FS_UTILS_BAD_OPERATION
 * @retval RESOURCE_DATA_CORRUPTED fs_utils実行結果コード:FS_UTILS_DATA_CORRUPTED
 * @retval RESOURCE_NO_MEMORY fs_utils実行結果コード:FS_UTILS_NO_MEMORY
 * @retval RESOURCE_LIMIT_EXCEEDED fs_utils実行結果コード:FS_UTILS_LIMIT_EXCEEDED
 * @retval RESOURCE_OVERFLOW fs_utils実行結果コード:FS_UTILS_OVERFLOW
 * @retval RESOURCE_FILE_OPEN_ERROR fs_utils実行結果コード:FS_UTILS_FILE_OPEN_ERROR
 * @retval RESOURCE_RUNTIME_ERROR fs_utils実行結果コード:FS_UTILS_RUNTIME_ERROR
 * @retval RESOURCE_UNDEFINED_ERROR 以下のいずれか
 * - fs_utils実行結果コード:FS_UTILS_UNDEFINED_ERROR
 * - 未定義のfs_utils実行結果コード
 */
resource_result_t resource_rslt_convert_fs_utils(fs_utils_result_t rslt_);

/**
 * @brief choco_stringモジュールの実行結果コードをResourceレイヤー実行結果コードに変換する
 *
 * @param[in] rslt_ choco_stringモジュール実行結果コード
 *
 * @retval RESOURCE_SUCCESS choco_string実行結果コード:CHOCO_STRING_SUCCESS
 * @retval RESOURCE_DATA_CORRUPTED choco_string実行結果コード:CHOCO_STRING_DATA_CORRUPTED
 * @retval RESOURCE_BAD_OPERATION choco_string実行結果コード:CHOCO_STRING_BAD_OPERATION
 * @retval RESOURCE_NO_MEMORY choco_string実行結果コード:CHOCO_STRING_NO_MEMORY
 * @retval RESOURCE_INVALID_ARGUMENT choco_string実行結果コード:CHOCO_STRING_INVALID_ARGUMENT
 * @retval RESOURCE_RUNTIME_ERROR choco_string実行結果コード:CHOCO_STRING_RUNTIME_ERROR
 * @retval RESOURCE_OVERFLOW choco_string実行結果コード:CHOCO_STRING_OVERFLOW
 * @retval RESOURCE_LIMIT_EXCEEDED choco_string実行結果コード:CHOCO_STRING_LIMIT_EXCEEDED
 * @retval RESOURCE_UNDEFINED_ERROR 以下のいずれか
 * - choco_string実行結果コード:CHOCO_STRING_UNDEFINED_ERROR
 * - 未定義のchoco_string実行結果コード
 */
resource_result_t resource_rslt_convert_choco_string(choco_string_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
