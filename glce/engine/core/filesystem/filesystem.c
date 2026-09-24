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
#include "engine/core/filesystem/filesystem.h"

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/file_io/fs_types.h"

/**
 * @brief ファイルシステムモジュール内部状態管理構造体
 *
 */
struct filesystem {
    FILE* file_handle;              /**< ファイルハンドル */
    fs_open_mode_t mode;    /**< ファイルオープンモード */
};

static FILE* mock_fopen(const char* fullpath_, const char* mode_);
static int mock_fclose(FILE* stream_);
static size_t mock_fread(void *ptr_, size_t size_, size_t nmemb_, FILE *stream_);
static int mock_ferror(FILE *stream_);
static int mock_feof(FILE *stream_);

static const char* result_to_str(filesystem_result_t result_);
static filesystem_result_t result_convert_general_allocator(general_allocator_result_t result_);

static const char* const s_result_str_success = "SUCCESS";                        /**< 実行結果コード文字列: 成功 */
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";      /**< 実行結果コード文字列: 無効な引数 */
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";            /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_result_str_no_memory = "NO_MEMORY";                    /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_result_str_file_open_error = "FILE_OPEN_ERROR";        /**< 実行結果コード文字列: ファイルオープン失敗 */
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";          /**< 実行結果コード文字列: システムリソースが使用可能範囲を超過 */
static const char* const s_result_str_bad_operation = "BAD_OPERATION";            /**< 実行結果コード文字列: API誤用 */
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";        /**< 実行結果コード文字列: 未定義エラー */
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";          /**< 実行結果コード文字列: 内部データ破損 */
static const char* const s_result_str_eof = "EOF";                                /**< 実行結果コード文字列: ファイル読み込みEOF */

filesystem_result_t filesystem_create(filesystem_t** out_filesystem_, const char* fullpath_, fs_open_mode_t mode_) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    filesystem_t* tmp_filesystem = NULL;

    const char* open_mode_str = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "out_filesystem_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "*out_filesystem_")
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "fullpath_")
    if('\0' == fullpath_[0] || '/' != fullpath_[0]) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_create(%s) - Provided fullpath_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(!fs_open_mode_is_valid(mode_)) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_create(%s) - Provided mode_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(filesystem_t), GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO, (void**)&tmp_filesystem);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("filesystem_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    open_mode_str = fs_open_mode_c_str(mode_);
    if(NULL == open_mode_str) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_create(%s) - Invalid open mode (mode=%d).", result_to_str(ret), mode_);
        goto cleanup;
    }
    tmp_filesystem->file_handle = mock_fopen(fullpath_, open_mode_str);
    if(NULL == tmp_filesystem->file_handle) {
        ret = FILESYSTEM_FILE_OPEN_ERROR;
        ERROR_MESSAGE("filesystem_create(%s) - Failed to open file: '%s'.", result_to_str(ret), fullpath_);
        goto cleanup;
    }
    tmp_filesystem->mode = mode_;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(tmp_filesystem)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_create(%s) - Postcondition validation failed for 'tmp_filesystem'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_filesystem_ = tmp_filesystem;
    tmp_filesystem = NULL;

    ret = FILESYSTEM_SUCCESS;

cleanup:
    if(NULL != tmp_filesystem) {
        general_allocator_free((void**)&tmp_filesystem, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);
    }
    return ret;
}

// NOTE: out_close_succeeded_はNULLを許可する(結果が不要な場合はNULLを指定する)
void filesystem_destroy(filesystem_t** filesystem_, bool* out_close_succeeded_) {
    if(NULL == filesystem_) {
        goto cleanup;
    }
    if(NULL == *filesystem_) {
        goto cleanup;
    }

    if(NULL != (*filesystem_)->file_handle) {
        if(EOF == mock_fclose((*filesystem_)->file_handle)) {
            ERROR_MESSAGE("filesystem_destroy - Failed to close file handle.");
            if(NULL != out_close_succeeded_) {
                *out_close_succeeded_ = false;
            }
        } else {
            if(NULL != out_close_succeeded_) {
                *out_close_succeeded_ = true;
            }
        }
    } else {
        if(NULL != out_close_succeeded_) {
            *out_close_succeeded_ = false;
        }
    }
    general_allocator_free((void**)filesystem_, GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO);

cleanup:
    return;
}

