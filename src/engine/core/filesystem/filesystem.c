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
// テスト時のみ使用するヘッダのinclude
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "test_controller.h"
#include "engine/core/filesystem/test_filesystem.h"
#include "engine/core/memory/test_choco_memory.h"

// モジュール保有APIテストシナリオ用データ構造定義
/**
 * @brief mock_fopen()テストシナリオリスト
 *
 */
typedef enum {
    TEST_SCENARIO_FOPEN_SUCCESS = 0,    /**< fopen成功 */
    TEST_SCENARIO_FOPEN_OPEN_ERROR,     /**< fopen失敗(fopenはNULLを返す) */
} test_scenario_fopen_t;

/**
 * @brief mock_fclose()テストシナリオリスト
 *
 */
typedef enum {
    TEST_SCENARIO_FCLOSE_SUCCESS = 0,   /**< fclose成功(fcloseは0を返す) */
    TEST_SCENARIO_FCLOSE_ERROR,         /**< fclose失敗(fcloseはEOFを返す) */
} test_scenario_fclose_t;

/**
 * @brief filesystem_byte_read()テストシナリオリスト
 *
 */
typedef enum {
    TEST_SCENARIO_BYTE_READ_SUCCESS = 0,              /**< 指定バイト数の読み取りに成功 */
    TEST_SCENARIO_BYTE_READ_READ_ERROR,               /**< 読み込みでエラーが発生(freadが指定バイト数未満を読み取り && ferrorが0以外) */
    TEST_SCENARIO_BYTE_READ_EOF,                      /**< 読み取り位置がEOFで0byte読み取り(freadが0byte読み取り && ferrorが0 && feofが0以外) */
    TEST_SCENARIO_BYTE_READ_SUCCESS_EOF,              /**< 指定バイト数未満+EOFを読み取り(freadが0byte以上指定バイト数未満を読み取り && ferrorが0 && feofが0以外) */
    TEST_SCENARIO_BYTE_READ_UNDEFINED_ERROR,          /**< 上記以外(freadが指定バイト数未満を読み取り && ferrorが0 && feofが0) */
} test_scenario_byte_read_t;

/**
 * @brief mock_fopen()テストシナリオ制御構造体
 *
 */
typedef struct test_scenario_control_fopen {
    bool enable_test_scenario;          /**< テストシナリオを使用したテスト有効/無効フラグ */
    test_scenario_fopen_t scenario;     /**< テストシナリオを使用したテストを実施する際のシナリオ選択値 */
} test_scenario_control_fopen_t;

/**
 * @brief mock_fclose()テストシナリオ制御構造体
 *
 * @warning シナリオにTEST_SCENARIO_FCLOSE_ERRORを使用する際、テスト終了時に必ずファイルハンドルをクローズする処理を呼び出すこと
 *
 */
typedef struct test_scenario_control_fclose {
    bool enable_test_scenario;          /**< テストシナリオを使用したテスト有効/無効フラグ */
    test_scenario_fclose_t scenario;     /**< テストシナリオを使用したテストを実施する際のシナリオ選択値 */
} test_scenario_control_fclose_t;

/**
 * @brief filesystem_byte_read()テストシナリオ制御構造体
 *
 */
typedef struct test_scenario_control_byte_read {
    bool enable_test_scenario;              /**< テストシナリオを使用したテスト有効/無効フラグ */
    test_scenario_byte_read_t scenario;     /**< テストシナリオを使用したテストを実施する際のシナリオ選択値 */
} test_scenario_control_byte_read_t;

// 外部公開APIテスト設定
static test_call_control_t s_test_config_filesystem_create;      /**< filesystem_create()テスト設定 */
static test_call_control_t s_test_config_filesystem_open;        /**< filesystem_open()テスト設定 */
static test_call_control_t s_test_config_filesystem_close;       /**< filesystem_close()テスト設定 */
static test_call_control_t s_test_config_filesystem_byte_read;   /**< filesystem_byte_read()テスト設定 */

// プライベート関数テスト設定
static test_scenario_control_fopen_t s_test_scenario_control_fopen;             /**< mock_fopen()用テストシナリオ制御構造体インスタンス */
static test_scenario_control_fclose_t s_test_scenario_control_fclose;           /**< mock_fclose()用テストシナリオ制御構造体インスタンス */
static test_scenario_control_byte_read_t s_test_scenario_control_byte_read;     /**< filesystem_byte_read()用テストシナリオ制御構造体インスタンス */

