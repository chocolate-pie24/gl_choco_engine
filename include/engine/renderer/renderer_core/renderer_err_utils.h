/** @ingroup renderer_core
 *
 * @file renderer_err_utils.h
 * @author chocolate-pie24
 * @brief renderer_err_utilsは、レンダラーレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能を提供する
 *
 * @version 0.1
 * @date 2026-02-12
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RENDERER_CORE_RENDERER_ERR_UTILS_H
#define GLCE_ENGINE_RENDERER_CORE_RENDERER_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/renderer/renderer_core/renderer_types.h"
#include "engine/core/memory/linear_allocator.h"
#include "engine/core/memory/choco_memory.h"

/**
 * @brief ログ出力用にレンダラーレイヤーの実行結果コードを文字列に変換する
 *
 * @param rslt_ 実行結果コード
 * @return const char* 文字列化された実行結果コード
 */
const char* renderer_rslt_to_str(renderer_result_t rslt_);

/**
 * @brief 下位モジュールであるlinear_allocatorが出力する実行結果コードをレンダラーレイヤーの実行結果コードに変換する
 *
 * @param rslt_ linear_allocatorモジュールが出力する実行結果コード
 * @return renderer_result_t 変換されたレンダラーレイヤーの実行結果コード
 */
renderer_result_t renderer_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

/**
 * @brief 下位モジュールであるchoco_memoryが出力する実行結果コードをレンダラーレイヤーの実行結果コードに変換する
 *
 * @param rslt_ choco_memoryモジュールが出力する実行結果コード
 * @return renderer_result_t 変換されたレンダラーレイヤーの実行結果コード
 */
renderer_result_t renderer_rslt_convert_choco_memory(memory_system_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
