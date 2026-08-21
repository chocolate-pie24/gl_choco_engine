#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

#include "engine/resource/core/resource_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

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

const char* resource_pipeline_rslt_to_str(resource_pipeline_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_PIPELINE_SUCCESS:
        return s_rslt_str_success;
    case RESOURCE_PIPELINE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RESOURCE_PIPELINE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RESOURCE_PIPELINE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RESOURCE_PIPELINE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RESOURCE_PIPELINE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RESOURCE_PIPELINE_OVERFLOW:
        return s_rslt_str_overflow;
    case RESOURCE_PIPELINE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RESOURCE_PIPELINE_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case RESOURCE_PIPELINE_FILE_READ_ERROR:
        return s_rslt_str_file_read_error;
    case RESOURCE_PIPELINE_FILE_CLOSE_ERROR:
        return s_rslt_str_file_close_error;
    case RESOURCE_PIPELINE_UNSUPPORTED_FILE:
        return s_rslt_str_unsupported_file;
    case RESOURCE_PIPELINE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

resource_pipeline_result_t resource_pipeline_rslt_convert_resource(resource_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case RESOURCE_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case RESOURCE_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case RESOURCE_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case RESOURCE_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case RESOURCE_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case RESOURCE_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case RESOURCE_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case RESOURCE_FILE_OPEN_ERROR:
        return RESOURCE_PIPELINE_FILE_OPEN_ERROR;
    case RESOURCE_FILE_READ_ERROR:
        return RESOURCE_PIPELINE_FILE_READ_ERROR;
    case RESOURCE_FILE_CLOSE_ERROR:
        return RESOURCE_PIPELINE_FILE_CLOSE_ERROR;
    case RESOURCE_UNSUPPORTED_FILE:
        return RESOURCE_PIPELINE_UNSUPPORTED_FILE;
    case RESOURCE_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

// TODO: シェーダーリソースはパイプラインでは扱わない(シェーダーは各プログラムにつき1個のため、registryを作るまでもない、であればpipelineも作るまでもない)
resource_pipeline_result_t resource_pipeline_rslt_convert_renderer_backend(renderer_backend_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_BACKEND_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR; // Resource Pipelineではシェーダーのコンパイル, リンクを行わないためUNDEFINED_ERRORに変換
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR; // Resource Pipelineではシェーダーのコンパイル, リンクを行わないためUNDEFINED_ERRORに変換
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_rslt_convert_resource_registry(resource_registry_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_REGISTRY_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case RESOURCE_REGISTRY_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case RESOURCE_REGISTRY_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case RESOURCE_REGISTRY_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case RESOURCE_REGISTRY_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case RESOURCE_REGISTRY_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case RESOURCE_REGISTRY_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case RESOURCE_REGISTRY_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case RESOURCE_REGISTRY_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_rslt_convert_shader(shader_result_t rslt_) {
    switch(rslt_) {
    case SHADER_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case SHADER_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case SHADER_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case SHADER_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case SHADER_COMPILE_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR; // Resource Pipelineではシェーダーのコンパイル, リンクを行わないためUNDEFINED_ERRORに変換
    case SHADER_LINK_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR; // Resource Pipelineではシェーダーのコンパイル, リンクを行わないためUNDEFINED_ERRORに変換
    case SHADER_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case SHADER_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case SHADER_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case SHADER_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case SHADER_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_rslt_convert_choco_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_rslt_convert_fs_utils(fs_utils_result_t rslt_) {
    switch(rslt_) {
    case FS_UTILS_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case FS_UTILS_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case FS_UTILS_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case FS_UTILS_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case FS_UTILS_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case FS_UTILS_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case FS_UTILS_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case FS_UTILS_FILE_OPEN_ERROR:
        return RESOURCE_PIPELINE_FILE_OPEN_ERROR;
    case FS_UTILS_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case FS_UTILS_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    case FS_UTILS_EOF:
        return RESOURCE_PIPELINE_FILE_READ_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_rslt_convert_texture_gpu_resource(texture_gpu_resource_result_t rslt_) {
    switch(rslt_) {
    case TEXTURE_GPU_RESOURCE_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case TEXTURE_GPU_RESOURCE_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case TEXTURE_GPU_RESOURCE_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case TEXTURE_GPU_RESOURCE_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case TEXTURE_GPU_RESOURCE_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case TEXTURE_GPU_RESOURCE_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case TEXTURE_GPU_RESOURCE_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case TEXTURE_GPU_RESOURCE_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case TEXTURE_GPU_RESOURCE_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}