// 全テスト関数プロトタイプ宣言
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

static const char* const s_rslt_str_success = "SUCCESS";                        /**< 実行結果コード文字列: 成功 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";      /**< 実行結果コード文字列: 無効な引数 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";            /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                    /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";        /**< 実行結果コード文字列: ファイルオープン失敗 */
static const char* const s_rslt_str_file_close_error = "FILE_CLOSE_ERROR";      /**< 実行結果コード文字列: ファイルクローズ失敗 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";          /**< 実行結果コード文字列: システムリソースが使用可能範囲を超過 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";            /**< 実行結果コード文字列: API誤用 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";        /**< 実行結果コード文字列: 未定義エラー */
static const char* const s_rslt_str_eof = "EOF";                                /**< 実行結果コード文字列: ファイル読み込みEOF */

filesystem_result_t filesystem_create(filesystem_t** filesystem_) {
#ifdef TEST_BUILD
    s_test_config_filesystem_create.call_count++;
    if(s_test_config_filesystem_create.fail_on_call != 0) {
        if(s_test_config_filesystem_create.call_count == s_test_config_filesystem_create.fail_on_call) {
            return (filesystem_result_t)s_test_config_filesystem_create.forced_result;
        }
    }
#endif

    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;
    filesystem_t* tmp = NULL;
    memory_system_result_t mem_result = MEMORY_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "filesystem_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_create", "*filesystem_")

    mem_result = memory_system_allocate(sizeof(filesystem_t), MEMORY_TAG_FILE_IO, (void**)&tmp);
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
    s_test_config_filesystem_open.call_count++;
    if(s_test_config_filesystem_open.fail_on_call != 0) {
        if(s_test_config_filesystem_open.call_count == s_test_config_filesystem_open.fail_on_call) {
            return (filesystem_result_t)s_test_config_filesystem_open.forced_result;
        }
    }
#endif

    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;
    const char* open_mode_str = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(filesystem_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_open", "filesystem_")
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, FILESYSTEM_INVALID_ARGUMENT, rslt_to_str(FILESYSTEM_INVALID_ARGUMENT), "filesystem_open", "fullpath_")

    if(NULL != filesystem_->file_handle) {
        ret = FILESYSTEM_RUNTIME_ERROR;
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

    ret = FILESYSTEM_SUCCESS;

cleanup:
    return ret;
}

