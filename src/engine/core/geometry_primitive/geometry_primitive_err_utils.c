/** @ingroup core
 *
 * @file geometry_primitive_err_utils.c
 * @author chocolate-pie24
 * @brief geometry_primitive内でのエラー処理仕様を統一するため、実行結果コード変換機能の実装
 *
 * @version 0.1
 * @date 2026-06-09
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/core/geometry_primitive/geometry_primitive_err_utils.h"
#include "engine/core/geometry_primitive/geometry_primitive_types.h"

static const char* const s_rslt_str_success = "SUCCESS";                        /**< 実行結果コード文字列: 正常終了 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";      /**< 実行結果コード文字列: 引数異常 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";            /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";          /**< 実行結果コード文字列: システム使用可能範囲上限超過 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";            /**< 実行結果コード文字列: API誤用 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                    /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";          /**< 実行結果コード文字列: 幾何データ不正 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";        /**< 実行結果コード文字列: 不明なエラー */

const char* geometry_primitive_rslt_to_str(geometry_primitive_result_t rslt_) {
    switch(rslt_) {
    case GEOMETRY_PRIMITIVE_SUCCESS:
        return s_rslt_str_success;
    case GEOMETRY_PRIMITIVE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case GEOMETRY_PRIMITIVE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case GEOMETRY_PRIMITIVE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case GEOMETRY_PRIMITIVE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case GEOMETRY_PRIMITIVE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case GEOMETRY_PRIMITIVE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case GEOMETRY_PRIMITIVE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}
