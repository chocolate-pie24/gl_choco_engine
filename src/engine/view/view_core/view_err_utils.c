/** @ingroup view
 *
 * @file view_err_utils.c
 * @author chocolate-pie24
 * @brief ビューレイヤー内でのエラー処理仕様を統一する実行結果コード変換機能の実装
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/view/view_core/view_err_utils.h"

#include "engine/view/view_core/view_types.h"

#include "engine/containers/choco_string.h"

#include "engine/core/memory/choco_memory.h"

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コード(成功)文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(無効な引数)文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用範囲上限超過) */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(内部データ破損) */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コード(未定義エラー)文字列 */

const char* view_rslt_to_str(view_result_t rslt_) {
    switch(rslt_) {
    case VIEW_SUCCESS:
        return s_rslt_str_success;
    case VIEW_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case VIEW_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case VIEW_NO_MEMORY:
        return s_rslt_str_no_memory;
    case VIEW_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case VIEW_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case VIEW_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case VIEW_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

view_result_t view_rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return VIEW_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return VIEW_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return VIEW_RUNTIME_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return VIEW_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_NO_MEMORY:
        return VIEW_NO_MEMORY;
    default:
        return VIEW_UNDEFINED_ERROR;
    }
}

view_result_t view_rslt_convert_choco_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return VIEW_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return VIEW_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return VIEW_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return VIEW_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return VIEW_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return VIEW_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return VIEW_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return VIEW_RUNTIME_ERROR;  // OVERFLOW -> RUNTIMEERRORに伝播
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return VIEW_LIMIT_EXCEEDED;
    default:
        return VIEW_UNDEFINED_ERROR;
    }
}
