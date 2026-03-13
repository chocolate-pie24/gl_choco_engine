/** @ingroup filesystem
 *
 * @file filesystem.c
 * @author chocolate-pie24
 * @brief ファイルシステムモジュールAPIの実装
 *
 * @version 0.1
 * @date 2025-12-23
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "engine/core/filesystem/filesystem.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

// #define TEST_BUILD

#ifdef TEST_BUILD

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "test_controller.h"
#include "core/test_filesystem.h"

static test_config_filesystem_t s_test_config;  /**< File Systemモジュールの外部公開APIテスト設定値 */
static test_call_control_t s_test_mock_fopen;   /**< mock_fopenのテスト設定で、失敗時にはfopenはNULLを返す(test_call_control_tのforced_resultは使用しない) */
static test_call_control_t s_test_mock_fclose;  /**< mock_fcloseのテスト設定で、失敗時にはEOFを返す(test_call_control_tのforced_resultは使用しない) */

/**
 * @brief fread, ferror, feofの返り値をケースごとに設定するためのテストケースリスト
 *
 */
typedef enum {
    FREAD_SUCCESS,      /**< 指定バイト数読み込み成功 */
    FREAD_ERROR,        /**< 読み込みエラー発生 */
    FREAD_EOF_0READ,    /**< EOFかつ、読み込みバイト数0 */
    FREAD_EOF,          /**< EOFかつ、読み込みバイト数が指定バイト数未満 */
    FREAD_UNDEFINED,    /**< 指定バイト数未満の読み込み、エラーなし、EOFなしの異常ケース */
} fread_test_t;

/**
 * @brief ファイルシステムモジュールテスト用構造体
 *
 */
typedef struct filesystem_test {
    bool fread_test_enable;         /**< fread, ferror, feofの挙動をfread_test_caseによって制御する機能を有効にするフラグ(true: 有効 / false: 無効) */
    fread_test_t fread_test_case;   /**< fread, ferror, feofの返り値をケースごとに制御するフラグ */
} filesystem_test_t;

static filesystem_test_t s_fs_test_param;   /**< ファイルシステムモジュールテスト用構造体インスタンス */

static void test_param_reset(void);
static void test_filesystem_create(void);
static void test_filesystem_destroy(void);
static void test_filesystem_open(void);
static void test_filesystem_close(void);
static void test_filesystem_open_mode_c_str(void);
static void test_rslt_to_str(void);
static void test_filesystem_byte_read(void);
static void test_open_mode_readable(void);

#endif

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

static const char* s_open_mode_read = "r";                  /**< ファイルオープンモード文字列: READ */
static const char* s_open_mode_write = "w";                 /**< ファイルオープンモード文字列: WRITE */
static const char* s_open_mode_append = "a";                /**< ファイルオープンモード文字列: APPEND */
static const char* s_open_mode_read_plus = "r+";            /**< ファイルオープンモード文字列: READ+ */
static const char* s_open_mode_write_plus = "w+";           /**< ファイルオープンモード文字列: WRITE+ */
static const char* s_open_mode_append_plus = "a+";          /**< ファイルオープンモード文字列: APPEND+ */
static const char* s_open_mode_read_binary = "rb";          /**< ファイルオープンモード文字列: READ(Binary) */
static const char* s_open_mode_write_binary = "wb";         /**< ファイルオープンモード文字列: WRITE(Binary) */
static const char* s_open_mode_append_binary = "ab";        /**< ファイルオープンモード文字列: APPEND(Binary) */
static const char* s_open_mode_read_plus_binary = "r+b";    /**< ファイルオープンモード文字列: READ+(Binary) */
static const char* s_open_mode_write_plus_binary = "w+b";   /**< ファイルオープンモード文字列: WRITE*(Binary) */
static const char* s_open_mode_append_plus_binary = "a+b";  /**< ファイルオープンモード文字列: APPEND+(Binary) */
static const char* s_open_mode_undefined = "undefined";     /**< ファイルオープンモード文字列: 不明なモード */

static const char* s_rslt_str_success = "SUCCESS";                     /**< 実行結果コード文字列: 成功 */
static const char* s_rslt_str_invalid_argument = "INVALID_ARGUMENT";   /**< 実行結果コード文字列: 無効な引数 */
static const char* s_rslt_str_runtime_error = "RUNTIME_ERROR";         /**< 実行結果コード文字列: 実行時エラー */
static const char* s_rslt_str_no_memory = "NO_MEMORY";                 /**< 実行結果コード文字列: メモリ不足 */
static const char* s_rslt_str_file_open_error = "FILE_OPEN_ERROR";     /**< 実行結果コード文字列: ファイルオープン失敗 */
static const char* s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";   /**< 実行結果コード文字列: ファイルクローズ失敗 */
static const char* s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";       /**< 実行結果コード文字列: システムリソースが使用可能範囲を超過 */
static const char* s_rslt_str_undefined_error = "UNDEFINED_ERROR";     /**< 実行結果コード文字列: 未定義エラー */
static const char* s_rslt_str_eof = "EOF";                             /**< 実行結果コード文字列: ファイル読み込みEOF */

