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
    FS_OPEN_MODE_NONE = 0,               /**< オープンモード: デフォルト(未オープン) */
    FS_OPEN_MODE_READ,                   /**< オープンモード: 読み取り */
    FS_OPEN_MODE_WRITE,                  /**< オープンモード: 書き込み */
    FS_OPEN_MODE_APPEND,                 /**< オープンモード: 追記 */
    FS_OPEN_MODE_READ_PLUS,              /**< オープンモード: 読み書き可(既存ファイルの内容は消さない、ファイルがなければ失敗) */
    FS_OPEN_MODE_WRITE_PLUS,             /**< オープンモード: 読み書き可(新規作成or既存ファイルの中身を消去) */
    FS_OPEN_MODE_APPEND_PLUS,            /**< オープンモード: 読み書き可(既存ファイルがあれば追記、ファイルがなければ新規作成) */
    FS_OPEN_MODE_READ_BINARY,            /**< オープンモード: 読み取り(バイナリファイル) */
    FS_OPEN_MODE_WRITE_BINARY,           /**< オープンモード: 書き込み(バイナリファイル) */
    FS_OPEN_MODE_APPEND_BINARY,          /**< オープンモード: 追記(バイナリファイル) */
    FS_OPEN_MODE_READ_PLUS_BINARY,       /**< オープンモード: 読み書き可(既存ファイルの内容は消さない、ファイルがなければ失敗)(バイナリファイル) */
    FS_OPEN_MODE_WRITE_PLUS_BINARY,      /**< オープンモード: 読み書き可(新規作成or既存ファイルの中身を消去)(バイナリファイル) */
    FS_OPEN_MODE_APPEND_PLUS_BINARY,     /**< オープンモード: 読み書き可(既存ファイルがあれば追記、ファイルがなければ新規作成)(バイナリファイル) */
} fs_open_mode_t;

const char* fs_open_mode_c_str(fs_open_mode_t mode_);

bool fs_open_mode_is_readable(fs_open_mode_t mode_);

bool fs_open_mode_is_writable(fs_open_mode_t mode_);

bool fs_open_mode_is_valid(fs_open_mode_t mode_);

#ifdef __cplusplus
}
#endif
#endif
