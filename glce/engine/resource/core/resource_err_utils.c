// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file resource_err_utils.c
 * @author chocolate-pie24
 * @brief Resourceレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能の実装
 *
 * @date 2026-05-05
 *
 */
#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/geometry_primitive_types.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_stream.h"

static const char* const s_rslt_str_success = "SUCCESS";                      /**< 実行結果コード文字列: 正常終了 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                  /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";          /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";    /**< 実行結果コード文字列: 引数異常 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";        /**< 実行結果コード文字列: メモリ破壊 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";          /**< 実行結果コード文字列: API誤用、未初期化 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                    /**< 実行結果コード文字列: 計算過程でオーバーフロー発生 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";        /**< 実行結果コード文字列: システム使用可能範囲上限超過 */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";      /**< 実行結果コード文字列: ファイルオープンエラー */
static const char* const s_rslt_str_file_read_error = "FILE_READ_ERROR";      /**< 実行結果コード文字列: ファイル読み込みエラー */
static const char* const s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";    /**< 実行結果コード文字列: ファイルクローズエラー */
static const char* const s_rslt_str_unsupported_file = "UNSUPPORTED_FILE";    /**< 実行結果コード文字列: 未対応のファイル形式エラー */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";      /**< 実行結果コード文字列: 未定義エラー */

const char* resource_rslt_to_str(resource_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_SUCCESS:
        return s_rslt_str_success;
    case RESOURCE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RESOURCE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RESOURCE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RESOURCE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RESOURCE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RESOURCE_OVERFLOW:
        return s_rslt_str_overflow;
    case RESOURCE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RESOURCE_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case RESOURCE_FILE_READ_ERROR:
        return s_rslt_str_file_read_error;
    case RESOURCE_FILE_CLOSE_ERROR:
        return s_rslt_str_file_close_error;
    case RESOURCE_UNSUPPORTED_FILE:
        return s_rslt_str_unsupported_file;
    case RESOURCE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

resource_result_t resource_rslt_convert_choco_memory(memory_system_result_t result_) {
    switch(result_) {
    case MEMORY_SYSTEM_SUCCESS:
        return RESOURCE_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return RESOURCE_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return RESOURCE_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return RESOURCE_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return RESOURCE_NO_MEMORY;
    default:
        return RESOURCE_UNDEFINED_ERROR;
    }
}

resource_result_t resource_rslt_convert_fs_stream(fs_stream_result_t result_) {
    switch(result_) {
    case FS_STREAM_SUCCESS:
        return RESOURCE_SUCCESS;
    case FS_STREAM_INVALID_ARGUMENT:
        return RESOURCE_INVALID_ARGUMENT;
    case FS_STREAM_BAD_OPERATION:
        return RESOURCE_BAD_OPERATION;
    case FS_STREAM_DATA_CORRUPTED:
        return RESOURCE_DATA_CORRUPTED;
    case FS_STREAM_NO_MEMORY:
        return RESOURCE_NO_MEMORY;
    case FS_STREAM_LIMIT_EXCEEDED:
        return RESOURCE_LIMIT_EXCEEDED;
    case FS_STREAM_OVERFLOW:
        return RESOURCE_OVERFLOW;
    case FS_STREAM_FILE_OPEN_ERROR:
        return RESOURCE_FILE_OPEN_ERROR;
    case FS_STREAM_FILE_CLOSE_ERROR:
        return RESOURCE_FILE_CLOSE_ERROR;
    case FS_STREAM_RUNTIME_ERROR:
        return RESOURCE_RUNTIME_ERROR;
    case FS_STREAM_EOF:
        return RESOURCE_RUNTIME_ERROR;  // 基本的にEOFをそのまま伝播させることはないのでとりあえずRUNTIME_ERRORに変換する
    case FS_STREAM_UNDEFINED_ERROR:
        return RESOURCE_UNDEFINED_ERROR;
    default:
        return RESOURCE_UNDEFINED_ERROR;
    }
}

resource_result_t resource_rslt_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return RESOURCE_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return RESOURCE_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return RESOURCE_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return RESOURCE_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return RESOURCE_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return RESOURCE_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return RESOURCE_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return RESOURCE_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return RESOURCE_LIMIT_EXCEEDED;
    default:
        return RESOURCE_UNDEFINED_ERROR;
    }
}

resource_result_t resource_rslt_convert_geometry_primitive(geometry_primitive_result_t rslt_) {
    switch(rslt_) {
    case GEOMETRY_PRIMITIVE_SUCCESS:
        return RESOURCE_SUCCESS;
    case GEOMETRY_PRIMITIVE_INVALID_ARGUMENT:
        return RESOURCE_INVALID_ARGUMENT;
    case GEOMETRY_PRIMITIVE_RUNTIME_ERROR:
        return RESOURCE_RUNTIME_ERROR;
    case GEOMETRY_PRIMITIVE_LIMIT_EXCEEDED:
        return RESOURCE_LIMIT_EXCEEDED;
    case GEOMETRY_PRIMITIVE_BAD_OPERATION:
        return RESOURCE_BAD_OPERATION;
    case GEOMETRY_PRIMITIVE_NO_MEMORY:
        return RESOURCE_NO_MEMORY;
    case GEOMETRY_PRIMITIVE_DATA_CORRUPTED:
        return RESOURCE_DATA_CORRUPTED;
    case GEOMETRY_PRIMITIVE_UNDEFINED_ERROR:
        return RESOURCE_UNDEFINED_ERROR;
    default:
        return RESOURCE_UNDEFINED_ERROR;
    }
}
