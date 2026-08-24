// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup core
 *
 * @file filesystem.c
 * @author chocolate-pie24
 * @brief ファイルシステムモジュールAPIの実装
 *
 * @date 2025-12-23
 *
 */
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "engine/core/filesystem/filesystem.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

/**
 * @brief ファイルシステムモジュール内部状態管理構造体
 *
 */
struct filesystem {
    FILE* file_handle;              /**< ファイルハンドル */
    filesystem_open_mode_t mode;    /**< ファイルオープンモード */
};

static const char* rslt_to_str(filesystem_result_t rslt_);
static bool open_mode_readable(filesystem_open_mode_t mode_);
static FILE* mock_fopen(const char* fullpath_, const char* mode_);
static int mock_fclose(FILE* stream_);
static size_t mock_fread(void *ptr_, size_t size_, size_t nmemb_, FILE *stream_);
static int mock_ferror(FILE *stream_);
static int mock_feof(FILE *stream_);

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

static const char* const s_rslt_str_success = "SUCCESS";                        /**< 実行結果コード文字列: 成功 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";      /**< 実行結果コード文字列: 無効な引数 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";            /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                    /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";        /**< 実行結果コード文字列: ファイルオープン失敗 */
static const char* const s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";      /**< 実行結果コード文字列: ファイルクローズ失敗 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";          /**< 実行結果コード文字列: システムリソースが使用可能範囲を超過 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";            /**< 実行結果コード文字列: API誤用 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";        /**< 実行結果コード文字列: 未定義エラー */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";          /**< 実行結果コード文字列: 内部データ破損 */
static const char* const s_rslt_str_eof = "EOF";                                /**< 実行結果コード文字列: ファイル読み込みEOF */

filesystem_result_t filesystem_create(filesystem_t** filesystem_) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    memory_system_result_t mem_result = MEMORY_SYSTEM_INVALID_ARGUMENT;

    filesystem_t* tmp_filesystem = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "filesystem_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "*filesystem_")

    mem_result = memory_system_allocate(sizeof(filesystem_t), MEMORY_TAG_FILE_IO, (void**)&tmp_filesystem);
    if(MEMORY_SYSTEM_INVALID_ARGUMENT == mem_result) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_create(%s) - memory_system_allocate returned INVALID_ARGUMENT.", rslt_to_str(ret));
        goto cleanup;
    } else if(MEMORY_SYSTEM_NO_MEMORY == mem_result) {
        ret = FILESYSTEM_NO_MEMORY;
        ERROR_MESSAGE("filesystem_create(%s) - memory_system_allocate returned NO_MEMORY.", rslt_to_str(ret));
        goto cleanup;
    } else if(MEMORY_SYSTEM_LIMIT_EXCEEDED == mem_result) {
        ret = FILESYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("filesystem_create(%s) - memory_sytem_allocate returned LIMIT_EXCEEDED.", rslt_to_str(ret));
        goto cleanup;
    } else if(MEMORY_SYSTEM_BAD_OPERATION == mem_result) {
        ret = FILESYSTEM_BAD_OPERATION;
        ERROR_MESSAGE("filesystem_create(%s) - memory_sytem_allocate returned BAD_OPERATION.", rslt_to_str(ret));
        goto cleanup;
    } else if(MEMORY_SYSTEM_SUCCESS != mem_result) {
        ret = FILESYSTEM_UNDEFINED_ERROR;
        ERROR_MESSAGE("filesystem_create(%s) - Undefined error.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp_filesystem->file_handle = NULL;
    tmp_filesystem->mode = FILESYSTEM_MODE_NONE;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(tmp_filesystem)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_create(%s) - Postcondition validation failed for 'tmp_filesystem'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *filesystem_ = tmp_filesystem;
    tmp_filesystem = NULL;

    ret = FILESYSTEM_SUCCESS;

cleanup:
    if(NULL != tmp_filesystem) {
        memory_system_free(tmp_filesystem, sizeof(filesystem_t), MEMORY_TAG_FILE_IO);
        tmp_filesystem = NULL;
    }
    return ret;
}

void filesystem_destroy(filesystem_t** filesystem_) {
    if(NULL == filesystem_) {
        goto cleanup;
    }
    if(NULL == *filesystem_) {
        goto cleanup;
    }

    if(NULL != (*filesystem_)->file_handle) {
        if(FILESYSTEM_SUCCESS != filesystem_close(*filesystem_)) {
            // エラーが発生しても何もできず、かつ、ハンドルの再利用もできないため、ワーニング出力に留める
            WARN_MESSAGE("filesystem_destroy - Failed to close file handle.");
        }
    }
    memory_system_free((void*)(*filesystem_), sizeof(filesystem_t), MEMORY_TAG_FILE_IO);
    *filesystem_ = NULL;

cleanup:
    return;
}

filesystem_result_t filesystem_open(const char* fullpath_, filesystem_open_mode_t mode_, filesystem_t* filesystem_) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    const char* open_mode_str = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_open", "filesystem_")
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_open", "fullpath_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(filesystem_)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_open(%s) - Precondition validation failed for 'filesystem_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    if(NULL != filesystem_->file_handle) {
        ret = FILESYSTEM_BAD_OPERATION;
        ERROR_MESSAGE("filesystem_open(%s) - File is already open; close it before opening another file.", rslt_to_str(ret));
        goto cleanup;
    }
    open_mode_str = filesystem_open_mode_c_str(mode_);
    if(NULL == open_mode_str) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_open(%s) - Invalid open mode (mode=%d).", rslt_to_str(ret), mode_);
        goto cleanup;
    }
    filesystem_->file_handle = mock_fopen(fullpath_, open_mode_str);
    if(NULL == filesystem_->file_handle) {
        ret = FILESYSTEM_FILE_OPEN_ERROR;
        ERROR_MESSAGE("filesystem_open(%s) - Failed to open file: '%s'.", rslt_to_str(ret), fullpath_);
        goto cleanup;
    }
    filesystem_->mode = mode_;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(filesystem_)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_open(%s) - Postcondition validation failed for 'filesystem_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = FILESYSTEM_SUCCESS;

