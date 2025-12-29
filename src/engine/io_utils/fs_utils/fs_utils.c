#include <string.h> // for memset
#include <stddef.h>

#include "engine/io_utils/fs_utils/fs_utils.h"

#include "engine/containers/choco_string.h"

#include "engine/core/filesystem/filesystem.h"
#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#define FS_READ_UNIT_SIZE 512  /**< ファイル読み込みの際に一度に読み込むバイト数(メモリの動的確保回数を減らすため,固定値にした) */

/**
 * @brief ファイルシステムユーティリティ内部状態管理構造体
 *
 */
struct fs_utils {
    filesystem_t* filesystem;       /**< ファイルシステム構造体インスタンスへのポインタ */
    choco_string_t* filepath;       /**< ファイルパス文字列格納コンテナ */
    choco_string_t* filename;       /**< ファイル名文字列格納コンテナ */
    choco_string_t* extension;      /**< ファイル拡張子文字列格納コンテナ */
    filesystem_open_mode_t mode;    /**< ファイルオープンモード */
};

static const char* s_rslt_str_success = "SUCCESS";                      /**< 実行結果コード文字列: 正常終了 */
static const char* s_rslt_str_invalid_argument = "INVALID_ARGUMENT";    /**< 実行結果コード文字列: 無効な引数 */
static const char* s_rslt_str_bad_operation = "BAD_OPERATION";          /**< 実行結果コード文字列: API誤用 */
static const char* s_rslt_str_data_corrupted = "DATA_CORRUPTED";        /**< 実行結果コード文字列: 内部データ破損or未初期化 */
static const char* s_rslt_str_no_memory = "NO_MEMORY";                  /**< 実行結果コード文字列: メモリ不足 */
static const char* s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";        /**< 実行結果コード文字列: システム使用可能範囲超過 */
static const char* s_rslt_str_overflow = "OVERFLOW";                    /**< 実行結果コード文字列: 計算オーバーフロー */
static const char* s_rslt_str_file_open_error = "FILE_OPEN_ERROR";      /**< 実行結果コード文字列: ファイルオープンエラー */
static const char* s_rslt_str_runtime_error = "RUNTIME_ERROR";          /**< 実行結果コード文字列: 実行時エラー */
static const char* s_rslt_str_undefined_error = "UNDEFINED_ERROR";      /**< 実行結果コード文字列: 想定していないエラーが発生 */

static const char* rslt_to_str(fs_utils_result_t result_);
static bool fs_utils_valid_check(fs_utils_t* fs_utils_);
static fs_utils_result_t filesystem_result_convert(filesystem_result_t result_);
static fs_utils_result_t choco_string_result_convert(choco_string_result_t result_);
static fs_utils_result_t memory_system_result_convert(memory_system_result_t result_);