filesystem_result_t filesystem_create(filesystem_t** filesystem_) {
#ifdef TEST_BUILD
    s_test_config.test_filesystem_create.call_count++;
    if(s_test_config.test_filesystem_create.fail_on_call != 0) {
        if(s_test_config.test_filesystem_create.call_count == s_test_config.test_filesystem_create.fail_on_call) {
            return (filesystem_result_t)s_test_config.test_filesystem_create.forced_result;
        }
    }
#endif

    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;
    filesystem_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "filesystem_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "*filesystem_")

    const memory_system_result_t mem_result = memory_system_allocate(sizeof(filesystem_t), MEMORY_TAG_FILE_IO, (void**)&tmp);
    if(MEMORY_SYSTEM_INVALID_ARGUMENT == mem_result) {
        ret = FILESYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("filesystem_create(%s) - memory_system_allocate returned INVALID_ARGUMENT..", rslt_to_str(ret));
        goto cleanup;
    } else if(MEMORY_SYSTEM_NO_MEMORY == mem_result) {
        ret = FILESYSTEM_NO_MEMORY;
        ERROR_MESSAGE("filesystem_create(%s) - memory_system_allocate returned NO_MEMORY..", rslt_to_str(ret));
        goto cleanup;
    } else if(MEMORY_SYSTEM_LIMIT_EXCEEDED == mem_result) {
        ret = FILESYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("filesystem_create(%s) - memory_ssytem_allocate returned LIMIT_EXCEEDED..", rslt_to_str(ret));
        goto cleanup;
    } else if(MEMORY_SYSTEM_SUCCESS != mem_result) {
        ret = FILESYSTEM_UNDEFINED_ERROR;
        ERROR_MESSAGE("filesystem_create(%s) - Undefined error.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp->file_handle = NULL;
    tmp->mode = FILESYSTEM_MODE_NONE;
    *filesystem_ = tmp;

    ret = FILESYSTEM_SUCCESS;

cleanup:
    // NOTE: memory_system_allocate以降でエラーの発生は現状ではないためリソース解放コードはなし
    // ただし、将来的に構造体への変数追加等で仕様変更が発生し、リソース解放が必要になった際に即気付けるよう、assertを仕込んでおく
#ifdef TEST_BUILD
    if(FILESYSTEM_SUCCESS != ret) {
        assert(NULL == tmp);
    }
#endif
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

filesystem_result_t filesystem_open(filesystem_t* filesystem_, const char* fullpath_, filesystem_open_mode_t mode_) {
#ifdef TEST_BUILD
    s_test_config.test_filesystem_open.call_count++;
    if(s_test_config.test_filesystem_open.fail_on_call != 0) {
        if(s_test_config.test_filesystem_open.call_count == s_test_config.test_filesystem_open.fail_on_call) {
            return (filesystem_result_t)s_test_config.test_filesystem_open.forced_result;
        }
    }
#endif

    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_open", "filesystem_")
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_open", "fullpath_")

    if(NULL != filesystem_->file_handle) {
        ret = FILESYSTEM_RUNTIME_ERROR;
        ERROR_MESSAGE("filesystem_open(%s) - File is already open; close it before opening another file.", rslt_to_str(ret));
        goto cleanup;
    }
    const char* open_mode_str = filesystem_open_mode_c_str(mode_);
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
    ret = FILESYSTEM_SUCCESS;

cleanup:
    return ret;
}

filesystem_result_t filesystem_close(filesystem_t* filesystem_) {
#ifdef TEST_BUILD
    s_test_config.test_filesystem_close.call_count++;
    if(s_test_config.test_filesystem_close.fail_on_call != 0) {
        if(s_test_config.test_filesystem_close.call_count == s_test_config.test_filesystem_close.fail_on_call) {
            return (filesystem_result_t)s_test_config.test_filesystem_close.forced_result;
        }
    }
#endif

    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_close", "filesystem_")
    if(NULL == filesystem_->file_handle) {
        ret = FILESYSTEM_RUNTIME_ERROR;
        ERROR_MESSAGE("filesystem_close(%s) - File is already closed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(EOF == mock_fclose(filesystem_->file_handle)) {
        ret = FILESYSTEM_FILE_CLOSE_ERROR;
        filesystem_->file_handle = NULL;    // closeに失敗してもハンドルは再使用不可になっているため、NULLに戻す
        filesystem_->mode = FILESYSTEM_MODE_NONE;
        ERROR_MESSAGE("filesystem_close(%s) - Failed to close file.", rslt_to_str(ret));
        goto cleanup;
    }
    filesystem_->file_handle = NULL;
    filesystem_->mode = FILESYSTEM_MODE_NONE;
    ret = FILESYSTEM_SUCCESS;

cleanup:
    return ret;
}

filesystem_result_t filesystem_byte_read(filesystem_t* filesystem_, size_t read_bytes_, size_t* result_n_, char* buffer_) {
#ifdef TEST_BUILD
    s_test_config.test_filesystem_byte_read.call_count++;
    if(s_test_config.test_filesystem_byte_read.fail_on_call != 0) {
        if(s_test_config.test_filesystem_byte_read.call_count == s_test_config.test_filesystem_byte_read.fail_on_call) {
            return (filesystem_result_t)s_test_config.test_filesystem_byte_read.forced_result;
        }
    }
#endif

    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "filesystem_")
    IF_ARG_NULL_GOTO_CLEANUP(result_n_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "result_n_")
    IF_ARG_NULL_GOTO_CLEANUP(buffer_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "buffer_")
    IF_ARG_NULL_GOTO_CLEANUP(filesystem_->file_handle, ret, FILESYSTEM_RUNTIME_ERROR, rslt_to_str(FILESYSTEM_RUNTIME_ERROR), "filesystem_byte_read", "filesystem_->file_handle")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < read_bytes_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_byte_read", "read_bytes_")

    if(open_mode_readable(filesystem_->mode)) {
        *result_n_ = mock_fread(buffer_, 1, read_bytes_, filesystem_->file_handle);
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
            ret = s_open_mode_undefined;
            break;
    }
    return ret;
}

/**
 * @brief filesystemモジュール実行結果コードを文字列に変換する
 *
 * @param rslt_ 実行結果コード
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
    case FILESYSTEM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

/**
 * @brief mode_がREAD可能なファイルオープンモードかを判定する
 *
 * @param mode_ 判定対象モード
 * @return true READ可能
 * @return false READ不可
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

/**
 * @brief fopenのモック関数で、fopenの実行結果をテスト設定に合わせて制御する
 *
 * @note 以下の条件では強制的にNULLを出力する
 * - 0 != s_test_mock_fopen.fail_on_call && s_test_mock_fopen.call_count == s_test_mock_fopen.fail_on_call
 *
 * @param fullpath_ オープン対象ファイルのフルパス
 * @param mode_ ファイルオープンモード
 * @return FILE* オープンされたファイルハンドル
 */
static FILE* NO_COVERAGE mock_fopen(const char* fullpath_, const char* mode_) {
#ifdef TEST_BUILD
    s_test_mock_fopen.call_count++;
    if(0 != s_test_mock_fopen.fail_on_call) {
        if(s_test_mock_fopen.call_count == s_test_mock_fopen.fail_on_call) {
            return NULL;
        }
    }
#endif
    return fopen(fullpath_, mode_);
}

/**
 * @brief fcloseのモック関数で、fcloseの実行結果をテスト設定に合わせて制御する
 *
 * @note 以下の条件では強制的にEOFを出力する
 * - 0 != s_test_mock_fclose.fail_on_call && s_test_mock_fclose.call_count == s_test_mock_fclose.fail_on_call
 *
 * @param stream_ クローズ対象ファイルハンドル
 * @return int クローズ結果
 */
static int NO_COVERAGE mock_fclose(FILE* stream_) {
#ifdef TEST_BUILD
    s_test_mock_fclose.call_count++;
    if(0 != s_test_mock_fclose.fail_on_call) {
        if(s_test_mock_fclose.call_count == s_test_mock_fclose.fail_on_call) {
            return EOF;
        }
    }
#endif
    return fclose(stream_);
}

/**
 * @brief freadのモック関数で、freadの実行結果をテスト設定に合わせて制御する
 *
 * @note TEST_BUILDかつ、s_fs_test_param.fread_test_enableがtrueの時にテスト設定が有効となり、下記の動作を行う
 * - s_fs_test_param.fread_test_case == FREAD_EOF: EOFかつ、読み込みバイト数が指定バイト数未満とするため、nmemb_ - 1を返す
 * - s_fs_test_param.fread_test_case == FREAD_EOF_0READ: EOFかつ、読み込みバイト数0とするため、0を返す
 * - s_fs_test_param.fread_test_case == FREAD_ERROR: 読み込みエラーとするため、nmemb - 1を返す
 * - s_fs_test_param.fread_test_case == FREAD_SUCCESS: 通常のfread処理を行う
 * - s_fs_test_param.fread_test_case == FREAD_UNDEFINED: 未定義テストケースでnmemb - 1を返す
 * - 上記以外は通常のfread処理を行う
 *
 * @param ptr_ 読み込んだ文字列の格納先バッファ
 * @param size_ 読み込むデータ１つのバイト数
 * @param nmemb_ 読み込むデータの個数
 * @param stream_ 読み込み対象ファイルハンドル
 * @return size_t 読み込んだデータの個数
 */
static size_t NO_COVERAGE mock_fread(void *ptr_, size_t size_, size_t nmemb_, FILE *stream_) {
#ifdef TEST_BUILD
    if(s_fs_test_param.fread_test_enable) {
        if(FREAD_EOF == s_fs_test_param.fread_test_case) {  // EOFかつ、読み込みバイト数が指定バイト数未満
            return nmemb_ - 1;
        } else if(FREAD_EOF_0READ == s_fs_test_param.fread_test_case) { // EOFかつ、読み込みバイト数0
            return 0;
        } else if(FREAD_ERROR == s_fs_test_param.fread_test_case) {     // 読み込みエラー
            return nmemb_ - 1;
        } else if(FREAD_SUCCESS == s_fs_test_param.fread_test_case) {
            return fread(ptr_, size_, nmemb_, stream_);
        } else if(FREAD_UNDEFINED == s_fs_test_param.fread_test_case) {
            return nmemb_ - 1;
        } else {
            assert(false);
        }
    } else {
        return fread(ptr_, size_, nmemb_, stream_);
    }
#else
    return fread(ptr_, size_, nmemb_, stream_);
#endif
}

/**
 * @brief ferrorのモック関数で、ferrorの実行結果をテスト設定に合わせて制御する
 *
 * @note TEST_BUILDかつ、s_fs_test_param.fread_test_enableがtrueの時にテスト設定が有効となり、下記の動作を行う
 * - s_fs_test_param.fread_test_case == FREAD_EOF: freadの結果がEOFかつ、読み込みバイト数0の状況を想定し、0(エラーなし)を返す
 * - s_fs_test_param.fread_test_case == FREAD_EOF_0READ: freadの結果がEOFかつ、読み込みバイト数0の状況を想定し、0(エラーなし)を返す
 * - s_fs_test_param.fread_test_case == FREAD_ERROR: freadでエラーが発生した状況を想定し、1(エラーあり)を返す
 * - s_fs_test_param.fread_test_case == FREAD_SUCCESS: 通常のferrorを実行する
 * - s_fs_test_param.fread_test_case == FREAD_UNDEFINED: 無効なテストケースで0(エラーなし)を返す
 * - 上記以外は通常のferrorを実行する
 *
 * @param stream_ ストリームのテスト対象となるファイルハンドル
 * @retval 0 エラーなし
 * @retval 0以外 エラーあり
 */
static int NO_COVERAGE mock_ferror(FILE *stream_) {
#ifdef TEST_BUILD
    if(s_fs_test_param.fread_test_enable) {
        if(FREAD_EOF == s_fs_test_param.fread_test_case) {  // EOFかつ、読み込みバイト数が指定バイト数未満
            return 0;
        } else if(FREAD_EOF_0READ == s_fs_test_param.fread_test_case) { // EOFかつ、読み込みバイト数0
            return 0;
        } else if(FREAD_ERROR == s_fs_test_param.fread_test_case) {     // 読み込みエラー
            return 1;
        } else if(FREAD_SUCCESS == s_fs_test_param.fread_test_case) {
            return ferror(stream_);
        } else if(FREAD_UNDEFINED == s_fs_test_param.fread_test_case) {
            return 0;
        } else {
            assert(false);
        }
    } else {
        return ferror(stream_);
    }
#else
    return ferror(stream_);
#endif
}

/**
 * @brief feofのモック関数で、feofの実行結果をテスト設定に合わせて制御する
 *
 * @note TEST_BUILDかつ、s_fs_test_param.fread_test_enableがtrueの時にテスト設定が有効となり、下記の動作を行う
 * - s_fs_test_param.fread_test_case == FREAD_EOF: freadの実行結果がEOFかつ、読み込みバイト数が指定バイト数未満の状況を想定し、1(EOF)を返す
 * - s_fs_test_param.fread_test_case == FREAD_EOF_0READ: freadの実行結果がEOFかつ、読み込みバイト数0の状況を想定し、1(EOF)を返す
 * - s_fs_test_param.fread_test_case == FREAD_ERROR: freadの実行結果がエラーの状況を想定し、0(EOF以外)を返す
 * - s_fs_test_param.fread_test_case == FREAD_SUCCESS: 通常のfeofを実行する
 * - s_fs_test_param.fread_test_case == FREAD_UNDEFINED: 無効なテストケースで0(EOF以外)を返す
 * - 上記以外は通常のfeofを実行する
 *
 * @param stream_ EOF判定対象ファイルハンドル
 * @retval 0 EOF以外
 * @retval 0以外 EOF
 */
static int NO_COVERAGE mock_feof(FILE *stream_) {
#ifdef TEST_BUILD
    if(s_fs_test_param.fread_test_enable) {
        if(FREAD_EOF == s_fs_test_param.fread_test_case) {  // EOFかつ、読み込みバイト数が指定バイト数未満
            return 1;
        } else if(FREAD_EOF_0READ == s_fs_test_param.fread_test_case) { // EOFかつ、読み込みバイト数0
            return 1;
        } else if(FREAD_ERROR == s_fs_test_param.fread_test_case) {     // 読み込みエラー
            return 0;
        } else if(FREAD_SUCCESS == s_fs_test_param.fread_test_case) {
            return feof(stream_);
        } else if(FREAD_UNDEFINED == s_fs_test_param.fread_test_case) {
            return 0;
        } else {
            assert(false);
        }
    } else {
        return feof(stream_);
    }
#else
    return feof(stream_);
#endif
}

#ifdef TEST_BUILD

void test_filesystem_config_set(const test_config_filesystem_t* config_) {
    test_call_control_set(config_->test_filesystem_create.fail_on_call, config_->test_filesystem_create.forced_result, &s_test_config.test_filesystem_create);
    test_call_control_set(config_->test_filesystem_open.fail_on_call, config_->test_filesystem_open.forced_result, &s_test_config.test_filesystem_open);
    test_call_control_set(config_->test_filesystem_close.fail_on_call, config_->test_filesystem_close.forced_result, &s_test_config.test_filesystem_close);
    test_call_control_set(config_->test_filesystem_byte_read.fail_on_call, config_->test_filesystem_byte_read.forced_result, &s_test_config.test_filesystem_byte_read);
}

void test_filesystem_config_reset(void) {
    test_call_control_reset(&s_test_config.test_filesystem_create);
    test_call_control_reset(&s_test_config.test_filesystem_open);
    test_call_control_reset(&s_test_config.test_filesystem_close);
    test_call_control_reset(&s_test_config.test_filesystem_byte_read);
}

void test_filesystem(void) {
    test_filesystem_config_reset();
    test_call_control_reset(&s_test_mock_fopen);
    test_call_control_reset(&s_test_mock_fclose);

    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    test_filesystem_create();
    test_filesystem_destroy();
    test_filesystem_open();
    test_filesystem_close();
    test_filesystem_byte_read();
    test_filesystem_open_mode_c_str();
    test_rslt_to_str();
    test_open_mode_readable();

    memory_system_destroy();

    test_call_control_reset(&s_test_mock_fopen);
    test_call_control_reset(&s_test_mock_fclose);
    test_filesystem_config_reset();
}

static void NO_COVERAGE test_filesystem_create(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;
    {
        // 強制エラー出力テスト
        // filesystem_t* tmp = NULL;

        // filesystem_rslt_code_set(FILESYSTEM_INVALID_ARGUMENT);
        // ret = filesystem_create(&tmp);
        // assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        // filesystem_test_param_reset();

        // filesystem_rslt_code_set(FILESYSTEM_RUNTIME_ERROR);
        // ret = filesystem_create(&tmp);
        // assert(FILESYSTEM_RUNTIME_ERROR == ret);
        // filesystem_test_param_reset();

        // filesystem_rslt_code_set(FILESYSTEM_NO_MEMORY);
        // ret = filesystem_create(&tmp);
        // assert(FILESYSTEM_NO_MEMORY == ret);
        // filesystem_test_param_reset();

        // filesystem_rslt_code_set(FILESYSTEM_FILE_OPEN_ERROR);
        // ret = filesystem_create(&tmp);
        // assert(FILESYSTEM_FILE_OPEN_ERROR == ret);
        // filesystem_test_param_reset();

        // filesystem_rslt_code_set(FILESYSTEM_UNDEFINED_ERROR);
        // ret = filesystem_create(&tmp);
        // assert(FILESYSTEM_UNDEFINED_ERROR == ret);
        // filesystem_test_param_reset();

        // filesystem_rslt_code_set(FILESYSTEM_EOF);
        // ret = filesystem_create(&tmp);
        // assert(FILESYSTEM_EOF == ret);
        // filesystem_test_param_reset();
    }
    {
        // 引数チェックエラー
        ret = filesystem_create(NULL);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);

        filesystem_t* tmp = NULL;
        memory_system_result_t ret_mem = memory_system_allocate(sizeof(filesystem_t), MEMORY_TAG_FILE_IO, (void**)&tmp);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != tmp);
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        assert(NULL != tmp);
        memory_system_free(tmp, sizeof(filesystem_t), MEMORY_TAG_FILE_IO);
    }
    {
        // メモリーシステムallocateエラー
        // filesystem_t* tmp = NULL;

        // memory_system_rslt_code_set(MEMORY_SYSTEM_INVALID_ARGUMENT);
        // ret = filesystem_create(&tmp);
        // assert(NULL == tmp);
        // assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        // memory_system_test_param_reset();

        // memory_system_rslt_code_set(MEMORY_SYSTEM_NO_MEMORY);
        // ret = filesystem_create(&tmp);
        // assert(NULL == tmp);
        // assert(FILESYSTEM_NO_MEMORY == ret);
        // memory_system_test_param_reset();

        // memory_system_rslt_code_set(MEMORY_SYSTEM_RUNTIME_ERROR);
        // ret = filesystem_create(&tmp);
        // assert(NULL == tmp);
        // assert(FILESYSTEM_UNDEFINED_ERROR == ret);
        // memory_system_test_param_reset();

        // memory_system_rslt_code_set(MEMORY_SYSTEM_LIMIT_EXCEEDED);
        // ret = filesystem_create(&tmp);
        // assert(NULL == tmp);
        // assert(FILESYSTEM_LIMIT_EXCEEDED == ret);
        // memory_system_test_param_reset();
    }
    {
        // 正常系
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(NULL != tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);
        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
}

static void NO_COVERAGE test_filesystem_destroy(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;
    {
        // 引数NULL, 2重デストロイ
        filesystem_destroy(NULL);

        filesystem_t* tmp = NULL;
        filesystem_destroy(&tmp);
    }
    {
        // file_handle != NULL
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        // closeでエラーが発生し、ワーニングメッセージ
        // s_fs_test_param.fclose_mock_return_eof = true;
        // filesystem_t* tmp = NULL;
        // ret = filesystem_create(&tmp);
        // assert(FILESYSTEM_SUCCESS == ret);
        // assert(NULL != tmp);

        // // NOTE: このテストの実行はプロジェクトルートで行うこと
        // ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        // assert(FILESYSTEM_SUCCESS == ret);
        // assert(FILESYSTEM_MODE_READ == tmp->mode);
        // assert(NULL != tmp->file_handle);

        // FILE* fp = tmp->file_handle;

        // filesystem_destroy(&tmp);
        // assert(NULL == tmp);
        // s_fs_test_param.fclose_mock_return_eof = false;
        // fclose(fp);
    }
}

static void NO_COVERAGE test_filesystem_open(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;
    {
        // 強制エラー出力
        // filesystem_rslt_code_set(FILESYSTEM_FILE_CLOSE_ERROR);
        // ret = filesystem_open(NULL, NULL, FILESYSTEM_MODE_APPEND);
        // assert(FILESYSTEM_FILE_CLOSE_ERROR == ret);
        // filesystem_test_param_reset();
    }
    {
        // filesystem_ == NULL
        ret = filesystem_open(NULL, "aaa", FILESYSTEM_MODE_APPEND);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
    }
    {
        // fullpath_ == NULL
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_open(tmp, NULL, FILESYSTEM_MODE_APPEND);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);

        // destroyを正常に行うためにopenする, 正常系のテストもついで行う
        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);
        assert(NULL == tmp->file_handle);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        // file_handle != NULL
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // destroyを正常に行うためにopenする, 正常系のテストもついで行う
        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);
        assert(NULL == tmp->file_handle);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        // open_mode_str = NULL
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_NONE);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);
        assert(NULL == tmp->file_handle);

        // destroyを正常に行うためにopenする, 正常系のテストもついで行う
        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);
        assert(NULL == tmp->file_handle);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        // fopen失敗
        // filesystem_t* tmp = NULL;
        // ret = filesystem_create(&tmp);
        // assert(FILESYSTEM_SUCCESS == ret);
        // assert(NULL != tmp);

        // s_fs_test_param.fopen_mock_return_null = true;
        // // NOTE: このテストの実行はプロジェクトルートで行うこと
        // ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        // assert(FILESYSTEM_FILE_OPEN_ERROR == ret);
        // assert(FILESYSTEM_MODE_NONE == tmp->mode);
        // assert(NULL == tmp->file_handle);
        // s_fs_test_param.fopen_mock_return_null = false;

        // // destroyを正常に行うためにopenする, 正常系のテストもついで行う
        // // NOTE: このテストの実行はプロジェクトルートで行うこと
        // ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        // assert(FILESYSTEM_SUCCESS == ret);
        // assert(FILESYSTEM_MODE_READ == tmp->mode);
        // assert(NULL != tmp->file_handle);

        // ret = filesystem_close(tmp);
        // assert(FILESYSTEM_SUCCESS == ret);
        // assert(FILESYSTEM_MODE_NONE == tmp->mode);
        // assert(NULL == tmp->file_handle);

        // filesystem_destroy(&tmp);
        // assert(NULL == tmp);
    }
}

