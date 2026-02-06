/** @ingroup fs_utils
 *
 * @file fs_utils.c
 * @author chocolate-pie24
 * @brief ファイル処理に関するユーティリティAPIの実装
 *
 * @version 0.1
 * @date 2025-12.26
 *
 * @todo ターゲットコントローラに応じてFS_READ_UNIT_SIZEを変える
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <string.h> // for memset
#include <stddef.h>
#include <stdbool.h>

#include "engine/io_utils/fs_utils/fs_utils.h"

#include "engine/containers/choco_string.h"

#include "engine/core/filesystem/filesystem.h"
#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#define FS_READ_UNIT_SIZE 512  /**< ファイル読み込みの際に一度に読み込むバイト数(メモリの動的確保回数を減らすため,固定値にした) */

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include <stdio.h>

/**
 * @brief 呼び出し側のテスト用強制エラー出力データ構造体
 *
 */
typedef struct fail_injection {
    fs_utils_result_t result_code;  /**< 強制的に出力する実行結果コード */
    bool is_enabled;                /**< 実行結果コード強制出力機能有効/無効フラグ */
} fail_injection_t;

static fail_injection_t s_fail_injection;   /**< 上位モジュールテスト用構造体インスタンス */

static void test_fs_utils_create(void);
static void test_fs_utils_destroy(void);
static void test_fs_utils_text_file_read(void);
static void test_fs_utils_fullpath_get(void);
static void test_rslt_to_str(void);
static void test_fs_utils_valid_check(void);
static void test_filesystem_result_convert(void);
static void test_choco_string_result_convert(void);
static void test_memory_system_result_convert(void);
#endif

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

static const char* rslt_to_str(fs_utils_result_t rslt_);
static bool fs_utils_valid_check(fs_utils_t* fs_utils_);
static fs_utils_result_t filesystem_result_convert(filesystem_result_t result_);
static fs_utils_result_t choco_string_result_convert(choco_string_result_t result_);
static fs_utils_result_t memory_system_result_convert(memory_system_result_t result_);

