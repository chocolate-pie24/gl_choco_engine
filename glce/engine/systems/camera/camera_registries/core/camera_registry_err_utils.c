// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/camera/camera_registries/core/camera_registry_err_utils.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/camera/core/camera_types.h"

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コード(処理成功)文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(引数異常)文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(メモリ破壊, 未初期化)文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< 実行結果コード(計算過程でオーバーフロー発生)文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用可能範囲上限超過)文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コード(未定義エラー)文字列 */

const char* camera_registry_rslt_to_str(camera_registry_result_t rslt_) {
    switch(rslt_) {
    case CAMERA_REGISTRY_SUCCESS:
        return s_rslt_str_success;
    case CAMERA_REGISTRY_NO_MEMORY:
        return s_rslt_str_no_memory;
    case CAMERA_REGISTRY_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case CAMERA_REGISTRY_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case CAMERA_REGISTRY_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case CAMERA_REGISTRY_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case CAMERA_REGISTRY_OVERFLOW:
        return s_rslt_str_overflow;
    case CAMERA_REGISTRY_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case CAMERA_REGISTRY_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

camera_registry_result_t camera_registry_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return CAMERA_REGISTRY_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return CAMERA_REGISTRY_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return CAMERA_REGISTRY_INVALID_ARGUMENT;
    default:
        return CAMERA_REGISTRY_UNDEFINED_ERROR;
    }
}

camera_registry_result_t camera_registry_rslt_convert_choco_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return CAMERA_REGISTRY_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return CAMERA_REGISTRY_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return CAMERA_REGISTRY_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return CAMERA_REGISTRY_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return CAMERA_REGISTRY_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return CAMERA_REGISTRY_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return CAMERA_REGISTRY_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return CAMERA_REGISTRY_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return CAMERA_REGISTRY_LIMIT_EXCEEDED;
    default:
        return CAMERA_REGISTRY_UNDEFINED_ERROR;
    }
}
