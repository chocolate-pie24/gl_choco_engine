/** @ingroup io_utils
 *
 * @file fs_utils.c
 * @author chocolate-pie24
 * @brief ファイル処理に関するユーティリティAPIの実装
 *
 * @version 0.1
 * @date 2025-12-26
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
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <string.h>

#include "test_controller.h"

#include "engine/base/choco_macros.h"

#include "engine/core/memory/test_choco_memory.h"
#include "engine/core/filesystem/test_filesystem.h"

#include "engine/containers/test_choco_string.h"

#include "engine/io_utils/fs_utils/test_fs_utils.h"

// fs_utils用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_fs_utils_create;           /**< fs_utils_create()テスト設定 */
static test_call_control_t s_test_config_fs_utils_text_file_read;   /**< fs_utils_text_file_read()テスト設定 */
static test_call_control_t s_test_config_fs_utils_fullpath_get;     /**< fs_utils_fullpath_get()テスト設定 */

// プライベート関数テスト設定
static test_call_control_bool_t s_test_config_fs_utils_valid_check; /**< fs_utils_valid_check()テスト設定 */

// 全テスト関数プロトタイプ宣言
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

static const char* const s_rslt_str_success = "SUCCESS";                      /**< 実行結果コード文字列: 正常終了 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";    /**< 実行結果コード文字列: 無効な引数 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";          /**< 実行結果コード文字列: API誤用 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";        /**< 実行結果コード文字列: 内部データ破損or未初期化 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                  /**< 実行結果コード文字列: メモリ不足 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";        /**< 実行結果コード文字列: システム使用可能範囲超過 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                    /**< 実行結果コード文字列: 計算オーバーフロー */
static const char* const s_rslt_str_file_open_error = "FILE_OPEN_ERROR";      /**< 実行結果コード文字列: ファイルオープンエラー */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";          /**< 実行結果コード文字列: 実行時エラー */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";      /**< 実行結果コード文字列: 想定していないエラーが発生 */

static const char* rslt_to_str(fs_utils_result_t rslt_);
static bool fs_utils_valid_check(const fs_utils_t* fs_utils_);
static fs_utils_result_t filesystem_result_convert(filesystem_result_t result_);
static fs_utils_result_t choco_string_result_convert(choco_string_result_t result_);
static fs_utils_result_t memory_system_result_convert(memory_system_result_t result_);