cleanup:
    return ret;
}

filesystem_result_t filesystem_close(filesystem_t* filesystem_) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_close", "filesystem_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(filesystem_)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_close(%s) - Precondition validation failed for 'filesystem_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    if(NULL == filesystem_->file_handle) {
        ret = FILESYSTEM_RUNTIME_ERROR;
        ERROR_MESSAGE("filesystem_close(%s) - File is already closed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(EOF == mock_fclose(filesystem_->file_handle)) {
        ret = FILESYSTEM_FILE_CLOSE_ERROR;
        ERROR_MESSAGE("filesystem_close(%s) - Failed to close file.", rslt_to_str(ret));
    } else {
        ret = FILESYSTEM_SUCCESS;
    }

    filesystem_->file_handle = NULL;
    filesystem_->mode = FILESYSTEM_MODE_NONE;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(filesystem_)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_close(%s) - Postcondition validation failed for 'filesystem_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

cleanup:
    return ret;
}

filesystem_result_t filesystem_byte_read(size_t read_bytes_, filesystem_t* filesystem_, size_t* result_n_, char* buffer_) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "filesystem_")
    IF_ARG_NULL_GOTO_CLEANUP(result_n_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "result_n_")
    IF_ARG_NULL_GOTO_CLEANUP(buffer_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "buffer_")
    if(0 == read_bytes_) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_byte_read(%s) - provided read_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(filesystem_)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_byte_read(%s) - Precondition validation failed for 'filesystem_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    if(NULL == filesystem_->file_handle) {
        ret = FILESYSTEM_BAD_OPERATION;
        ERROR_MESSAGE("filesystem_byte_read(%s) - provided filehandle is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    if(open_mode_readable(filesystem_->mode)) {
        *result_n_ = mock_fread(buffer_, 1, read_bytes_, filesystem_->file_handle); // (1 x read_bytes_)を読み取り
        if(*result_n_ == read_bytes_) {
            ret = FILESYSTEM_SUCCESS;
        } else {
            if(mock_ferror(filesystem_->file_handle)) {
                ret = FILESYSTEM_RUNTIME_ERROR;
                ERROR_MESSAGE("filesystem_byte_read(%s) - Read failed.", rslt_to_str(ret));
                goto cleanup;
            } else if(mock_feof(filesystem_->file_handle)) {
                if(0 == *result_n_) {
                    ret = FILESYSTEM_EOF;
                } else {
                    ret = FILESYSTEM_SUCCESS;
                }
            } else {
                ret = FILESYSTEM_UNDEFINED_ERROR;
                ERROR_MESSAGE("filesystem_byte_read(%s) - Undefined error.", rslt_to_str(ret));
                goto cleanup;
            }
        }
    } else {
        ret = FILESYSTEM_RUNTIME_ERROR;
        ERROR_MESSAGE("filesystem_byte_read(%s) - File is not opened in a readable mode (mode=%d).", rslt_to_str(ret), filesystem_->mode);
        goto cleanup;
    }

    // filesystem_tのfield間不変条件は変更されないため、postcondition validationは行わない

cleanup:
    if(NULL != result_n_ && FILESYSTEM_SUCCESS != ret) {
        *result_n_ = 0;
    }
    return ret;
}