filesystem_result_t filesystem_byte_read(filesystem_t* filesystem_, size_t read_bytes_, size_t* out_read_bytes_, char* out_buffer_) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "filesystem_")
    IF_ARG_NULL_GOTO_CLEANUP(out_read_bytes_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "out_read_bytes_")
    IF_ARG_NULL_GOTO_CLEANUP(out_buffer_, ret, FILESYSTEM_INVALID_ARGUMENT, result_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "out_buffer_")
    if(0 == read_bytes_) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_byte_read(%s) - provided read_bytes_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!filesystem_is_valid(filesystem_)) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_byte_read(%s) - Precondition validation failed for 'filesystem_'.", result_to_str(ret));
        goto cleanup;
    }
#endif
    if(NULL == filesystem_->file_handle) {
        ret = FILESYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("filesystem_byte_read(%s) - provided filehandle is not valid.", result_to_str(ret));
        goto cleanup;
    }

    if(fs_open_mode_is_readable(filesystem_->mode)) {
        *out_read_bytes_ = mock_fread(out_buffer_, 1, read_bytes_, filesystem_->file_handle); // (1 x read_bytes_)を読み取り
        if(*out_read_bytes_ == read_bytes_) {
            ret = FILESYSTEM_SUCCESS;
        } else {
            if(mock_ferror(filesystem_->file_handle)) {
                ret = FILESYSTEM_RUNTIME_ERROR;
                ERROR_MESSAGE("filesystem_byte_read(%s) - Read failed.", result_to_str(ret));
                goto cleanup;
            } else if(mock_feof(filesystem_->file_handle)) {
                if(0 == *out_read_bytes_) {
                    ret = FILESYSTEM_EOF;
                } else {
                    ret = FILESYSTEM_SUCCESS;
                }
            } else {
                ret = FILESYSTEM_UNDEFINED_ERROR;
                ERROR_MESSAGE("filesystem_byte_read(%s) - Undefined error.", result_to_str(ret));
                goto cleanup;
            }
        }
    } else {
        ret = FILESYSTEM_BAD_OPERATION;
        ERROR_MESSAGE("filesystem_byte_read(%s) - File is not opened in a readable mode (mode=%d).", result_to_str(ret), filesystem_->mode);
        goto cleanup;
    }

    // filesystem_tのfield間不変条件は変更されないため、postcondition validationは行わない

cleanup:
    if(NULL != out_read_bytes_ && FILESYSTEM_SUCCESS != ret) {
        *out_read_bytes_ = 0;
    }
    return ret;
}

bool filesystem_is_valid(const filesystem_t* filesystem_) {
    if(NULL == filesystem_) {
        return false;
    }
    if(!fs_open_mode_is_valid(filesystem_->mode)) {
        return false;
    }

    return (NULL != filesystem_->file_handle);
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

/**
 * @brief filesystemモジュール実行結果コードを文字列に変換する
 *
 * @param[in] result_ 実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* result_to_str(filesystem_result_t result_) {
    switch(result_) {
    case FILESYSTEM_SUCCESS:
        return s_result_str_success;
    case FILESYSTEM_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case FILESYSTEM_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case FILESYSTEM_NO_MEMORY:
        return s_result_str_no_memory;
    case FILESYSTEM_FILE_OPEN_ERROR:
        return s_result_str_file_open_error;
    case FILESYSTEM_EOF:
        return s_result_str_eof;
    case FILESYSTEM_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case FILESYSTEM_BAD_OPERATION:
        return s_result_str_bad_operation;
    case FILESYSTEM_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case FILESYSTEM_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
    }
}

static filesystem_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return FILESYSTEM_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return FILESYSTEM_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return FILESYSTEM_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return FILESYSTEM_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return FILESYSTEM_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return FILESYSTEM_UNDEFINED_ERROR;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return FILESYSTEM_UNDEFINED_ERROR;
    default:
        return FILESYSTEM_UNDEFINED_ERROR;
    }
}
