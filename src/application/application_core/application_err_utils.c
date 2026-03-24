#include "application/application_core/application_err_utils.h"

#include "application/application_core/application_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/camera_system/camera_core/camera_types.h"

#include "engine/platform/platform_core/platform_types.h"

#include "engine/renderer/renderer_core/renderer_types.h"

static const char* const s_rslt_str_success = "SUCCESS";                    /**< アプリケーション実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< アプリケーション実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< アプリケーション実行結果コード(ランタイムエラー)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< アプリケーション実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< アプリケーション実行結果コード(メモリ破損,未初期化)に対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< アプリケーション実行結果コード(API誤用)に対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< アプリケーション実行結果コード(計算過程でオーバーフロー発生)に対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< アプリケーション実行結果コード(システム使用可能範囲上限超過)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< アプリケーション実行結果コード(未定義エラー)に対応する文字列 */

const char* app_rslt_to_str(application_result_t rslt_) {
    switch(rslt_) {
    case APPLICATION_SUCCESS:
        return s_rslt_str_success;
    case APPLICATION_NO_MEMORY:
        return s_rslt_str_no_memory;
    case APPLICATION_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case APPLICATION_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case APPLICATION_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case APPLICATION_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case APPLICATION_OVERFLOW:
        return s_rslt_str_overflow;
    case APPLICATION_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case APPLICATION_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

application_result_t app_rslt_convert_mem_sys(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return APPLICATION_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case MEMORY_SYSTEM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return APPLICATION_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_platform(platform_result_t rslt_) {
    switch(rslt_) {
    case PLATFORM_SUCCESS:
        return APPLICATION_SUCCESS;
    case PLATFORM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case PLATFORM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case PLATFORM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case PLATFORM_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case PLATFORM_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case PLATFORM_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case PLATFORM_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case PLATFORM_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case PLATFORM_WINDOW_CLOSE:
        return APPLICATION_SUCCESS; // これはエラーではないので、成功扱いにする
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_ring_queue(ring_queue_result_t rslt_) {
    switch(rslt_) {
    case RING_QUEUE_SUCCESS:
        return APPLICATION_SUCCESS;
    case RING_QUEUE_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RING_QUEUE_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RING_QUEUE_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RING_QUEUE_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case RING_QUEUE_EMPTY:
        return APPLICATION_RUNTIME_ERROR;   // リングキュー空はRuntime errorに変換
    case RING_QUEUE_OVERFLOW:
        return APPLICATION_RUNTIME_ERROR;   // オーバーフローもRuntime errorに変換
    case RING_QUEUE_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RING_QUEUE_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_renderer(renderer_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_SUCCESS:
        return APPLICATION_SUCCESS;
    case RENDERER_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RENDERER_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RENDERER_SHADER_COMPILE_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_SHADER_LINK_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RENDERER_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RENDERER_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case RENDERER_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_camera(camera_result_t rslt_) {
    switch(rslt_) {
    case CAMERA_SUCCESS:
        return APPLICATION_SUCCESS;
    case CAMERA_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case CAMERA_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case CAMERA_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case CAMERA_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case CAMERA_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case CAMERA_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case CAMERA_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}