fs_utils_result_t fs_utils_create(const char* filepath_, const char* filename_, const char* extension_, filesystem_open_mode_t open_mode_, fs_utils_t** fs_utils_) {
    fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;
    fs_utils_t* tmp_fs_utils = NULL;

    choco_string_t* tmp_fullpath = NULL;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(filepath_, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_create", "filepath_")
    IF_ARG_NULL_GOTO_CLEANUP(filename_, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_create", "filename_")
    IF_ARG_NULL_GOTO_CLEANUP(fs_utils_, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_create", "fs_utils_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*fs_utils_, FS_UTILS_INVALID_ARGUMENT, "fs_utils_create", "*fs_utils_")
    IF_ARG_FALSE_GOTO_CLEANUP(open_mode_ != FILESYSTEM_MODE_NONE, FS_UTILS_INVALID_ARGUMENT, "fs_utils_create", "open_mode_")
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

fs_utils_result_t fs_utils_text_file_read(fs_utils_t* fs_utils_, choco_string_t* out_string_) {
    fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

    IF_ARG_NULL_GOTO_CLEANUP(fs_utils_, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_text_file_read", "fs_utils_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_text_file_read", "out_string_")
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

#ifdef TEST_BUILD
    if(s_fail_injection.is_enabled) {
        return s_fail_injection.result_code;
    }
#endif

    IF_ARG_NULL_GOTO_CLEANUP(fs_utils_, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_fullpath_get", "fs_utils_")
    IF_ARG_NULL_GOTO_CLEANUP(out_fullpath_, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_fullpath_get", "out_fullpath_")
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

static const char* rslt_to_str(fs_utils_result_t rslt_) {
    switch(rslt_) {
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

#ifdef TEST_BUILD

void fs_utils_fail_enable(fs_utils_result_t result_code_) {
    s_fail_injection.is_enabled = true;
    s_fail_injection.result_code = result_code_;
}

void fs_utils_fail_disable(void) {
    s_fail_injection.is_enabled = false;
    s_fail_injection.result_code = FS_UTILS_SUCCESS;
}

void test_fs_utils(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    test_fs_utils_create();
    test_fs_utils_destroy();
    test_fs_utils_text_file_read();
    test_fs_utils_fullpath_get();
    test_rslt_to_str();
    test_fs_utils_valid_check();
    test_filesystem_result_convert();
    test_choco_string_result_convert();
    test_memory_system_result_convert();

    memory_system_destroy();
}

static void NO_COVERAGE test_fs_utils_create(void) {
    {
        // 強制エラー出力
        fs_utils_fail_enable(FS_UTILS_DATA_CORRUPTED);
        fs_utils_result_t ret = fs_utils_create(NULL, NULL, NULL, FILESYSTEM_MODE_READ, NULL);
        assert(FS_UTILS_DATA_CORRUPTED == ret);
        fs_utils_fail_disable();
    }
    {
        // 正常系: extensionあり（test_file.txt を READ でオープンできること）
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
        fs_utils_t* fs = NULL;

        ret = fs_utils_create("assets/test/filesystem/", "test_file", ".txt", FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        // Commit後の最低限の状態確認
        assert(FILESYSTEM_MODE_READ == fs->mode);
        assert(NULL != fs->filesystem);
        assert(NULL != fs->filepath);
        assert(NULL != fs->filename);
        assert(NULL != fs->extension);

        assert(0 == strcmp("assets/test/filesystem/", choco_string_c_str(fs->filepath)));
        assert(0 == strcmp("test_file", choco_string_c_str(fs->filename)));
        assert(0 == strcmp(".txt", choco_string_c_str(fs->extension)));

        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }
    {
        // 正常系: extensionなし（filename側に拡張子を含めてオープンできること / extensionポインタがNULL）
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
        fs_utils_t* fs = NULL;

        ret = fs_utils_create("assets/test/filesystem/", "test_file.txt", NULL, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        assert(FILESYSTEM_MODE_READ == fs->mode);
        assert(NULL != fs->filesystem);
        assert(NULL != fs->filepath);
        assert(NULL != fs->filename);
        assert(NULL == fs->extension);

        assert(0 == strcmp("assets/test/filesystem/", choco_string_c_str(fs->filepath)));
        assert(0 == strcmp("test_file.txt", choco_string_c_str(fs->filename)));

        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }
    {
        // 異常系: filepath_ == NULL -> FS_UTILS_INVALID_ARGUMENT
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        ret = fs_utils_create(NULL, "test_file", ".txt", FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);
        assert(NULL == fs);
    }
    {
        // 異常系: filename_ == NULL -> FS_UTILS_INVALID_ARGUMENT
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        ret = fs_utils_create("assets/test/filesystem/", NULL, ".txt", FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);
        assert(NULL == fs);
    }
    {
        // 異常系: fs_utils_ == NULL -> FS_UTILS_INVALID_ARGUMENT
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;

        ret = fs_utils_create("assets/test/filesystem/", "test_file", ".txt", FILESYSTEM_MODE_READ, NULL);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);
    }
    {
        // 異常系: *fs_utils_ != NULL -> FS_UTILS_INVALID_ARGUMENT（sentinelのまま不変であること）
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = (fs_utils_t*)0x1;

        ret = fs_utils_create("assets/test/filesystem/", "test_file", ".txt", FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);
        assert((fs_utils_t*)0x1 == fs);
    }
    {
        // 異常系: open_mode_ == FILESYSTEM_MODE_NONE -> FS_UTILS_INVALID_ARGUMENT（Preconditionsで弾く）
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        ret = fs_utils_create("assets/test/filesystem/", "test_file.txt", NULL, FILESYSTEM_MODE_NONE, &fs);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);
        assert(NULL == fs);
    }
    {
        // 異常系: tmp_fs_utils のメモリ確保失敗 -> FS_UTILS_NO_MEMORY
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        memory_system_test_param_set(0); // 1回目の確保（tmp_fs_utils）を失敗させる

        ret = fs_utils_create("assets/test/filesystem/", "test_file", ".txt", FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_NO_MEMORY == ret);
        assert(NULL == fs);

        memory_system_test_param_reset();
    }
    {
        // 異常系: filepath 文字列生成失敗 -> FS_UTILS_NO_MEMORY
        // allocation順: tmp_fs_utils(0) -> filepath構造体(1) ...
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        memory_system_test_param_set(1); // filepath用の確保を失敗させる

        ret = fs_utils_create("assets/test/filesystem/", "test_file", ".txt", FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_NO_MEMORY == ret);
        assert(NULL == fs);

        memory_system_test_param_reset();
    }
    {
        // 異常系: filename 文字列生成失敗 -> FS_UTILS_NO_MEMORY
        // allocation順: tmp_fs_utils(0) -> filepath(1,2) -> filename構造体(3) ...
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        memory_system_test_param_set(3); // filename構造体の確保を失敗させる

        ret = fs_utils_create("assets/test/filesystem/", "test_file", ".txt", FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_NO_MEMORY == ret);
        assert(NULL == fs);

        memory_system_test_param_reset();
    }
    {
        // 異常系: extension 文字列生成失敗 -> FS_UTILS_NO_MEMORY
        // allocation順: tmp_fs_utils(0) -> filepath(1,2) -> filename(3,4) -> extension構造体(5) ...
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        memory_system_test_param_set(5); // extension構造体の確保を失敗させる

        ret = fs_utils_create("assets/test/filesystem/", "test_file", ".txt", FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_NO_MEMORY == ret);
        assert(NULL == fs);

        memory_system_test_param_reset();
    }
    {
        // 異常系: filesystem_create のメモリ確保失敗 -> FS_UTILS_NO_MEMORY
        // （extensionなしルートで allocation順を固定: tmp_fs_utils(0) -> filepath(1,2) -> filename(3,4) -> filesystem(5)）
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        memory_system_test_param_set(5); // filesystem_create 内の確保を失敗させる想定

        ret = fs_utils_create("assets/test/filesystem/", "test_file.txt", NULL, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_NO_MEMORY == ret);
        assert(NULL == fs);

        memory_system_test_param_reset();
    }
    {
        // 異常系: tmp_fullpath 生成失敗 -> FS_UTILS_NO_MEMORY
        // allocation順（extensionなし）: tmp_fs_utils(0) -> filepath(1,2) -> filename(3,4) -> filesystem(5) -> tmp_fullpath(6)
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        memory_system_test_param_set(6); // tmp_fullpath 構造体の確保を失敗させる想定

        ret = fs_utils_create("assets/test/filesystem/", "test_file.txt", NULL, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_NO_MEMORY == ret);
        assert(NULL == fs);

        memory_system_test_param_reset();
    }
    {
        // 異常系: fs_utils_fullpath_get 内部の choco_string_copy でバッファ確保失敗 -> FS_UTILS_NO_MEMORY
        // allocation順（extensionなし）:
        // tmp_fs_utils(0) -> filepath(1,2) -> filename(3,4) -> filesystem(5) -> tmp_fullpath構造体(6) -> tmp_fullpathバッファ(7)
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        memory_system_test_param_set(7); // fullpath生成(copy)のバッファ確保を失敗させる想定

        ret = fs_utils_create("assets/test/filesystem/", "test_file.txt", NULL, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_NO_MEMORY == ret);
        assert(NULL == fs);

        memory_system_test_param_reset();
    }
    {
        // 異常系: filesystem_open 失敗（存在しないファイル） -> FS_UTILS_FILE_OPEN_ERROR
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_SUCCESS;
        fs_utils_t* fs = NULL;

        ret = fs_utils_create(
            "assets/test/filesystem/",
            "this_file_should_not_exist_0123456789",
            ".txt",
            FILESYSTEM_MODE_READ,
            &fs
        );
        assert(FS_UTILS_FILE_OPEN_ERROR == ret);
        assert(NULL == fs);
    }
}

static void NO_COVERAGE test_fs_utils_destroy(void) {
    {
        // 異常系: fs_utils_ == NULL -> 何もせずreturn（クラッシュしないこと）
        fs_utils_destroy(NULL);
    }
    {
        // 異常系: *fs_utils_ == NULL -> 何もせずreturn（クラッシュしないこと）
        fs_utils_t* fs = NULL;

        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }
    {
        // 正常系: extensionあり -> 破棄後にNULLになること（内部メンバ破棄 + free が通る）
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
        fs_utils_t* fs = NULL;

        ret = fs_utils_create("assets/test/filesystem/", "test_file", ".txt", FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        // Destroy前の最低限状態確認
        assert(NULL != fs->filesystem);
        assert(NULL != fs->filepath);
        assert(NULL != fs->filename);
        assert(NULL != fs->extension);

        fs_utils_destroy(&fs);
        assert(NULL == fs);

        // 追加確認: 2回呼んでも安全（*fs_utils_==NULLの早期returnを通る）
        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }
    {
        // 正常系: extensionなし -> 破棄後にNULLになること（extension==NULLでも安全に破棄できる）
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
        fs_utils_t* fs = NULL;

        ret = fs_utils_create("assets/test/filesystem/", "test_file.txt", NULL, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        // Destroy前の最低限状態確認
        assert(NULL != fs->filesystem);
        assert(NULL != fs->filepath);
        assert(NULL != fs->filename);
        assert(NULL == fs->extension);

        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }
}

static void NO_COVERAGE test_fs_utils_text_file_read(void) {
    const char* dir = "assets/test/filesystem/";
    const char* f_empty = "tmp_fs_utils_text_read_empty";
    const char* f_512   = "tmp_fs_utils_text_read_512";
    const char* f_600   = "tmp_fs_utils_text_read_600";
    const char* ext = ".txt";

    const char* path_empty = "assets/test/filesystem/tmp_fs_utils_text_read_empty.txt";
    const char* path_512   = "assets/test/filesystem/tmp_fs_utils_text_read_512.txt";
    const char* path_600   = "assets/test/filesystem/tmp_fs_utils_text_read_600.txt";

    // ---- テスト用ファイル生成（実ファイルで EOF / partial read を確実に踏む）----
    {
        // 空ファイル
        FILE* fp = fopen(path_empty, "wb");
        assert(NULL != fp);
        fclose(fp);

        // 512 bytes（1回 SUCCESS(result=512) → 次回 EOF を踏む）
        fp = fopen(path_512, "wb");
        assert(NULL != fp);
        for(size_t i = 0; i < 512; ++i) {
            const unsigned char c = (unsigned char)'A';
            assert(1 == fwrite(&c, 1, 1, fp));
        }
        fclose(fp);

        // 600 bytes（1回目 SUCCESS(result=512) → 2回目 SUCCESS(result=88<512) で complete=true を踏む）
        fp = fopen(path_600, "wb");
        assert(NULL != fp);
        for(size_t i = 0; i < 600; ++i) {
            const unsigned char c = (unsigned char)'B';
            assert(1 == fwrite(&c, 1, 1, fp));
        }
        fclose(fp);
    }

    // ---- fail injection（fs_utils 側の早期 return 分岐）----
    {
        fs_utils_fail_enable(FS_UTILS_OVERFLOW);
        fs_utils_result_t ret = fs_utils_text_file_read(NULL, NULL);
        assert(FS_UTILS_OVERFLOW == ret);
        fs_utils_fail_disable();
    }

    // ---- 引数チェック：fs_utils_ == NULL -> FS_UTILS_INVALID_ARGUMENT ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        fs_utils_result_t ret = fs_utils_text_file_read(NULL, out);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);

        choco_string_destroy(&out);
        assert(NULL == out);
    }

    // ---- 引数チェック：out_string_ == NULL -> FS_UTILS_INVALID_ARGUMENT ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t* fs = NULL;
        fs_utils_result_t ret = fs_utils_create(dir, f_600, ext, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        ret = fs_utils_text_file_read(fs, NULL);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);

        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }

    // ---- 破損/未初期化：fs_utils_valid_check NG -> FS_UTILS_DATA_CORRUPTED ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        // ※fs_utils_create を経由しない未初期化インスタンスを渡して valid_check false を踏む
        fs_utils_t dummy;
        memset(&dummy, 0, sizeof(dummy));

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        fs_utils_result_t ret = fs_utils_text_file_read(&dummy, out);
        assert(FS_UTILS_DATA_CORRUPTED == ret);

        choco_string_destroy(&out);
        assert(NULL == out);
    }

    // ---- API誤用：open mode 不正（READ/READ_PLUS 以外） -> FS_UTILS_BAD_OPERATION ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t* fs = NULL;
        fs_utils_result_t ret = fs_utils_create(dir, "tmp_fs_utils_text_read_write_open", ext, FILESYSTEM_MODE_WRITE, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        ret = fs_utils_text_file_read(fs, out);
        assert(FS_UTILS_BAD_OPERATION == ret);

        choco_string_destroy(&out);
        fs_utils_destroy(&fs);
        assert(NULL == fs);

        remove("assets/test/filesystem/tmp_fs_utils_text_read_write_open.txt");
    }

    // ---- filesystem 注入：filesystem_byte_read -> FILESYSTEM_INVALID_ARGUMENT を強制 -> FS_UTILS_INVALID_ARGUMENT ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t* fs = NULL;
        fs_utils_result_t ret = fs_utils_create(dir, f_600, ext, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        filesystem_err_code_set(FILESYSTEM_INVALID_ARGUMENT);
        ret = fs_utils_text_file_read(fs, out);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);
        filesystem_test_param_reset(); // 注入解除

        choco_string_destroy(&out);
        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }

    // ---- filesystem 注入：filesystem_byte_read -> FILESYSTEM_RUNTIME_ERROR を強制 -> FS_UTILS_RUNTIME_ERROR ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t* fs = NULL;
        fs_utils_result_t ret = fs_utils_create(dir, f_600, ext, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        filesystem_err_code_set(FILESYSTEM_RUNTIME_ERROR);
        ret = fs_utils_text_file_read(fs, out);
        assert(FS_UTILS_RUNTIME_ERROR == ret);
        filesystem_test_param_reset(); // 注入解除

        choco_string_destroy(&out);
        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }

    // ---- 正常系：空ファイル（filesystem_byte_read が EOF を返す）----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t* fs = NULL;
        fs_utils_result_t ret = fs_utils_create(dir, f_empty, ext, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        ret = fs_utils_text_file_read(fs, out);
        assert(FS_UTILS_SUCCESS == ret);
        assert(0 == strcmp("", choco_string_c_str(out)));

        choco_string_destroy(&out);
        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }

    // ---- 正常系：600 bytes（複数ループ + SUCCESS(result<512) 終了分岐）----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t* fs = NULL;
        fs_utils_result_t ret = fs_utils_create(dir, f_600, ext, FILESYSTEM_MODE_READ_PLUS, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        ret = fs_utils_text_file_read(fs, out);
        assert(FS_UTILS_SUCCESS == ret);

        const char* s = choco_string_c_str(out);
        assert(NULL != s);
        assert(600 == strlen(s));
        assert('B' == s[0]);
        assert('B' == s[599]);

        choco_string_destroy(&out);
        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }

    // ---- 正常系：512 bytes（SUCCESS(result=512) → 次回 EOF）----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t* fs = NULL;
        fs_utils_result_t ret = fs_utils_create(dir, f_512, ext, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        ret = fs_utils_text_file_read(fs, out);
        assert(FS_UTILS_SUCCESS == ret);

        const char* s = choco_string_c_str(out);
        assert(NULL != s);
        assert(512 == strlen(s));
        assert('A' == s[0]);
        assert('A' == s[511]);

        choco_string_destroy(&out);
        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }

    // ---- concat 失敗分岐：choco_string_concat_from_c_string が NO_MEMORY -> FS_UTILS_NO_MEMORY ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t* fs = NULL;
        fs_utils_result_t ret = fs_utils_create(dir, f_600, ext, FILESYSTEM_MODE_READ, &fs);
        assert(FS_UTILS_SUCCESS == ret);
        assert(NULL != fs);

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        // read開始直前からの最初の確保を落とす（concat 内の確保を狙う）
        memory_system_test_param_reset();
        memory_system_test_param_set(0);

        ret = fs_utils_text_file_read(fs, out);
        assert(FS_UTILS_NO_MEMORY == ret);

        memory_system_test_param_reset();

        choco_string_destroy(&out);
        fs_utils_destroy(&fs);
        assert(NULL == fs);
    }

    // ---- 後始末：生成ファイル削除 ----
    {
        remove(path_empty);
        remove(path_512);
        remove(path_600);
    }
}

static void NO_COVERAGE test_fs_utils_fullpath_get(void) {
    // ---- fail injection（fs_utils 側の早期 return 分岐）----
    {
        fs_utils_fail_enable(FS_UTILS_DATA_CORRUPTED);
        fs_utils_result_t ret = fs_utils_fullpath_get(NULL, NULL);
        assert(FS_UTILS_DATA_CORRUPTED == ret);
        fs_utils_fail_disable();
    }

    // ---- 引数チェック：fs_utils_ == NULL -> FS_UTILS_INVALID_ARGUMENT ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        fs_utils_result_t ret = fs_utils_fullpath_get(NULL, out);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);

        choco_string_destroy(&out);
        assert(NULL == out);
    }

    // ---- 引数チェック：out_fullpath_ == NULL -> FS_UTILS_INVALID_ARGUMENT ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        // fs_utils_valid_check を通すための最小限の“擬似 fs_utils”
        fs_utils_t fs;
        memset(&fs, 0, sizeof(fs));
        fs.filesystem = (filesystem_t*)0x1;
        fs.mode = FILESYSTEM_MODE_READ;

        // filepath/filename は valid_check で必須
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filepath, "assets/test/filesystem/"));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filename, "test_file"));
        fs.extension = NULL;

        fs_utils_result_t ret = fs_utils_fullpath_get(&fs, NULL);
        assert(FS_UTILS_INVALID_ARGUMENT == ret);

        choco_string_destroy(&fs.filename);
        choco_string_destroy(&fs.filepath);
    }

    // ---- 破損/未初期化：fs_utils_valid_check NG -> FS_UTILS_DATA_CORRUPTED ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t dummy;
        memset(&dummy, 0, sizeof(dummy)); // filesystem/filename/filepath/mode が全部NGになる

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        fs_utils_result_t ret = fs_utils_fullpath_get(&dummy, out);
        assert(FS_UTILS_DATA_CORRUPTED == ret);

        choco_string_destroy(&out);
        assert(NULL == out);
    }

    // ---- 正常系：extension == NULL（filepath + filename）----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t fs;
        memset(&fs, 0, sizeof(fs));
        fs.filesystem = (filesystem_t*)0x1;
        fs.mode = FILESYSTEM_MODE_READ;
        fs.extension = NULL;

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filepath, "assets/test/filesystem/"));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filename, "test_file"));

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        fs_utils_result_t ret = fs_utils_fullpath_get(&fs, out);
        assert(FS_UTILS_SUCCESS == ret);
        assert(0 == strcmp("assets/test/filesystem/test_file", choco_string_c_str(out)));

        choco_string_destroy(&out);
        choco_string_destroy(&fs.filename);
        choco_string_destroy(&fs.filepath);
    }

    // ---- 正常系：extension != NULL（filepath + filename + extension）----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t fs;
        memset(&fs, 0, sizeof(fs));
        fs.filesystem = (filesystem_t*)0x1;
        fs.mode = FILESYSTEM_MODE_READ;

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filepath, "assets/test/filesystem/"));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filename, "test_file"));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.extension, ".txt"));

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        fs_utils_result_t ret = fs_utils_fullpath_get(&fs, out);
        assert(FS_UTILS_SUCCESS == ret);
        assert(0 == strcmp("assets/test/filesystem/test_file.txt", choco_string_c_str(out)));

        choco_string_destroy(&out);
        choco_string_destroy(&fs.extension);
        choco_string_destroy(&fs.filename);
        choco_string_destroy(&fs.filepath);
    }

    // ---- 異常系：choco_string_copy が NO_MEMORY -> FS_UTILS_NO_MEMORY ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        fs_utils_t fs;
        memset(&fs, 0, sizeof(fs));
        fs.filesystem = (filesystem_t*)0x1;
        fs.mode = FILESYSTEM_MODE_READ;
        fs.extension = NULL;

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filepath, "assets/test/filesystem/"));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filename, "test_file"));

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        // fullpath_get 内で最初に発生する確保（copy 側）を落とす
        memory_system_test_param_reset();
        memory_system_test_param_set(0);

        fs_utils_result_t ret = fs_utils_fullpath_get(&fs, out);
        assert(FS_UTILS_NO_MEMORY == ret);

        memory_system_test_param_reset();

        choco_string_destroy(&out);
        choco_string_destroy(&fs.filename);
        choco_string_destroy(&fs.filepath);
    }

    // ---- 異常系：filename concat が NO_MEMORY -> FS_UTILS_NO_MEMORY ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        // concat が確実に再確保を要するように、長めの filepath / filename を作る
        char filepath_buf[2048 + 2] = { 0 };
        char filename_buf[2048 + 1] = { 0 };
        memset(filepath_buf, 'a', 2048);
        filepath_buf[2048] = '/';
        filepath_buf[2049] = '\0';
        memset(filename_buf, 'b', 2048);
        filename_buf[2048] = '\0';

        fs_utils_t fs;
        memset(&fs, 0, sizeof(fs));
        fs.filesystem = (filesystem_t*)0x1;
        fs.mode = FILESYSTEM_MODE_READ;
        fs.extension = NULL;

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filepath, filepath_buf));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filename, filename_buf));

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        // 想定：copy で 1回確保(0) → filename concat の再確保(1) を落とす
        memory_system_test_param_reset();
        memory_system_test_param_set(1);

        fs_utils_result_t ret = fs_utils_fullpath_get(&fs, out);
        assert(FS_UTILS_NO_MEMORY == ret);

        memory_system_test_param_reset();

        choco_string_destroy(&out);
        choco_string_destroy(&fs.filename);
        choco_string_destroy(&fs.filepath);
    }

    // ---- 異常系：extension concat が NO_MEMORY -> FS_UTILS_NO_MEMORY ----
    {
        filesystem_test_param_reset();
        memory_system_test_param_reset();

        char filepath_buf[2048 + 2] = { 0 };
        char filename_buf[2048 + 1] = { 0 };
        char extension_buf[4096 + 2] = { 0 };

        memset(filepath_buf, 'a', 2048);
        filepath_buf[2048] = '/';
        filepath_buf[2049] = '\0';

        memset(filename_buf, 'b', 2048);
        filename_buf[2048] = '\0';

        extension_buf[0] = '.';
        memset(&extension_buf[1], 'c', 4096);
        extension_buf[4097] = '\0';

        fs_utils_t fs;
        memset(&fs, 0, sizeof(fs));
        fs.filesystem = (filesystem_t*)0x1;
        fs.mode = FILESYSTEM_MODE_READ;

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filepath, filepath_buf));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.filename, filename_buf));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(&fs.extension, extension_buf));

        choco_string_t* out = NULL;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out));

        // 想定：copy(0) → filename concat(1) → extension concat(2) を落とす
        // ※extension を巨大化して「2回目のconcat後もまだ再確保が必要」を強制
        memory_system_test_param_reset();
        memory_system_test_param_set(2);

        fs_utils_result_t ret = fs_utils_fullpath_get(&fs, out);
        assert(FS_UTILS_NO_MEMORY == ret);

        memory_system_test_param_reset();

        choco_string_destroy(&out);
        choco_string_destroy(&fs.extension);
        choco_string_destroy(&fs.filename);
        choco_string_destroy(&fs.filepath);
    }
}

