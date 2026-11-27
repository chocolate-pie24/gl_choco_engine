// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file buffer_manager_err_utils.c
 * @brief Buffer Managerの実行結果コードutility APIの実装
 *
 * @details
 * buffer_manager_result_tに対応する文字列と、
 * 下位モジュールの実行結果コードをBuffer Manager層の
 * 結果コードへ変換する処理を実装する。
 *
 * @par 結果コード文字列
 * 各buffer_manager_result_tに対応するEnum定数名を
 * STRINGIFY()によって文字列化して返す。
 *
 * 返される文字列は静的記憶域期間を持つ。
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
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/gpu_allocators/range_allocator/range_allocator.h"
#include "engine/memory/cpu_allocators/general_allocator/general_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"

const char* buffer_manager_result_to_str(buffer_manager_result_t result_) {
    switch(result_) {
    case BUFFER_MANAGER_SUCCESS:
        return STRINGIFY(BUFFER_MANAGER_SUCCESS);
    case BUFFER_MANAGER_INVALID_ARGUMENT:
        return STRINGIFY(BUFFER_MANAGER_INVALID_ARGUMENT);
    case BUFFER_MANAGER_RUNTIME_ERROR:
        return STRINGIFY(BUFFER_MANAGER_RUNTIME_ERROR);
    case BUFFER_MANAGER_LIMIT_EXCEEDED:
        return STRINGIFY(BUFFER_MANAGER_LIMIT_EXCEEDED);
    case BUFFER_MANAGER_NO_MEMORY:
        return STRINGIFY(BUFFER_MANAGER_NO_MEMORY);
    case BUFFER_MANAGER_DATA_CORRUPTED:
        return STRINGIFY(BUFFER_MANAGER_DATA_CORRUPTED);
    case BUFFER_MANAGER_BAD_OPERATION:
        return STRINGIFY(BUFFER_MANAGER_BAD_OPERATION);
    case BUFFER_MANAGER_OVERFLOW:
        return STRINGIFY(BUFFER_MANAGER_OVERFLOW);
    case BUFFER_MANAGER_UNDEFINED_ERROR:
        return STRINGIFY(BUFFER_MANAGER_UNDEFINED_ERROR);
    default:
        return STRINGIFY(BUFFER_MANAGER_UNDEFINED_ERROR);
    }
}

buffer_manager_result_t buffer_manager_result_convert_range_allocator(range_allocator_result_t result_) {
    switch(result_) {
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

buffer_manager_result_t buffer_manager_result_convert_renderer_backend(renderer_backend_result_t result_) {
    switch(result_) {
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

buffer_manager_result_t buffer_manager_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return BUFFER_MANAGER_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return BUFFER_MANAGER_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return BUFFER_MANAGER_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return BUFFER_MANAGER_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return BUFFER_MANAGER_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return BUFFER_MANAGER_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return BUFFER_MANAGER_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    default:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    }
}
