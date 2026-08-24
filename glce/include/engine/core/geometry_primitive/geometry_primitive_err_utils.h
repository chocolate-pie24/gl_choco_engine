// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup core
 *
 * @file geometry_primitive_err_utils.h
 * @author chocolate-pie24
 * @brief geometry_primitive内でのエラー処理仕様を統一するため、実行結果コード変換機能を提供する
 *
 * @date 2026-06-09
 *
 */
#ifndef GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_GEOMETRY_PRIMITIVE_ERR_UTILS_H
#define GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_GEOMETRY_PRIMITIVE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/geometry_primitive/geometry_primitive_types.h"

/**
 * @brief geometry primitive実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ geometry primitiveレイヤー実行結果コード
 *
 * @retval "SUCCESS" 実行結果コード:GEOMETRY_PRIMITIVE_SUCCESS(正常終了)
 * @retval "INVALID_ARGUMENT" 実行結果コード:GEOMETRY_PRIMITIVE_INVALID_ARGUMENT(引数異常)
 * @retval "RUNTIME_ERROR" 実行結果コード:GEOMETRY_PRIMITIVE_RUNTIME_ERROR(実行時エラー)
 * @retval "LIMIT_EXCEEDED" 実行結果コード:GEOMETRY_PRIMITIVE_LIMIT_EXCEEDED(システム使用可能範囲上限超過)
 * @retval "BAD_OPERATION" 実行結果コード:GEOMETRY_PRIMITIVE_BAD_OPERATION(API誤用)
 * @retval "NO_MEMORY" 実行結果コード:GEOMETRY_PRIMITIVE_NO_MEMORY(メモリ不足)
 * @retval "DATA_CORRUPTED" 実行結果コード:GEOMETRY_PRIMITIVE_DATA_CORRUPTED(幾何データ不正)
 * @retval "UNDEFINED_ERROR" 実行結果コード:GEOMETRY_PRIMITIVE_UNDEFINED_ERROR(不明なエラー)
 */
const char* geometry_primitive_rslt_to_str(geometry_primitive_result_t rslt_);

#ifdef __cplusplus
}
#endif
#endif
