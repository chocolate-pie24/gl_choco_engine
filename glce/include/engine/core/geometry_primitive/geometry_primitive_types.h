// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup core
 *
 * @file geometry_primitive_types.h
 * @author chocolate-pie24
 * @brief geometry_primitive内で共通して使用するデータ型を提供する
 *
 * @date 2026-06-09
 *
 */
#ifndef GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_GEOMETRY_PRIMITIVE_TYPES_H
#define GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_GEOMETRY_PRIMITIVE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief geometry primitive実行結果コード定義
 *
 */
typedef enum {
    GEOMETRY_PRIMITIVE_SUCCESS = 0,         /**< 実行結果正常 */
    GEOMETRY_PRIMITIVE_INVALID_ARGUMENT,    /**< 実行結果: 引数異常 */
    GEOMETRY_PRIMITIVE_RUNTIME_ERROR,       /**< 実行結果: 実行時エラー */
    GEOMETRY_PRIMITIVE_LIMIT_EXCEEDED,      /**< 実行結果: システム使用可能範囲上限超過 */
    GEOMETRY_PRIMITIVE_BAD_OPERATION,       /**< 実行結果: API誤用 */
    GEOMETRY_PRIMITIVE_NO_MEMORY,           /**< 実行結果: メモリ不足 */
    GEOMETRY_PRIMITIVE_DATA_CORRUPTED,      /**< 実行結果: 内部データ破損 */
    GEOMETRY_PRIMITIVE_UNDEFINED_ERROR,     /**< 実行結果: 不明なエラー */
} geometry_primitive_result_t;

#ifdef __cplusplus
}
#endif
#endif
