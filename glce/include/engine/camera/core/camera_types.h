// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_CAMERA_CORE_CAMERA_TYPES_H
#define GLCE_ENGINE_CAMERA_CORE_CAMERA_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAMERA_SUCCESS = 0,           /**< 処理成功 */
    CAMERA_INVALID_ARGUMENT,      /**< 無効な引数 */
    CAMERA_RUNTIME_ERROR,         /**< 実行時エラー */
    CAMERA_NO_MEMORY,             /**< メモリ不足 */
    CAMERA_LIMIT_EXCEEDED,        /**< システム使用可能範囲超過 */
    CAMERA_BAD_OPERATION,         /**< API誤用 */
    CAMERA_DATA_CORRUPTED,        /**< メモリ破損,未初期化 */
    CAMERA_UNDEFINED_ERROR,       /**< 不明なエラー */
} camera_result_t;

#ifdef __cplusplus
}
#endif
#endif