static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* str = rslt_to_str(FS_UTILS_SUCCESS);
        assert(0 == strcmp(str, s_rslt_str_success));
    }
    {
        const char* str = rslt_to_str(FS_UTILS_INVALID_ARGUMENT);
        assert(0 == strcmp(str, s_rslt_str_invalid_argument));
    }
    {
        const char* str = rslt_to_str(FS_UTILS_BAD_OPERATION);
        assert(0 == strcmp(str, s_rslt_str_bad_operation));
    }
    {
        const char* str = rslt_to_str(FS_UTILS_DATA_CORRUPTED);
        assert(0 == strcmp(str, s_rslt_str_data_corrupted));
    }
    {
        const char* str = rslt_to_str(FS_UTILS_NO_MEMORY);
        assert(0 == strcmp(str, s_rslt_str_no_memory));
    }
    {
        const char* str = rslt_to_str(FS_UTILS_LIMIT_EXCEEDED);
        assert(0 == strcmp(str, s_rslt_str_limit_exceeded));
    }
    {
        const char* str = rslt_to_str(FS_UTILS_OVERFLOW);
        assert(0 == strcmp(str, s_rslt_str_overflow));
    }
    {
        const char* str = rslt_to_str(FS_UTILS_FILE_OPEN_ERROR);
        assert(0 == strcmp(str, s_rslt_str_file_open_error));
    }
    {
        const char* str = rslt_to_str(FS_UTILS_RUNTIME_ERROR);
        assert(0 == strcmp(str, s_rslt_str_runtime_error));
    }
    {
        const char* str = rslt_to_str(FS_UTILS_UNDEFINED_ERROR);
        assert(0 == strcmp(str, s_rslt_str_undefined_error));
    }
    {
        // default 分岐（未定義値）
        const char* str = rslt_to_str((fs_utils_result_t)100);
        assert(0 == strcmp(str, s_rslt_str_undefined_error));
    }
}

