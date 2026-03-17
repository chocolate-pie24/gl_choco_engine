/** @ingroup choco_string
 *
 * @file choco_string.c
 * @author chocolate-pie24
 * @brief 文字列を格納するコンテナモジュールAPIの実装
 *
 * @version 0.1
 * @date 2025-09-26
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h> // for SIZE_MAX

#include "engine/containers/choco_string.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include "test_controller.h"
#include "engine/containers/test_choco_string.h"
#include "engine/core/memory/test_choco_memory.h"

// choco_stringモジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_choco_string_default_create;           /**< choco_string_default_create()テスト設定 */
static test_call_control_t s_test_config_choco_string_create_from_c_string;     /**< choco_string_create_from_c_string()テスト設定 */
static test_call_control_t s_test_config_choco_string_copy;                     /**< choco_string_copy()テスト設定 */
static test_call_control_t s_test_config_choco_string_copy_from_c_string;       /**< choco_string_copy_from_c_string()テスト設定 */
static test_call_control_t s_test_config_choco_string_concat;                   /**< choco_string_concat()テスト設定 */
static test_call_control_t s_test_config_choco_string_concat_from_c_string;     /**< choco_string_concat_from_c_string()テスト設定 */
static test_call_control_size_t_t s_test_config_choco_string_length;            /**< choco_string_length()テスト設定 */

// プライベート関数テスト設定
static test_call_control_t s_test_config_string_malloc;         /**< string_malloc()テスト設定 */
static test_call_control_t s_test_config_buffer_reserve;        /**< buffer_reserve()テスト設定 */
static test_call_control_t s_test_config_buffer_resize;         /**< buffer_resize()テスト設定 */
static test_call_control_bool_t s_test_config_is_string_valid;  /**< is_string_valid()テスト設定 */
static test_call_control_size_t_t s_test_config_mock_strlen;    /**< mock_str_len()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_choco_string_default_create(void);
static void test_choco_string_create_from_c_string(void);
static void test_choco_string_destroy(void);
static void test_choco_string_copy(void);
static void test_choco_string_copy_from_c_string(void);
static void test_choco_string_concat(void);
static void test_choco_string_concat_from_c_string(void);
static void test_choco_string_length(void);
static void test_choco_string_c_str(void);
static void test_rslt_to_str(void);
static void test_string_malloc(void);
static void test_buffer_reserve(void);
static void test_buffer_resize(void);
static void test_is_string_valid(void);
#endif

/**
 * @brief 文字列コンテナ内部状態管理構造体
 *
 */
struct choco_string {
    size_t len;         /**< 文字列長さ(終端文字は含まない) */
    size_t capacity;    /**< バッファサイズ */
    char* buffer;       /**< 文字列格納バッファ */
};

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コード(成功)文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(内部データ整合異常)文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(無効な引数)文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コード(未定義エラー)文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< 実行結果コード(計算過程でオーバーフロー発生)文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用範囲上限超過) */

static const char* rslt_to_str(choco_string_result_t rslt_);
static choco_string_result_t string_malloc(size_t size_, void** out_ptr_);
static choco_string_result_t buffer_reserve(size_t size_, choco_string_t* string_);
static choco_string_result_t buffer_resize(size_t size_, choco_string_t* string_);
static bool is_string_valid(const choco_string_t* string_);
static size_t mock_strlen(const char* str_);

choco_string_result_t choco_string_default_create(choco_string_t** string_) {
#ifdef TEST_BUILD
    s_test_config_choco_string_default_create.call_count++;
    if(s_test_config_choco_string_default_create.fail_on_call != 0) {
        if(s_test_config_choco_string_default_create.call_count == s_test_config_choco_string_default_create.fail_on_call) {
            return (choco_string_result_t)s_test_config_choco_string_default_create.forced_result;
        }
    }
#endif
    choco_string_t* tmp_string = NULL;
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_default_create", "string_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_default_create", "*string_")

    // Simulation.
    ret = string_malloc(sizeof(*tmp_string), (void**)&tmp_string);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_default_create(%s) - Failed to allocate memory for 'tmp_string'.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_string, 0, sizeof(*tmp_string));

    // Commit.
    *string_ = tmp_string;
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_SUCCESS != ret) {
#ifdef TEST_BUILD
        // このassertに引っかかったらtmp_stringのリソース解放を追加する
        assert(NULL == tmp_string);
#endif
    }
    return ret;
}

choco_string_result_t choco_string_create_from_c_string(choco_string_t** string_, const char* src_) {
#ifdef TEST_BUILD
    s_test_config_choco_string_create_from_c_string.call_count++;
    if(s_test_config_choco_string_create_from_c_string.fail_on_call != 0) {
        if(s_test_config_choco_string_create_from_c_string.call_count == s_test_config_choco_string_create_from_c_string.fail_on_call) {
            return (choco_string_result_t)s_test_config_choco_string_create_from_c_string.forced_result;
        }
    }
#endif
    choco_string_t* tmp_string = NULL;
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    size_t src_len = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_create_from_c_string", "string_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_create_from_c_string", "*string_")
    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_create_from_c_string", "src_")

    // Simulation.
    ret = string_malloc(sizeof(*tmp_string), (void**)&tmp_string);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_create_from_c_string(%s) - Failed to allocate memory for 'tmp_string'.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_string, 0, sizeof(*tmp_string));

    src_len = mock_strlen(src_);
    if(0 != src_len) {
        if((SIZE_MAX - 1) < src_len) {
            ret = CHOCO_STRING_OVERFLOW;
            ERROR_MESSAGE("choco_string_create_from_c_string(%s) - Provided string is too large.", rslt_to_str(ret));
            goto cleanup;
        }
        ret = buffer_reserve(src_len + 1, tmp_string);
        if(CHOCO_STRING_SUCCESS != ret) {
            ERROR_MESSAGE("choco_string_create_from_c_string(%s) - Failed to reserve buffer space.", rslt_to_str(ret));
            goto cleanup;
        }
        memcpy(tmp_string->buffer, src_, src_len + 1);
        tmp_string->len = src_len;
    }

    // Commit.
    *string_ = tmp_string;
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_SUCCESS != ret) {
        choco_string_destroy(&tmp_string);
    }
    return ret;
}

void choco_string_destroy(choco_string_t** string_) {
    if(NULL == string_) {
        goto cleanup;
    }
    if(NULL == *string_) {
        goto cleanup;
    }
    if(NULL != (*string_)->buffer) {
        memory_system_free((*string_)->buffer, (*string_)->capacity, MEMORY_TAG_STRING);
        (*string_)->buffer = NULL;
    }
    memory_system_free(*string_, sizeof(choco_string_t), MEMORY_TAG_STRING);
    *string_ = NULL;
cleanup:
    return;
}

