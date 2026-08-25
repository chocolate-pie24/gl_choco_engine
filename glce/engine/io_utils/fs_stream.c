// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/io_utils/fs_stream.h"

#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/filesystem/filesystem.h"
#include "engine/core/file_io/fs_types.h"

#include "engine/containers/choco_string.h"

#define FS_STREAM_READ_UNIT_SIZE 512  /**< ファイル読み込みの際に一度に読み込むバイト数(メモリの動的確保回数を減らすため,固定値にした) */
#define FS_STREAM_TEXT_FILE_LINE_BUFFER_SIZE 1024    /**< 改行コードを除く1行本文の最大長は1023bytes(改行コードがCRLFの場合は1022bytesとなる) */

struct fs_stream {
    filesystem_t* filesystem;
};

static const char* const s_rslt_str_success = "SUCCESS";                      /**< 実行結果コード文字列: 正常終了 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";    /**< 実行結果コード文字列: 無効な引数 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";          /**< 実行結果コード文字列: API誤用 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";        /**< 実行結果コード文字列: 内部データ破損or未初期化 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                  /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";        /**< 実行結果コード文字列: システム使用可能範囲超過 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                    /**< 実行結果コード文字列: 計算オーバーフロー */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";      /**< 実行結果コード文字列: ファイルオープンエラー */
static const char* const s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";    /**< 実行結果コード文字列: ファイルクローズエラー */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";          /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";      /**< 実行結果コード文字列: 想定していないエラーが発生 */
static const char* const s_rslt_str_eof = "EOF";                              /**< 実行結果コード文字列: EOF */

static const char* rslt_to_str(fs_stream_result_t rslt_);
static fs_stream_result_t filesystem_result_convert(filesystem_result_t result_);
static fs_stream_result_t memory_system_result_convert(memory_system_result_t result_);
static fs_stream_result_t choco_string_result_convert(choco_string_result_t result_);

