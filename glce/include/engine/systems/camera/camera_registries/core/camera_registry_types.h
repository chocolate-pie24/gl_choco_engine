// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_CAMERA_CAMERA_REGISTRIES_CORE_CAMERA_REGISTRY_TYPES_H
#define GLCE_ENGINE_SYSTEMS_CAMERA_CAMERA_REGISTRIES_CORE_CAMERA_REGISTRY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAMERA_REGISTRY_SUCCESS = 0,        /**< 処理成功 */
    CAMERA_REGISTRY_NO_MEMORY,          /**< メモリ不足 */
    CAMERA_REGISTRY_RUNTIME_ERROR,      /**< 実行時エラー */
    CAMERA_REGISTRY_INVALID_ARGUMENT,   /**< 引数異常 */
    CAMERA_REGISTRY_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    CAMERA_REGISTRY_BAD_OPERATION,      /**< API誤用 */
    CAMERA_REGISTRY_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    CAMERA_REGISTRY_OVERFLOW,
    CAMERA_REGISTRY_UNDEFINED_ERROR,    /**< 未定義エラー */
} camera_registry_result_t;

#ifdef __cplusplus
}
#endif
#endif
