// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_CORE_FILE_IO_FS_TYPES_H
#define GLCE_ENGINE_CORE_FILE_IO_FS_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief ファイルオープンモードリスト
 *
 */
typedef enum {
    FS_OPEN_MODE_READ = 0,                   /**< オープンモード: 読み取り */
    FS_OPEN_MODE_WRITE,                  /**< オープンモード: 書き込み */
    FS_OPEN_MODE_APPEND,                 /**< オープンモード: 追記 */
    FS_OPEN_MODE_READ_BINARY,            /**< オープンモード: 読み取り(バイナリファイル) */
    FS_OPEN_MODE_WRITE_BINARY,           /**< オープンモード: 書き込み(バイナリファイル) */
    FS_OPEN_MODE_APPEND_BINARY,          /**< オープンモード: 追記(バイナリファイル) */
} fs_open_mode_t;

const char* fs_open_mode_c_str(fs_open_mode_t mode_);

bool fs_open_mode_is_readable(fs_open_mode_t mode_);

bool fs_open_mode_is_writable(fs_open_mode_t mode_);

bool fs_open_mode_is_valid(fs_open_mode_t mode_);

#ifdef __cplusplus
}
#endif
#endif