static void NO_COVERAGE test_filesystem_close(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;
    {
        // 強制エラー出力
        // filesystem_rslt_code_set(FILESYSTEM_FILE_CLOSE_ERROR);
        // filesystem_t* tmp = NULL;
        // ret = filesystem_close(tmp);
        // assert(NULL == tmp);
        // assert(FILESYSTEM_FILE_CLOSE_ERROR == ret);
        // filesystem_test_param_reset();
    }
    {
        // 引数NULL
        ret = filesystem_close(NULL);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
    }
    {
        // file_handle == NULL
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);
        ret = filesystem_close(tmp);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);
        assert(NULL != tmp);
        filesystem_destroy(&tmp);
        tmp = NULL;
    }
    {
        // fclose失敗
        // s_fs_test_param.fclose_mock_return_eof = true;
        // filesystem_t* tmp = NULL;
        // ret = filesystem_create(&tmp);
        // assert(FILESYSTEM_SUCCESS == ret);
        // assert(NULL != tmp);
        // assert(NULL == tmp->file_handle);
        // assert(FILESYSTEM_MODE_NONE == tmp->mode);

        // // PROJECT_DIRECTORYがbinではなく、ルートになるよう、実行すること
        // ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        // assert(FILESYSTEM_SUCCESS == ret);
        // assert(NULL != tmp->file_handle);
        // assert(FILESYSTEM_MODE_READ == tmp->mode);

        // FILE* fp = tmp->file_handle;

        // ret = filesystem_close(tmp);
        // assert(FILESYSTEM_FILE_CLOSE_ERROR == ret);
        // assert(FILESYSTEM_MODE_NONE == tmp->mode);
        // assert(NULL == tmp->file_handle);
        // filesystem_destroy(&tmp);
        // s_fs_test_param.fclose_mock_return_eof = false;
        // fclose(fp);
    }
    {
        // 正常系
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        // PROJECT_DIRECTORYがbinではなく、ルートになるよう、実行すること
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp->file_handle);
        assert(FILESYSTEM_MODE_READ == tmp->mode);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
    }
}