static void NO_COVERAGE test_fs_utils_valid_check(void) {
    {
        // 異常系: fs_utils_ == NULL
        assert(false == fs_utils_valid_check(NULL));
    }
    {
        // 異常系: filename == NULL
        fs_utils_t tmp = { 0 };
        tmp.filename = NULL;
        tmp.filepath = (choco_string_t*)0x1;
        tmp.filesystem = (filesystem_t*)0x1;
        tmp.mode = FILESYSTEM_MODE_READ;
        assert(false == fs_utils_valid_check(&tmp));
    }
    {
        // 異常系: filepath == NULL
        fs_utils_t tmp = { 0 };
        tmp.filename = (choco_string_t*)0x1;
        tmp.filepath = NULL;
        tmp.filesystem = (filesystem_t*)0x1;
        tmp.mode = FILESYSTEM_MODE_READ;
        assert(false == fs_utils_valid_check(&tmp));
    }
    {
        // 異常系: filesystem == NULL
        fs_utils_t tmp = { 0 };
        tmp.filename = (choco_string_t*)0x1;
        tmp.filepath = (choco_string_t*)0x1;
        tmp.filesystem = NULL;
        tmp.mode = FILESYSTEM_MODE_READ;
        assert(false == fs_utils_valid_check(&tmp));
    }
    {
        // 異常系: mode == FILESYSTEM_MODE_NONE
        fs_utils_t tmp = { 0 };
        tmp.filename = (choco_string_t*)0x1;
        tmp.filepath = (choco_string_t*)0x1;
        tmp.filesystem = (filesystem_t*)0x1;
        tmp.mode = FILESYSTEM_MODE_NONE;
        assert(false == fs_utils_valid_check(&tmp));
    }
    {
        // 正常系: extension は NULL 許容（この関数では参照しない）
        fs_utils_t tmp = { 0 };
        tmp.filename = (choco_string_t*)0x1;
        tmp.filepath = (choco_string_t*)0x1;
        tmp.filesystem = (filesystem_t*)0x1;
        tmp.extension = NULL;
        tmp.mode = FILESYSTEM_MODE_READ;
        assert(true == fs_utils_valid_check(&tmp));
    }
}