// filepath_の末尾には必ず'/'をつけること
// extensionは.から始めること
fs_utils_result_t fs_utils_create(const char* filepath_, const char* filename_, const char* extension_, filesystem_open_mode_t open_mode_, fs_utils_t** fs_utils_) {
    fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;
    fs_utils_t* tmp_fs_utils = NULL;

    choco_string_t* tmp_fullpath = NULL;

    // Preconditions.
    CHECK_ARG_NULL_GOTO_CLEANUP(filepath_, FS_UTILS_INVALID_ARGUMENT, "fs_utils_create", "filepath_")
    CHECK_ARG_NULL_GOTO_CLEANUP(filename_, FS_UTILS_INVALID_ARGUMENT, "fs_utils_create", "filename_")
    CHECK_ARG_NULL_GOTO_CLEANUP(fs_utils_, FS_UTILS_INVALID_ARGUMENT, "fs_utils_create", "fs_utils_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*fs_utils_, FS_UTILS_INVALID_ARGUMENT, "fs_utils_create", "*fs_utils_")
    // extensionはない場合があるのでNULLを許可

    // Simulation.
    ret_mem = memory_system_allocate(sizeof(fs_utils_t), MEMORY_TAG_FILE_IO, (void**)&tmp_fs_utils);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = memory_system_result_convert(ret_mem);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to allocate memory for 'tmp_fs_utils'.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_fs_utils, 0, sizeof(fs_utils_t));
    tmp_fs_utils->mode = open_mode_;

    ret_str = choco_string_create_from_c_string(&tmp_fs_utils->filepath, filepath_);
    if(CHOCO_STRING_SUCCESS != ret_str) {
        ret = choco_string_result_convert(ret_str);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to initialize string from char for 'tmp_fs_utils->filepath'.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_str = choco_string_create_from_c_string(&tmp_fs_utils->filename, filename_);
    if(CHOCO_STRING_SUCCESS != ret_str) {
        ret = choco_string_result_convert(ret_str);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to initialize string from char for 'tmp_fs_utils->filename'.", rslt_to_str(ret));
        goto cleanup;
    }

    if(NULL == extension_) {
        tmp_fs_utils->extension = NULL;
    } else {
        ret_str = choco_string_create_from_c_string(&tmp_fs_utils->extension, extension_);
        if(CHOCO_STRING_SUCCESS != ret_str) {
            ret = choco_string_result_convert(ret_str);
            ERROR_MESSAGE("fs_utils_create(%s) - Failed to initialize string from char for 'tmp_fs_utils->extension'.", rslt_to_str(ret));
            goto cleanup;
        }
    }

    ret_fs = filesystem_create(&tmp_fs_utils->filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to create filesystem for 'tmp_fs_utils->filesystem'.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_str = choco_string_default_create(&tmp_fullpath);
    if(CHOCO_STRING_SUCCESS != ret_str) {
        ret = choco_string_result_convert(ret_str);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to create fullpath string instance.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = fs_utils_fullpath_get(tmp_fs_utils, tmp_fullpath);
    if(FS_UTILS_SUCCESS != ret) {
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to build fullpath string.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs = filesystem_open(tmp_fs_utils->filesystem, choco_string_c_str(tmp_fullpath), open_mode_);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to open file. File open mode = '%s'.", rslt_to_str(ret), filesystem_open_mode_c_str(open_mode_));
        goto cleanup;
    }
    choco_string_destroy(&tmp_fullpath);

    *fs_utils_ = tmp_fs_utils;
    ret = FS_UTILS_SUCCESS;

cleanup:
    if(FS_UTILS_SUCCESS != ret) {
        choco_string_destroy(&tmp_fullpath);
        fs_utils_destroy(&tmp_fs_utils);
    }
    return ret;
}

void fs_utils_destroy(fs_utils_t** fs_utils_) {
    if(NULL == fs_utils_) {
        return;
    }
    if(NULL == *fs_utils_) {
        return;
    }
    choco_string_destroy(&(*fs_utils_)->extension);
    choco_string_destroy(&(*fs_utils_)->filepath);
    choco_string_destroy(&(*fs_utils_)->filename);
    filesystem_destroy(&(*fs_utils_)->filesystem);
    memory_system_free(*fs_utils_, sizeof(fs_utils_t), MEMORY_TAG_FILE_IO);
    *fs_utils_ = NULL;
}

/*
バイナリファイルの読み込み禁止
*/
fs_utils_result_t fs_utils_text_file_read(fs_utils_t* fs_utils_, choco_string_t* out_string_) {
    fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(fs_utils_, FS_UTILS_INVALID_ARGUMENT, "fs_utils_text_file_read", "fs_utils_")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_string_, FS_UTILS_INVALID_ARGUMENT, "fs_utils_text_file_read", "out_string_")
    if(!fs_utils_valid_check(fs_utils_)) {
        // fs_utilsはfs_utils_createを経由して生成されたものであればvalidであることが保証されるため,ここを通るということはデータが壊れているかデータが未初期化
        ret = FS_UTILS_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_utils_text_file_read(%s) - The provided fs_utils_ is corrupted or uninitialized.", rslt_to_str(ret));
        goto cleanup;
    }

    // APPEND_PLUSはオープンした際のファイル位置がEOFになるため、想定した動作を行うことができないため禁止
    if(fs_utils_->mode != FILESYSTEM_MODE_READ && fs_utils_->mode != FILESYSTEM_MODE_READ_PLUS ) {
        ret = FS_UTILS_BAD_OPERATION;
        ERROR_MESSAGE("fs_utils_text_file_read(%s) - Invalid open mode for text file read.", rslt_to_str(ret));
        goto cleanup;
    }

    bool complete = false;
    while(!complete) {
        char tmp_buffer[FS_READ_UNIT_SIZE + 1] = { 0 };
        size_t result = 0;
        ret_fs = filesystem_byte_read(fs_utils_->filesystem, FS_READ_UNIT_SIZE, &result, tmp_buffer);
        if(FILESYSTEM_INVALID_ARGUMENT == ret_fs) {
            ret = FS_UTILS_INVALID_ARGUMENT;
            ERROR_MESSAGE("fs_utils_text_file_read(%s) - Failed to read from file.", rslt_to_str(ret));
            goto cleanup;
        } else if(FILESYSTEM_RUNTIME_ERROR == ret_fs) {
            ret = FS_UTILS_RUNTIME_ERROR;
            ERROR_MESSAGE("fs_utils_text_file_read(%s) - Failed to read from file.", rslt_to_str(ret));
            goto cleanup;
        } else if(FILESYSTEM_EOF == ret_fs) {
            complete = true;
        } else if(FILESYSTEM_SUCCESS == ret_fs) {
            ret_str = choco_string_concat_from_c_string(tmp_buffer, out_string_);
            if(CHOCO_STRING_SUCCESS != ret_str) {
                ret = choco_string_result_convert(ret_str);
                ERROR_MESSAGE("fs_utils_text_file_read(%s) - Failed to append to output string.", rslt_to_str(ret));
                goto cleanup;
            }
            if(FS_READ_UNIT_SIZE > result) {
                complete = true;
            }
        }
    }
    ret = FS_UTILS_SUCCESS;

cleanup:
    return ret;
}

fs_utils_result_t fs_utils_fullpath_get(fs_utils_t* fs_utils_, choco_string_t* out_fullpath_) {
    fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(fs_utils_, FS_UTILS_INVALID_ARGUMENT, "fs_utils_fullpath_get", "fs_utils_")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_fullpath_, FS_UTILS_INVALID_ARGUMENT, "fs_utils_fullpath_get", "out_fullpath_")
    if(!fs_utils_valid_check(fs_utils_)) {
        // fs_utilsはfs_utils_createを経由して生成されたものであればvalidであることが保証されるため,ここを通るということはデータが壊れているかデータが未初期化
        ret = FS_UTILS_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_utils_fullpath_get(%s) - The provided fs_utils_ is corrupted or uninitialized.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_str = choco_string_copy(out_fullpath_, fs_utils_->filepath);
    if(CHOCO_STRING_SUCCESS != ret_str) {
        ret = choco_string_result_convert(ret_str);
        ERROR_MESSAGE("fs_utils_fullpath_get(%s) - Failed to copy filepath string.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_str = choco_string_concat(fs_utils_->filename, out_fullpath_);
    if(CHOCO_STRING_SUCCESS != ret_str) {
        ret = choco_string_result_convert(ret_str);
        ERROR_MESSAGE("fs_utils_fullpath_get(%s) - Failed to append filename string.", rslt_to_str(ret));
        goto cleanup;
    }

    if(NULL != fs_utils_->extension) {
        ret_str = choco_string_concat(fs_utils_->extension, out_fullpath_);
        if(CHOCO_STRING_SUCCESS != ret_str) {
            ret = choco_string_result_convert(ret_str);
            ERROR_MESSAGE("fs_utils_fullpath_get(%s) - Failed to append extension string.", rslt_to_str(ret));
            goto cleanup;
        }
    }
    ret = FS_UTILS_SUCCESS;

cleanup:
    return ret;
}

static const char* rslt_to_str(fs_utils_result_t result_) {
    switch(result_) {
    case FS_UTILS_SUCCESS:
        return s_rslt_str_success;
    case FS_UTILS_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case FS_UTILS_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case FS_UTILS_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case FS_UTILS_NO_MEMORY:
        return s_rslt_str_no_memory;
    case FS_UTILS_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case FS_UTILS_OVERFLOW:
        return s_rslt_str_overflow;
    case FS_UTILS_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case FS_UTILS_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case FS_UTILS_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

static bool fs_utils_valid_check(fs_utils_t* fs_utils_) {
    // extensionはNULLを許可
    if(NULL == fs_utils_) {
        return false;
    } else if(NULL == fs_utils_->filename) {
        return false;
    } else if(NULL == fs_utils_->filepath) {
        return false;
    } else if(NULL == fs_utils_->filesystem) {
        return false;
    } else if(FILESYSTEM_MODE_NONE == fs_utils_->mode) {
        return false;
    } else {
        return true;
    }
}

static fs_utils_result_t filesystem_result_convert(filesystem_result_t result_) {
    switch(result_) {
    case FILESYSTEM_SUCCESS:
        return FS_UTILS_SUCCESS;
    case FILESYSTEM_INVALID_ARGUMENT:
        return FS_UTILS_INVALID_ARGUMENT;
    case FILESYSTEM_RUNTIME_ERROR:
        return FS_UTILS_RUNTIME_ERROR;
    case FILESYSTEM_NO_MEMORY:
        return FS_UTILS_NO_MEMORY;
    case FILESYSTEM_FILE_OPEN_ERROR:
        return FS_UTILS_FILE_OPEN_ERROR;
    case FILESYSTEM_UNDEFINED_ERROR:
        return FS_UTILS_UNDEFINED_ERROR;
    case FILESYSTEM_LIMIT_EXCEEDED:
        return FS_UTILS_LIMIT_EXCEEDED;
    case FILESYSTEM_EOF:
        return FS_UTILS_UNDEFINED_ERROR;    // EOFを出力するAPIを使う場所では本関数ではなく、直接実行結果コードを見る必要があるため、本関数ではUNDEFINEDとする
    case FILESYSTEM_FILE_CLOSE_ERROR:
        return FS_UTILS_UNDEFINED_ERROR;    // fs_utilsではdestoryでファイルクローズを行うため、クローズエラーは発生しないはず
    default:
        return FS_UTILS_UNDEFINED_ERROR;
    }
}

static fs_utils_result_t choco_string_result_convert(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return FS_UTILS_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return FS_UTILS_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return FS_UTILS_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return FS_UTILS_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return FS_UTILS_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return FS_UTILS_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return FS_UTILS_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return FS_UTILS_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return FS_UTILS_LIMIT_EXCEEDED;
    default:
        return FS_UTILS_UNDEFINED_ERROR;
    }
}

static fs_utils_result_t memory_system_result_convert(memory_system_result_t result_) {
    switch(result_) {
    case MEMORY_SYSTEM_SUCCESS:
        return FS_UTILS_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return FS_UTILS_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return FS_UTILS_RUNTIME_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return FS_UTILS_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_NO_MEMORY:
        return FS_UTILS_NO_MEMORY;
    default:
        return FS_UTILS_UNDEFINED_ERROR;
    }
}
