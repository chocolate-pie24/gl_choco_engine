// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/platform_system/core/platform_system_err_utils.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/platform_system/core/platform_system_types.h"

static const char* const s_result_str_success = "SUCCESS";                    /**< プラットフォームAPI実行結果コード(処理成功)に対応する文字列 */
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";  /**< プラットフォームAPI実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";        /**< プラットフォームAPI実行結果コード(実行時エラー)に対応する文字列 */
static const char* const s_result_str_no_memory = "NO_MEMORY";                /**< プラットフォームAPI実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";      /**< プラットフォームAPI実行結果コード(メモリ破損)に対応する文字列 */
static const char* const s_result_str_bad_operation = "BAD_OPERATION";        /**< プラットフォームAPI実行結果コード(API誤用)に対応する文字列 */
static const char* const s_result_str_overflow = "OVERFLOW";                  /**< プラットフォームAPI実行結果コード(オーバーフロー)に対応する文字列 */
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< プラットフォームAPI実行結果コード(システム使用可能範囲上限超過)に対応する文字列 */
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";    /**< プラットフォームAPI実行結果コード(未定義エラー)に対応する文字列 */

const char* platform_system_result_to_str(platform_system_result_t result_) {
    switch(result_) {
    case PLATFORM_SYSTEM_SUCCESS:
        return s_result_str_success;
    case PLATFORM_SYSTEM_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case PLATFORM_SYSTEM_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case PLATFORM_SYSTEM_NO_MEMORY:
        return s_result_str_no_memory;
    case PLATFORM_SYSTEM_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case PLATFORM_SYSTEM_BAD_OPERATION:
        return s_result_str_bad_operation;
    case PLATFORM_SYSTEM_OVERFLOW:
        return s_result_str_overflow;
    case PLATFORM_SYSTEM_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case PLATFORM_SYSTEM_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

platform_system_result_t platform_system_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return PLATFORM_SYSTEM_SUCCESS;
    case CHOCO_STRING_NO_MEMORY:
        return PLATFORM_SYSTEM_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return PLATFORM_SYSTEM_INVALID_ARGUMENT;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return PLATFORM_SYSTEM_UNDEFINED_ERROR;
    case CHOCO_STRING_DATA_CORRUPTED:
        return PLATFORM_SYSTEM_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return PLATFORM_SYSTEM_BAD_OPERATION;
    case CHOCO_STRING_RUNTIME_ERROR:
        return PLATFORM_SYSTEM_RUNTIME_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return PLATFORM_SYSTEM_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return PLATFORM_SYSTEM_LIMIT_EXCEEDED;
    default:
        return PLATFORM_SYSTEM_UNDEFINED_ERROR;
    }
}

platform_system_result_t platform_system_result_convert_subsystem_allocator(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return PLATFORM_SYSTEM_SUCCESS;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return PLATFORM_SYSTEM_BAD_OPERATION;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return PLATFORM_SYSTEM_DATA_CORRUPTED;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return PLATFORM_SYSTEM_NO_MEMORY;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return PLATFORM_SYSTEM_INVALID_ARGUMENT;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return PLATFORM_SYSTEM_UNDEFINED_ERROR;
    default:
        return PLATFORM_SYSTEM_UNDEFINED_ERROR;
    }
}
