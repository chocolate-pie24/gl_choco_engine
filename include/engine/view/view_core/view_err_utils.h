/** @ingroup view
 *
 * @file view_err_utils.h
 * @author chocolate-pie24
 * @brief ビューレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能を提供する
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_VIEW_VIEW_ERR_UTILS_H
#define GLCE_ENGINE_VIEW_VIEW_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/view/view_core/view_types.h"

#include "engine/containers/choco_string.h"

#include "engine/core/memory/choco_memory.h"

/**
 * @brief ビューレイヤー実行結果コードを文字列に変換する
 *
 * @param rslt_ ビューレイヤー実行結果コード
 * @return const char* 変換された文字列
 */
const char* view_rslt_to_str(view_result_t rslt_);

/**
 * @brief choco_memoryモジュールの実行結果コードをビューレイヤー実行結果コードに変換する
 *
 * @param rslt_ choco_memoryモジュール実行結果コード
 *
 * @return view_result_t 変換されたビューレイヤー実行結果コード
 */
view_result_t view_rslt_convert_choco_memory(memory_system_result_t rslt_);

/**
 * @brief choco_stringモジュールの実行結果コードをビューレイヤー実行結果コードに変換する
 *
 * @param rslt_ choco_stringモジュール実行結果コード
 *
 * @return camera_result_t 変換されたビューレイヤー実行結果コード
 */
view_result_t view_rslt_convert_choco_string(choco_string_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
