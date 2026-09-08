// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup application
 *
 * @file application_err_utils.c
 * @author chocolate-pie24
 * @brief アプリケーションレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能の実装
 *
 * @date 2026-03-25
 *
 */
#include "application/core/application_err_utils.h"

#include "application/core/application_types.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/systems/platform/core/platform_types.h"

#include "engine/resource/core/resource_types.h"

static const char* const s_rslt_str_success = "SUCCESS";                    /**< アプリケーション実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< アプリケーション実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< アプリケーション実行結果コード(ランタイムエラー)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< アプリケーション実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< アプリケーション実行結果コード(メモリ破損,未初期化)に対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< アプリケーション実行結果コード(API誤用)に対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< アプリケーション実行結果コード(計算過程でオーバーフロー発生)に対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< アプリケーション実行結果コード(システム使用可能範囲上限超過)に対応する文字列 */
static const char* const s_rslt_str_unsupported_file = "UNSUPPORTED_FILE";  /**< アプリケーション実行結果コード(未対応のファイル形式)に対応する文字列 */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";    /**< アプリケーション実行結果コード(ファイルオープンエラー)に対応する文字列 */
static const char* const s_rslt_str_file_read_error = "FILE_READ_ERROR";    /**< アプリケーション実行結果コード(ファイル読み込みエラー)に対応する文字列 */
static const char* const s_rslt_str_window_close = "WINDOW_CLOSE";
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
    case APPLICATION_UNSUPPORTED_FILE:
        return s_rslt_str_unsupported_file;
    case APPLICATION_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case APPLICATION_FILE_READ_ERROR:
        return s_rslt_str_file_read_error;
    case APPLICATION_WINDOW_CLOSE:
        return s_rslt_str_window_close;
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
    case MEMORY_SYSTEM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
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
        return APPLICATION_WINDOW_CLOSE;
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
    case RING_QUEUE_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RING_QUEUE_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_renderer_backend(renderer_backend_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_BACKEND_SUCCESS:
        return APPLICATION_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_resource(resource_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_SUCCESS:
        return APPLICATION_SUCCESS;
    case RESOURCE_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RESOURCE_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RESOURCE_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RESOURCE_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case RESOURCE_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RESOURCE_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case RESOURCE_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RESOURCE_FILE_OPEN_ERROR:
        return APPLICATION_FILE_OPEN_ERROR;
    case RESOURCE_FILE_READ_ERROR:
        return APPLICATION_FILE_READ_ERROR;
    case RESOURCE_UNSUPPORTED_FILE:
        return APPLICATION_UNSUPPORTED_FILE;
    case RESOURCE_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_geometry_primitive(geometry_primitive_result_t rslt_) {
    switch(rslt_) {
    case GEOMETRY_PRIMITIVE_SUCCESS:
        return APPLICATION_SUCCESS;
    case GEOMETRY_PRIMITIVE_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case GEOMETRY_PRIMITIVE_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case GEOMETRY_PRIMITIVE_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case GEOMETRY_PRIMITIVE_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case GEOMETRY_PRIMITIVE_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case GEOMETRY_PRIMITIVE_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case GEOMETRY_PRIMITIVE_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_shader(shader_result_t rslt_) {
    switch(rslt_) {
    case SHADER_SUCCESS:
        return APPLICATION_SUCCESS;
    case SHADER_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case SHADER_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case SHADER_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case SHADER_COMPILE_ERROR:
        return APPLICATION_UNDEFINED_ERROR;   // Applicationではシェーダーのコンパイル, リンクを行わないのでUNDEFINED_ERRORに変換
    case SHADER_LINK_ERROR:
        return APPLICATION_UNDEFINED_ERROR;   // Applicationではシェーダーのコンパイル, リンクを行わないのでUNDEFINED_ERRORに変換
    case SHADER_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case SHADER_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case SHADER_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case SHADER_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case SHADER_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t app_rslt_convert_camera_registry(camera_registry_result_t rslt_) {
    switch(rslt_) {
    case CAMERA_REGISTRY_SUCCESS:
        return APPLICATION_SUCCESS;
    case CAMERA_REGISTRY_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case CAMERA_REGISTRY_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case CAMERA_REGISTRY_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case CAMERA_REGISTRY_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case CAMERA_REGISTRY_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case CAMERA_REGISTRY_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case CAMERA_REGISTRY_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case CAMERA_REGISTRY_UNDEFINED_ERROR:
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
    case CAMERA_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case CAMERA_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case CAMERA_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case CAMERA_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case CAMERA_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}
