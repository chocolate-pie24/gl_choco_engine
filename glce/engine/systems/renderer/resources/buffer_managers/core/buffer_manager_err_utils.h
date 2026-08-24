// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file buffer_manager_err_utils.h
 * @brief Buffer Managerの実行結果コードに関するutility API
 *
 * @details
 * buffer_manager_result_tを診断用文字列へ変換するAPIと、
 * Buffer Managerが使用する下位モジュールの実行結果コードを
 * Buffer Manager層の結果コードへ変換するAPIを提供する。
 *
 * @par 結果コード文字列
 * buffer_manager_rslt_to_str()は、Buffer Managerの実行結果コードに
 * 対応する静的な診断用文字列を返す。
 *
 * 本機能は主にエラーメッセージ、status表示、およびdebug表示で使用する。
 *
 * @par 下位モジュールからの変換
 * 現在は、次の実行結果コードからbuffer_manager_result_tへの変換を提供する。
 *
 * - range_allocator_result_t
 * - renderer_backend_result_t
 * - memory_system_result_t
 *
 * 意味が対応する結果コードは、Buffer Manager層の同等の結果コードへ変換する。
 * Buffer Manager層に対応する結果コードが存在しない場合、または
 * 入力側のenumに定義されていない値を受け取った場合は、
 * BUFFER_MANAGER_UNDEFINED_ERRORへ変換する。
 *
 * Buffer Manager層から上位層への結果コード変換は、
 * 本headerの責務には含めない。
 *
 * @par 状態と副作用
 * 本headerで宣言する各変換関数は、外部状態を参照または変更せず、
 * 動的メモリ確保および動的メモリ解放を行わない。
 *
 * @date 2026-07-31
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_BUFFER_MANAGERS_CORE_BUFFER_MANAGER_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_BUFFER_MANAGERS_CORE_BUFFER_MANAGER_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/allocators/range_allocator.h"
#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"