static void NO_COVERAGE test_filesystem_result_convert(void) {
    {
        // SUCCESS -> FS_UTILS_SUCCESS
        assert(FS_UTILS_SUCCESS == filesystem_result_convert(FILESYSTEM_SUCCESS));
    }
    {
        // INVALID_ARGUMENT -> FS_UTILS_INVALID_ARGUMENT
        assert(FS_UTILS_INVALID_ARGUMENT == filesystem_result_convert(FILESYSTEM_INVALID_ARGUMENT));
    }
    {
        // RUNTIME_ERROR -> FS_UTILS_RUNTIME_ERROR
        assert(FS_UTILS_RUNTIME_ERROR == filesystem_result_convert(FILESYSTEM_RUNTIME_ERROR));
    }
    {
        // NO_MEMORY -> FS_UTILS_NO_MEMORY
        assert(FS_UTILS_NO_MEMORY == filesystem_result_convert(FILESYSTEM_NO_MEMORY));
    }
    {
        // FILE_OPEN_ERROR -> FS_UTILS_FILE_OPEN_ERROR
        assert(FS_UTILS_FILE_OPEN_ERROR == filesystem_result_convert(FILESYSTEM_FILE_OPEN_ERROR));
    }
    {
        // UNDEFINED_ERROR -> FS_UTILS_UNDEFINED_ERROR
        assert(FS_UTILS_UNDEFINED_ERROR == filesystem_result_convert(FILESYSTEM_UNDEFINED_ERROR));
    }
    {
        // LIMIT_EXCEEDED -> FS_UTILS_LIMIT_EXCEEDED
        assert(FS_UTILS_LIMIT_EXCEEDED == filesystem_result_convert(FILESYSTEM_LIMIT_EXCEEDED));
    }
    {
        // EOF は convert 内では UNDEFINED 扱い（コメント仕様通り）
        assert(FS_UTILS_UNDEFINED_ERROR == filesystem_result_convert(FILESYSTEM_EOF));
    }
    {
        // FILE_CLOSE_ERROR も fs_utils では発生しない想定なので UNDEFINED 扱い（コメント仕様通り）
        assert(FS_UTILS_UNDEFINED_ERROR == filesystem_result_convert(FILESYSTEM_FILE_CLOSE_ERROR));
    }
    {
        // default: 未定義値 -> FS_UTILS_UNDEFINED_ERROR
        assert(FS_UTILS_UNDEFINED_ERROR == filesystem_result_convert((filesystem_result_t)999));
    }
}