choco_string_result_t choco_string_copy(choco_string_t* dst_, const choco_string_t* src_) {
#ifdef TEST_BUILD
    s_test_config_choco_string_copy.call_count++;
    if(s_test_config_choco_string_copy.fail_on_call != 0) {
        if(s_test_config_choco_string_copy.call_count == s_test_config_choco_string_copy.fail_on_call) {
            return (choco_string_result_t)s_test_config_choco_string_copy.forced_result;
        }
    }
#endif
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy", "dst_")
    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy", "src_")
    if(!is_string_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Destination string (dst_) is corrupted.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!is_string_valid(src_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Source string (src_) is corrupted.", rslt_to_str(ret));
        goto cleanup;
    }

    // Commit.
    if(0 == src_->len) {
        if(NULL != dst_->buffer) {
            dst_->buffer[0] = '\0';
        }
        dst_->len = 0;
        ret = CHOCO_STRING_SUCCESS;
        goto cleanup;
    }

    if((SIZE_MAX - 1) < src_->len) {
        ret = CHOCO_STRING_OVERFLOW;
        ERROR_MESSAGE("choco_string_copy(%s) - Provided string is too large.", rslt_to_str(ret));
        goto cleanup;
    }
    if(dst_->capacity >= (src_->len + 1)) {
        memcpy(dst_->buffer, src_->buffer, src_->len + 1);  // 終端文字を含めてコピー
        dst_->len = src_->len;
    } else {
        ret = buffer_resize(src_->len + 1, dst_);
        if(CHOCO_STRING_SUCCESS != ret) {
            ERROR_MESSAGE("choco_string_copy(%s) - Failed to reserve buffer space.", rslt_to_str(ret));
            goto cleanup;
        }
        memcpy(dst_->buffer, src_->buffer, src_->len + 1);  // 終端文字を含めてコピー
        dst_->len = src_->len;
    }
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

choco_string_result_t choco_string_copy_from_c_string(choco_string_t* dst_, const char* src_) {
#ifdef TEST_BUILD
    s_test_config_choco_string_copy_from_c_string.call_count++;
    if(s_test_config_choco_string_copy_from_c_string.fail_on_call != 0) {
        if(s_test_config_choco_string_copy_from_c_string.call_count == s_test_config_choco_string_copy_from_c_string.fail_on_call) {
            return (choco_string_result_t)s_test_config_choco_string_copy_from_c_string.forced_result;
        }
    }
#endif
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    size_t src_len = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy_from_c_string", "dst_")
    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy_from_c_string", "src_")
    if(!is_string_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Destination string (dst_) is corrupted.", rslt_to_str(ret));
        goto cleanup;
    }

    src_len = mock_strlen(src_);
    if(0 == src_len) {
        if(NULL != dst_->buffer) {
            dst_->buffer[0] = '\0';
        }
        dst_->len = 0;
        ret = CHOCO_STRING_SUCCESS;
        goto cleanup;
    }

    // Simulation, Commit.
    if((SIZE_MAX - 1) < src_len) {
        ret = CHOCO_STRING_OVERFLOW;
        ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Provided string is too large.", rslt_to_str(ret));
        goto cleanup;
    }
    if(dst_->capacity >= (src_len + 1)) {
        memcpy(dst_->buffer, src_, src_len + 1);
        dst_->len = src_len;
    } else {
        ret = buffer_resize(src_len + 1, dst_);
        if(CHOCO_STRING_SUCCESS != ret) {
            ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Failed to resize the buffer.", rslt_to_str(ret));
            goto cleanup;
        }
        memcpy(dst_->buffer, src_, src_len + 1);
        dst_->len = src_len;
    }
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

choco_string_result_t choco_string_concat(const choco_string_t* string_, choco_string_t* dst_) {
#ifdef TEST_BUILD
    s_test_config_choco_string_concat.call_count++;
    if(s_test_config_choco_string_concat.fail_on_call != 0) {
        if(s_test_config_choco_string_concat.call_count == s_test_config_choco_string_concat.fail_on_call) {
            return (choco_string_result_t)s_test_config_choco_string_concat.forced_result;
        }
    }
#endif
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    size_t dst_len_new = 0;
    char* tmp_buffer = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_concat", "dst_")
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_concat", "string_")
    IF_ARG_FALSE_GOTO_CLEANUP((const void*)dst_ != (const void*)string_, ret, CHOCO_STRING_BAD_OPERATION, rslt_to_str(CHOCO_STRING_BAD_OPERATION), "choco_string_concat", "dst_, string_")
    if(!is_string_valid(string_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Source string (string_) is corrupted.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!is_string_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Destination string (dst_) is corrupted.", rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX - dst_->len - 1) < string_->len) {
        ret = CHOCO_STRING_OVERFLOW;
        ERROR_MESSAGE("choco_string_concat(%s) - Resulting string length is too large.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 != string_->len) {
        dst_len_new = string_->len + dst_->len;
        if((dst_len_new + 1) <= dst_->capacity) {
            memcpy(dst_->buffer + dst_->len, string_->buffer, string_->len + 1);
            dst_->len = dst_len_new;
        } else {
            ret = string_malloc(dst_len_new + 1, (void**)&tmp_buffer);
            if(CHOCO_STRING_SUCCESS != ret) {
                ERROR_MESSAGE("choco_string_concat(%s) - Failed to allocate memory for 'tmp_buffer'.", rslt_to_str(ret));
                goto cleanup;
            }
            memset(tmp_buffer, 0, dst_len_new + 1);
            if(0 != dst_->len) {
                memcpy(tmp_buffer, dst_->buffer, dst_->len);
            }
            memcpy(tmp_buffer + dst_->len, string_->buffer, string_->len + 1);
            if(0 != dst_->capacity) {
                memory_system_free(dst_->buffer, dst_->capacity, MEMORY_TAG_STRING);
                dst_->buffer = NULL;
            }

            dst_->buffer = tmp_buffer;
            dst_->capacity = dst_len_new + 1;
            dst_->len = dst_len_new;
        }
    }
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

choco_string_result_t choco_string_concat_from_c_string(const char* string_, choco_string_t* dst_) {
#ifdef TEST_BUILD
    s_test_config_choco_string_concat_from_c_string.call_count++;
    if(s_test_config_choco_string_concat_from_c_string.fail_on_call != 0) {
        if(s_test_config_choco_string_concat_from_c_string.call_count == s_test_config_choco_string_concat_from_c_string.fail_on_call) {
            return (choco_string_result_t)s_test_config_choco_string_concat_from_c_string.forced_result;
        }
    }
#endif
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    size_t dst_len_new = 0;
    size_t src_len = 0;
    char* tmp_buffer = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_concat_from_c_string", "dst_")
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_concat_from_c_string", "string_")
    if(!is_string_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Destination string (dst_) is corrupted.", rslt_to_str(ret));
        goto cleanup;
    }

    src_len = mock_strlen(string_);
    if((SIZE_MAX - dst_->len - 1) < src_len) {
        ret = CHOCO_STRING_OVERFLOW;
        ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Resulting string length is too large.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 != src_len) {
        dst_len_new = src_len + dst_->len;
        if((dst_len_new + 1) <= dst_->capacity) {
            memcpy(dst_->buffer + dst_->len, string_, src_len + 1);
            dst_->len = dst_len_new;
        } else {
            ret = string_malloc(dst_len_new + 1, (void**)&tmp_buffer);
            if(CHOCO_STRING_SUCCESS != ret) {
                ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Failed to allocate memory for 'tmp_buffer'.", rslt_to_str(ret));
                goto cleanup;
            }
            memset(tmp_buffer, 0, dst_len_new + 1);
            if(0 != dst_->len) {
                memcpy(tmp_buffer, dst_->buffer, dst_->len);
            }
            memcpy(tmp_buffer + dst_->len, string_, src_len + 1);
            if(0 != dst_->capacity) {
                memory_system_free(dst_->buffer, dst_->capacity, MEMORY_TAG_STRING);
                dst_->buffer = NULL;
            }

            dst_->buffer = tmp_buffer;
            dst_->capacity = dst_len_new + 1;
            dst_->len = dst_len_new;
        }
    }
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

size_t choco_string_length(const choco_string_t* string_) {
#ifdef TEST_BUILD
    s_test_config_choco_string_length.call_count++;
    if(s_test_config_choco_string_length.fail_on_call != 0) {
        if(s_test_config_choco_string_length.call_count == s_test_config_choco_string_length.fail_on_call) {
            return s_test_config_choco_string_length.forced_result;
        }
    }
#endif
    if(NULL == string_) {
        return 0;
    } else {
        return string_->len;
    }
}

const char* choco_string_c_str(const choco_string_t* string_) {
    if(NULL == string_) {
        return "";
    } else {
        if(NULL != string_->buffer) {
            return string_->buffer;
        } else {
            return "";
        }
    }
}

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param rslt_ 文字列に変換する実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* rslt_to_str(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return s_rslt_str_success;
    case CHOCO_STRING_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case CHOCO_STRING_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case CHOCO_STRING_NO_MEMORY:
        return s_rslt_str_no_memory;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case CHOCO_STRING_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case CHOCO_STRING_OVERFLOW:
        return s_rslt_str_overflow;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    default:
        return s_rslt_str_undefined_error;
    }
}

/**
 * @brief memory_system_allocateのラッパ関数で、指定されたサイズのメモリを確保する
 *
 * @note
 * - 実行結果コードをchoco_stringモジュールの実行結果コードに変換して出力する
 * - メモリタグはMEMORY_TAG_STRING固定
 *
 * @param size_ 確保するメモリサイズ
 * @param out_ptr_ 確保したメモリの先頭アドレス
 * @retval CHOCO_STRING_INVALID_ARGUMENT 下記のいずれか
 * - out_ptr_ == NULL
 * - *out_ptr_ != NULL
 * - memory_system_allocateの実行結果がMEMORY_SYSTEM_INVALID_ARGUMENT
 * @retval CHOCO_STRING_NO_MEMORY メモリ確保失敗
 * @retval CHOCO_STRING_LIMIT_EXCEEDED 以下のいずれか
 * - 割り当てサイズを割り当てた結果、mem_tag_allocatedがSIZE_MAX超過
 * - 割り当てサイズを割り当てた結果、total_allocatedがSIZE_MAX超過
 * @retval CHOCO_STRING_RUNTIME_ERROR memory_system_allocateがMEMORY_SYSTEM_RUNTIME_ERRORを返した
 * @retval CHOCO_STRING_SUCCESS メモリ確保に成功し、正常終了
 */
static choco_string_result_t string_malloc(size_t size_, void** out_ptr_) {
#ifdef TEST_BUILD
    s_test_config_string_malloc.call_count++;
    if(s_test_config_string_malloc.fail_on_call != 0) {
        if(s_test_config_string_malloc.call_count == s_test_config_string_malloc.fail_on_call) {
            return (choco_string_result_t)s_test_config_string_malloc.forced_result;
        }
    }
#endif
    void* tmp_ptr = NULL;
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "string_malloc", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "string_malloc", "*out_ptr_")

    ret_mem = memory_system_allocate(size_, MEMORY_TAG_STRING, &tmp_ptr);
    switch(ret_mem) {
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        ret = CHOCO_STRING_INVALID_ARGUMENT;
        goto cleanup;
    case MEMORY_SYSTEM_NO_MEMORY:
        ret = CHOCO_STRING_NO_MEMORY;
        goto cleanup;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        ret = CHOCO_STRING_LIMIT_EXCEEDED;
        goto cleanup;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        ret = CHOCO_STRING_RUNTIME_ERROR;
        goto cleanup;
    case MEMORY_SYSTEM_SUCCESS:
        ret = CHOCO_STRING_SUCCESS;
        break;
    default:
        ret = CHOCO_STRING_UNDEFINED_ERROR;
        goto cleanup;
    }
    *out_ptr_ = tmp_ptr;
    ret = CHOCO_STRING_SUCCESS;
cleanup:
    return ret;
}

// string_のbufferのメモリを初回に確保するためのAPI。既にbufferのメモリを確保済の場合にはbuffer_resizeを使用する
// 処理に失敗した場合(返り値がCHOCO_STRING_SUCCESS以外)には引数のstring_の状態は不変。
static choco_string_result_t buffer_reserve(size_t size_, choco_string_t* string_) {
#ifdef TEST_BUILD
    s_test_config_buffer_reserve.call_count++;
    if(s_test_config_buffer_reserve.fail_on_call != 0) {
        if(s_test_config_buffer_reserve.call_count == s_test_config_buffer_reserve.fail_on_call) {
            return (choco_string_result_t)s_test_config_buffer_reserve.forced_result;
        }
    }
#endif
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    char* tmp_buffer = NULL;

    IF_ARG_FALSE_GOTO_CLEANUP(size_ > 0, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_reserve", "size_")
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_reserve", "string_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == string_->capacity, ret, CHOCO_STRING_BAD_OPERATION, rslt_to_str(CHOCO_STRING_BAD_OPERATION), "buffer_reserve", "string_->capacity")
    if(!is_string_valid(string_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        goto cleanup;
    }

    ret = string_malloc(size_, (void**)&tmp_buffer);
    if(CHOCO_STRING_SUCCESS != ret) {
        goto cleanup;
    }
    memset(tmp_buffer, 0, size_);
    string_->buffer = tmp_buffer;
    string_->len = 0;
    string_->capacity = size_;
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_SUCCESS != ret) {
#ifdef TEST_BUILD
        // このassertに引っかかったらtmp_stringのリソース解放を追加する
        assert(NULL == tmp_buffer);
#endif
    }
    return ret;
}

// 既に確保済のbufferサイズを変更(拡張or縮小)する場合に使用する。
// 初回のメモリ確保に使用することも可能だが、実行速度がbuffer_reserveの方が若干速いため、そちらの使用を推奨する。
// 処理に失敗した場合(返り値がCHOCO_STRING_SUCCESS以外)はstring_の状態は不変。
// サイズを変更後、bufferのデータは全て0に初期化され、lenの値も0になる。
// データをそのまま残してもよいが、サイズを縮小した場合にはデータが削られることになる。
// この関数を読んだ後のbufferの状態を拡大、縮小共に共通にしたいため、全て0に初期化することにする
// このため、バッファの拡張を目的に本関数を使用する場合には一旦内部データを退避してから呼び出すこと。
static choco_string_result_t buffer_resize(size_t size_, choco_string_t* string_) {
#ifdef TEST_BUILD
    s_test_config_buffer_resize.call_count++;
    if(s_test_config_buffer_resize.fail_on_call != 0) {
        if(s_test_config_buffer_resize.call_count == s_test_config_buffer_resize.fail_on_call) {
            return (choco_string_result_t)s_test_config_buffer_resize.forced_result;
        }
    }
#endif
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    char* tmp_buffer = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_resize", "string_")
    IF_ARG_FALSE_GOTO_CLEANUP(size_ > 0, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_resize", "size_")
    if(!is_string_valid(string_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        goto cleanup;
    }

    // Simulation.
    ret = string_malloc(size_, (void**)&tmp_buffer);
    if(CHOCO_STRING_SUCCESS != ret) {
        goto cleanup;
    }
    memset(tmp_buffer, 0, size_);

    // Commit.
    if(0 != string_->capacity) {
        memory_system_free(string_->buffer, string_->capacity, MEMORY_TAG_STRING);
        string_->buffer = NULL;
    }
    string_->buffer = tmp_buffer;
    string_->len = 0;
    string_->capacity = size_;
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_SUCCESS != ret) {
#ifdef TEST_BUILD
        // このassertに引っかかったらtmp_stringのリソース解放を追加する
        assert(NULL == tmp_buffer);
#endif
    }
    return ret;
}

// 本関数を呼び出す前に必ず引数が非NULLであることを保証すること
// (本関数内でNULLチェックを行う場合,事前NULLチェックとの競合によりそのコードを動かすことができないため)
static bool is_string_valid(const choco_string_t* string_) {
#ifdef TEST_BUILD
    s_test_config_is_string_valid.call_count++;
    if(s_test_config_is_string_valid.fail_on_call != 0) {
        if(s_test_config_is_string_valid.call_count == s_test_config_is_string_valid.fail_on_call) {
            return s_test_config_is_string_valid.forced_result;
        }
    }
#endif
    if((SIZE_MAX - 1) < string_->len) {
        return false;
    } else if(string_->capacity < (string_->len + 1) && 0 != string_->len) {
        return false;
    } else if(0 == string_->capacity && NULL != string_->buffer) {
        return false;
    } else if(0 != string_->capacity && NULL == string_->buffer) {
        return false;
    } else if(0 != string_->len && '\0' != string_->buffer[string_->len]) {
        return false;
    } else if(0 == string_->len && 0 < string_->capacity && '\0' != string_->buffer[0]) {
        return false;
    }
    return true;
}

static size_t NO_COVERAGE mock_strlen(const char* str_) {
#ifdef TEST_BUILD
    s_test_config_mock_strlen.call_count++;
    if(s_test_config_mock_strlen.fail_on_call != 0) {
        if(s_test_config_mock_strlen.call_count == s_test_config_mock_strlen.fail_on_call) {
            return s_test_config_mock_strlen.forced_result;
        }
    }
#endif
    return strlen(str_);
}

#ifdef TEST_BUILD
void test_choco_string_default_create_config_set(const test_call_control_t* config_) {
    s_test_config_choco_string_default_create.fail_on_call = config_->fail_on_call;
    s_test_config_choco_string_default_create.forced_result = config_->forced_result;
}

void test_choco_string_create_from_c_string_config_set(const test_call_control_t* config_) {
    s_test_config_choco_string_create_from_c_string.fail_on_call = config_->fail_on_call;
    s_test_config_choco_string_create_from_c_string.forced_result = config_->forced_result;
}

void test_choco_string_copy_config_set(const test_call_control_t* config_) {
    s_test_config_choco_string_copy.fail_on_call = config_->fail_on_call;
    s_test_config_choco_string_copy.forced_result = config_->forced_result;
}

void test_choco_string_copy_from_c_string_config_set(const test_call_control_t* config_) {
    s_test_config_choco_string_copy_from_c_string.fail_on_call = config_->fail_on_call;
    s_test_config_choco_string_copy_from_c_string.forced_result = config_->forced_result;
}

void test_choco_string_concat_config_set(const test_call_control_t* config_) {
    s_test_config_choco_string_concat.fail_on_call = config_->fail_on_call;
    s_test_config_choco_string_concat.forced_result = config_->forced_result;
}

void test_choco_string_concat_from_c_string_config_set(const test_call_control_t* config_) {
    s_test_config_choco_string_concat_from_c_string.fail_on_call = config_->fail_on_call;
    s_test_config_choco_string_concat_from_c_string.forced_result = config_->forced_result;
}

void test_choco_string_length_config_set(const test_call_control_size_t_t* config_) {
    s_test_config_choco_string_length.fail_on_call = config_->fail_on_call;
    s_test_config_choco_string_length.forced_result = config_->forced_result;
}

void test_choco_string_config_reset(void) {
    test_call_control_reset(&s_test_config_choco_string_default_create);
    test_call_control_reset(&s_test_config_choco_string_create_from_c_string);
    test_call_control_reset(&s_test_config_choco_string_copy);
    test_call_control_reset(&s_test_config_choco_string_copy_from_c_string);
    test_call_control_reset(&s_test_config_choco_string_concat);
    test_call_control_reset(&s_test_config_choco_string_concat_from_c_string);
    test_call_control_size_t_reset(&s_test_config_choco_string_length);

    test_call_control_reset(&s_test_config_string_malloc);
    test_call_control_reset(&s_test_config_buffer_reserve);
    test_call_control_reset(&s_test_config_buffer_resize);
    test_call_control_bool_reset(&s_test_config_is_string_valid);
    test_call_control_size_t_reset(&s_test_config_mock_strlen);
}

void test_choco_string(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    test_choco_string_default_create();
    test_choco_string_create_from_c_string();
    test_choco_string_destroy();
    test_choco_string_copy();
    test_choco_string_copy_from_c_string();
    test_choco_string_concat();
    test_choco_string_concat_from_c_string();
    test_choco_string_length();
    test_choco_string_c_str();
    test_rslt_to_str();
    test_string_malloc();
    test_buffer_reserve();
    test_buffer_resize();
    test_is_string_valid();

    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_choco_string_default_create(void) {
    memory_system_create();
    {
        // choco_string_default_create() 冒頭で強制的に CHOCO_STRING_NO_MEMORY を返させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        test_call_control_t config = {0};

        test_choco_string_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_default_create_config_set(&config);

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;

        test_choco_string_config_reset();

        ret = choco_string_default_create(NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        test_choco_string_config_reset();
    }
    {
        // *string_ != NULL -> CHOCO_STRING_INVALID_ARGUMENT
        // 既存ポインタは変更されないことを確認
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = (choco_string_t*)0x1;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert((choco_string_t*)0x1 == string);

        test_choco_string_config_reset();
    }
    {
        // string_malloc() 失敗 -> CHOCO_STRING_NO_MEMORY
        // *string_ は NULL のまま
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // 正常系
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);

        assert(0U == string->len);
        assert(0U == string->capacity);
        assert(NULL == string->buffer);

        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_choco_string_create_from_c_string(void) {
    memory_system_create();
    {
        // choco_string_create_from_c_string() 冒頭で強制的に CHOCO_STRING_NO_MEMORY を返させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        test_call_control_t config = {0};

        test_choco_string_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(NULL, "aaa");
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        test_choco_string_config_reset();
    }
    {
        // *string_ != NULL -> CHOCO_STRING_INVALID_ARGUMENT
        // 既存ポインタは変更されないことを確認
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = (choco_string_t*)0x1;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert((choco_string_t*)0x1 == string);

        test_choco_string_config_reset();
    }
    {
        // src_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&string, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // src_ == "" -> SUCCESS
        // buffer_reserve() は呼ばれないため、注入を入れても成功することを確認
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        s_test_config_buffer_reserve.fail_on_call = 1U;
        s_test_config_buffer_reserve.forced_result = (int)CHOCO_STRING_LIMIT_EXCEEDED;

        ret = choco_string_create_from_c_string(&string, "");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(0U == string->len);
        assert(0U == string->capacity);
        assert(NULL == string->buffer);

        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // tmp_string 確保失敗 -> CHOCO_STRING_NO_MEMORY
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // src_len overflow -> CHOCO_STRING_OVERFLOW
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        s_test_config_mock_strlen.fail_on_call = 1U;
        s_test_config_mock_strlen.forced_result = SIZE_MAX;

        ret = choco_string_create_from_c_string(&string, "a");
        assert(CHOCO_STRING_OVERFLOW == ret);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // buffer_reserve 注入失敗 -> 注入結果が返る
        // cleanup により *string_ は NULL のまま
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        s_test_config_buffer_reserve.fail_on_call = 1U;
        s_test_config_buffer_reserve.forced_result = (int)CHOCO_STRING_LIMIT_EXCEEDED;

        ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_LIMIT_EXCEEDED == ret);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // buffer_reserve 内の string_malloc 失敗 -> CHOCO_STRING_NO_MEMORY
        // 1回目: tmp_string, 2回目: buffer_reserve 内のバッファ確保
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        s_test_config_string_malloc.fail_on_call = 2U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // 正常系（非空文字列）
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(3U == string->len);
        assert(4U == string->capacity);
        assert(NULL != string->buffer);
        assert(0 == strcmp(string->buffer, "aaa"));

        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_choco_string_destroy(void) {
    memory_system_create();
    {
        // string_ == NULL -> no-op
        test_choco_string_config_reset();

        choco_string_destroy(NULL);

        test_choco_string_config_reset();
    }
    {
        // *string_ == NULL -> no-op
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // *string_ != NULL かつ buffer == NULL -> 構造体のみ解放
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(0U == string->len);
        assert(0U == string->capacity);
        assert(NULL == string->buffer);

        choco_string_destroy(&string);
        assert(NULL == string);

        // 2回目も no-op
        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // *string_ != NULL かつ buffer != NULL -> buffer と構造体を解放
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&string, "abc");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(3U == string->len);
        assert(4U == string->capacity);
        assert(NULL != string->buffer);
        assert(0 == strcmp(string->buffer, "abc"));

        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_choco_string_copy(void) {
    memory_system_create();
    {
        // choco_string_copy() 冒頭で強制的に CHOCO_STRING_NO_MEMORY を返させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        test_call_control_t config = {0};

        choco_string_t* dst = NULL;
        choco_string_t* src = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        ret = choco_string_create_from_c_string(&src, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_copy_config_set(&config);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_NO_MEMORY == ret);

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
        assert(NULL == dst);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // dst_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy(NULL, src);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&src);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // src_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy(dst, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // 壊れた dst_ -> CHOCO_STRING_DATA_CORRUPTED
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;

        struct choco_string corrupted_dst;
        memset(&corrupted_dst, 0, sizeof(corrupted_dst));
        corrupted_dst.len = 0U;
        corrupted_dst.capacity = 0U;
        corrupted_dst.buffer = (char*)0x1;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy((choco_string_t*)&corrupted_dst, src);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        choco_string_destroy(&src);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // 壊れた src_ -> CHOCO_STRING_DATA_CORRUPTED
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        struct choco_string corrupted_src;
        memset(&corrupted_src, 0, sizeof(corrupted_src));
        corrupted_src.len = 0U;
        corrupted_src.capacity = 0U;
        corrupted_src.buffer = (char*)0x1;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy(dst, (const choco_string_t*)&corrupted_src);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // src_->len == 0 かつ dst_->buffer != NULL -> dst_->buffer[0] = '\0'
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&src);
        assert(CHOCO_STRING_SUCCESS == ret);
        ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == dst->len);
        assert(NULL != dst->buffer);
        assert(0 == strcmp(dst->buffer, ""));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
        assert(NULL == dst);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // src_->len == 0 かつ dst_->buffer == NULL -> buffer には触れず SUCCESS
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&src);
        assert(CHOCO_STRING_SUCCESS == ret);
        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == dst->len);
        assert(0U == dst->capacity);
        assert(NULL == dst->buffer);

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
        assert(NULL == dst);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // OVERFLOW 分岐
        // 2回目の is_string_valid(src_) だけ強制的に true にして到達させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        struct choco_string fake_src;
        memset(&fake_src, 0, sizeof(fake_src));
        fake_src.len = SIZE_MAX;
        fake_src.capacity = 0U;
        fake_src.buffer = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        s_test_config_is_string_valid.fail_on_call = 2U;
        s_test_config_is_string_valid.forced_result = true;

        ret = choco_string_copy(dst, (const choco_string_t*)&fake_src);
        assert(CHOCO_STRING_OVERFLOW == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // dst_->capacity >= (src_->len + 1) -> resize なしでコピー成功
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_create_from_c_string(&dst, "bbbbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(6U == dst->capacity);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3U == dst->len);
        assert(6U == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaa"));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
        assert(NULL == dst);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // dst_->capacity < (src_->len + 1) -> buffer_resize 経由でコピー成功
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(4U == dst->capacity);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5U == dst->len);
        assert(6U == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaaaa"));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
        assert(NULL == dst);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // buffer_resize 注入失敗 -> 注入結果が返る & dst 不変
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;
        char* old_ptr = NULL;
        size_t old_len = 0U;
        size_t old_cap = 0U;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_ptr = dst->buffer;
        old_len = dst->len;
        old_cap = dst->capacity;

        s_test_config_buffer_resize.fail_on_call = 1U;
        s_test_config_buffer_resize.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(old_ptr == dst->buffer);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
        assert(NULL == dst);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // buffer_resize 内の string_malloc 失敗 -> CHOCO_STRING_NO_MEMORY & dst 不変
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;
        char* old_ptr = NULL;
        size_t old_len = 0U;
        size_t old_cap = 0U;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_ptr = dst->buffer;
        old_len = dst->len;
        old_cap = dst->capacity;

        test_choco_string_config_reset();

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(old_ptr == dst->buffer);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
        assert(NULL == dst);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_choco_string_copy_from_c_string(void) {
    memory_system_create();
    {
        // choco_string_copy_from_c_string() 冒頭で強制的に CHOCO_STRING_NO_MEMORY を返させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        test_call_control_t config = {0};
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_copy_from_c_string_config_set(&config);

        ret = choco_string_copy_from_c_string(dst, "aaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // dst_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        test_choco_string_config_reset();

        ret = choco_string_copy_from_c_string(NULL, "aaa");
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        test_choco_string_config_reset();
    }
    {
        // src_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy_from_c_string(dst, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // 壊れた dst_ -> CHOCO_STRING_DATA_CORRUPTED
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        struct choco_string corrupted_dst;

        memset(&corrupted_dst, 0, sizeof(corrupted_dst));
        corrupted_dst.len = 0U;
        corrupted_dst.capacity = 0U;
        corrupted_dst.buffer = (char*)0x1;

        test_choco_string_config_reset();

        ret = choco_string_copy_from_c_string((choco_string_t*)&corrupted_dst, "aaa");
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        test_choco_string_config_reset();
    }
    {
        // src_len == 0 かつ dst_->buffer != NULL -> buffer[0] を '\0' にする
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != dst->buffer);

        ret = choco_string_copy_from_c_string(dst, "");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == dst->len);
        assert(0 == strcmp(dst->buffer, ""));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // src_len == 0 かつ dst_->buffer == NULL -> buffer には触れず SUCCESS
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL == dst->buffer);

        ret = choco_string_copy_from_c_string(dst, "");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == dst->len);
        assert(0U == dst->capacity);
        assert(NULL == dst->buffer);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // src_len overflow -> CHOCO_STRING_OVERFLOW
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        s_test_config_mock_strlen.fail_on_call = 1U;
        s_test_config_mock_strlen.forced_result = SIZE_MAX;

        ret = choco_string_copy_from_c_string(dst, "a");
        assert(CHOCO_STRING_OVERFLOW == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // dst_->capacity >= (src_len + 1) -> resize なしで SUCCESS
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbbbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(6U == dst->capacity);

        ret = choco_string_copy_from_c_string(dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3U == dst->len);
        assert(6U == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // dst_->capacity < (src_len + 1) -> buffer_resize 経由で SUCCESS
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(4U == dst->capacity);

        ret = choco_string_copy_from_c_string(dst, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5U == dst->len);
        assert(6U == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaaaa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // dst_->capacity == 0 かつ src non-empty -> buffer_resize 経由で SUCCESS
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == dst->capacity);
        assert(NULL == dst->buffer);

        ret = choco_string_copy_from_c_string(dst, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5U == dst->len);
        assert(6U == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaaaa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // buffer_resize 注入失敗 -> 注入結果が返る & dst 不変
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        char* old_ptr = NULL;
        size_t old_len = 0U;
        size_t old_cap = 0U;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_ptr = dst->buffer;
        old_len = dst->len;
        old_cap = dst->capacity;

        s_test_config_buffer_resize.fail_on_call = 1U;
        s_test_config_buffer_resize.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_copy_from_c_string(dst, "aaaaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(old_ptr == dst->buffer);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // buffer_resize 内の string_malloc 失敗 -> CHOCO_STRING_NO_MEMORY & dst 不変
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        char* old_ptr = NULL;
        size_t old_len = 0U;
        size_t old_cap = 0U;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_ptr = dst->buffer;
        old_len = dst->len;
        old_cap = dst->capacity;

        test_choco_string_config_reset();

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_copy_from_c_string(dst, "aaaaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(old_ptr == dst->buffer);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_choco_string_concat(void) {
    memory_system_create();
    {
        // choco_string_concat() 冒頭で強制的に CHOCO_STRING_NO_MEMORY を返させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        test_call_control_t config = {0};
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "a");
        assert(CHOCO_STRING_SUCCESS == ret);
        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_concat_config_set(&config);

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_NO_MEMORY == ret);

        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // dst_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "a");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(src, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&src);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(NULL, dst);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // dst_ == string_ -> CHOCO_STRING_BAD_OPERATION
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* s = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&s, "a");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(s, s);
        assert(CHOCO_STRING_BAD_OPERATION == ret);

        choco_string_destroy(&s);
        assert(NULL == s);

        test_choco_string_config_reset();
    }
    {
        // 壊れた string_ -> CHOCO_STRING_DATA_CORRUPTED
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        size_t old_len = 0U;
        size_t old_cap = 0U;
        char* old_ptr = NULL;

        struct choco_string corrupted_src;
        memset(&corrupted_src, 0, sizeof(corrupted_src));
        corrupted_src.len = 0U;
        corrupted_src.capacity = 0U;
        corrupted_src.buffer = (char*)0x1;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_len = dst->len;
        old_cap = dst->capacity;
        old_ptr = dst->buffer;

        ret = choco_string_concat((const choco_string_t*)&corrupted_src, dst);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // 壊れた dst_ -> CHOCO_STRING_DATA_CORRUPTED
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;

        struct choco_string corrupted_dst;
        memset(&corrupted_dst, 0, sizeof(corrupted_dst));
        corrupted_dst.len = 0U;
        corrupted_dst.capacity = 0U;
        corrupted_dst.buffer = (char*)0x1;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "a");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(src, (choco_string_t*)&corrupted_dst);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        choco_string_destroy(&src);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // overflow -> CHOCO_STRING_OVERFLOW
        // source は実体で valid、dst は 2回目の is_string_valid() を強制 true にして到達
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;

        struct choco_string fake_dst;
        memset(&fake_dst, 0, sizeof(fake_dst));
        fake_dst.len = SIZE_MAX - 2U;
        fake_dst.capacity = 0U;
        fake_dst.buffer = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&src, "aa");
        assert(CHOCO_STRING_SUCCESS == ret);

        test_choco_string_config_reset();

        s_test_config_is_string_valid.fail_on_call = 2U;
        s_test_config_is_string_valid.forced_result = true;

        ret = choco_string_concat(src, (choco_string_t*)&fake_dst);
        assert(CHOCO_STRING_OVERFLOW == ret);

        choco_string_destroy(&src);
        assert(NULL == src);

        test_choco_string_config_reset();
    }
    {
        // string_->len == 0 -> SUCCESS(no-op)
        // 次のメモリ確保失敗を設定しても成功することを確認
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;
        size_t old_len = 0U;
        size_t old_cap = 0U;
        char* old_ptr = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&src);
        assert(CHOCO_STRING_SUCCESS == ret);
        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_len = dst->len;
        old_cap = dst->capacity;
        old_ptr = dst->buffer;

        test_choco_string_config_reset();

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // (dst_len_new + 1) <= dst_->capacity -> in-place concat
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;
        size_t old_cap = 0U;
        char* old_ptr = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = buffer_resize(16U, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(16U == dst->capacity);

        ret = choco_string_copy_from_c_string(dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3U == dst->len);

        ret = choco_string_create_from_c_string(&src, "aa");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_cap = dst->capacity;
        old_ptr = dst->buffer;

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5U == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbbaa"));

        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // capacity不足 -> 再確保経由で SUCCESS（dst_->len != 0 かつ dst_->capacity != 0）
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(4U == dst->capacity);

        ret = choco_string_create_from_c_string(&src, "aa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5U == dst->len);
        assert(6U == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbbaa"));

        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // capacity不足 -> 再確保経由で SUCCESS（dst_->len == 0 かつ dst_->capacity == 0）
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == dst->capacity);
        assert(NULL == dst->buffer);

        ret = choco_string_create_from_c_string(&src, "aa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(2U == dst->len);
        assert(3U == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aa"));

        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // 再確保側で string_malloc 失敗 -> CHOCO_STRING_NO_MEMORY、dst不変
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        choco_string_t* dst = NULL;
        size_t old_len = 0U;
        size_t old_cap = 0U;
        char* old_ptr = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_create_from_c_string(&src, "aa");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_len = dst->len;
        old_cap = dst->capacity;
        old_ptr = dst->buffer;

        test_choco_string_config_reset();

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_choco_string_concat_from_c_string(void) {
    memory_system_create();
    {
        // choco_string_concat_from_c_string() 冒頭で強制的に CHOCO_STRING_NO_MEMORY を返させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        test_call_control_t config = {0};
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_concat_from_c_string_config_set(&config);

        ret = choco_string_concat_from_c_string("aa", dst);
        assert(CHOCO_STRING_NO_MEMORY == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // dst_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        test_choco_string_config_reset();

        ret = choco_string_concat_from_c_string("a", NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        test_choco_string_config_reset();
    }
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat_from_c_string(NULL, dst);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // 壊れた dst_ -> CHOCO_STRING_DATA_CORRUPTED
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        struct choco_string corrupted_dst;

        memset(&corrupted_dst, 0, sizeof(corrupted_dst));
        corrupted_dst.len = 0U;
        corrupted_dst.capacity = 0U;
        corrupted_dst.buffer = (char*)0x1;

        test_choco_string_config_reset();

        ret = choco_string_concat_from_c_string("a", (choco_string_t*)&corrupted_dst);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        test_choco_string_config_reset();
    }
    {
        // overflow -> CHOCO_STRING_OVERFLOW
        // dst_ の妥当性判定だけ強制 true にして到達させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        struct choco_string fake_dst;

        memset(&fake_dst, 0, sizeof(fake_dst));
        fake_dst.len = SIZE_MAX - 2U;
        fake_dst.capacity = 0U;
        fake_dst.buffer = NULL;

        test_choco_string_config_reset();

        s_test_config_is_string_valid.fail_on_call = 1U;
        s_test_config_is_string_valid.forced_result = true;

        ret = choco_string_concat_from_c_string("aa", (choco_string_t*)&fake_dst);
        assert(CHOCO_STRING_OVERFLOW == ret);

        test_choco_string_config_reset();
    }
    {
        // src_len == 0 -> SUCCESS(no-op)
        // 次のメモリ確保失敗を設定しても成功することを確認
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        size_t old_len = 0U;
        size_t old_cap = 0U;
        char* old_ptr = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_len = dst->len;
        old_cap = dst->capacity;
        old_ptr = dst->buffer;

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_concat_from_c_string("", dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // (dst_len_new + 1) <= dst_->capacity -> in-place concat
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        size_t old_cap = 0U;
        char* old_ptr = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = buffer_resize(16U, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(16U == dst->capacity);

        ret = choco_string_copy_from_c_string(dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3U == dst->len);

        old_cap = dst->capacity;
        old_ptr = dst->buffer;

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_concat_from_c_string("aa", dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5U == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbbaa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // capacity不足 -> 再確保経由で SUCCESS（dst_->len != 0 かつ dst_->capacity != 0）
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(4U == dst->capacity);

        ret = choco_string_concat_from_c_string("aa", dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5U == dst->len);
        assert(6U == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbbaa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // capacity不足 -> 再確保経由で SUCCESS（dst_->len == 0 かつ dst_->capacity == 0）
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == dst->capacity);
        assert(NULL == dst->buffer);

        ret = choco_string_concat_from_c_string("aa", dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(2U == dst->len);
        assert(3U == dst->capacity);
        assert(NULL != dst->buffer);
        assert(0 == strcmp(dst->buffer, "aa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    {
        // 再確保側で string_malloc 失敗 -> CHOCO_STRING_NO_MEMORY、dst不変
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        size_t old_len = 0U;
        size_t old_cap = 0U;
        char* old_ptr = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        old_len = dst->len;
        old_cap = dst->capacity;
        old_ptr = dst->buffer;

        test_choco_string_config_reset();

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = choco_string_concat_from_c_string("aa", dst);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_destroy(&dst);
        assert(NULL == dst);

        test_choco_string_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_choco_string_length(void) {
    memory_system_create();
    {
        // choco_string_length() 冒頭で強制的に値を返させる
        size_t len = 0U;
        test_call_control_size_t_t config = {0};

        test_choco_string_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = 123U;
        test_choco_string_length_config_set(&config);

        len = choco_string_length(NULL);
        assert(123U == len);

        test_choco_string_config_reset();
    }
    {
        // string_ == NULL -> 0
        size_t len = 999U;

        test_choco_string_config_reset();

        len = choco_string_length(NULL);
        assert(0U == len);

        test_choco_string_config_reset();
    }
    {
        // 空文字列(len == 0) -> 0
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        size_t len = 999U;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(0U == string->len);

        len = choco_string_length(string);
        assert(0U == len);

        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // 非空文字列(len == 3) -> 3
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        size_t len = 0U;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(3U == string->len);

        len = choco_string_length(string);
        assert(3U == len);

        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_choco_string_c_str(void) {
    memory_system_create();
    {
        // string_ == NULL -> ""
        const char* c_ptr = NULL;

        test_choco_string_config_reset();

        c_ptr = choco_string_c_str(NULL);
        assert(NULL != c_ptr);
        assert(0 == strcmp(c_ptr, ""));

        test_choco_string_config_reset();
    }
    {
        // string_ != NULL かつ buffer == NULL -> ""
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        const char* c_ptr = NULL;

        test_choco_string_config_reset();

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(NULL == string->buffer);
        assert(0U == string->len);
        assert(0U == string->capacity);

        c_ptr = choco_string_c_str(string);
        assert(NULL != c_ptr);
        assert(0 == strcmp(c_ptr, ""));

        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    {
        // string_ != NULL かつ buffer != NULL -> buffer内容
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        const char* c_ptr = NULL;

        test_choco_string_config_reset();

        ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(NULL != string->buffer);
        assert(3U == string->len);
        assert(4U == string->capacity);
        assert(0 == strcmp(string->buffer, "aaa"));

        c_ptr = choco_string_c_str(string);
        assert(NULL != c_ptr);
        assert(string->buffer == c_ptr);
        assert(0 == strcmp(c_ptr, "aaa"));

        choco_string_destroy(&string);
        assert(NULL == string);

        test_choco_string_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* s = rslt_to_str(CHOCO_STRING_SUCCESS);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_success));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_DATA_CORRUPTED);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_data_corrupted));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_BAD_OPERATION);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_bad_operation));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_NO_MEMORY);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_no_memory));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_invalid_argument));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_RUNTIME_ERROR);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_runtime_error));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_UNDEFINED_ERROR);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_undefined_error));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_OVERFLOW);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_overflow));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_LIMIT_EXCEEDED);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_limit_exceeded));
    }
    {
        // default 分岐
        const char* s = rslt_to_str((choco_string_result_t)0x7fffffff);
        assert(NULL != s);
        assert(0 == strcmp(s, s_rslt_str_undefined_error));
    }
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_string_malloc(void) {
    memory_system_create();
    {
        // string_malloc() 冒頭で強制的に CHOCO_STRING_NO_MEMORY を返させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        void* p = NULL;
        test_call_control_t config = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        s_test_config_string_malloc.fail_on_call = config.fail_on_call;
        s_test_config_string_malloc.forced_result = config.forced_result;

        ret = string_malloc(16U, &p);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == p);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_ptr_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = string_malloc(16U, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *out_ptr_ != NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        void* p = (void*)0x1;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = string_malloc(16U, &p);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert((void*)0x1 == p);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() -> MEMORY_SYSTEM_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        void* p = NULL;
        test_call_control_t config = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_INVALID_ARGUMENT;
        test_memory_system_allocate_config_set(&config);

        ret = string_malloc(16U, &p);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert(NULL == p);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() -> MEMORY_SYSTEM_NO_MEMORY
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        void* p = NULL;
        test_call_control_t config = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = string_malloc(16U, &p);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == p);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() -> MEMORY_SYSTEM_LIMIT_EXCEEDED
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        void* p = NULL;
        test_call_control_t config = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_LIMIT_EXCEEDED;
        test_memory_system_allocate_config_set(&config);

        ret = string_malloc(16U, &p);
        assert(CHOCO_STRING_LIMIT_EXCEEDED == ret);
        assert(NULL == p);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() -> MEMORY_SYSTEM_RUNTIME_ERROR
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        void* p = NULL;
        test_call_control_t config = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_RUNTIME_ERROR;
        test_memory_system_allocate_config_set(&config);

        ret = string_malloc(16U, &p);
        assert(CHOCO_STRING_RUNTIME_ERROR == ret);
        assert(NULL == p);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() -> 未定義値
        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        void* p = NULL;
        test_call_control_t config = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)0x7fffffff;
        test_memory_system_allocate_config_set(&config);

        ret = string_malloc(16U, &p);
        assert(CHOCO_STRING_UNDEFINED_ERROR == ret);
        assert(NULL == p);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        void* p = NULL;
        const size_t size = 32U;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = string_malloc(size, &p);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != p);

        memory_system_free(p, size, MEMORY_TAG_STRING);
        p = NULL;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_buffer_reserve(void) {
    memory_system_create();
    {
        // buffer_reserve() 冒頭で強制的に CHOCO_STRING_RUNTIME_ERROR を返させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_buffer_reserve.fail_on_call = 1U;
        s_test_config_buffer_reserve.forced_result = (int)CHOCO_STRING_RUNTIME_ERROR;

        ret = buffer_reserve(16U, &string);
        assert(CHOCO_STRING_RUNTIME_ERROR == ret);
        assert(0U == string.len);
        assert(0U == string.capacity);
        assert(NULL == string.buffer);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // size_ == 0 -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = buffer_reserve(0U, &string);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert(0U == string.len);
        assert(0U == string.capacity);
        assert(NULL == string.buffer);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = buffer_reserve(8U, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // string_->capacity != 0 -> CHOCO_STRING_BAD_OPERATION
        // この経路では is_string_valid() には到達しない
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        char dummy_buf[2] = {'\0', '\0'};
        choco_string_t string = {0};

        string.len = 0U;
        string.capacity = 2U;
        string.buffer = dummy_buf;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = buffer_reserve(8U, &string);
        assert(CHOCO_STRING_BAD_OPERATION == ret);
        assert(0U == string.len);
        assert(2U == string.capacity);
        assert(dummy_buf == string.buffer);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // is_string_valid(string_) == false -> CHOCO_STRING_DATA_CORRUPTED
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        string.len = 0U;
        string.capacity = 0U;
        string.buffer = (char*)0x1;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = buffer_reserve(8U, &string);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);
        assert(0U == string.len);
        assert(0U == string.capacity);
        assert((char*)0x1 == string.buffer);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // string_malloc() 失敗 -> CHOCO_STRING_NO_MEMORY
        // 失敗時は状態不変
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = buffer_reserve(8U, &string);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(0U == string.len);
        assert(0U == string.capacity);
        assert(NULL == string.buffer);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: SUCCESS
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = buffer_reserve(8U, &string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == string.len);
        assert(8U == string.capacity);
        assert(NULL != string.buffer);

        for(size_t i = 0; i < string.capacity; ++i) {
            assert(0 == (unsigned char)string.buffer[i]);
        }

        memory_system_free(string.buffer, string.capacity, MEMORY_TAG_STRING);
        string.buffer = NULL;
        string.capacity = 0U;
        string.len = 0U;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_buffer_resize(void) {
    memory_system_create();
    {
        // buffer_resize() 冒頭で強制的に CHOCO_STRING_RUNTIME_ERROR を返させる
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_buffer_resize.fail_on_call = 1U;
        s_test_config_buffer_resize.forced_result = (int)CHOCO_STRING_RUNTIME_ERROR;

        ret = buffer_resize(16U, &string);
        assert(CHOCO_STRING_RUNTIME_ERROR == ret);
        assert(0U == string.len);
        assert(0U == string.capacity);
        assert(NULL == string.buffer);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = buffer_resize(8U, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // size_ == 0 -> CHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = buffer_resize(0U, &string);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert(0U == string.len);
        assert(0U == string.capacity);
        assert(NULL == string.buffer);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // is_string_valid(string_) == false -> CHOCO_STRING_DATA_CORRUPTED
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        string.len = 0U;
        string.capacity = 0U;
        string.buffer = (char*)0x1;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = buffer_resize(8U, &string);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);
        assert(0U == string.len);
        assert(0U == string.capacity);
        assert((char*)0x1 == string.buffer);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // string_malloc() 失敗 -> CHOCO_STRING_NO_MEMORY
        // 失敗時は状態不変
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_string_malloc.fail_on_call = 1U;
        s_test_config_string_malloc.forced_result = (int)CHOCO_STRING_NO_MEMORY;

        ret = buffer_resize(8U, &string);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(0U == string.len);
        assert(0U == string.capacity);
        assert(NULL == string.buffer);

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 元バッファなし（string_->capacity == 0）
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = buffer_resize(8U, &string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == string.len);
        assert(8U == string.capacity);
        assert(NULL != string.buffer);

        for(size_t i = 0; i < string.capacity; ++i) {
            assert(0 == (unsigned char)string.buffer[i]);
        }

        memory_system_free(string.buffer, string.capacity, MEMORY_TAG_STRING);
        string.buffer = NULL;
        string.capacity = 0U;
        string.len = 0U;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 元バッファあり（string_->capacity != 0）
        // free 分岐を通る
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t string = {0};
        void* old_buf_void = NULL;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        char* old_buf = NULL;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret_mem = memory_system_allocate(4U, MEMORY_TAG_STRING, &old_buf_void);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != old_buf_void);

        old_buf = (char*)old_buf_void;
        string.buffer = old_buf;
        string.capacity = 4U;
        string.len = 0U;

        ret = buffer_resize(8U, &string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0U == string.len);
        assert(8U == string.capacity);
        assert(NULL != string.buffer);
        assert(old_buf != string.buffer);

        for(size_t i = 0; i < string.capacity; ++i) {
            assert(0 == (unsigned char)string.buffer[i]);
        }

        memory_system_free(string.buffer, string.capacity, MEMORY_TAG_STRING);
        string.buffer = NULL;
        string.capacity = 0U;
        string.len = 0U;

        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT 5.4 Thinking
static void NO_COVERAGE test_is_string_valid(void) {
    {
        // is_string_valid() 冒頭で強制的に true を返させる
        choco_string_t string = {0};
        bool ret = false;

        test_choco_string_config_reset();

        s_test_config_is_string_valid.fail_on_call = 1U;
        s_test_config_is_string_valid.forced_result = true;

        ret = is_string_valid(&string);
        assert(true == ret);

        test_choco_string_config_reset();
    }
    {
        // is_string_valid() 冒頭で強制的に false を返させる
        choco_string_t string = {0};
        bool ret = true;

        test_choco_string_config_reset();

        s_test_config_is_string_valid.fail_on_call = 1U;
        s_test_config_is_string_valid.forced_result = false;

        ret = is_string_valid(&string);
        assert(false == ret);

        test_choco_string_config_reset();
    }
    {
        // (SIZE_MAX - 1) < string_->len -> false
        choco_string_t string = {0};
        bool ret = true;

        string.len = SIZE_MAX;
        string.capacity = 0U;
        string.buffer = NULL;

        test_choco_string_config_reset();

        ret = is_string_valid(&string);
        assert(false == ret);

        test_choco_string_config_reset();
    }
    {
        // string_->capacity < (string_->len + 1) && 0 != string_->len -> false
        choco_string_t string = {0};
        bool ret = true;

        string.len = 5U;
        string.capacity = 3U;
        string.buffer = NULL;

        test_choco_string_config_reset();

        ret = is_string_valid(&string);
        assert(false == ret);

        test_choco_string_config_reset();
    }
    {
        // 0 == string_->capacity && NULL != string_->buffer -> false
        choco_string_t string = {0};
        bool ret = true;
        char dummy = '\0';

        string.len = 0U;
        string.capacity = 0U;
        string.buffer = &dummy;

        test_choco_string_config_reset();

        ret = is_string_valid(&string);
        assert(false == ret);

        test_choco_string_config_reset();
    }
    {
        // 0 != string_->capacity && NULL == string_->buffer -> false
        choco_string_t string = {0};
        bool ret = true;

        string.len = 0U;
        string.capacity = 1U;
        string.buffer = NULL;

        test_choco_string_config_reset();

        ret = is_string_valid(&string);
        assert(false == ret);

        test_choco_string_config_reset();
    }
    {
        // 0 != string_->len && '\0' != string_->buffer[string_->len] -> false
        choco_string_t string = {0};
        bool ret = true;
        char buf[4] = {'a', 'b', 'c', 'X'};

        string.len = 3U;
        string.capacity = 4U;
        string.buffer = buf;

        test_choco_string_config_reset();

        ret = is_string_valid(&string);
        assert(false == ret);

        test_choco_string_config_reset();
    }
    {
        // 0 == string_->len && 0 < string_->capacity && '\0' != string_->buffer[0] -> false
        choco_string_t string = {0};
        bool ret = true;
        char buf[1] = {'X'};

        string.len = 0U;
        string.capacity = 1U;
        string.buffer = buf;

        test_choco_string_config_reset();

        ret = is_string_valid(&string);
        assert(false == ret);

        test_choco_string_config_reset();
    }
    {
        // 正常系: len > 0 の妥当な文字列 -> true
        choco_string_t string = {0};
        bool ret = false;
        char buf[4] = {'a', 'b', 'c', '\0'};

        string.len = 3U;
        string.capacity = 4U;
        string.buffer = buf;

        test_choco_string_config_reset();

        ret = is_string_valid(&string);
        assert(true == ret);

        test_choco_string_config_reset();
    }
    {
        // 正常系: len == 0, capacity > 0, buffer[0] == '\0' -> true
        choco_string_t string = {0};
        bool ret = false;
        char buf[1] = {'\0'};

        string.len = 0U;
        string.capacity = 1U;
        string.buffer = buf;

        test_choco_string_config_reset();

        ret = is_string_valid(&string);
        assert(true == ret);

        test_choco_string_config_reset();
    }
    {
        // 正常系: len == 0, capacity == 0, buffer == NULL -> true
        choco_string_t string = {0};
        bool ret = false;

        string.len = 0U;
        string.capacity = 0U;
        string.buffer = NULL;

        test_choco_string_config_reset();

        ret = is_string_valid(&string);
        assert(true == ret);

        test_choco_string_config_reset();
    }
}
#endif
