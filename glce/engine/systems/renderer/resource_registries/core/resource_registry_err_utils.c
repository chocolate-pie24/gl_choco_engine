// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file resource_registry_err_utils.c
 * @author chocolate-pie24
 *
 * @brief resource_registry_err_utilsは、resource_registries内でのエラー処理仕様を統一するため、実行結果コード変換機能の実装
 *
 * @date 2026-06-20
 *
 */
#include "engine/systems/renderer/resource_registries/core/resource_registry_err_utils.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/core/resource_types.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コード(処理成功)文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(引数異常)文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(メモリ破壊, 未初期化)文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< 実行結果コード(計算過程でオーバーフロー発生)文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用可能範囲上限超過)文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コード(未定義エラー)文字列 */

const char* resource_registry_rslt_to_str(resource_registry_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_REGISTRY_SUCCESS:
        return s_rslt_str_success;
    case RESOURCE_REGISTRY_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RESOURCE_REGISTRY_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RESOURCE_REGISTRY_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RESOURCE_REGISTRY_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RESOURCE_REGISTRY_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RESOURCE_REGISTRY_OVERFLOW:
        return s_rslt_str_overflow;
    case RESOURCE_REGISTRY_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RESOURCE_REGISTRY_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

resource_registry_result_t resource_registry_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return RESOURCE_REGISTRY_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return RESOURCE_REGISTRY_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return RESOURCE_REGISTRY_INVALID_ARGUMENT;
    default:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;
    }
}

resource_registry_result_t resource_registry_rslt_convert_resource(resource_result_t rslt_) {
    switch(rslt_) {
    case RESOURCE_SUCCESS:
        return RESOURCE_REGISTRY_SUCCESS;
    case RESOURCE_NO_MEMORY:
        return RESOURCE_REGISTRY_NO_MEMORY;
    case RESOURCE_RUNTIME_ERROR:
        return RESOURCE_REGISTRY_RUNTIME_ERROR;
    case RESOURCE_INVALID_ARGUMENT:
        return RESOURCE_REGISTRY_INVALID_ARGUMENT;
    case RESOURCE_DATA_CORRUPTED:
        return RESOURCE_REGISTRY_DATA_CORRUPTED;
    case RESOURCE_BAD_OPERATION:
        return RESOURCE_REGISTRY_BAD_OPERATION;
    case RESOURCE_OVERFLOW:
        return RESOURCE_REGISTRY_OVERFLOW;
    case RESOURCE_LIMIT_EXCEEDED:
        return RESOURCE_REGISTRY_LIMIT_EXCEEDED;
    case RESOURCE_FILE_OPEN_ERROR:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;   // registryではi/oを扱わないため、i/oエラーは起こり得ないはず
    case RESOURCE_FILE_READ_ERROR:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;   // registryではi/oを扱わないため、i/oエラーは起こり得ないはず
    case RESOURCE_FILE_CLOSE_ERROR:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;   // registryではi/oを扱わないため、i/oエラーは起こり得ないはず
    case RESOURCE_UNSUPPORTED_FILE:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;   // registryではi/oを扱わないため、i/oエラーは起こり得ないはず
    case RESOURCE_UNDEFINED_ERROR:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;
    default:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;
    }
}

resource_registry_result_t resource_registry_rslt_convert_choco_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return RESOURCE_REGISTRY_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return RESOURCE_REGISTRY_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return RESOURCE_REGISTRY_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return RESOURCE_REGISTRY_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return RESOURCE_REGISTRY_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return RESOURCE_REGISTRY_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return RESOURCE_REGISTRY_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return RESOURCE_REGISTRY_LIMIT_EXCEEDED;
    default:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;
    }
}