static void NO_COVERAGE test_choco_string_result_convert(void) {
    {
        // SUCCESS -> FS_UTILS_SUCCESS
        assert(FS_UTILS_SUCCESS == choco_string_result_convert(CHOCO_STRING_SUCCESS));
    }
    {
        // DATA_CORRUPTED -> FS_UTILS_DATA_CORRUPTED
        assert(FS_UTILS_DATA_CORRUPTED == choco_string_result_convert(CHOCO_STRING_DATA_CORRUPTED));
    }
    {
        // BAD_OPERATION -> FS_UTILS_BAD_OPERATION
        assert(FS_UTILS_BAD_OPERATION == choco_string_result_convert(CHOCO_STRING_BAD_OPERATION));
    }
    {
        // NO_MEMORY -> FS_UTILS_NO_MEMORY
        assert(FS_UTILS_NO_MEMORY == choco_string_result_convert(CHOCO_STRING_NO_MEMORY));
    }
    {
        // INVALID_ARGUMENT -> FS_UTILS_INVALID_ARGUMENT
        assert(FS_UTILS_INVALID_ARGUMENT == choco_string_result_convert(CHOCO_STRING_INVALID_ARGUMENT));
    }
    {
        // RUNTIME_ERROR -> FS_UTILS_RUNTIME_ERROR
        assert(FS_UTILS_RUNTIME_ERROR == choco_string_result_convert(CHOCO_STRING_RUNTIME_ERROR));
    }
    {
        // UNDEFINED_ERROR -> FS_UTILS_UNDEFINED_ERROR
        assert(FS_UTILS_UNDEFINED_ERROR == choco_string_result_convert(CHOCO_STRING_UNDEFINED_ERROR));
    }
    {
        // OVERFLOW -> FS_UTILS_OVERFLOW
        assert(FS_UTILS_OVERFLOW == choco_string_result_convert(CHOCO_STRING_OVERFLOW));
    }
    {
        // LIMIT_EXCEEDED -> FS_UTILS_LIMIT_EXCEEDED
        assert(FS_UTILS_LIMIT_EXCEEDED == choco_string_result_convert(CHOCO_STRING_LIMIT_EXCEEDED));
    }
    {
        // default: 未定義値 -> FS_UTILS_UNDEFINED_ERROR
        assert(FS_UTILS_UNDEFINED_ERROR == choco_string_result_convert((choco_string_result_t)999));
    }
}

