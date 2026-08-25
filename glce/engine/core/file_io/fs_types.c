// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/core/file_io/fs_types.h"

#include <stdbool.h>
#include <stddef.h>

static const char* const s_open_mode_read = "r";                  /**< ファイルオープンモード文字列: READ */
static const char* const s_open_mode_write = "w";                 /**< ファイルオープンモード文字列: WRITE */
static const char* const s_open_mode_append = "a";                /**< ファイルオープンモード文字列: APPEND */
static const char* const s_open_mode_read_plus = "r+";            /**< ファイルオープンモード文字列: READ+ */
static const char* const s_open_mode_write_plus = "w+";           /**< ファイルオープンモード文字列: WRITE+ */
static const char* const s_open_mode_append_plus = "a+";          /**< ファイルオープンモード文字列: APPEND+ */
static const char* const s_open_mode_read_binary = "rb";          /**< ファイルオープンモード文字列: READ(Binary) */
static const char* const s_open_mode_write_binary = "wb";         /**< ファイルオープンモード文字列: WRITE(Binary) */
static const char* const s_open_mode_append_binary = "ab";        /**< ファイルオープンモード文字列: APPEND(Binary) */
static const char* const s_open_mode_read_plus_binary = "r+b";    /**< ファイルオープンモード文字列: READ+(Binary) */
static const char* const s_open_mode_write_plus_binary = "w+b";   /**< ファイルオープンモード文字列: WRITE*(Binary) */
static const char* const s_open_mode_append_plus_binary = "a+b";  /**< ファイルオープンモード文字列: APPEND+(Binary) */

const char* fs_open_mode_c_str(fs_open_mode_t mode_) {
    switch(mode_) {
    case FS_OPEN_MODE_NONE:
        return NULL;
    case FS_OPEN_MODE_READ:
        return s_open_mode_read;
    case FS_OPEN_MODE_WRITE:
        return s_open_mode_write;
    case FS_OPEN_MODE_APPEND:
        return s_open_mode_append;
    case FS_OPEN_MODE_READ_PLUS:
        return s_open_mode_read_plus;
    case FS_OPEN_MODE_WRITE_PLUS:
        return s_open_mode_write_plus;
    case FS_OPEN_MODE_APPEND_PLUS:
        return s_open_mode_append_plus;
    case FS_OPEN_MODE_READ_BINARY:
        return s_open_mode_read_binary;
    case FS_OPEN_MODE_WRITE_BINARY:
        return s_open_mode_write_binary;
    case FS_OPEN_MODE_APPEND_BINARY:
        return s_open_mode_append_binary;
    case FS_OPEN_MODE_READ_PLUS_BINARY:
        return s_open_mode_read_plus_binary;
    case FS_OPEN_MODE_WRITE_PLUS_BINARY:
        return s_open_mode_write_plus_binary;
    case FS_OPEN_MODE_APPEND_PLUS_BINARY:
        return s_open_mode_append_plus_binary;
    default:
        return NULL;
    }
}

bool fs_open_mode_is_readable(fs_open_mode_t mode_) {
    switch(mode_) {
    case FS_OPEN_MODE_NONE:
        return false;
    case FS_OPEN_MODE_READ:
        return true;
    case FS_OPEN_MODE_WRITE:
        return false;
    case FS_OPEN_MODE_APPEND:
        return false;
    case FS_OPEN_MODE_READ_PLUS:
        return true;
    case FS_OPEN_MODE_WRITE_PLUS:
        return true;
    case FS_OPEN_MODE_APPEND_PLUS:
        return true;
    case FS_OPEN_MODE_READ_BINARY:
        return true;
    case FS_OPEN_MODE_WRITE_BINARY:
        return false;
    case FS_OPEN_MODE_APPEND_BINARY:
        return false;
    case FS_OPEN_MODE_READ_PLUS_BINARY:
        return true;
    case FS_OPEN_MODE_WRITE_PLUS_BINARY:
        return true;
    case FS_OPEN_MODE_APPEND_PLUS_BINARY:
        return true;
    default:
        return false;
    }
}

bool fs_open_mode_is_writable(fs_open_mode_t mode_) {
    switch(mode_) {
    case FS_OPEN_MODE_NONE:
        return false;
    case FS_OPEN_MODE_READ:
        return false;
    case FS_OPEN_MODE_WRITE:
        return true;
    case FS_OPEN_MODE_APPEND:
        return true;
    case FS_OPEN_MODE_READ_PLUS:
        return true;
    case FS_OPEN_MODE_WRITE_PLUS:
        return true;
    case FS_OPEN_MODE_APPEND_PLUS:
        return true;
    case FS_OPEN_MODE_READ_BINARY:
        return false;
    case FS_OPEN_MODE_WRITE_BINARY:
        return true;
    case FS_OPEN_MODE_APPEND_BINARY:
        return true;
    case FS_OPEN_MODE_READ_PLUS_BINARY:
        return true;
    case FS_OPEN_MODE_WRITE_PLUS_BINARY:
        return true;
    case FS_OPEN_MODE_APPEND_PLUS_BINARY:
        return true;
    default:
        return false;
    }
}

bool fs_open_mode_is_valid(fs_open_mode_t mode_) {
    switch(mode_) {
    case FS_OPEN_MODE_NONE:
        return true;    // 初期化直後の状態はvalid
    case FS_OPEN_MODE_READ:
        return true;
    case FS_OPEN_MODE_WRITE:
        return true;
    case FS_OPEN_MODE_APPEND:
        return true;
    case FS_OPEN_MODE_READ_PLUS:
        return true;
    case FS_OPEN_MODE_WRITE_PLUS:
        return true;
    case FS_OPEN_MODE_APPEND_PLUS:
        return true;
    case FS_OPEN_MODE_READ_BINARY:
        return true;
    case FS_OPEN_MODE_WRITE_BINARY:
        return true;
    case FS_OPEN_MODE_APPEND_BINARY:
        return true;
    case FS_OPEN_MODE_READ_PLUS_BINARY:
        return true;
    case FS_OPEN_MODE_WRITE_PLUS_BINARY:
        return true;
    case FS_OPEN_MODE_APPEND_PLUS_BINARY:
        return true;
    default:
        return false;
    }
}
