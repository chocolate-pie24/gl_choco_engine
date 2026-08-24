// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_CORE_RESOURCE_PIPELINE_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_CORE_RESOURCE_PIPELINE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RESOURCE_PIPELINE_SUCCESS = 0,        /**< 処理成功 */
    RESOURCE_PIPELINE_NO_MEMORY,          /**< メモリ不足 */
    RESOURCE_PIPELINE_RUNTIME_ERROR,      /**< 実行時エラー */
    RESOURCE_PIPELINE_INVALID_ARGUMENT,   /**< 引数異常 */
    RESOURCE_PIPELINE_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    RESOURCE_PIPELINE_BAD_OPERATION,      /**< API誤用 */
    RESOURCE_PIPELINE_OVERFLOW,           /**< 計算過程でオーバーフロー発生 */
    RESOURCE_PIPELINE_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    RESOURCE_PIPELINE_FILE_OPEN_ERROR,    /**< ファイルオープン失敗 */
    RESOURCE_PIPELINE_FILE_READ_ERROR,    /**< ファイル読み込み失敗 */
    RESOURCE_PIPELINE_FILE_CLOSE_ERROR,   /**< ファイルクローズ失敗 */
    RESOURCE_PIPELINE_UNSUPPORTED_FILE,   /**< 未対応ファイル形式 */
    RESOURCE_PIPELINE_UNDEFINED_ERROR,    /**< 未定義エラー */
} resource_pipeline_result_t;

#ifdef __cplusplus
}
#endif
#endif
