// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_IO_UTILS_FS_PATH_H
#define GLCE_ENGINE_IO_UTILS_FS_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct fs_path fs_path_t;

typedef enum {
    FS_PATH_SUCCESS = 0,       /**< 実行結果コード: 成功 */
    FS_PATH_INVALID_ARGUMENT,  /**< 実行結果コード: 無効な引数 */
    FS_PATH_BAD_OPERATION,     /**< 実行結果コード: API誤用 */
    FS_PATH_DATA_CORRUPTED,    /**< 実行結果コード: データ破損or未初期化 */
    FS_PATH_NO_MEMORY,         /**< 実行結果コード: メモリ不足 */
    FS_PATH_LIMIT_EXCEEDED,    /**< 実行結果コード: システム使用可能範囲上限超過 */
    FS_PATH_OVERFLOW,          /**< 実行結果コード: 計算過程でオーバーフロー発生 */
    FS_PATH_RUNTIME_ERROR,     /**< 実行結果コード: 実行時エラー */
    FS_PATH_UNDEFINED_ERROR,   /**< 実行結果コード: 想定していないエラーが発生 */
} fs_path_result_t;

fs_path_result_t fs_path_create(fs_path_t** fs_path_, const char* path_, const char* name_, const char* extension_);

fs_path_result_t fs_path_create_from_executable_directory(fs_path_t** out_fs_path_);

void fs_path_destroy(fs_path_t** fs_path_);

const char* fs_path_fullpath_get(const fs_path_t* fs_path_);

bool fs_path_is_valid(const fs_path_t* fs_path_);

#ifdef __cplusplus
}
#endif
#endif
