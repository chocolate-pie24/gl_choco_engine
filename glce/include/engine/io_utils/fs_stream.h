// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_IO_UTILS_FS_STREAM_H
#define GLCE_ENGINE_IO_UTILS_FS_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "engine/core/file_io/fs_types.h"

typedef enum {
    FS_STREAM_SUCCESS = 0,       /**< 実行結果コード: 成功 */
    FS_STREAM_INVALID_ARGUMENT,  /**< 実行結果コード: 無効な引数 */
    FS_STREAM_BAD_OPERATION,     /**< 実行結果コード: API誤用 */
    FS_STREAM_DATA_CORRUPTED,    /**< 実行結果コード: データ破損or未初期化 */
    FS_STREAM_NO_MEMORY,         /**< 実行結果コード: メモリ不足 */
    FS_STREAM_LIMIT_EXCEEDED,    /**< 実行結果コード: システム使用可能範囲上限超過 */
    FS_STREAM_OVERFLOW,          /**< 実行結果コード: 計算過程でオーバーフロー発生 */
    FS_STREAM_FILE_OPEN_ERROR,   /**< 実行結果コード: ファイルオープンエラー */
    FS_STREAM_FILE_CLOSE_ERROR,  /**< 実行結果コード: ファイルクローズエラー */
    FS_STREAM_RUNTIME_ERROR,     /**< 実行結果コード: 実行時エラー */
    FS_STREAM_UNDEFINED_ERROR,   /**< 実行結果コード: 想定していないエラーが発生 */
    FS_STREAM_EOF,               /**< 実行結果コード: ファイルを読み込んだ結果がEOF */
} fs_stream_result_t;

typedef struct fs_stream fs_stream_t;
typedef struct choco_string choco_string_t;

fs_stream_result_t fs_stream_create(fs_stream_t** out_fs_stream_);

void fs_stream_destroy(fs_stream_t** fs_stream_);

fs_stream_result_t fs_stream_open(fs_stream_t* fs_stream_, const char* fullpath_, fs_open_mode_t open_mode_);

fs_stream_result_t fs_stream_close(fs_stream_t* fs_stream_);

fs_stream_result_t fs_stream_byte_read(fs_stream_t* fs_stream_, size_t read_bytes_, size_t* result_n_, char* buffer_);

fs_stream_result_t fs_stream_text_file_read(fs_stream_t* fs_stream_, choco_string_t* out_string_);

fs_stream_result_t fs_stream_text_file_line_read(fs_stream_t* fs_stream_, choco_string_t* out_string_);

bool fs_stream_is_valid(const fs_stream_t* fs_stream_);

#ifdef __cplusplus
}
#endif
#endif
