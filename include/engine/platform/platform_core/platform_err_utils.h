/**
 * @ingroup platform_core
 * @file platform_err_utils.h
 * @author chocolate-pie24
 * @brief プラットフォームレイヤーのエラー処理を統一するため、下位モジュールの実行結果コードの変換と、実行結果コードの文字列化処理を提供する
 * @version 0.1
 * @date 2026-02-13
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef GLCE_ENGINE_PLATFORM_PLATFORM_CORE_PLATFORM_ERR_UTILS_H
#define GLCE_ENGINE_PLATFORM_PLATFORM_CORE_PLATFORM_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/platform/platform_core/platform_types.h"

#include "engine/containers/choco_string.h"
#include "engine/core/memory/linear_allocator.h"

/**
 * @brief プラットフォームレイヤーの実行結果コードを文字列に変換する
 *
 * @param rslt_ 変換するプラットフォームシステム実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
const char* platform_rslt_to_str(platform_result_t rslt_);

/**
 * @brief choco_stringモジュールの実行結果コードをプラットフォームレイヤーの実行結果コードに変換する
 *
 * @param rslt_ choco_stringモジュール実行結果コード
 * @return platform_result_t プラットフォームレイヤー実行結果コード
 */
platform_result_t platform_rslt_convert_choco_string(choco_string_result_t rslt_);

/**
 * @brief linear_allocatorモジュールの実行結果コードをプラットフォームレイヤーの実行結果コードに変換する
 *
 * @param rslt_ linear_allocatorモジュール実行結果コード
 * @return platform_result_t プラットフォームレイヤー実行結果コード
 */
platform_result_t platform_rslt_convert_linear_alloc(linear_allocator_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