static void NO_COVERAGE test_filesystem_byte_read(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;
    {
        // 強制エラー出力
        // filesystem_rslt_code_set(FILESYSTEM_FILE_CLOSE_ERROR);
        // ret = filesystem_byte_read(NULL, 64, NULL, NULL);
        // assert(FILESYSTEM_FILE_CLOSE_ERROR == ret);
        // filesystem_test_param_reset();
    }
    {
        // filesystem_ == NULL
        size_t result = 0;
        char buffer[128] = { 0 };
        ret = filesystem_byte_read(NULL, 64, &result, buffer);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
    }
    {
        // result_n_ == NULL
        char buffer[128] = { 0 };
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_byte_read(tmp, 64, NULL, buffer);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        // buffer_ == NULL
        size_t result = 0;
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_byte_read(tmp, 64, &result, NULL);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        // file_handle == NULL
        size_t result = 0;
        char buffer[128] = { 0 };
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_byte_read(tmp, 64, &result, buffer);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        // read_bytes_ == 0
        size_t result = 0;
        char buffer[128] = { 0 };
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file_w.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 0, &result, buffer);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);

        ret = filesystem_close(tmp);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        // mode = write
        size_t result = 0;
        char buffer[128] = { 0 };
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file_w.txt", FILESYSTEM_MODE_WRITE);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_WRITE == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 128, &result, buffer);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);
    }
    {
        // ferrorでエラーを発生させる
        s_fs_test_param.fread_test_enable = true;
        s_fs_test_param.fread_test_case = FREAD_ERROR;

        size_t result = 0;
        char buffer[128] = { 0 };
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 128, &result, buffer);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        s_fs_test_param.fread_test_enable = false;
    }
    {
        // fread成功, ferrorなし, EOFかつ0byte読み込み
        s_fs_test_param.fread_test_enable = true;
        s_fs_test_param.fread_test_case = FREAD_EOF_0READ;

        size_t result = 0;
        char buffer[128] = { 0 };
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 128, &result, buffer);
        assert(FILESYSTEM_EOF == ret);
        assert(0 == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        s_fs_test_param.fread_test_enable = false;
    }
    {
        // fread成功, ferrorなし, EOFかつ1byte以上読み込み
        s_fs_test_param.fread_test_enable = true;
        s_fs_test_param.fread_test_case = FREAD_EOF;

        size_t result = 0;
        char buffer[128] = { 0 };
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 128, &result, buffer);
        assert(FILESYSTEM_SUCCESS == ret);
        assert((128 - 1) == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        s_fs_test_param.fread_test_enable = false;
    }
    {
        // エラーなし、 feofなし、指定バイト数未満読み込み(ありえないケース)
        s_fs_test_param.fread_test_enable = true;
        s_fs_test_param.fread_test_case = FREAD_UNDEFINED;

        size_t result = 0;
        char buffer[128] = { 0 };
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 128, &result, buffer);
        assert(FILESYSTEM_UNDEFINED_ERROR == ret);
        assert(0 == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        s_fs_test_param.fread_test_enable = false;
    }
    {
        // 正常系
        size_t result = 0;
        char buffer[128] = { 0 };
        filesystem_t* tmp = NULL;
        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        // NOTE: このテストの実行はプロジェクトルートで行うこと
        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 2, &result, buffer);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(2 == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        s_fs_test_param.fread_test_enable = false;
    }
}

