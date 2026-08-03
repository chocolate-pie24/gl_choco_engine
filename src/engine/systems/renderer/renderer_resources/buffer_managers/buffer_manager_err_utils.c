/**
 * @file buffer_manager_err_utils.c
 * @brief Buffer Managerの実行結果コードutility APIの実装
 *
 * @details
 * buffer_manager_result_tに対応する静的な診断用文字列と、
 * 下位モジュールの実行結果コードをBuffer Manager層の
 * 結果コードへ変換する処理を実装する。
 *
 * @par 結果コード文字列
 * 各buffer_manager_result_tに対応する文字列を、
 * file scopeの静的な文字列として保持する。
 *
 * buffer_manager_rslt_to_str()は入力された結果コードに対応する
 * 文字列へのpointerを返す。
 * 入力値がbuffer_manager_result_tに定義されていない場合は、
 * UNDEFINED_ERRORに対応する文字列を返す。
 *
 * @par 結果コード変換
 * 下位モジュールのenum値とbuffer_manager_result_tの数値または
 * 宣言順が一致することには依存せず、switch文によって
 * 各結果コードの意味を明示的に対応付ける。
 *
 * 意味が対応する結果コードは、Buffer Manager層の同等の
 * 結果コードへ変換する。
 *
 * Buffer Manager層に対応する変換先が存在しない結果コード、
 * または入力側のenumに定義されていない値は、
 * BUFFER_MANAGER_UNDEFINED_ERRORへ変換する。
 *
 * Rendererのシェーダーコンパイルおよびリンクに関する結果コードは、
 * Buffer Managerの処理範囲外であり対応する結果コードを持たないため、
 * BUFFER_MANAGER_UNDEFINED_ERRORへ変換する。
 *
 * @par 状態と副作用
 * 本ファイルで実装する各関数は外部状態を参照または変更せず、
 * 動的メモリ確保および動的メモリ解放を行わない。
 *
 * @date 2026-07-31
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_err_utils.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/renderer_core/allocators/range_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_types.h"

/**
 * @name Buffer Manager実行結果コード文字列
 *
 * @details
 * buffer_manager_result_tをログ出力用文字列へ変換するための静的文字列定数。
 *
 * すべて静的記憶域期間を持ち、呼び出し側へ所有権を移動しない。
 * 文字列および文字列pointerは変更できない。
 *
 * buffer_manager_rslt_to_str()は対応する結果コードの文字列を返す。
 * 定義されていない結果コードにはs_rslt_str_undefined_errorを使用する。
 *
 * @{
 */
static const char* const s_rslt_str_success = "SUCCESS";                        /** @brief BUFFER_MANAGER_SUCCESSに対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";      /** @brief BUFFER_MANAGER_INVALID_ARGUMENTに対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";            /** @brief BUFFER_MANAGER_RUNTIME_ERRORに対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";          /** @brief BUFFER_MANAGER_LIMIT_EXCEEDEDに対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                    /** @brief BUFFER_MANAGER_NO_MEMORYに対応する文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";          /** @brief BUFFER_MANAGER_DATA_CORRUPTEDに対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";            /** @brief BUFFER_MANAGER_BAD_OPERATIONに対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                      /** @brief BUFFER_MANAGER_OVERFLOWに対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";        /** @brief BUFFER_MANAGER_UNDEFINED_ERRORおよび定義されていない結果コードに対応する文字列 */
/** @} */

const char* buffer_manager_rslt_to_str(buffer_manager_result_t rslt_) {
    switch(rslt_) {
    case BUFFER_MANAGER_SUCCESS:
        return s_rslt_str_success;
    case BUFFER_MANAGER_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case BUFFER_MANAGER_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case BUFFER_MANAGER_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case BUFFER_MANAGER_NO_MEMORY:
        return s_rslt_str_no_memory;
    case BUFFER_MANAGER_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case BUFFER_MANAGER_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case BUFFER_MANAGER_OVERFLOW:
        return s_rslt_str_overflow;
    case BUFFER_MANAGER_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

buffer_manager_result_t buffer_manager_rslt_convert_range_allocator(range_allocator_result_t rslt_) {
    switch(rslt_) {
    case RANGE_ALLOCATOR_SUCCESS:
        return BUFFER_MANAGER_SUCCESS;
    case RANGE_ALLOCATOR_INVALID_ARGUMENT:
        return BUFFER_MANAGER_INVALID_ARGUMENT;
    case RANGE_ALLOCATOR_LIMIT_EXCEEDED:
        return BUFFER_MANAGER_LIMIT_EXCEEDED;
    case RANGE_ALLOCATOR_NO_MEMORY:
        return BUFFER_MANAGER_NO_MEMORY;
    case RANGE_ALLOCATOR_DATA_CORRUPTED:
        return BUFFER_MANAGER_DATA_CORRUPTED;
    case RANGE_ALLOCATOR_BAD_OPERATION:
        return BUFFER_MANAGER_BAD_OPERATION;
    case RANGE_ALLOCATOR_OVERFLOW:
        return BUFFER_MANAGER_OVERFLOW;
    case RANGE_ALLOCATOR_UNDEFINED_ERROR:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    default:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    }
}

buffer_manager_result_t buffer_manager_rslt_convert_renderer_backend(renderer_backend_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_BACKEND_SUCCESS:
        return BUFFER_MANAGER_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return BUFFER_MANAGER_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return BUFFER_MANAGER_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return BUFFER_MANAGER_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return BUFFER_MANAGER_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return BUFFER_MANAGER_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return BUFFER_MANAGER_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return BUFFER_MANAGER_OVERFLOW;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return BUFFER_MANAGER_UNDEFINED_ERROR;    // Buffer Managerではシェーダーのコンパイル, リンクを行わないのでUNDEFINED_ERRORに変換
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return BUFFER_MANAGER_UNDEFINED_ERROR;    // Buffer Managerではシェーダーのコンパイル, リンクを行わないのでUNDEFINED_ERRORに変換
    default:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    }
}

buffer_manager_result_t buffer_manager_rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return BUFFER_MANAGER_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return BUFFER_MANAGER_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return BUFFER_MANAGER_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return BUFFER_MANAGER_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return BUFFER_MANAGER_NO_MEMORY;
    default:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    }
}