fs_utils_result_t fs_utils_create(const char* filepath_, const char* filename_, const char* extension_, filesystem_open_mode_t open_mode_, fs_utils_t** fs_utils_) {
#ifdef TEST_BUILD
    s_test_config_fs_utils_create.call_count++;
    if(s_test_config_fs_utils_create.fail_on_call != 0) {
        if(s_test_config_fs_utils_create.call_count == s_test_config_fs_utils_create.fail_on_call) {
            return (fs_utils_result_t)s_test_config_fs_utils_create.forced_result;
        }
    }
#endif
    fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;
    fs_utils_t* tmp_fs_utils = NULL;
    choco_string_t* tmp_fullpath = NULL;
    const char* open_mode_str = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(filepath_, ret, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_create", "filepath_")
    IF_ARG_NULL_GOTO_CLEANUP(filename_, ret, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_create", "filename_")
    IF_ARG_NULL_GOTO_CLEANUP(fs_utils_, ret, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_create", "fs_utils_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*fs_utils_, ret, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_create", "*fs_utils_")
    // extensionはない場合があるのでNULLを許可

    open_mode_str = filesystem_open_mode_c_str(open_mode_);
    if(NULL == open_mode_str) {
        ret = FS_UTILS_INVALID_ARGUMENT;
        ERROR_MESSAGE("fs_utils_create(%s) - Provided file open mode is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    // Simulation.
    ret_mem = memory_system_allocate(sizeof(fs_utils_t), MEMORY_TAG_FILE_IO, (void**)&tmp_fs_utils);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = memory_system_result_convert(ret_mem);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to allocate memory for 'tmp_fs_utils'.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_fs_utils, 0, sizeof(fs_utils_t));
    tmp_fs_utils->mode = open_mode_;

    ret_str = choco_string_create_from_c_string(filepath_, &tmp_fs_utils->filepath);
    if(CHOCO_STRING_SUCCESS != ret_str) {
        ret = choco_string_result_convert(ret_str);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to initialize string from char for 'tmp_fs_utils->filepath'.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_str = choco_string_create_from_c_string(filename_, &tmp_fs_utils->filename);
    if(CHOCO_STRING_SUCCESS != ret_str) {
        ret = choco_string_result_convert(ret_str);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to initialize string from char for 'tmp_fs_utils->filename'.", rslt_to_str(ret));
        goto cleanup;
    }

    if(NULL == extension_) {
        tmp_fs_utils->extension = NULL;
    } else {
        ret_str = choco_string_create_from_c_string(extension_, &tmp_fs_utils->extension);
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

    ret_fs = filesystem_open(choco_string_c_str(tmp_fullpath), open_mode_, tmp_fs_utils->filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("fs_utils_create(%s) - Failed to open file. File open mode = '%s'.", rslt_to_str(ret), open_mode_str);
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
#ifdef TEST_BUILD
    s_test_config_fs_utils_text_file_read.call_count++;
    if(s_test_config_fs_utils_text_file_read.fail_on_call != 0) {
        if(s_test_config_fs_utils_text_file_read.call_count == s_test_config_fs_utils_text_file_read.fail_on_call) {
            return (fs_utils_result_t)s_test_config_fs_utils_text_file_read.forced_result;
        }
    }
#endif
    fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;
    bool complete = false;

    IF_ARG_NULL_GOTO_CLEANUP(fs_utils_, ret, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_text_file_read", "fs_utils_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_text_file_read", "out_string_")

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

    while(!complete) {
        char tmp_buffer[FS_READ_UNIT_SIZE + 1] = { 0 };
        size_t result = 0;
        ret_fs = filesystem_byte_read(FS_READ_UNIT_SIZE, fs_utils_->filesystem, &result, tmp_buffer);
        if(FILESYSTEM_INVALID_ARGUMENT == ret_fs) {
            ret = FS_UTILS_INVALID_ARGUMENT;
            ERROR_MESSAGE("fs_utils_text_file_read(%s) - Failed to read from file.", rslt_to_str(ret));
            goto cleanup;
        } else if(FILESYSTEM_RUNTIME_ERROR == ret_fs) {
            ret = FS_UTILS_RUNTIME_ERROR;
            ERROR_MESSAGE("fs_utils_text_file_read(%s) - Failed to read from file.", rslt_to_str(ret));
            goto cleanup;
        } else if(FILESYSTEM_UNDEFINED_ERROR == ret_fs) {
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
#ifdef TEST_BUILD
    s_test_config_fs_utils_fullpath_get.call_count++;
    if(s_test_config_fs_utils_fullpath_get.fail_on_call != 0) {
        if(s_test_config_fs_utils_fullpath_get.call_count == s_test_config_fs_utils_fullpath_get.fail_on_call) {
            return (fs_utils_result_t)s_test_config_fs_utils_fullpath_get.forced_result;
        }
    }
#endif
    fs_utils_result_t ret = FS_UTILS_INVALID_ARGUMENT;
    choco_string_result_t ret_str = CHOCO_STRING_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(fs_utils_, ret, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_fullpath_get", "fs_utils_")
    IF_ARG_NULL_GOTO_CLEANUP(out_fullpath_, ret, FS_UTILS_INVALID_ARGUMENT, rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "fs_utils_fullpath_get", "out_fullpath_")

    if(!fs_utils_valid_check(fs_utils_)) {
        // fs_utilsはfs_utils_createを経由して生成されたものであればvalidであることが保証されるため,ここを通るということはデータが壊れているかデータが未初期化
        ret = FS_UTILS_DATA_CORRUPTED;
        ERROR_MESSAGE("fs_utils_fullpath_get(%s) - The provided fs_utils_ is corrupted or uninitialized.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_str = choco_string_copy(fs_utils_->filepath, out_fullpath_);
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

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ 文字列に変換する実行結果コード
 *
 * @return const char* 変換された文字列の先頭アドレス
 */
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

/**
 * @brief fs_utils_が管理する内部データが破損していないかを判定する
 *
 * @param[in] fs_utils_ 判定対象fs_utils_t構造体インスタンスへのポインタ
 *
 * @retval true 破損なしで正常
 * @return false 破損あり
 */
static bool fs_utils_valid_check(const fs_utils_t* fs_utils_) {
#ifdef TEST_BUILD
    s_test_config_fs_utils_valid_check.call_count++;
    if(s_test_config_fs_utils_valid_check.fail_on_call != 0) {
        if(s_test_config_fs_utils_valid_check.call_count == s_test_config_fs_utils_valid_check.fail_on_call) {
            return s_test_config_fs_utils_valid_check.forced_result;
        }
    }
#endif
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

/**
 * @brief filesystemモジュールの実行結果コードをfs_utilsの実行結果コードに変換する
 *
 * @param[in] result_ 変換するfilesystemモジュール実行結果コード
 * @return fs_utils_result_t 変換されたfs_utils実行結果コード
 */
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
    case FILESYSTEM_BAD_OPERATION:
        return FS_UTILS_BAD_OPERATION;
    case FILESYSTEM_EOF:
        return FS_UTILS_UNDEFINED_ERROR;    // EOFを出力するAPIを使う場所では本関数ではなく、直接実行結果コードを見る必要があるため、本関数ではUNDEFINEDとする
    case FILESYSTEM_FILE_CLOSE_ERROR:
        return FS_UTILS_UNDEFINED_ERROR;    // fs_utilsではdestoryでファイルクローズを行うため、クローズエラーは発生しないはず
    default:
        return FS_UTILS_UNDEFINED_ERROR;
    }
}

/**
 * @brief choco_stringモジュールの実行結果コードをfs_utils実行結果コードに変換する
 *
 * @param[in] result_ 変換するchoco_stringモジュール実行結果コード
 * @return fs_utils_result_t 変換されたfs_utilsモジュール実行結果コード
 */
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

/**
 * @brief memory_system実行結果コードをfs_utils実行結果コードに変換する
 *
 * @param[in] result_ 変換するmemory_system実行結果コード
 * @return fs_utils_result_t 変換されたfs_utils実行結果コード
 */
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
    case MEMORY_SYSTEM_BAD_OPERATION:
        return FS_UTILS_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return FS_UTILS_NO_MEMORY;
    default:
        return FS_UTILS_UNDEFINED_ERROR;
    }
}

#ifdef TEST_BUILD

void NO_COVERAGE test_fs_utils_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_fs_utils_create.fail_on_call = config_->fail_on_call;
    s_test_config_fs_utils_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_fs_utils_text_file_read_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_fs_utils_text_file_read.fail_on_call = config_->fail_on_call;
    s_test_config_fs_utils_text_file_read.forced_result = config_->forced_result;
}

void NO_COVERAGE test_fs_utils_fullpath_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_fs_utils_fullpath_get.fail_on_call = config_->fail_on_call;
    s_test_config_fs_utils_fullpath_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_fs_utils_config_reset(void) {
    test_call_control_reset(&s_test_config_fs_utils_create);
    test_call_control_reset(&s_test_config_fs_utils_text_file_read);
    test_call_control_reset(&s_test_config_fs_utils_fullpath_get);

    test_call_control_bool_reset(&s_test_config_fs_utils_valid_check);
}

void NO_COVERAGE test_fs_utils(void) {
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

// Generated by ChatGPT
static void NO_COVERAGE test_fs_utils_create(void) {
    {
        // fs_utils_create() 自体の失敗注入
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1;
        config.forced_result = (int)FS_UTILS_RUNTIME_ERROR;
        test_fs_utils_create_config_set(&config);

        assert(FS_UTILS_RUNTIME_ERROR == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // filepath_ == NULL
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_create(NULL, "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // filename_ == NULL
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_create("path/to/", NULL, ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // fs_utils_ == NULL
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, NULL));
    }
    {
        // *fs_utils_ != NULL
        fs_utils_t* fs_utils = (fs_utils_t*)1;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert((fs_utils_t*)1 == fs_utils);
    }
    {
        // open_mode_ == FILESYSTEM_MODE_NONE
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_NONE, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // memory_system_allocate() 失敗
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        assert(FS_UTILS_NO_MEMORY == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // choco_string_create_from_c_string() 失敗(filepath)
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        assert(FS_UTILS_NO_MEMORY == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // choco_string_create_from_c_string() 失敗(filename)
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 2;
        config.forced_result = (int)CHOCO_STRING_OVERFLOW;
        test_choco_string_create_from_c_string_config_set(&config);

        assert(FS_UTILS_OVERFLOW == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // choco_string_create_from_c_string() 失敗(extension)
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 3;
        config.forced_result = (int)CHOCO_STRING_LIMIT_EXCEEDED;
        test_choco_string_create_from_c_string_config_set(&config);

        assert(FS_UTILS_LIMIT_EXCEEDED == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // filesystem_create() 失敗
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_NO_MEMORY;
        test_filesystem_create_config_set(&config);

        assert(FS_UTILS_NO_MEMORY == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // choco_string_default_create() 失敗
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_default_create_config_set(&config);

        assert(FS_UTILS_NO_MEMORY == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // fs_utils_fullpath_get() 失敗
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1;
        config.forced_result = (int)FS_UTILS_RUNTIME_ERROR;
        test_fs_utils_fullpath_get_config_set(&config);

        assert(FS_UTILS_RUNTIME_ERROR == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // filesystem_open() 失敗
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_FILE_OPEN_ERROR;
        test_filesystem_open_config_set(&config);

        assert(FS_UTILS_FILE_OPEN_ERROR == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL == fs_utils);
    }
    {
        // 正常系(extensionあり)
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        // 実ファイル依存を避けるため filesystem_open() だけ成功注入
        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_SUCCESS;
        test_filesystem_open_config_set(&config);

        assert(FS_UTILS_SUCCESS == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL != fs_utils);
        assert(NULL != fs_utils->filesystem);
        assert(NULL != fs_utils->filepath);
        assert(NULL != fs_utils->filename);
        assert(NULL != fs_utils->extension);
        assert(FILESYSTEM_MODE_READ == fs_utils->mode);
        assert(0 == strcmp(choco_string_c_str(fs_utils->filepath), "path/to/"));
        assert(0 == strcmp(choco_string_c_str(fs_utils->filename), "file"));
        assert(0 == strcmp(choco_string_c_str(fs_utils->extension), ".txt"));

        fs_utils_destroy(&fs_utils);
        assert(NULL == fs_utils);
    }
    {
        // 正常系(extensionなし)
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        // 実ファイル依存を避けるため filesystem_open() だけ成功注入
        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_SUCCESS;
        test_filesystem_open_config_set(&config);

        assert(FS_UTILS_SUCCESS == fs_utils_create("path/to/", "file", NULL, FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL != fs_utils);
        assert(NULL != fs_utils->filesystem);
        assert(NULL != fs_utils->filepath);
        assert(NULL != fs_utils->filename);
        assert(NULL == fs_utils->extension);
        assert(FILESYSTEM_MODE_READ == fs_utils->mode);
        assert(0 == strcmp(choco_string_c_str(fs_utils->filepath), "path/to/"));
        assert(0 == strcmp(choco_string_c_str(fs_utils->filename), "file"));

        fs_utils_destroy(&fs_utils);
        assert(NULL == fs_utils);
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_fs_utils_destroy(void) {
    {
        // fs_utils_ == NULL
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        fs_utils_destroy(NULL);
    }
    {
        // *fs_utils_ == NULL
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        fs_utils_destroy(&fs_utils);
        assert(NULL == fs_utils);
    }
    {
        // 正常系(extensionあり)
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        // 実ファイル依存を避けるため filesystem_open() だけ成功注入
        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_SUCCESS;
        test_filesystem_open_config_set(&config);

        assert(FS_UTILS_SUCCESS == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL != fs_utils);

        fs_utils_destroy(&fs_utils);

        assert(NULL == fs_utils);
    }
    {
        // 正常系(extensionなし)
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        // 実ファイル依存を避けるため filesystem_open() だけ成功注入
        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_SUCCESS;
        test_filesystem_open_config_set(&config);

        assert(FS_UTILS_SUCCESS == fs_utils_create("path/to/", "file", NULL, FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL != fs_utils);

        fs_utils_destroy(&fs_utils);

        assert(NULL == fs_utils);
    }
    {
        // 2重destroy許可
        test_call_control_t config = { 0 };
        fs_utils_t* fs_utils = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        // 実ファイル依存を避けるため filesystem_open() だけ成功注入
        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_SUCCESS;
        test_filesystem_open_config_set(&config);

        assert(FS_UTILS_SUCCESS == fs_utils_create("path/to/", "file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL != fs_utils);

        fs_utils_destroy(&fs_utils);
        assert(NULL == fs_utils);

        fs_utils_destroy(&fs_utils);
        assert(NULL == fs_utils);
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_fs_utils_text_file_read(void) {
    {
        // fs_utils_text_file_read() 自体の失敗注入
        test_call_control_t config = { 0 };
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_string = NULL;

        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_string));

        config.fail_on_call = 1;
        config.forced_result = (int)FS_UTILS_RUNTIME_ERROR;
        test_fs_utils_text_file_read_config_set(&config);

        assert(FS_UTILS_RUNTIME_ERROR == fs_utils_text_file_read(&fs_utils, out_string));

        choco_string_destroy(&out_string);
        assert(NULL == out_string);
    }
    {
        // fs_utils_ == NULL
        choco_string_t* out_string = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_string));

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_text_file_read(NULL, out_string));

        choco_string_destroy(&out_string);
        assert(NULL == out_string);
    }
    {
        // out_string_ == NULL
        fs_utils_t fs_utils = { 0 };

        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_text_file_read(&fs_utils, NULL));
    }
    {
        // fs_utils_valid_check() が false
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_string = NULL;

        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_string));

        s_test_config_fs_utils_valid_check.fail_on_call = 1;
        s_test_config_fs_utils_valid_check.forced_result = false;

        assert(FS_UTILS_DATA_CORRUPTED == fs_utils_text_file_read(&fs_utils, out_string));

        choco_string_destroy(&out_string);
        assert(NULL == out_string);
    }
    {
        // mode が読み込み系ではない
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_string = NULL;

        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_WRITE;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_string));

        assert(FS_UTILS_BAD_OPERATION == fs_utils_text_file_read(&fs_utils, out_string));

        choco_string_destroy(&out_string);
        assert(NULL == out_string);
    }
    {
        // filesystem_byte_read() が INVALID_ARGUMENT
        test_call_control_t config = { 0 };
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_string = NULL;

        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_string));

        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_INVALID_ARGUMENT;
        test_filesystem_byte_read_config_set(&config);

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_text_file_read(&fs_utils, out_string));

        choco_string_destroy(&out_string);
        assert(NULL == out_string);
    }
    {
        // filesystem_byte_read() が RUNTIME_ERROR
        test_call_control_t config = { 0 };
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_string = NULL;

        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_string));

        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_RUNTIME_ERROR;
        test_filesystem_byte_read_config_set(&config);

        assert(FS_UTILS_RUNTIME_ERROR == fs_utils_text_file_read(&fs_utils, out_string));

        choco_string_destroy(&out_string);
        assert(NULL == out_string);
    }
    {
        // choco_string_concat_from_c_string() が失敗
        test_call_control_t config = { 0 };
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_string = NULL;

        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_string));

        config.fail_on_call = 1;
        config.forced_result = (int)FILESYSTEM_SUCCESS;
        test_filesystem_byte_read_config_set(&config);

        test_call_control_reset(&config);
        config.fail_on_call = 1;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_concat_from_c_string_config_set(&config);

        assert(FS_UTILS_NO_MEMORY == fs_utils_text_file_read(&fs_utils, out_string));

        choco_string_destroy(&out_string);
        assert(NULL == out_string);
    }
    {
        // 正常系: 中身ありファイル
        fs_utils_t* fs_utils = NULL;
        choco_string_t* out_string = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(FS_UTILS_SUCCESS == fs_utils_create("assets/test/filesystem/", "test_file", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL != fs_utils);

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_string));

        assert(FS_UTILS_SUCCESS == fs_utils_text_file_read(fs_utils, out_string));
        assert(0 == strcmp(choco_string_c_str(out_string), "aaa\nbbbb\n"));

        choco_string_destroy(&out_string);
        fs_utils_destroy(&fs_utils);
        assert(NULL == out_string);
        assert(NULL == fs_utils);
    }
    {
        // 正常系: 空ファイル
        fs_utils_t* fs_utils = NULL;
        choco_string_t* out_string = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(FS_UTILS_SUCCESS == fs_utils_create("assets/test/filesystem/", "test_file_w", ".txt", FILESYSTEM_MODE_READ, &fs_utils));
        assert(NULL != fs_utils);

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_string));

        assert(FS_UTILS_SUCCESS == fs_utils_text_file_read(fs_utils, out_string));
        assert(0 == strcmp(choco_string_c_str(out_string), ""));

        choco_string_destroy(&out_string);
        fs_utils_destroy(&fs_utils);
        assert(NULL == out_string);
        assert(NULL == fs_utils);
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_fs_utils_fullpath_get(void) {
    {
        // fs_utils_fullpath_get() 自体の失敗注入
        test_call_control_t config = { 0 };
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_fullpath = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("path/to/", &fs_utils.filepath));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("file", &fs_utils.filename));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(".txt", &fs_utils.extension));
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_fullpath));

        config.fail_on_call = 1;
        config.forced_result = (int)FS_UTILS_RUNTIME_ERROR;
        test_fs_utils_fullpath_get_config_set(&config);

        assert(FS_UTILS_RUNTIME_ERROR == fs_utils_fullpath_get(&fs_utils, out_fullpath));

        choco_string_destroy(&out_fullpath);
        choco_string_destroy(&fs_utils.extension);
        choco_string_destroy(&fs_utils.filename);
        choco_string_destroy(&fs_utils.filepath);
    }
    {
        // fs_utils_ == NULL
        choco_string_t* out_fullpath = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_fullpath));

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_fullpath_get(NULL, out_fullpath));

        choco_string_destroy(&out_fullpath);
        assert(NULL == out_fullpath);
    }
    {
        // out_fullpath_ == NULL
        fs_utils_t fs_utils = { 0 };

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("path/to/", &fs_utils.filepath));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("file", &fs_utils.filename));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(".txt", &fs_utils.extension));
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        assert(FS_UTILS_INVALID_ARGUMENT == fs_utils_fullpath_get(&fs_utils, NULL));

        choco_string_destroy(&fs_utils.extension);
        choco_string_destroy(&fs_utils.filename);
        choco_string_destroy(&fs_utils.filepath);
    }
    {
        // fs_utils_valid_check() が false
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_fullpath = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("path/to/", &fs_utils.filepath));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("file", &fs_utils.filename));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(".txt", &fs_utils.extension));
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_fullpath));

        s_test_config_fs_utils_valid_check.fail_on_call = 1;
        s_test_config_fs_utils_valid_check.forced_result = false;

        assert(FS_UTILS_DATA_CORRUPTED == fs_utils_fullpath_get(&fs_utils, out_fullpath));

        choco_string_destroy(&out_fullpath);
        choco_string_destroy(&fs_utils.extension);
        choco_string_destroy(&fs_utils.filename);
        choco_string_destroy(&fs_utils.filepath);
    }
    {
        // choco_string_copy() 失敗(filepath copy)
        test_call_control_t config = { 0 };
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_fullpath = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("path/to/", &fs_utils.filepath));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("file", &fs_utils.filename));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(".txt", &fs_utils.extension));
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_fullpath));

        config.fail_on_call = 1;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_copy_config_set(&config);

        assert(FS_UTILS_NO_MEMORY == fs_utils_fullpath_get(&fs_utils, out_fullpath));

        choco_string_destroy(&out_fullpath);
        choco_string_destroy(&fs_utils.extension);
        choco_string_destroy(&fs_utils.filename);
        choco_string_destroy(&fs_utils.filepath);
    }
    {
        // choco_string_concat() 失敗(filename concat)
        test_call_control_t config = { 0 };
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_fullpath = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("path/to/", &fs_utils.filepath));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("file", &fs_utils.filename));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(".txt", &fs_utils.extension));
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_fullpath));

        config.fail_on_call = 1;
        config.forced_result = (int)CHOCO_STRING_OVERFLOW;
        test_choco_string_concat_config_set(&config);

        assert(FS_UTILS_OVERFLOW == fs_utils_fullpath_get(&fs_utils, out_fullpath));

        choco_string_destroy(&out_fullpath);
        choco_string_destroy(&fs_utils.extension);
        choco_string_destroy(&fs_utils.filename);
        choco_string_destroy(&fs_utils.filepath);
    }
    {
        // choco_string_concat() 失敗(extension concat)
        test_call_control_t config = { 0 };
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_fullpath = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("path/to/", &fs_utils.filepath));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("file", &fs_utils.filename));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(".txt", &fs_utils.extension));
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_fullpath));

        config.fail_on_call = 2;
        config.forced_result = (int)CHOCO_STRING_LIMIT_EXCEEDED;
        test_choco_string_concat_config_set(&config);

        assert(FS_UTILS_LIMIT_EXCEEDED == fs_utils_fullpath_get(&fs_utils, out_fullpath));

        choco_string_destroy(&out_fullpath);
        choco_string_destroy(&fs_utils.extension);
        choco_string_destroy(&fs_utils.filename);
        choco_string_destroy(&fs_utils.filepath);
    }
    {
        // 正常系(extensionあり)
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_fullpath = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("path/to/", &fs_utils.filepath));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("file", &fs_utils.filename));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string(".txt", &fs_utils.extension));
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_fullpath));

        assert(FS_UTILS_SUCCESS == fs_utils_fullpath_get(&fs_utils, out_fullpath));
        assert(0 == strcmp(choco_string_c_str(out_fullpath), "path/to/file.txt"));

        choco_string_destroy(&out_fullpath);
        choco_string_destroy(&fs_utils.extension);
        choco_string_destroy(&fs_utils.filename);
        choco_string_destroy(&fs_utils.filepath);
    }
    {
        // 正常系(extensionなし)
        fs_utils_t fs_utils = { 0 };
        choco_string_t* out_fullpath = NULL;

        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("path/to/", &fs_utils.filepath));
        assert(CHOCO_STRING_SUCCESS == choco_string_create_from_c_string("file", &fs_utils.filename));
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;
        assert(CHOCO_STRING_SUCCESS == choco_string_default_create(&out_fullpath));

        assert(FS_UTILS_SUCCESS == fs_utils_fullpath_get(&fs_utils, out_fullpath));
        assert(0 == strcmp(choco_string_c_str(out_fullpath), "path/to/file"));

        choco_string_destroy(&out_fullpath);
        choco_string_destroy(&fs_utils.filename);
        choco_string_destroy(&fs_utils.filepath);
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_rslt_to_str(void) {
    {
        assert(0 == strcmp(rslt_to_str(FS_UTILS_SUCCESS), "SUCCESS"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_INVALID_ARGUMENT), "INVALID_ARGUMENT"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_BAD_OPERATION), "BAD_OPERATION"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_DATA_CORRUPTED), "DATA_CORRUPTED"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_NO_MEMORY), "NO_MEMORY"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_LIMIT_EXCEEDED), "LIMIT_EXCEEDED"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_BAD_OPERATION), "BAD_OPERATION"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_OVERFLOW), "OVERFLOW"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_FILE_OPEN_ERROR), "FILE_OPEN_ERROR"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_RUNTIME_ERROR), "RUNTIME_ERROR"));
        assert(0 == strcmp(rslt_to_str(FS_UTILS_UNDEFINED_ERROR), "UNDEFINED_ERROR"));
    }
    {
        // 想定外値
        assert(0 == strcmp(rslt_to_str((fs_utils_result_t)9999), "UNDEFINED_ERROR"));
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_fs_utils_valid_check(void) {
    {
        // fs_utils_valid_check() 自体の失敗注入
        test_fs_utils_config_reset();

        s_test_config_fs_utils_valid_check.fail_on_call = 1;
        s_test_config_fs_utils_valid_check.forced_result = false;

        {
            fs_utils_t fs_utils = { 0 };
            fs_utils.filepath = (choco_string_t*)1;
            fs_utils.filename = (choco_string_t*)1;
            fs_utils.extension = NULL;
            fs_utils.filesystem = (filesystem_t*)1;
            fs_utils.mode = FILESYSTEM_MODE_READ;

            assert(false == fs_utils_valid_check(&fs_utils));
        }
    }
    {
        // fs_utils_ == NULL
        test_fs_utils_config_reset();

        assert(false == fs_utils_valid_check(NULL));
    }
    {
        // filename == NULL
        test_fs_utils_config_reset();

        fs_utils_t fs_utils = { 0 };
        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = NULL;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        assert(false == fs_utils_valid_check(&fs_utils));
    }
    {
        // filepath == NULL
        test_fs_utils_config_reset();

        fs_utils_t fs_utils = { 0 };
        fs_utils.filepath = NULL;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        assert(false == fs_utils_valid_check(&fs_utils));
    }
    {
        // filesystem == NULL
        test_fs_utils_config_reset();

        fs_utils_t fs_utils = { 0 };
        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = NULL;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        assert(false == fs_utils_valid_check(&fs_utils));
    }
    {
        // mode == FILESYSTEM_MODE_NONE
        test_fs_utils_config_reset();

        fs_utils_t fs_utils = { 0 };
        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_NONE;

        assert(false == fs_utils_valid_check(&fs_utils));
    }
    {
        // 正常系(extensionあり)
        test_fs_utils_config_reset();

        fs_utils_t fs_utils = { 0 };
        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = (choco_string_t*)1;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ;

        assert(true == fs_utils_valid_check(&fs_utils));
    }
    {
        // 正常系(extensionなし)
        test_fs_utils_config_reset();

        fs_utils_t fs_utils = { 0 };
        fs_utils.filepath = (choco_string_t*)1;
        fs_utils.filename = (choco_string_t*)1;
        fs_utils.extension = NULL;
        fs_utils.filesystem = (filesystem_t*)1;
        fs_utils.mode = FILESYSTEM_MODE_READ_PLUS;

        assert(true == fs_utils_valid_check(&fs_utils));
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_filesystem_result_convert(void) {
    {
        assert(FS_UTILS_SUCCESS == filesystem_result_convert(FILESYSTEM_SUCCESS));
        assert(FS_UTILS_INVALID_ARGUMENT == filesystem_result_convert(FILESYSTEM_INVALID_ARGUMENT));
        assert(FS_UTILS_RUNTIME_ERROR == filesystem_result_convert(FILESYSTEM_RUNTIME_ERROR));
        assert(FS_UTILS_NO_MEMORY == filesystem_result_convert(FILESYSTEM_NO_MEMORY));
        assert(FS_UTILS_FILE_OPEN_ERROR == filesystem_result_convert(FILESYSTEM_FILE_OPEN_ERROR));
        assert(FS_UTILS_UNDEFINED_ERROR == filesystem_result_convert(FILESYSTEM_UNDEFINED_ERROR));
        assert(FS_UTILS_LIMIT_EXCEEDED == filesystem_result_convert(FILESYSTEM_LIMIT_EXCEEDED));
        assert(FS_UTILS_BAD_OPERATION == filesystem_result_convert(FILESYSTEM_BAD_OPERATION));
    }
    {
        // fs_utilsでは直接扱わない結果コードは UNDEFINED_ERROR に変換される
        assert(FS_UTILS_UNDEFINED_ERROR == filesystem_result_convert(FILESYSTEM_EOF));
        assert(FS_UTILS_UNDEFINED_ERROR == filesystem_result_convert(FILESYSTEM_FILE_CLOSE_ERROR));
    }
    {
        // 想定外値
        assert(FS_UTILS_UNDEFINED_ERROR == filesystem_result_convert((filesystem_result_t)9999));
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_choco_string_result_convert(void) {
    {
        assert(FS_UTILS_SUCCESS == choco_string_result_convert(CHOCO_STRING_SUCCESS));
        assert(FS_UTILS_DATA_CORRUPTED == choco_string_result_convert(CHOCO_STRING_DATA_CORRUPTED));
        assert(FS_UTILS_BAD_OPERATION == choco_string_result_convert(CHOCO_STRING_BAD_OPERATION));
        assert(FS_UTILS_NO_MEMORY == choco_string_result_convert(CHOCO_STRING_NO_MEMORY));
        assert(FS_UTILS_INVALID_ARGUMENT == choco_string_result_convert(CHOCO_STRING_INVALID_ARGUMENT));
        assert(FS_UTILS_RUNTIME_ERROR == choco_string_result_convert(CHOCO_STRING_RUNTIME_ERROR));
        assert(FS_UTILS_UNDEFINED_ERROR == choco_string_result_convert(CHOCO_STRING_UNDEFINED_ERROR));
        assert(FS_UTILS_OVERFLOW == choco_string_result_convert(CHOCO_STRING_OVERFLOW));
        assert(FS_UTILS_LIMIT_EXCEEDED == choco_string_result_convert(CHOCO_STRING_LIMIT_EXCEEDED));
    }
    {
        // 想定外値
        assert(FS_UTILS_UNDEFINED_ERROR == choco_string_result_convert((choco_string_result_t)9999));
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_memory_system_result_convert(void) {
    {
        assert(FS_UTILS_SUCCESS == memory_system_result_convert(MEMORY_SYSTEM_SUCCESS));
        assert(FS_UTILS_INVALID_ARGUMENT == memory_system_result_convert(MEMORY_SYSTEM_INVALID_ARGUMENT));
        assert(FS_UTILS_RUNTIME_ERROR == memory_system_result_convert(MEMORY_SYSTEM_RUNTIME_ERROR));
        assert(FS_UTILS_LIMIT_EXCEEDED == memory_system_result_convert(MEMORY_SYSTEM_LIMIT_EXCEEDED));
        assert(FS_UTILS_BAD_OPERATION == memory_system_result_convert(MEMORY_SYSTEM_BAD_OPERATION));
        assert(FS_UTILS_NO_MEMORY == memory_system_result_convert(MEMORY_SYSTEM_NO_MEMORY));
    }
    {
        // 想定外値
        assert(FS_UTILS_UNDEFINED_ERROR == memory_system_result_convert((memory_system_result_t)9999));
    }
}

#endif
