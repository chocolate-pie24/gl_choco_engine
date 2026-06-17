#include "engine/systems/geometry_system/geometry_system_core/geometry_system_err_utils.h"

#include "engine/systems/geometry_system/geometry_system_core/geometry_system_types.h"

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コード(成功)文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(無効な引数)文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用可能範囲上限超過)文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(内部データ破損)文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";    /**< 実行結果コード(ファイルオープンエラー)文字列 */
static const char* const s_rslt_str_file_read_error = "FILE_READ_ERROR";    /**< 実行結果コード(ファイル読み込みエラー)文字列 */
static const char* const s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";  /**< 実行結果コード(ファイルクローズエラー)文字列 */
static const char* const s_rslt_str_unsupported_file = "UNSUPPORTED_FILE";  /**< 実行結果コード(サポート外のファイルタイプ)文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 一向結果コード(未定義エラー)文字列 */

const char* geometry_system_rslt_to_str(geometry_system_result_t rslt_) {
    switch(rslt_) {
    case GEOMETRY_SYSTEM_SUCCESS:
        return s_rslt_str_success;
    case GEOMETRY_SYSTEM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case GEOMETRY_SYSTEM_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case GEOMETRY_SYSTEM_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case GEOMETRY_SYSTEM_NO_MEMORY:
        return s_rslt_str_no_memory;
    case GEOMETRY_SYSTEM_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case GEOMETRY_SYSTEM_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case GEOMETRY_SYSTEM_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case GEOMETRY_SYSTEM_FILE_READ_ERROR:
        return s_rslt_str_file_read_error;
    case GEOMETRY_SYSTEM_FILE_CLOSE_ERROR:
        return s_rslt_str_file_close_error;
    case GEOMETRY_SYSTEM_UNSUPPORTED_FILE:
        return s_rslt_str_unsupported_file;
    case GEOMETRY_SYSTEM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

geometry_system_result_t geometry_system_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return GEOMETRY_SYSTEM_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return GEOMETRY_SYSTEM_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return GEOMETRY_SYSTEM_INVALID_ARGUMENT;
    default:
        return GEOMETRY_SYSTEM_UNDEFINED_ERROR;
    }
}