static void NO_COVERAGE test_open_mode_readable(void) {
    assert(!open_mode_readable(FILESYSTEM_MODE_APPEND));
    assert(!open_mode_readable(FILESYSTEM_MODE_APPEND_BINARY));
    assert(open_mode_readable(FILESYSTEM_MODE_APPEND_PLUS));
    assert(open_mode_readable(FILESYSTEM_MODE_APPEND_PLUS_BINARY));
    assert(!open_mode_readable(FILESYSTEM_MODE_NONE));
    assert(open_mode_readable(FILESYSTEM_MODE_READ));
    assert(open_mode_readable(FILESYSTEM_MODE_READ_BINARY));
    assert(open_mode_readable(FILESYSTEM_MODE_READ_PLUS));
    assert(open_mode_readable(FILESYSTEM_MODE_READ_PLUS_BINARY));
    assert(!open_mode_readable(FILESYSTEM_MODE_WRITE));
    assert(!open_mode_readable(FILESYSTEM_MODE_WRITE_BINARY));
    assert(open_mode_readable(FILESYSTEM_MODE_WRITE_PLUS));
    assert(open_mode_readable(FILESYSTEM_MODE_WRITE_PLUS_BINARY));
    assert(!open_mode_readable(100));
}

static void NO_COVERAGE test_filesystem_open_mode_c_str(void) {
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_NONE);
        assert(NULL == str);
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_READ);
        assert(0 == strcmp(str, s_open_mode_read));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_WRITE);
        assert(0 == strcmp(str, s_open_mode_write));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_APPEND);
        assert(0 == strcmp(str, s_open_mode_append));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_READ_PLUS);
        assert(0 == strcmp(str, s_open_mode_read_plus));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_WRITE_PLUS);
        assert(0 == strcmp(str, s_open_mode_write_plus));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_APPEND_PLUS);
        assert(0 == strcmp(str, s_open_mode_append_plus));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_READ_BINARY);
        assert(0 == strcmp(str, s_open_mode_read_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_WRITE_BINARY);
        assert(0 == strcmp(str, s_open_mode_write_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_APPEND_BINARY);
        assert(0 == strcmp(str, s_open_mode_append_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_READ_PLUS_BINARY);
        assert(0 == strcmp(str, s_open_mode_read_plus_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_WRITE_PLUS_BINARY);
        assert(0 == strcmp(str, s_open_mode_write_plus_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_APPEND_PLUS_BINARY);
        assert(0 == strcmp(str, s_open_mode_append_plus_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(100);
        assert(0 == strcmp(str, s_open_mode_undefined));
    }
}

static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* str = rslt_to_str(FILESYSTEM_SUCCESS);
        assert(0 == strcmp(str, s_rslt_str_success));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_INVALID_ARGUMENT);
        assert(0 == strcmp(str, s_rslt_str_invalid_argument));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_RUNTIME_ERROR);
        assert(0 == strcmp(str, s_rslt_str_runtime_error));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_NO_MEMORY);
        assert(0 == strcmp(str, s_rslt_str_no_memory));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_FILE_OPEN_ERROR);
        assert(0 == strcmp(str, s_rslt_str_file_open_error));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_FILE_CLOSE_ERROR);
        assert(0 == strcmp(str, s_rslt_str_file_close_error));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_LIMIT_EXCEEDED);
        assert(0 == strcmp(str, s_rslt_str_limit_exceeded));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_UNDEFINED_ERROR);
        assert(0 == strcmp(str, s_rslt_str_undefined_error));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_EOF);
        assert(0 == strcmp(str, s_rslt_str_eof));
    }
    {
        const char* str = rslt_to_str(100);
        assert(0 == strcmp(str, s_rslt_str_undefined_error));
    }
}
#endif