filesystem_result_t filesystem_close(filesystem_t* filesystem_) {
#ifdef TEST_BUILD
    s_test_config_filesystem_close.call_count++;
    if(s_test_config_filesystem_close.fail_on_call != 0) {
        if(s_test_config_filesystem_close.call_count == s_test_config_filesystem_close.fail_on_call) {
            return (filesystem_result_t)s_test_config_filesystem_close.forced_result;
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
    s_test_config_filesystem_byte_read.call_count++;
    if(s_test_config_filesystem_byte_read.fail_on_call != 0) {
        if(s_test_config_filesystem_byte_read.call_count == s_test_config_filesystem_byte_read.fail_on_call) {
            return (filesystem_result_t)s_test_config_filesystem_byte_read.forced_result;
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

/**
 * @brief fopenのモック関数で、fopenの実行結果をテスト設定に合わせて制御する
 *
 * @note TEST_BUILD時に、mock_fopenのテストシナリオが有効だった場合には以下の値を返す
 * - シナリオ = TEST_SCENARIO_FOPEN_SUCCESS: 通常のfopen()の返り値
 * - シナリオ = TEST_SCENARIO_FOPEN_OPEN_ERROR: NULL
 *
 * @param[in] fullpath_ オープン対象ファイルのフルパス
 * @param[in] mode_ ファイルオープンモード
 *
 * @return FILE* オープンされたファイルハンドル
 */
static FILE* NO_COVERAGE mock_fopen(const char* fullpath_, const char* mode_) {
#ifdef TEST_BUILD
    if(s_test_scenario_control_fopen.enable_test_scenario) {
        switch(s_test_scenario_control_fopen.scenario) {
        case TEST_SCENARIO_FOPEN_SUCCESS:
            return fopen(fullpath_, mode_);
        case TEST_SCENARIO_FOPEN_OPEN_ERROR:
            return NULL;
        default:
            assert(false);
        }
    }
#endif
    return fopen(fullpath_, mode_);
}

/**
 * @brief fcloseのモック関数で、fcloseの実行結果をテスト設定に合わせて制御する
 *
 * @note TEST_BUILD時に、mock_fcloseのテストシナリオが有効だった場合には以下の値を返す
 * - シナリオ = TEST_SCENARIO_FCLOSE_SUCCESS: 通常のfclose()の返り値
 * - シナリオ = TEST_SCENARIO_FCLOSE_ERROR: EOF
 *
 * @param stream_ クローズ対象ファイルハンドル
 * @return int クローズ結果
 */
static int NO_COVERAGE mock_fclose(FILE* stream_) {
#ifdef TEST_BUILD
    if(s_test_scenario_control_fclose.enable_test_scenario) {
        switch(s_test_scenario_control_fclose.scenario) {
        case TEST_SCENARIO_FCLOSE_SUCCESS:
            return fclose(stream_);
        case TEST_SCENARIO_FCLOSE_ERROR:
            return EOF;
        default:
            assert(false);
        }
    }
#endif
    return fclose(stream_);
}

/**
 * @brief freadのモック関数で、freadの実行結果をテスト設定に合わせて制御する
 *
 * @note TEST_BUILD時に、filesystem_byte_readのテストシナリオが有効だった場合には以下の値を返す
 * - シナリオ = TEST_SCENARIO_BYTE_READ_EOF: 0
 * - シナリオ = TEST_SCENARIO_BYTE_READ_READ_ERROR: nmemb_ - 1
 * - シナリオ = TEST_SCENARIO_BYTE_READ_SUCCESS: 通常のfread()の返り値
 * - シナリオ = TEST_SCENARIO_BYTE_READ_SUCCESS_EOF: nmemb_ - 1
 * - シナリオ = TEST_SCENARIO_BYTE_READ_UNDEFINED_ERROR: nmemb_ - 1
 *
 * @param ptr_ 読み込んだ文字列の格納先バッファ
 * @param size_ 読み込むデータ１つのバイト数
 * @param nmemb_ 読み込むデータの個数
 * @param stream_ 読み込み対象ファイルハンドル
 * @return size_t 読み込んだデータの個数
 */
static size_t NO_COVERAGE mock_fread(void *ptr_, size_t size_, size_t nmemb_, FILE *stream_) {
#ifdef TEST_BUILD
    if(s_test_scenario_control_byte_read.enable_test_scenario) {
        switch(s_test_scenario_control_byte_read.scenario) {
        case TEST_SCENARIO_BYTE_READ_EOF:
            return 0;
        case TEST_SCENARIO_BYTE_READ_READ_ERROR:
            return nmemb_ - 1;
        case TEST_SCENARIO_BYTE_READ_SUCCESS:
            return fread(ptr_, size_, nmemb_, stream_);
        case TEST_SCENARIO_BYTE_READ_SUCCESS_EOF:
            return nmemb_ - 1;
        case TEST_SCENARIO_BYTE_READ_UNDEFINED_ERROR:
            return nmemb_ - 1;
        default:
            assert(false);
        }
    }
#endif
    return fread(ptr_, size_, nmemb_, stream_);
}

/**
 * @brief ferrorのモック関数で、ferrorの実行結果をテスト設定に合わせて制御する
 *
 * @note TEST_BUILD時に、filesystem_byte_readのテストシナリオが有効だった場合には以下の値を返す
 * - シナリオ = TEST_SCENARIO_BYTE_READ_EOF: 0
 * - シナリオ = TEST_SCENARIO_BYTE_READ_READ_ERROR: 1
 * - シナリオ = TEST_SCENARIO_BYTE_READ_SUCCESS: 0
 * - シナリオ = TEST_SCENARIO_BYTE_READ_SUCCESS_EOF: 0
 * - シナリオ = TEST_SCENARIO_BYTE_READ_UNDEFINED_ERROR: 0
 *
 * @param stream_ ストリームのテスト対象となるファイルハンドル
 * @retval 0 エラーなし
 * @retval 0以外 エラーあり
 */
static int NO_COVERAGE mock_ferror(FILE *stream_) {
#ifdef TEST_BUILD
    if(s_test_scenario_control_byte_read.enable_test_scenario) {
        switch(s_test_scenario_control_byte_read.scenario) {
        case TEST_SCENARIO_BYTE_READ_EOF:
            return 0;
        case TEST_SCENARIO_BYTE_READ_READ_ERROR:
            return 1;
        case TEST_SCENARIO_BYTE_READ_SUCCESS:
            return 0;
        case TEST_SCENARIO_BYTE_READ_SUCCESS_EOF:
            return 0;
        case TEST_SCENARIO_BYTE_READ_UNDEFINED_ERROR:
            return 0;
        default:
            assert(false);
        }
    }
#endif
    return ferror(stream_);
}

/**
 * @brief feofのモック関数で、feofの実行結果をテスト設定に合わせて制御する
 *
 * @note TEST_BUILD時に、filesystem_byte_readのテストシナリオが有効だった場合には以下の値を返す
 * - シナリオ = TEST_SCENARIO_BYTE_READ_EOF: 1
 * - シナリオ = TEST_SCENARIO_BYTE_READ_READ_ERROR: 1
 * - シナリオ = TEST_SCENARIO_BYTE_READ_SUCCESS: 0
 * - シナリオ = TEST_SCENARIO_BYTE_READ_SUCCESS_EOF: 1
 * - シナリオ = TEST_SCENARIO_BYTE_READ_UNDEFINED_ERROR: 0
 *
 * @param stream_ EOF判定対象ファイルハンドル
 * @retval 0 EOF以外
 * @retval 0以外 EOF
 */
static int NO_COVERAGE mock_feof(FILE *stream_) {
#ifdef TEST_BUILD
    if(s_test_scenario_control_byte_read.enable_test_scenario) {
        switch(s_test_scenario_control_byte_read.scenario) {
        case TEST_SCENARIO_BYTE_READ_EOF:
            return 1;
        case TEST_SCENARIO_BYTE_READ_READ_ERROR:
            return 1;
        case TEST_SCENARIO_BYTE_READ_SUCCESS:
            return 0;
        case TEST_SCENARIO_BYTE_READ_SUCCESS_EOF:
            return 1;
        case TEST_SCENARIO_BYTE_READ_UNDEFINED_ERROR:
            return 0;
        default:
            assert(false);
        }
    }
#endif
    return feof(stream_);
}

#ifdef TEST_BUILD
void test_filesystem_create_config_set(const test_call_control_t* config_) {
    s_test_config_filesystem_create.fail_on_call = config_->fail_on_call;
    s_test_config_filesystem_create.forced_result = config_->forced_result;
}

void test_filesystem_open_config_set(const test_call_control_t* config_) {
    s_test_config_filesystem_open.fail_on_call = config_->fail_on_call;
    s_test_config_filesystem_open.forced_result = config_->forced_result;
}

void test_filesystem_close_config_set(const test_call_control_t* config_) {
    s_test_config_filesystem_close.fail_on_call = config_->fail_on_call;
    s_test_config_filesystem_close.forced_result = config_->forced_result;
}

void test_filesystem_byte_read_config_set(const test_call_control_t* config_) {
    s_test_config_filesystem_byte_read.fail_on_call = config_->fail_on_call;
    s_test_config_filesystem_byte_read.forced_result = config_->forced_result;
}

void test_filesystem_config_reset(void) {
    test_call_control_reset(&s_test_config_filesystem_create);
    test_call_control_reset(&s_test_config_filesystem_open);
    test_call_control_reset(&s_test_config_filesystem_close);
    test_call_control_reset(&s_test_config_filesystem_byte_read);

    s_test_scenario_control_fopen.enable_test_scenario = false;
    s_test_scenario_control_fopen.scenario = TEST_SCENARIO_FOPEN_SUCCESS;

    s_test_scenario_control_fclose.enable_test_scenario = false;
    s_test_scenario_control_fclose.scenario = TEST_SCENARIO_FCLOSE_SUCCESS;

    s_test_scenario_control_byte_read.enable_test_scenario = false;
    s_test_scenario_control_byte_read.scenario = TEST_SCENARIO_BYTE_READ_SUCCESS;
}

void test_filesystem(void) {
    test_filesystem_config_reset();

    test_filesystem_create();
    test_filesystem_destroy();
    test_filesystem_open();
    test_filesystem_close();
    test_filesystem_byte_read();
    test_filesystem_open_mode_c_str();
    test_rslt_to_str();
    test_open_mode_readable();

    test_filesystem_config_reset();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_filesystem_create(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    memory_system_create();

    test_filesystem_config_reset();
    test_choco_memory_config_reset();

    {
        // filesystem_create() 冒頭で強制的に FILESYSTEM_NO_MEMORY を返させる
        filesystem_t* tmp = NULL;
        test_call_control_t config = {0};

        test_filesystem_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_NO_MEMORY;
        test_filesystem_create_config_set(&config);

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_NO_MEMORY == ret);
        assert(NULL == tmp);

        test_filesystem_config_reset();
    }
    {
        // filesystem_ == NULL -> FILESYSTEM_INVALID_ARGUMENT
        test_filesystem_config_reset();

        ret = filesystem_create(NULL);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);

        test_filesystem_config_reset();
    }
    {
        // *filesystem_ != NULL -> FILESYSTEM_INVALID_ARGUMENT
        filesystem_t* tmp = NULL;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_allocate(sizeof(filesystem_t), MEMORY_TAG_FILE_IO, (void**)&tmp);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != tmp);

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        assert(NULL != tmp);

        memory_system_free(tmp, sizeof(filesystem_t), MEMORY_TAG_FILE_IO);
        tmp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() が MEMORY_SYSTEM_INVALID_ARGUMENT を返す
        filesystem_t* tmp = NULL;
        test_call_control_t config = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_INVALID_ARGUMENT;
        test_memory_system_allocate_config_set(&config);

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        assert(NULL == tmp);

        test_choco_memory_config_reset();
        test_filesystem_config_reset();
    }
    {
        // memory_system_allocate() が MEMORY_SYSTEM_NO_MEMORY を返す
        filesystem_t* tmp = NULL;
        test_call_control_t config = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_NO_MEMORY == ret);
        assert(NULL == tmp);

        test_choco_memory_config_reset();
        test_filesystem_config_reset();
    }
    {
        // memory_system_allocate() が MEMORY_SYSTEM_LIMIT_EXCEEDED を返す
        filesystem_t* tmp = NULL;
        test_call_control_t config = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_LIMIT_EXCEEDED;
        test_memory_system_allocate_config_set(&config);

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_LIMIT_EXCEEDED == ret);
        assert(NULL == tmp);

        test_choco_memory_config_reset();
        test_filesystem_config_reset();
    }
    {
        // memory_system_allocate() が MEMORY_SYSTEM_BAD_OPERATION を返す
        filesystem_t* tmp = NULL;
        test_call_control_t config = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_BAD_OPERATION;
        test_memory_system_allocate_config_set(&config);

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_BAD_OPERATION == ret);
        assert(NULL == tmp);

        test_choco_memory_config_reset();
        test_filesystem_config_reset();
    }
    {
        // memory_system_allocate() が未定義エラーを返す -> FILESYSTEM_UNDEFINED_ERROR
        filesystem_t* tmp = NULL;
        test_call_control_t config = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_RUNTIME_ERROR;
        test_memory_system_allocate_config_set(&config);

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_UNDEFINED_ERROR == ret);
        assert(NULL == tmp);

        test_choco_memory_config_reset();
        test_filesystem_config_reset();
    }
    {
        // 正常系
        filesystem_t* tmp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_filesystem_destroy(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    memory_system_create();

    test_filesystem_config_reset();
    test_choco_memory_config_reset();

    {
        // filesystem_ == NULL -> no-op
        test_filesystem_config_reset();

        filesystem_destroy(NULL);

        test_filesystem_config_reset();
    }
    {
        // *filesystem_ == NULL -> no-op
        filesystem_t* tmp = NULL;

        test_filesystem_config_reset();

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
    }
    {
        // file_handle == NULL の正常破棄
        filesystem_t* tmp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        // 2重 destroy 許可
        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // file_handle != NULL かつ close 成功
        filesystem_t* tmp = NULL;
        FILE* fp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        fp = tmpfile(); // w+bの一時ファイルを生成、標準Cの機能
        assert(NULL != fp);

        tmp->file_handle = fp;
        tmp->mode = FILESYSTEM_MODE_READ;

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // file_handle != NULL かつ close 失敗 -> warning を出しつつ破棄
        filesystem_t* tmp = NULL;
        FILE* fp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        fp = tmpfile();
        assert(NULL != fp);

        tmp->file_handle = fp;
        tmp->mode = FILESYSTEM_MODE_READ;

        s_test_scenario_control_fclose.enable_test_scenario = true;
        s_test_scenario_control_fclose.scenario = TEST_SCENARIO_FCLOSE_ERROR;

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        // TEST_SCENARIO_FCLOSE_ERROR では実際の fclose は呼ばれていないため後始末する
        fclose(fp);
        fp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_filesystem_open(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    memory_system_create();

    test_filesystem_config_reset();
    test_choco_memory_config_reset();

    {
        // filesystem_open() 冒頭で強制的に FILESYSTEM_FILE_OPEN_ERROR を返させる
        test_call_control_t config = {0};

        test_filesystem_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_FILE_OPEN_ERROR;
        test_filesystem_open_config_set(&config);

        ret = filesystem_open(NULL, NULL, FILESYSTEM_MODE_APPEND);
        assert(FILESYSTEM_FILE_OPEN_ERROR == ret);

        test_filesystem_config_reset();
    }
    {
        // filesystem_ == NULL -> FILESYSTEM_INVALID_ARGUMENT
        test_filesystem_config_reset();

        ret = filesystem_open(NULL, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);

        test_filesystem_config_reset();
    }
    {
        // fullpath_ == NULL -> FILESYSTEM_INVALID_ARGUMENT
        filesystem_t* tmp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        ret = filesystem_open(tmp, NULL, FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // file_handle != NULL -> FILESYSTEM_RUNTIME_ERROR
        filesystem_t* tmp = NULL;
        FILE* fp_before = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        fp_before = tmp->file_handle;

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);
        assert(fp_before == tmp->file_handle);
        assert(FILESYSTEM_MODE_READ == tmp->mode);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // mode_ == FILESYSTEM_MODE_NONE -> FILESYSTEM_INVALID_ARGUMENT
        filesystem_t* tmp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_NONE);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // mock_fopen() シナリオでオープン失敗 -> FILESYSTEM_FILE_OPEN_ERROR
        filesystem_t* tmp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        s_test_scenario_control_fopen.enable_test_scenario = true;
        s_test_scenario_control_fopen.scenario = TEST_SCENARIO_FOPEN_OPEN_ERROR;

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_FILE_OPEN_ERROR == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系
        filesystem_t* tmp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp->file_handle);
        assert(FILESYSTEM_MODE_READ == tmp->mode);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_filesystem_close(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    memory_system_create();

    test_filesystem_config_reset();
    test_choco_memory_config_reset();

    {
        // filesystem_close() 冒頭で強制的に FILESYSTEM_FILE_CLOSE_ERROR を返させる
        test_call_control_t config = {0};

        test_filesystem_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_FILE_CLOSE_ERROR;
        test_filesystem_close_config_set(&config);

        ret = filesystem_close(NULL);
        assert(FILESYSTEM_FILE_CLOSE_ERROR == ret);

        test_filesystem_config_reset();
    }
    {
        // filesystem_ == NULL -> FILESYSTEM_INVALID_ARGUMENT
        test_filesystem_config_reset();

        ret = filesystem_close(NULL);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);

        test_filesystem_config_reset();
    }
    {
        // file_handle == NULL -> FILESYSTEM_RUNTIME_ERROR
        filesystem_t* tmp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // mock_fclose() シナリオでクローズ失敗 -> FILESYSTEM_FILE_CLOSE_ERROR
        filesystem_t* tmp = NULL;
        FILE* fp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        fp = tmpfile();
        assert(NULL != fp);

        tmp->file_handle = fp;
        tmp->mode = FILESYSTEM_MODE_READ;

        s_test_scenario_control_fclose.enable_test_scenario = true;
        s_test_scenario_control_fclose.scenario = TEST_SCENARIO_FCLOSE_ERROR;

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_FILE_CLOSE_ERROR == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        // TEST_SCENARIO_FCLOSE_ERROR では実際の fclose は呼ばれていないため後始末する
        fclose(fp);
        fp = NULL;

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系
        filesystem_t* tmp = NULL;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp->file_handle);
        assert(FILESYSTEM_MODE_READ == tmp->mode);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_filesystem_byte_read(void) {
    filesystem_result_t ret = FILESYSTEM_INVALID_ARGUMENT;

    memory_system_create();

    test_filesystem_config_reset();
    test_choco_memory_config_reset();

    {
        // filesystem_byte_read() 冒頭で強制的に FILESYSTEM_RUNTIME_ERROR を返させる
        test_call_control_t config = {0};
        size_t result = 123U;
        char buffer[16] = {0};

        test_filesystem_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_RUNTIME_ERROR;
        test_filesystem_byte_read_config_set(&config);

        ret = filesystem_byte_read(NULL, 16U, &result, buffer);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);
        assert(123U == result);

        test_filesystem_config_reset();
    }
    {
        // filesystem_ == NULL -> FILESYSTEM_INVALID_ARGUMENT
        size_t result = 0U;
        char buffer[128] = {0};

        test_filesystem_config_reset();

        ret = filesystem_byte_read(NULL, 64U, &result, buffer);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        assert(0U == result);

        test_filesystem_config_reset();
    }
    {
        // result_n_ == NULL -> FILESYSTEM_INVALID_ARGUMENT
        filesystem_t* tmp = NULL;
        char buffer[128] = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_byte_read(tmp, 64U, NULL, buffer);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // buffer_ == NULL -> FILESYSTEM_INVALID_ARGUMENT
        filesystem_t* tmp = NULL;
        size_t result = 999U;

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_byte_read(tmp, 64U, &result, NULL);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        assert(0U == result);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // file_handle == NULL -> FILESYSTEM_RUNTIME_ERROR
        filesystem_t* tmp = NULL;
        size_t result = 777U;
        char buffer[128] = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);
        assert(NULL == tmp->file_handle);
        assert(FILESYSTEM_MODE_NONE == tmp->mode);

        ret = filesystem_byte_read(tmp, 64U, &result, buffer);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);
        assert(0U == result);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // read_bytes_ == 0 -> FILESYSTEM_INVALID_ARGUMENT
        filesystem_t* tmp = NULL;
        size_t result = 888U;
        char buffer[128] = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file_w.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 0U, &result, buffer);
        assert(FILESYSTEM_INVALID_ARGUMENT == ret);
        assert(0U == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 読み取り不可モード -> FILESYSTEM_RUNTIME_ERROR
        filesystem_t* tmp = NULL;
        size_t result = 555U;
        char buffer[128] = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file_w.txt", FILESYSTEM_MODE_WRITE);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_WRITE == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 64U, &result, buffer);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);
        assert(0U == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // byte_read シナリオ: 読み込みエラー -> FILESYSTEM_RUNTIME_ERROR
        filesystem_t* tmp = NULL;
        size_t result = 0U;
        char buffer[128] = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        s_test_scenario_control_byte_read.enable_test_scenario = true;
        s_test_scenario_control_byte_read.scenario = TEST_SCENARIO_BYTE_READ_READ_ERROR;

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 128U, &result, buffer);
        assert(FILESYSTEM_RUNTIME_ERROR == ret);
        assert(0U == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // byte_read シナリオ: EOF で 0 byte 読み取り -> FILESYSTEM_EOF
        filesystem_t* tmp = NULL;
        size_t result = 999U;
        char buffer[128] = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        s_test_scenario_control_byte_read.enable_test_scenario = true;
        s_test_scenario_control_byte_read.scenario = TEST_SCENARIO_BYTE_READ_EOF;

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 128U, &result, buffer);
        assert(FILESYSTEM_EOF == ret);
        assert(0U == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // byte_read シナリオ: 指定バイト数未満 + EOF -> FILESYSTEM_SUCCESS
        filesystem_t* tmp = NULL;
        size_t result = 0U;
        char buffer[128] = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        s_test_scenario_control_byte_read.enable_test_scenario = true;
        s_test_scenario_control_byte_read.scenario = TEST_SCENARIO_BYTE_READ_SUCCESS_EOF;

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 128U, &result, buffer);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(127U == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // byte_read シナリオ: 未定義エラー -> FILESYSTEM_UNDEFINED_ERROR
        filesystem_t* tmp = NULL;
        size_t result = 123U;
        char buffer[128] = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        s_test_scenario_control_byte_read.enable_test_scenario = true;
        s_test_scenario_control_byte_read.scenario = TEST_SCENARIO_BYTE_READ_UNDEFINED_ERROR;

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 128U, &result, buffer);
        assert(FILESYSTEM_UNDEFINED_ERROR == ret);
        assert(0U == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系
        filesystem_t* tmp = NULL;
        size_t result = 0U;
        char buffer[128] = {0};

        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = filesystem_create(&tmp);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(NULL != tmp);

        ret = filesystem_open(tmp, "assets/test/filesystem/test_file.txt", FILESYSTEM_MODE_READ);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(FILESYSTEM_MODE_READ == tmp->mode);
        assert(NULL != tmp->file_handle);

        ret = filesystem_byte_read(tmp, 2U, &result, buffer);
        assert(FILESYSTEM_SUCCESS == ret);
        assert(2U == result);

        ret = filesystem_close(tmp);
        assert(FILESYSTEM_SUCCESS == ret);

        filesystem_destroy(&tmp);
        assert(NULL == tmp);

        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_filesystem_open_mode_c_str(void) {
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_NONE);
        assert(NULL == str);
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_READ);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_read));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_WRITE);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_write));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_APPEND);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_append));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_READ_PLUS);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_read_plus));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_WRITE_PLUS);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_write_plus));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_APPEND_PLUS);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_append_plus));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_READ_BINARY);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_read_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_WRITE_BINARY);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_write_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_APPEND_BINARY);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_append_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_READ_PLUS_BINARY);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_read_plus_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_WRITE_PLUS_BINARY);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_write_plus_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str(FILESYSTEM_MODE_APPEND_PLUS_BINARY);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_append_plus_binary));
    }
    {
        const char* str = filesystem_open_mode_c_str((filesystem_open_mode_t)100);
        assert(NULL != str);
        assert(0 == strcmp(str, s_open_mode_undefined));
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* str = rslt_to_str(FILESYSTEM_SUCCESS);
        assert(NULL != str);
        assert(0 == strcmp("SUCCESS", str));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_INVALID_ARGUMENT);
        assert(NULL != str);
        assert(0 == strcmp("INVALID_ARGUMENT", str));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_RUNTIME_ERROR);
        assert(NULL != str);
        assert(0 == strcmp("RUNTIME_ERROR", str));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_NO_MEMORY);
        assert(NULL != str);
        assert(0 == strcmp("NO_MEMORY", str));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_FILE_OPEN_ERROR);
        assert(NULL != str);
        assert(0 == strcmp("FILE_OPEN_ERROR", str));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_FILE_CLOSE_ERROR);
        assert(NULL != str);
        assert(0 == strcmp("FILE_CLOSE_ERROR", str));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_LIMIT_EXCEEDED);
        assert(NULL != str);
        assert(0 == strcmp("LIMIT_EXCEEDED", str));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_BAD_OPERATION);
        assert(NULL != str);
        assert(0 == strcmp("BAD_OPERATION", str));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_UNDEFINED_ERROR);
        assert(NULL != str);
        assert(0 == strcmp("UNDEFINED_ERROR", str));
    }
    {
        const char* str = rslt_to_str(FILESYSTEM_EOF);
        assert(NULL != str);
        assert(0 == strcmp("EOF", str));
    }
    {
        const char* str = rslt_to_str((filesystem_result_t)100);
        assert(NULL != str);
        assert(0 == strcmp("UNDEFINED_ERROR", str));
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_open_mode_readable(void) {
    assert(false == open_mode_readable(FILESYSTEM_MODE_NONE));
    assert(true == open_mode_readable(FILESYSTEM_MODE_READ));
    assert(false == open_mode_readable(FILESYSTEM_MODE_WRITE));
    assert(false == open_mode_readable(FILESYSTEM_MODE_APPEND));
    assert(true == open_mode_readable(FILESYSTEM_MODE_READ_PLUS));
    assert(true == open_mode_readable(FILESYSTEM_MODE_WRITE_PLUS));
    assert(true == open_mode_readable(FILESYSTEM_MODE_APPEND_PLUS));
    assert(true == open_mode_readable(FILESYSTEM_MODE_READ_BINARY));
    assert(false == open_mode_readable(FILESYSTEM_MODE_WRITE_BINARY));
    assert(false == open_mode_readable(FILESYSTEM_MODE_APPEND_BINARY));
    assert(true == open_mode_readable(FILESYSTEM_MODE_READ_PLUS_BINARY));
    assert(true == open_mode_readable(FILESYSTEM_MODE_WRITE_PLUS_BINARY));
    assert(true == open_mode_readable(FILESYSTEM_MODE_APPEND_PLUS_BINARY));
    assert(false == open_mode_readable(100));
}
#endif