const char* filesystem_open_mode_c_str(filesystem_open_mode_t mode_) {
    const char* ret;
    switch(mode_) {
        case FILESYSTEM_MODE_NONE:
            ret = NULL;
            break;
        case FILESYSTEM_MODE_READ:
            ret = s_open_mode_read;
            break;
        case FILESYSTEM_MODE_WRITE:
            ret = s_open_mode_write;
            break;
        case FILESYSTEM_MODE_APPEND:
            ret = s_open_mode_append;
            break;
        case FILESYSTEM_MODE_READ_PLUS:
            ret = s_open_mode_read_plus;
            break;
        case FILESYSTEM_MODE_WRITE_PLUS:
            ret = s_open_mode_write_plus;
            break;
        case FILESYSTEM_MODE_APPEND_PLUS:
            ret = s_open_mode_append_plus;
            break;
        case FILESYSTEM_MODE_READ_BINARY:
            ret = s_open_mode_read_binary;
            break;
        case FILESYSTEM_MODE_WRITE_BINARY:
            ret = s_open_mode_write_binary;
            break;
        case FILESYSTEM_MODE_APPEND_BINARY:
            ret = s_open_mode_append_binary;
            break;
        case FILESYSTEM_MODE_READ_PLUS_BINARY:
            ret = s_open_mode_read_plus_binary;
            break;
        case FILESYSTEM_MODE_WRITE_PLUS_BINARY:
            ret = s_open_mode_write_plus_binary;
            break;
        case FILESYSTEM_MODE_APPEND_PLUS_BINARY:
            ret = s_open_mode_append_plus_binary;
            break;
        default:
            ret = NULL;
            break;
    }
    return ret;
}

bool filesystem_is_valid(const filesystem_t* filesystem_) {
    if(NULL == filesystem_) {
        return false;
    }

    if(FILESYSTEM_MODE_NONE == filesystem_->mode) {
        return (NULL == filesystem_->file_handle);
    }

    if(NULL == filesystem_open_mode_c_str(filesystem_->mode)) {
        return false;
    }

    return (NULL != filesystem_->file_handle);
}

/**
 * @brief filesystemモジュール実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ 実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* rslt_to_str(filesystem_result_t rslt_) {
    switch(rslt_) {
    case FILESYSTEM_SUCCESS:
        return s_rslt_str_success;
    case FILESYSTEM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case FILESYSTEM_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case FILESYSTEM_NO_MEMORY:
        return s_rslt_str_no_memory;
    case FILESYSTEM_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case FILESYSTEM_FILE_CLOSE_ERROR:
        return s_rslt_str_file_close_error;
    case FILESYSTEM_EOF:
        return s_rslt_str_eof;
    case FILESYSTEM_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case FILESYSTEM_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case FILESYSTEM_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case FILESYSTEM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

/**
 * @brief mode_がREAD可能なファイルオープンモードかを判定する
 *
 * @param[in] mode_ 判定対象モード
 *
 * @retval true READ可能
 * @retval false READ不可
 */
static bool open_mode_readable(filesystem_open_mode_t mode_) {
    bool ret = false;
    switch(mode_) {
    case FILESYSTEM_MODE_NONE:
        ret = false;
        break;
    case FILESYSTEM_MODE_READ:
        ret = true;
        break;
    case FILESYSTEM_MODE_WRITE:
        ret = false;
        break;
    case FILESYSTEM_MODE_APPEND:
        ret = false;
        break;
    case FILESYSTEM_MODE_READ_PLUS:
        ret = true;
        break;
    case FILESYSTEM_MODE_WRITE_PLUS:
        ret = true;
        break;
    case FILESYSTEM_MODE_APPEND_PLUS:
        ret = true;
        break;
    case FILESYSTEM_MODE_READ_BINARY:
        ret = true;
        break;
    case FILESYSTEM_MODE_WRITE_BINARY:
        ret = false;
        break;
    case FILESYSTEM_MODE_APPEND_BINARY:
        ret = false;
        break;
    case FILESYSTEM_MODE_READ_PLUS_BINARY:
        ret = true;
        break;
    case FILESYSTEM_MODE_WRITE_PLUS_BINARY:
        ret = true;
        break;
    case FILESYSTEM_MODE_APPEND_PLUS_BINARY:
        ret = true;
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}

static FILE* NO_COVERAGE mock_fopen(const char* fullpath_, const char* mode_) {
    return fopen(fullpath_, mode_);
}

static int NO_COVERAGE mock_fclose(FILE* stream_) {
    return fclose(stream_);
}

static size_t NO_COVERAGE mock_fread(void *ptr_, size_t size_, size_t nmemb_, FILE *stream_) {
    return fread(ptr_, size_, nmemb_, stream_);
}

static int NO_COVERAGE mock_ferror(FILE *stream_) {
    return ferror(stream_);
}

static int NO_COVERAGE mock_feof(FILE *stream_) {
    return feof(stream_);
}