fs_stream_result_t fs_stream_create(fs_stream_t** out_fs_stream_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;
    filesystem_result_t ret_filesystem = FILESYSTEM_INVALID_ARGUMENT;

    fs_stream_t* tmp_fs_stream = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_fs_stream_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_create", "out_fs_stream_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_fs_stream_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_create", "*out_fs_stream_")

    ret_memory_system = memory_system_allocate(sizeof(fs_stream_t), MEMORY_TAG_FILE_IO, (void**)&tmp_fs_stream);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = memory_system_result_convert(ret_memory_system);
        ERROR_MESSAGE("fs_stream_create(%s) - memory_system_allocate failed.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_fs_stream, 0, sizeof(fs_stream_t));

    ret_filesystem = filesystem_create(&tmp_fs_stream->filesystem);
    if(FILESYSTEM_SUCCESS != ret_filesystem) {
        ret = filesystem_result_convert(ret_filesystem);
        ERROR_MESSAGE("fs_stream_create(%s) - filesystem_create failed.", rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_stream_is_valid(tmp_fs_stream)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_create(%s) - Postcondition validation failed for 'tmp_fs_stream'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_fs_stream_ = tmp_fs_stream;
    tmp_fs_stream = NULL;

    ret = FS_STREAM_SUCCESS;

cleanup:
    if(NULL != tmp_fs_stream) {
        filesystem_destroy(&tmp_fs_stream->filesystem);
        memory_system_free(tmp_fs_stream, sizeof(fs_stream_t), MEMORY_TAG_FILE_IO);
        tmp_fs_stream = NULL;
    }
    return ret;
}

void fs_stream_destroy(fs_stream_t** fs_stream_) {
    if(NULL == fs_stream_) {
        return;
    }
    if(NULL == *fs_stream_) {
        return;
    }
    filesystem_destroy(&(*fs_stream_)->filesystem);
    memory_system_free(*fs_stream_, sizeof(fs_stream_t), MEMORY_TAG_FILE_IO);
    *fs_stream_ = NULL;
}

fs_stream_result_t fs_stream_open(fs_stream_t* fs_stream_, const char* fullpath_, fs_open_mode_t open_mode_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    filesystem_result_t ret_filesystem = FILESYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(fs_stream_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_open", "fs_stream_")
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_open", "fullpath_")
    if('\0' == fullpath_[0]) {
        ret = FS_STREAM_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_stream_open(%s) - Provided fullpath_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!fs_open_mode_is_valid(open_mode_) || FS_OPEN_MODE_NONE == open_mode_) {
        ret = FS_STREAM_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_stream_open(%s) - Provided open_mode_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_stream_is_valid(fs_stream_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_open(%s) - Precondition validation failed for 'fs_stream_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_filesystem = filesystem_open(fullpath_, open_mode_, fs_stream_->filesystem);
    if(FILESYSTEM_SUCCESS != ret_filesystem) {
        ret = filesystem_result_convert(ret_filesystem);
        ERROR_MESSAGE("fs_stream_open(%s) - filesystem_open failed.", rslt_to_str(ret));
        goto cleanup;
    }

    // 下位filesystemが成功時postconditionを保証し、fs_stream固有fieldは変更されない

    ret = FS_STREAM_SUCCESS;

cleanup:
    return ret;
}

fs_stream_result_t fs_stream_close(fs_stream_t* fs_stream_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    filesystem_result_t ret_filesystem = FILESYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(fs_stream_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_close", "fs_stream_")

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_stream_is_valid(fs_stream_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_close(%s) - Precondition validation failed for 'fs_stream_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_filesystem = filesystem_close(fs_stream_->filesystem);
    if(FILESYSTEM_SUCCESS != ret_filesystem) {
        ret = filesystem_result_convert(ret_filesystem);
        ERROR_MESSAGE("fs_stream_close(%s) - filesystem_close failed.", rslt_to_str(ret));
        goto cleanup;
    }

    // 下位filesystemが成功時postconditionを保証し、fs_stream固有fieldは変更されない

    ret = FS_STREAM_SUCCESS;

cleanup:
    return ret;
}

fs_stream_result_t fs_stream_byte_read(fs_stream_t* fs_stream_, size_t read_bytes_, size_t* result_n_, char* buffer_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    filesystem_result_t ret_filesystem = FILESYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(fs_stream_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_byte_read", "fs_stream_")
    IF_ARG_NULL_GOTO_CLEANUP(result_n_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_byte_read", "result_n_")
    IF_ARG_NULL_GOTO_CLEANUP(buffer_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_byte_read", "buffer_")
    if(0 == read_bytes_) {
        ret = FS_STREAM_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_stream_byte_read(%s) - Provided read_bytes_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!fs_stream_is_valid(fs_stream_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_byte_read(%s) - Precondition validation failed for 'fs_stream_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_filesystem = filesystem_byte_read(fs_stream_->filesystem, read_bytes_, result_n_, buffer_);
    if(FILESYSTEM_SUCCESS != ret_filesystem) {
        ret = filesystem_result_convert(ret_filesystem);
        if(FS_STREAM_EOF != ret) {
            ERROR_MESSAGE("fs_stream_byte_read(%s) - filesystem_byte_read failed.", rslt_to_str(ret));
        }
        goto cleanup;
    }

    // fs_stream／filesystemのfield間不変条件は変更されない

    ret = FS_STREAM_SUCCESS;

cleanup:
    if(NULL != result_n_ && FS_STREAM_SUCCESS != ret) {
        *result_n_ = 0;
    }
    return ret;
}

bool fs_stream_is_valid(const fs_stream_t* fs_stream_) {
    if(NULL == fs_stream_) {
        return false;
    }
    if(!filesystem_is_valid(fs_stream_->filesystem)) {
        return false;
    }

    return true;
}

fs_stream_result_t fs_stream_text_file_read(fs_stream_t* fs_stream_, choco_string_t* out_string_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;

    bool complete = false;

    IF_ARG_NULL_GOTO_CLEANUP(fs_stream_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_text_file_read", "fs_stream_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_text_file_read", "out_string_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(out_string_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_text_file_read(%s) - Precondition validation failed for 'out_string_'.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!fs_stream_is_valid(fs_stream_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_text_file_read(%s) - Precondition validation failed for 'fs_stream_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    while(!complete) {
        char tmp_buffer[FS_STREAM_READ_UNIT_SIZE + 1] = { 0 };
        size_t result = 0;
        ret = fs_stream_byte_read(fs_stream_, FS_STREAM_READ_UNIT_SIZE, &result, tmp_buffer);
        if(FS_STREAM_EOF == ret) {
            complete = true;
        } else if(FS_STREAM_SUCCESS == ret) {
            ret_str = choco_string_concat_from_c_string(tmp_buffer, out_string_);
            if(CHOCO_STRING_SUCCESS != ret_str) {
                ret = choco_string_result_convert(ret_str);
                ERROR_MESSAGE("fs_stream_text_file_read(%s) - Failed to append to output string.", rslt_to_str(ret));
                goto cleanup;
            }
            if(FS_STREAM_READ_UNIT_SIZE > result) {
                complete = true;
            }
        } else {
            ERROR_MESSAGE("fs_stream_text_file_read(%s) - fs_stream_byte_read failed.", rslt_to_str(ret));
            goto cleanup;
        }
    }

    ret = FS_STREAM_SUCCESS;

cleanup:
    return ret;
}

fs_stream_result_t fs_stream_text_file_line_read(fs_stream_t* fs_stream_, choco_string_t* out_string_) {
    fs_stream_result_t ret = FS_STREAM_INVALID_ARGUMENT;

    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;

    bool complete = false;
    size_t read_byte = 0;
    char tmp_buffer[FS_STREAM_TEXT_FILE_LINE_BUFFER_SIZE] = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(fs_stream_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_text_file_line_read", "fs_stream_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, FS_STREAM_INVALID_ARGUMENT, rslt_to_str(FS_STREAM_INVALID_ARGUMENT), "fs_stream_text_file_line_read", "out_string_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(out_string_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_text_file_line_read(%s) - Precondition validation failed for 'out_string_'.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!fs_stream_is_valid(fs_stream_)) {
        ret = FS_STREAM_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_stream_text_file_line_read(%s) - Precondition validation failed for 'fs_stream_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    while(!complete) {
        char tmp = 0;   // 改行コードは文字数カウントに含めないため、一時的にtmpにコピーし、改行コード以外だったらtmp_bufferにコピー
        size_t result = 0;
        ret = fs_stream_byte_read(fs_stream_, 1, &result, &tmp);
        if(FS_STREAM_EOF == ret) {
            complete = true;
            if(0 == read_byte) {
                ret = FS_STREAM_EOF;
                goto cleanup;
            }
        } else if(FS_STREAM_SUCCESS == ret) {
            if('\n' == tmp) {
                complete = true;
            } else {
                tmp_buffer[read_byte] = tmp;
                read_byte++;
            }
            if(read_byte == FS_STREAM_TEXT_FILE_LINE_BUFFER_SIZE) {
                ret = FS_STREAM_LIMIT_EXCEEDED;
                ERROR_MESSAGE("fs_stream_text_file_line_read(%s) - Line length exceeded. Max body length is 1023 bytes for LF/EOF lines, or 1022 bytes for CRLF lines.", rslt_to_str(ret));
                goto cleanup;
            }
        } else {
            ERROR_MESSAGE("fs_stream_text_file_line_read(%s) - Failed to read 1 byte while reading a text line.", rslt_to_str(ret));
            goto cleanup;
        }
    }

    if(read_byte != 0 && '\r' == tmp_buffer[read_byte - 1]) {
        tmp_buffer[read_byte - 1] = '\0';
    }

    ret_str = choco_string_copy_from_c_string(tmp_buffer, out_string_);
    if(CHOCO_STRING_SUCCESS != ret_str) {
        ret = choco_string_result_convert(ret_str);
        ERROR_MESSAGE("fs_stream_text_file_line_read(%s) - Failed to copy the read line to out_string_.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = FS_STREAM_SUCCESS;

cleanup:
    return ret;
}

static const char* rslt_to_str(fs_stream_result_t rslt_) {
    switch(rslt_) {
    case FS_STREAM_SUCCESS:
        return s_rslt_str_success;
    case FS_STREAM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case FS_STREAM_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case FS_STREAM_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case FS_STREAM_NO_MEMORY:
        return s_rslt_str_no_memory;
    case FS_STREAM_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case FS_STREAM_OVERFLOW:
        return s_rslt_str_overflow;
    case FS_STREAM_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case FS_STREAM_FILE_CLOSE_ERROR:
        return s_rslt_str_file_close_error;
    case FS_STREAM_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case FS_STREAM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case FS_STREAM_EOF:
        return s_rslt_str_eof;
    default:
        return s_rslt_str_undefined_error;
    }
}

static fs_stream_result_t filesystem_result_convert(filesystem_result_t result_) {
    switch(result_) {
    case FILESYSTEM_SUCCESS:
        return FS_STREAM_SUCCESS;
    case FILESYSTEM_INVALID_ARGUMENT:
        return FS_STREAM_INVALID_ARGUMENT;
    case FILESYSTEM_RUNTIME_ERROR:
        return FS_STREAM_RUNTIME_ERROR;
    case FILESYSTEM_NO_MEMORY:
        return FS_STREAM_NO_MEMORY;
    case FILESYSTEM_FILE_OPEN_ERROR:
        return FS_STREAM_FILE_OPEN_ERROR;
    case FILESYSTEM_UNDEFINED_ERROR:
        return FS_STREAM_UNDEFINED_ERROR;
    case FILESYSTEM_LIMIT_EXCEEDED:
        return FS_STREAM_LIMIT_EXCEEDED;
    case FILESYSTEM_BAD_OPERATION:
        return FS_STREAM_BAD_OPERATION;
    case FILESYSTEM_EOF:
        return FS_STREAM_EOF;
    case FILESYSTEM_FILE_CLOSE_ERROR:
        return FS_STREAM_FILE_CLOSE_ERROR;
    case FILESYSTEM_DATA_CORRUPTED:
        return FS_STREAM_DATA_CORRUPTED;
    default:
        return FS_STREAM_UNDEFINED_ERROR;
    }
}

static fs_stream_result_t memory_system_result_convert(memory_system_result_t result_) {
    switch(result_) {
    case MEMORY_SYSTEM_SUCCESS:
        return FS_STREAM_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return FS_STREAM_UNDEFINED_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return FS_STREAM_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return FS_STREAM_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return FS_STREAM_NO_MEMORY;
    default:
        return FS_STREAM_UNDEFINED_ERROR;
    }
}

static fs_stream_result_t choco_string_result_convert(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return FS_STREAM_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return FS_STREAM_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return FS_STREAM_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return FS_STREAM_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return FS_STREAM_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return FS_STREAM_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return FS_STREAM_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return FS_STREAM_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return FS_STREAM_LIMIT_EXCEEDED;
    default:
        return FS_STREAM_UNDEFINED_ERROR;
    }
}