/**
 * @brief Buffer Managerの結果コードを文字列へ変換する
 *
 * @details
 * rslt_に対応する静的文字列定数を返す。
 *
 * 定義済みの各buffer_manager_result_tについて、
 * 結果コード名を表す文字列を返す。
 *
 * BUFFER_MANAGER_UNDEFINED_ERRORまたは定義されていない値が
 * 指定された場合は、UNDEFINED_ERRORを表す文字列を返す。
 *
 * 返される文字列は静的記憶域期間を持つ。
 * 呼び出し元へ所有権は移動せず、解放または変更してはならない。
 *
 * @param[in] rslt_
 * 文字列へ変換するBuffer Managerの結果コード。
 *
 * @return
 * rslt_に対応するNULLではない静的文字列。
 * 定義されていない値の場合はUNDEFINED_ERRORを表す文字列。
 *
 * @note
 * 本関数はBuffer Managerおよび結果コードを変更しない。
 *
 * @note
 * 返される文字列は読み取り専用であり、複数の呼び出し間で共有される。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
const char* buffer_manager_rslt_to_str(buffer_manager_result_t rslt_);

/**
 * @brief Range Allocatorの結果コードをBuffer Managerの結果コードへ変換する
 *
 * @details
 * 下位モジュールであるRange Allocatorが返したrange_allocator_result_tを、
 * 同じ意味を持つbuffer_manager_result_tへ変換する。
 *
 * 次の対応で変換する。
 *
 * - RANGE_ALLOCATOR_SUCCESS
 *   → BUFFER_MANAGER_SUCCESS
 * - RANGE_ALLOCATOR_INVALID_ARGUMENT
 *   → BUFFER_MANAGER_INVALID_ARGUMENT
 * - RANGE_ALLOCATOR_LIMIT_EXCEEDED
 *   → BUFFER_MANAGER_LIMIT_EXCEEDED
 * - RANGE_ALLOCATOR_NO_MEMORY
 *   → BUFFER_MANAGER_NO_MEMORY
 * - RANGE_ALLOCATOR_DATA_CORRUPTED
 *   → BUFFER_MANAGER_DATA_CORRUPTED
 * - RANGE_ALLOCATOR_BAD_OPERATION
 *   → BUFFER_MANAGER_BAD_OPERATION
 * - RANGE_ALLOCATOR_OVERFLOW
 *   → BUFFER_MANAGER_OVERFLOW
 * - RANGE_ALLOCATOR_UNDEFINED_ERROR
 *   → BUFFER_MANAGER_UNDEFINED_ERROR
 *
 * range_allocator_result_tに定義されていない値は、
 * 意味を安全に変換できないためBUFFER_MANAGER_UNDEFINED_ERRORへ変換する。
 *
 * 本関数は結果コードの変換だけを行い、ログ出力、状態変更、
 * rollback、およびメモリ操作を行わない。
 *
 * @param[in] rslt_
 * 変換するRange Allocatorの結果コード。
 *
 * @retval BUFFER_MANAGER_SUCCESS
 * rslt_がRANGE_ALLOCATOR_SUCCESSである。
 *
 * @retval BUFFER_MANAGER_INVALID_ARGUMENT
 * rslt_がRANGE_ALLOCATOR_INVALID_ARGUMENTである。
 *
 * @retval BUFFER_MANAGER_LIMIT_EXCEEDED
 * rslt_がRANGE_ALLOCATOR_LIMIT_EXCEEDEDである。
 *
 * @retval BUFFER_MANAGER_NO_MEMORY
 * rslt_がRANGE_ALLOCATOR_NO_MEMORYである。
 *
 * @retval BUFFER_MANAGER_DATA_CORRUPTED
 * rslt_がRANGE_ALLOCATOR_DATA_CORRUPTEDである。
 *
 * @retval BUFFER_MANAGER_BAD_OPERATION
 * rslt_がRANGE_ALLOCATOR_BAD_OPERATIONである。
 *
 * @retval BUFFER_MANAGER_OVERFLOW
 * rslt_がRANGE_ALLOCATOR_OVERFLOWである。
 *
 * @retval BUFFER_MANAGER_UNDEFINED_ERROR
 * rslt_がRANGE_ALLOCATOR_UNDEFINED_ERROR、
 * またはrange_allocator_result_tに定義されていない値である。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
buffer_manager_result_t buffer_manager_rslt_convert_range_allocator(range_allocator_result_t rslt_);

/**
 * @brief Rendererの結果コードをBuffer Managerの結果コードへ変換する
 *
 * @details
 * Renderer Backendから返されたrenderer_backend_result_tを、
 * 同じ意味を持つbuffer_manager_result_tへ変換する。
 *
 * 次の対応で変換する。
 *
 * - RENDERER_BACKEND_SUCCESS
 *   → BUFFER_MANAGER_SUCCESS
 * - RENDERER_BACKEND_INVALID_ARGUMENT
 *   → BUFFER_MANAGER_INVALID_ARGUMENT
 * - RENDERER_BACKEND_RUNTIME_ERROR
 *   → BUFFER_MANAGER_RUNTIME_ERROR
 * - RENDERER_BACKEND_NO_MEMORY
 *   → BUFFER_MANAGER_NO_MEMORY
 * - RENDERER_BACKEND_LIMIT_EXCEEDED
 *   → BUFFER_MANAGER_LIMIT_EXCEEDED
 * - RENDERER_BACKEND_BAD_OPERATION
 *   → BUFFER_MANAGER_BAD_OPERATION
 * - RENDERER_BACKEND_DATA_CORRUPTED
 *   → BUFFER_MANAGER_DATA_CORRUPTED
 * - RENDERER_BACKEND_OVERFLOW
 *   → BUFFER_MANAGER_OVERFLOW
 * - RENDERER_BACKEND_UNDEFINED_ERROR
 *   → BUFFER_MANAGER_UNDEFINED_ERROR
 *
 * RENDERER_BACKEND_SHADER_COMPILE_ERRORおよびRENDERER_BACKEND_SHADER_LINK_ERRORは、
 * Buffer Managerの処理範囲外であり、対応する結果コードが存在しないため、
 * BUFFER_MANAGER_UNDEFINED_ERRORへ変換する。
 *
 * renderer_backend_result_tに定義されていない値も、意味を安全に変換できないため
 * BUFFER_MANAGER_UNDEFINED_ERRORへ変換する。
 *
 * 本関数は結果コードの変換だけを行い、ログ出力、状態変更、
 * rollback、およびメモリ操作を行わない。
 *
 * @param[in] rslt_
 * 変換するRendererの結果コード。
 *
 * @retval BUFFER_MANAGER_SUCCESS
 * rslt_がRENDERER_BACKEND_SUCCESSである。
 *
 * @retval BUFFER_MANAGER_INVALID_ARGUMENT
 * rslt_がRENDERER_BACKEND_INVALID_ARGUMENTである。
 *
 * @retval BUFFER_MANAGER_RUNTIME_ERROR
 * rslt_がRENDERER_BACKEND_RUNTIME_ERRORである。
 *
 * @retval BUFFER_MANAGER_NO_MEMORY
 * rslt_がRENDERER_BACKEND_NO_MEMORYである。
 *
 * @retval BUFFER_MANAGER_LIMIT_EXCEEDED
 * rslt_がRENDERER_BACKEND_LIMIT_EXCEEDEDである。
 *
 * @retval BUFFER_MANAGER_BAD_OPERATION
 * rslt_がRENDERER_BACKEND_BAD_OPERATIONである。
 *
 * @retval BUFFER_MANAGER_DATA_CORRUPTED
 * rslt_がRENDERER_BACKEND_DATA_CORRUPTEDである。
 *
 * @retval BUFFER_MANAGER_OVERFLOW
 * rslt_がRENDERER_BACKEND_OVERFLOWである。
 *
 * @retval BUFFER_MANAGER_UNDEFINED_ERROR
 * 次のいずれか。
 * - rslt_がRENDERER_BACKEND_SHADER_COMPILE_ERRORである
 * - rslt_がRENDERER_BACKEND_SHADER_LINK_ERRORである
 * - rslt_がRENDERER_BACKEND_UNDEFINED_ERRORである
 * - rslt_がrenderer_backend_result_tに定義されていない値である
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
buffer_manager_result_t buffer_manager_rslt_convert_renderer_backend(renderer_backend_result_t rslt_);

/**
 * @brief Memory Systemの結果コードをBuffer Managerの結果コードへ変換する
 *
 * @details
 * 下位モジュールであるMemory Systemが返したmemory_system_result_tを、
 * 同じ意味を持つbuffer_manager_result_tへ変換する。
 *
 * 次の対応で変換する。
 *
 * - MEMORY_SYSTEM_SUCCESS
 *   → BUFFER_MANAGER_SUCCESS
 * - MEMORY_SYSTEM_INVALID_ARGUMENT
 *   → BUFFER_MANAGER_INVALID_ARGUMENT
 * - MEMORY_SYSTEM_LIMIT_EXCEEDED
 *   → BUFFER_MANAGER_LIMIT_EXCEEDED
 * - MEMORY_SYSTEM_BAD_OPERATION
 *   → BUFFER_MANAGER_BAD_OPERATION
 * - MEMORY_SYSTEM_NO_MEMORY
 *   → BUFFER_MANAGER_NO_MEMORY
 *
 * memory_system_result_tに定義されていない値は、
 * 意味を安全に変換できないためBUFFER_MANAGER_UNDEFINED_ERRORへ変換する。
 *
 * 本関数は結果コードの変換だけを行い、ログ出力、状態変更、
 * rollback、およびメモリ操作を行わない。
 *
 * @param[in] rslt_
 * 変換するMemory Systemの結果コード。
 *
 * @retval BUFFER_MANAGER_SUCCESS
 * rslt_がMEMORY_SYSTEM_SUCCESSである。
 *
 * @retval BUFFER_MANAGER_INVALID_ARGUMENT
 * rslt_がMEMORY_SYSTEM_INVALID_ARGUMENTである。
 *
 * @retval BUFFER_MANAGER_LIMIT_EXCEEDED
 * rslt_がMEMORY_SYSTEM_LIMIT_EXCEEDEDである。
 *
 * @retval BUFFER_MANAGER_BAD_OPERATION
 * rslt_がMEMORY_SYSTEM_BAD_OPERATIONである。
 *
 * @retval BUFFER_MANAGER_NO_MEMORY
 * rslt_がMEMORY_SYSTEM_NO_MEMORYである。
 *
 * @retval BUFFER_MANAGER_UNDEFINED_ERROR
 * rslt_がmemory_system_result_tに定義されていない値である。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
buffer_manager_result_t buffer_manager_rslt_convert_choco_memory(memory_system_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