static void NO_COVERAGE test_memory_system_result_convert(void) {
    {
        // SUCCESS -> FS_UTILS_SUCCESS
        assert(FS_UTILS_SUCCESS == memory_system_result_convert(MEMORY_SYSTEM_SUCCESS));
    }
    {
        // INVALID_ARGUMENT -> FS_UTILS_INVALID_ARGUMENT
        assert(FS_UTILS_INVALID_ARGUMENT == memory_system_result_convert(MEMORY_SYSTEM_INVALID_ARGUMENT));
    }
    {
        // RUNTIME_ERROR -> FS_UTILS_RUNTIME_ERROR
        assert(FS_UTILS_RUNTIME_ERROR == memory_system_result_convert(MEMORY_SYSTEM_RUNTIME_ERROR));
    }
    {
        // LIMIT_EXCEEDED -> FS_UTILS_LIMIT_EXCEEDED
        assert(FS_UTILS_LIMIT_EXCEEDED == memory_system_result_convert(MEMORY_SYSTEM_LIMIT_EXCEEDED));
    }
    {
        // NO_MEMORY -> FS_UTILS_NO_MEMORY
        assert(FS_UTILS_NO_MEMORY == memory_system_result_convert(MEMORY_SYSTEM_NO_MEMORY));
    }
    {
        // default: 未定義値 -> FS_UTILS_UNDEFINED_ERROR
        assert(FS_UTILS_UNDEFINED_ERROR == memory_system_result_convert((memory_system_result_t)999));
    }
}

#endif
