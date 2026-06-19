#include "engine/systems/renderer/resource_pipelines/resource_pipelines_core/resource_pipeline_err_utils.h"

#include "engine/systems/renderer/resource_pipelines/resource_pipelines_core/resource_pipeline_types.h"

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コード(処理成功)文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(引数異常)文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(メモリ破壊, 未初期化)文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< 実行結果コード(計算過程でオーバーフロー発生)文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用可能範囲上限超過)文字列 */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";    /**< 実行結果コード(ファイルオープン失敗)文字列 */
static const char* const s_rslt_str_file_read_error = "FILE_READ_ERROR";    /**< 実行結果コード(ファイル読み込み失敗)文字列 */
static const char* const s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";  /**< 実行結果コード(ファイルクローズ失敗)文字列 */
static const char* const s_rslt_str_unsupported_file = "UNSUPPORTED_FILE";  /**< 実行結果コード(未対応ファイル形式)文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コード(未定義エラー)文字列 */


const char* renderer_pipeline_rslt_to_str(renderer_pipeline_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_PIPELINE_SUCCESS:
        return s_rslt_str_success;
    case RENDERER_PIPELINE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RENDERER_PIPELINE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RENDERER_PIPELINE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RENDERER_PIPELINE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RENDERER_PIPELINE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RENDERER_PIPELINE_OVERFLOW:
        return s_rslt_str_overflow;
    case RENDERER_PIPELINE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RENDERER_PIPELINE_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case RENDERER_PIPELINE_FILE_READ_ERROR:
        return s_rslt_str_file_read_error;
    case RENDERER_PIPELINE_FILE_CLOSE_ERROR:
        return s_rslt_str_file_close_error;
    case RENDERER_PIPELINE_UNSUPPORTED_FILE:
        return s_rslt_str_unsupported_file;
    case RENDERER_PIPELINE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}
