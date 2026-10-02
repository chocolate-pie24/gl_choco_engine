// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file resource_types.h
 * @author chocolate-pie24
 * @brief Resourceレイヤー内で共通して使用するデータ型を提供する
 *
 * @date 2026-05-14
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_CORE_RESOURCE_TYPES_H
#define GLCE_ENGINE_RESOURCE_CORE_RESOURCE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Resourceレイヤー実行結果コード定義
 *
 */
typedef enum {
    RESOURCE_SUCCESS = 0,        /**< 処理成功 */
    RESOURCE_NO_MEMORY,          /**< メモリ不足 */
    RESOURCE_RUNTIME_ERROR,      /**< 実行時エラー */
    RESOURCE_INVALID_ARGUMENT,   /**< 引数異常 */
    RESOURCE_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    RESOURCE_BAD_OPERATION,      /**< API誤用 */
    RESOURCE_OVERFLOW,           /**< 計算過程でオーバーフロー発生 */
    RESOURCE_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    RESOURCE_FILE_OPEN_ERROR,    /**< ファイルオープン失敗 */
    RESOURCE_FILE_READ_ERROR,    /**< ファイル読み込み失敗 */
    RESOURCE_UNSUPPORTED_FILE,   /**< 未対応ファイル形式 */
    RESOURCE_UNDEFINED_ERROR,    /**< 未定義エラー */
} resource_result_t;

typedef struct texture_resource_info {
    size_t pixel_data_size;
    int32_t width;
    int32_t height;
    uint8_t channel_count;
} texture_resource_info_t;

bool texture_resource_info_is_valid(const texture_resource_info_t* resource_info_);

#ifdef __cplusplus
}
#endif
#endif
