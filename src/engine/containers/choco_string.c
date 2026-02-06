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
#include <assert.h>
typedef struct choco_string_test {
    bool reserve_test_enable;
    choco_string_result_t reserve_test_result;

    bool resize_test_enable;
    choco_string_result_t resize_test_result;

    bool is_string_valid_test_enable;
    bool string_valid_result;

    bool mock_strlen_test_enable;
    size_t mock_strlen_result;
} choco_string_test_t;

static choco_string_test_t s_choco_string_test;

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
static void test_param_reset(void);

choco_string_result_t choco_string_default_create(choco_string_t** string_) {
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

    src_len = strlen(string_);
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

// memory_system_allocateのラッパで,実行結果コードをメモリシステムから文字列モジュールのコードへ変換して出力する
static choco_string_result_t string_malloc(size_t size_, void** out_ptr_) {
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
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    char* tmp_buffer = NULL;
#ifdef TEST_BUILD
    if(s_choco_string_test.reserve_test_enable) {
        return s_choco_string_test.reserve_test_result;
    }
#endif

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
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    char* tmp_buffer = NULL;
#ifdef TEST_BUILD
    if(s_choco_string_test.resize_test_enable) {
        return s_choco_string_test.resize_test_result;
    }
#endif

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
    if(s_choco_string_test.is_string_valid_test_enable) {
        return s_choco_string_test.string_valid_result;
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
    if(s_choco_string_test.mock_strlen_test_enable) {
        return s_choco_string_test.mock_strlen_result;
    } else {
        return strlen(str_);
    }
#else
    return strlen(str_);
#endif
}

#ifdef TEST_BUILD
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

static void NO_COVERAGE test_choco_string_default_create(void) {
    {
        // 正常系: SUCCESS (Commitまで到達し、構造体が0初期化されていること)
        memory_system_test_param_reset();

        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);

        // memset(tmp_string, 0, ...) の効果確認
        assert(0 == string->len);
        assert(0 == string->capacity);
        assert(NULL == string->buffer);

        choco_string_destroy(&string);
        assert(NULL == string);

        memory_system_report();
    }
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();

        choco_string_result_t ret = CHOCO_STRING_SUCCESS; // 初期値は何でもよいが、明示しておく
        ret = choco_string_default_create(NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        memory_system_report();
    }
    {
        // *string_ != NULL -> CHOCO_STRING_INVALID_ARGUMENT
        // （この経路では *string_ をデリファレンスしない想定のため、ダミーポインタでも安全）
        memory_system_test_param_reset();

        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = (choco_string_t*)0x1; // sentinel（非NULL）

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        // 不変確認（仕様固定：既存ポインタは変更しない）
        assert((choco_string_t*)0x1 == string);

        memory_system_report();
    }
    {
        // tmp_stringメモリ確保失敗 -> CHOCO_STRING_NO_MEMORY
        // かつ *string_ は不変（NULLのまま）
        memory_system_test_param_reset();

        choco_string_result_t ret = CHOCO_STRING_SUCCESS;
        choco_string_t* string = NULL;

        memory_system_test_param_set(0); // 次の(=1回目の)メモリ確保を失敗させる

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        memory_system_test_param_reset();

        memory_system_report();
    }
}

static void NO_COVERAGE test_choco_string_create_from_c_string(void) {
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        const choco_string_result_t ret = choco_string_create_from_c_string(NULL, "aaa");
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
    }
    {
        // *string_ != NULL -> CHOCO_STRING_INVALID_ARGUMENT
        // （この経路では *string_ の内容は参照しない想定なので sentinel でよい）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = (choco_string_t*)0x1;
        const choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        // 不変確認（仕様固定：既存ポインタは変更しない）
        assert((choco_string_t*)0x1 == string);
    }
    {
        // src_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        const choco_string_result_t ret = choco_string_create_from_c_string(&string, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert(NULL == string);
    }
    {
        // src_ == "" -> SUCCESS（buffer_reserve は呼ばれない）
        // 注入ONでも成功することを固定化
        test_param_reset();
        memory_system_test_param_reset();

        s_choco_string_test.reserve_test_enable = true;
        s_choco_string_test.reserve_test_result = CHOCO_STRING_NO_MEMORY;

        choco_string_t* string = NULL;
        const choco_string_result_t ret = choco_string_create_from_c_string(&string, "");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(NULL == string->buffer);
        assert(0 == string->len);
        assert(0 == string->capacity);

        choco_string_destroy(&string);
        assert(NULL == string);

        test_param_reset();
    }
    {
        // tmp_string 確保失敗 -> CHOCO_STRING_NO_MEMORY
        test_param_reset();
        memory_system_test_param_reset();

        memory_system_test_param_set(0); // 1回目のメモリ確保で失敗(tmp_string)
        choco_string_t* string = NULL;
        const choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        memory_system_test_param_reset();
    }
    {
        // src_len overflow -> CHOCO_STRING_OVERFLOW
        // mock_strlen により安全に分岐へ到達（memcpy に到達しないので読み越しなし）
        test_param_reset();
        memory_system_test_param_reset();

        s_choco_string_test.mock_strlen_test_enable = true;
        s_choco_string_test.mock_strlen_result = SIZE_MAX; // (SIZE_MAX - 1) < src_len を成立させる

        choco_string_t* string = NULL;
        const choco_string_result_t ret = choco_string_create_from_c_string(&string, "a");
        assert(CHOCO_STRING_OVERFLOW == ret);
        assert(NULL == string);

        test_param_reset();
    }
    {
        // buffer_reserve 注入失敗 -> 注入結果が返る & クリーンアップされる(*string_==NULL)
        test_param_reset();
        memory_system_test_param_reset();

        s_choco_string_test.reserve_test_enable = true;
        s_choco_string_test.reserve_test_result = CHOCO_STRING_LIMIT_EXCEEDED;

        choco_string_t* string = NULL;
        const choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_LIMIT_EXCEEDED == ret);
        assert(NULL == string);

        test_param_reset();
    }
    {
        // buffer_reserve 内の malloc 失敗 -> CHOCO_STRING_NO_MEMORY
        // 1回目: tmp_string, 2回目: buffer_reserve の buffer 確保
        test_param_reset();
        memory_system_test_param_reset();

        memory_system_test_param_set(1); // 2回目のメモリ確保で失敗(buffer_reserve内)
        choco_string_t* string = NULL;
        const choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        memory_system_test_param_reset();
    }
    {
        // 正常系（非空）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        const choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(3 == string->len);
        assert(4 == string->capacity);
        assert(NULL != string->buffer);
        assert(0 == strcmp(string->buffer, "aaa"));

        choco_string_destroy(&string);
        assert(NULL == string);
    }
}

static void NO_COVERAGE test_choco_string_destroy(void) {
    {
        // string_ == NULL -> 先頭の goto cleanup を通る
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_destroy(NULL);
    }
    {
        // *string_ == NULL -> 2つ目の goto cleanup を通る
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        choco_string_destroy(&string);
        assert(NULL == string);
    }
    {
        // *string_ != NULL かつ buffer == NULL -> buffer解放分岐は通らず、構造体解放のみ通る
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        choco_string_result_t ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(0 == string->len);
        assert(0 == string->capacity);
        assert(NULL == string->buffer);

        choco_string_destroy(&string);
        assert(NULL == string);

        // 追加：2重destroyで *string_==NULL 分岐も再確認（カバレッジ的には不要だが仕様固定に有効）
        choco_string_destroy(&string);
        assert(NULL == string);
    }
    {
        // *string_ != NULL かつ buffer != NULL -> buffer解放分岐と構造体解放を通る
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&string, "abc");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(3 == string->len);
        assert(4 == string->capacity);
        assert(NULL != string->buffer);
        assert(0 == strcmp(string->buffer, "abc"));

        choco_string_destroy(&string);
        assert(NULL == string);
    }
}

static void NO_COVERAGE test_choco_string_copy(void) {
    {
        // dst_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&src, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy(NULL, src);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&src);
        assert(NULL == src);
    }
    {
        // src_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy(dst, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // 壊れた dst_ -> CHOCO_STRING_DATA_CORRUPTED（bufferは参照しない壊し方）
        test_param_reset();
        memory_system_test_param_reset();

        struct choco_string corrupted_dst;
        memset(&corrupted_dst, 0, sizeof(corrupted_dst));
        corrupted_dst.len = 0;
        corrupted_dst.capacity = 0;
        corrupted_dst.buffer = (char*)0x1; // capacity==0なのにbuffer!=NULL

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&src, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy((choco_string_t*)&corrupted_dst, src);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        choco_string_destroy(&src);
        corrupted_dst.buffer = NULL; // 念のため
    }
    {
        // 壊れた src_ -> CHOCO_STRING_DATA_CORRUPTED（bufferは参照しない壊し方）
        test_param_reset();
        memory_system_test_param_reset();

        struct choco_string corrupted_src;
        memset(&corrupted_src, 0, sizeof(corrupted_src));
        corrupted_src.len = 0;
        corrupted_src.capacity = 0;
        corrupted_src.buffer = (char*)0x1; // capacity==0なのにbuffer!=NULL

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy(dst, (const choco_string_t*)&corrupted_src);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        choco_string_destroy(&dst);
        corrupted_src.buffer = NULL; // 念のため
    }
    {
        // src_->len == 0 -> dst_->buffer[0]='\0' が実行される（dst_->buffer != NULL の分岐）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_default_create(&src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == src->len);

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != dst->buffer);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == dst->len);
        assert(0 == strcmp(dst->buffer, ""));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
    {
        // src_->len == 0 -> dst_->buffer が NULL のため buffer[0] には触れない（dst_->buffer == NULL の分岐）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_default_create(&src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == src->len);

        choco_string_t* dst = NULL;
        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL == dst->buffer);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == dst->len);
        assert(0 == dst->capacity);
        assert(NULL == dst->buffer);

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
    {
        // OVERFLOW 分岐を通す（通常は is_string_valid で弾かれるため、注入で is_string_valid を true に固定）
        test_param_reset();
        memory_system_test_param_reset();

        s_choco_string_test.is_string_valid_test_enable = true;
        s_choco_string_test.string_valid_result = true;

        struct choco_string fake_dst;
        memset(&fake_dst, 0, sizeof(fake_dst));

        struct choco_string fake_src;
        memset(&fake_src, 0, sizeof(fake_src));
        fake_src.len = SIZE_MAX; // (SIZE_MAX - 1) < len を成立させる

        choco_string_result_t ret = choco_string_copy((choco_string_t*)&fake_dst, (const choco_string_t*)&fake_src);
        assert(CHOCO_STRING_OVERFLOW == ret);

        test_param_reset();
    }
    {
        // dst_->capacity >= (src_->len + 1) -> resizeなしでコピー成功
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&src, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_c_string(&dst, "bbbbb"); // cap=6
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(6 == dst->capacity);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaa"));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
    {
        // dst_->capacity < (src_->len + 1) -> buffer_resize 経由でコピー成功
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&src, "aaaaa"); // len=5 cap=6
        assert(CHOCO_STRING_SUCCESS == ret);

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_c_string(&dst, "bbb"); // len=3 cap=4
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(4 == dst->capacity);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaaaa"));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
    {
        // resize 注入失敗 -> 注入結果が返る & dst不変（resize分岐を確実に通す）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&src, "aaaaa"); // len=5 cap=6
        assert(CHOCO_STRING_SUCCESS == ret);

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_c_string(&dst, "bbb"); // len=3 cap=4
        assert(CHOCO_STRING_SUCCESS == ret);

        char*  old_ptr = dst->buffer;
        size_t old_len = dst->len;
        size_t old_cap = dst->capacity;

        s_choco_string_test.resize_test_enable = true;
        s_choco_string_test.resize_test_result = CHOCO_STRING_NO_MEMORY;

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_NO_MEMORY == ret);

        // 不変確認
        assert(old_ptr == dst->buffer);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        test_param_reset();
        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
    {
        // malloc失敗(buffer_resize内の確保失敗) -> NO_MEMORY & dst不変（resize分岐を確実に通す）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&src, "aaaaa"); // len=5 cap=6
        assert(CHOCO_STRING_SUCCESS == ret);

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_c_string(&dst, "bbb"); // len=3 cap=4
        assert(CHOCO_STRING_SUCCESS == ret);

        char*  old_ptr = dst->buffer;
        size_t old_len = dst->len;
        size_t old_cap = dst->capacity;

        memory_system_test_param_set(0); // 次の(=buffer_resize内の)メモリ確保を失敗させる
        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_NO_MEMORY == ret);

        // 不変確認
        assert(old_ptr == dst->buffer);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_reset();
        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
}

static void NO_COVERAGE test_choco_string_copy_from_c_string(void) {
    {
        // dst_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_result_t ret = choco_string_copy_from_c_string(NULL, "aaa");
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
    }
    {
        // src_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy_from_c_string(dst, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // 壊れた dst_ -> CHOCO_STRING_DATA_CORRUPTED（bufferは参照しない壊し方）
        test_param_reset();
        memory_system_test_param_reset();

        struct choco_string corrupted_dst;
        memset(&corrupted_dst, 0, sizeof(corrupted_dst));
        corrupted_dst.len = 0;
        corrupted_dst.capacity = 0;
        corrupted_dst.buffer = (char*)0x1; // capacity==0なのにbuffer!=NULL

        choco_string_result_t ret = choco_string_copy_from_c_string((choco_string_t*)&corrupted_dst, "aaa");
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        corrupted_dst.buffer = NULL; // 念のため
    }
    {
        // src_len == 0 かつ dst_->buffer != NULL -> buffer[0] を '\0' にする分岐
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != dst->buffer);
        assert(3 == dst->len);

        ret = choco_string_copy_from_c_string(dst, "");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == dst->len);
        assert(0 == strcmp(dst->buffer, ""));

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // src_len == 0 かつ dst_->buffer == NULL -> buffer[0] には触れない分岐
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL == dst->buffer);
        assert(0 == dst->len);
        assert(0 == dst->capacity);

        ret = choco_string_copy_from_c_string(dst, "");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == dst->len);
        assert(0 == dst->capacity);
        assert(NULL == dst->buffer);

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // OVERFLOW 分岐（mock_strlen を使って強制的に SIZE_MAX を返す）
        // dst は valid である必要がある（default_create の 0初期化状態なら valid）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        s_choco_string_test.mock_strlen_test_enable = true;
        s_choco_string_test.mock_strlen_result = SIZE_MAX;

        ret = choco_string_copy_from_c_string(dst, "a"); // mock_strlen が優先されるので入力は何でもよい
        assert(CHOCO_STRING_OVERFLOW == ret);

        test_param_reset();
        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // dst_->capacity >= (src_len + 1) -> resizeなしでSUCCESS
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbbbb"); // len=5 cap=6
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(6 == dst->capacity);

        ret = choco_string_copy_from_c_string(dst, "aaa"); // len=3
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // dst_->capacity < (src_len + 1) -> buffer_resize経由でSUCCESS
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb"); // len=3 cap=4
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(4 == dst->capacity);

        ret = choco_string_copy_from_c_string(dst, "aaaaa"); // len=5 -> 要resize
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaaaa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // dst_->capacity == 0 かつ src non-empty -> buffer_resize経由でSUCCESS
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_default_create(&dst); // cap=0 buffer=NULL
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_copy_from_c_string(dst, "aaaaa"); // len=5 -> 要resize
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaaaa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // resize 注入失敗 -> 注入結果が返る & dst不変（resize分岐を確実に通す）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb"); // len=3 cap=4
        assert(CHOCO_STRING_SUCCESS == ret);

        char*  old_ptr = dst->buffer;
        size_t old_len = dst->len;
        size_t old_cap = dst->capacity;

        s_choco_string_test.resize_test_enable = true;
        s_choco_string_test.resize_test_result = CHOCO_STRING_NO_MEMORY;

        ret = choco_string_copy_from_c_string(dst, "aaaaa"); // len=5 -> resize経由
        assert(CHOCO_STRING_NO_MEMORY == ret);

        // 不変確認
        assert(old_ptr == dst->buffer);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        test_param_reset();
        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // malloc失敗(buffer_resize内の確保失敗) -> NO_MEMORY & dst不変（resize分岐を確実に通す）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb"); // len=3 cap=4
        assert(CHOCO_STRING_SUCCESS == ret);

        char*  old_ptr = dst->buffer;
        size_t old_len = dst->len;
        size_t old_cap = dst->capacity;

        memory_system_test_param_set(0); // 次の(=buffer_resize内の)確保を失敗させる
        ret = choco_string_copy_from_c_string(dst, "aaaaa"); // len=5 -> resize経由
        assert(CHOCO_STRING_NO_MEMORY == ret);

        // 不変確認
        assert(old_ptr == dst->buffer);
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_reset();
        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
}

static void NO_COVERAGE test_choco_string_concat(void) {
    {
        // dst_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&src, "a");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(src, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&src);
        assert(NULL == src);
    }
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(NULL, dst);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // dst_ == string_ -> CHOCO_STRING_BAD_OPERATION (自己連結禁止)
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* s = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&s, "a");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(s, s);
        assert(CHOCO_STRING_BAD_OPERATION == ret);

        choco_string_destroy(&s);
        assert(NULL == s);
    }
    {
        // 壊れた string_ -> CHOCO_STRING_DATA_CORRUPTED (is_string_valid(string_) false)
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        // dst不変確認用
        const size_t old_len = dst->len;
        const size_t old_cap = dst->capacity;
        char* const old_ptr = dst->buffer;

        struct choco_string corrupted_src;
        memset(&corrupted_src, 0, sizeof(corrupted_src));
        corrupted_src.len = 0;
        corrupted_src.capacity = 0;
        corrupted_src.buffer = (char*)0x1; // capacity==0なのにbuffer!=NULL

        ret = choco_string_concat((const choco_string_t*)&corrupted_src, dst);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        // dst不変
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        corrupted_src.buffer = NULL;
        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // 壊れた dst_ -> CHOCO_STRING_DATA_CORRUPTED (is_string_valid(dst_) false)
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&src, "a");
        assert(CHOCO_STRING_SUCCESS == ret);

        struct choco_string corrupted_dst;
        memset(&corrupted_dst, 0, sizeof(corrupted_dst));
        corrupted_dst.len = 0;
        corrupted_dst.capacity = 0;
        corrupted_dst.buffer = (char*)0x1; // capacity==0なのにbuffer!=NULL

        ret = choco_string_concat(src, (choco_string_t*)&corrupted_dst);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        corrupted_dst.buffer = NULL;
        choco_string_destroy(&src);
        assert(NULL == src);
    }
    {
        // overflow -> CHOCO_STRING_OVERFLOW
        // ※実サイズでの再現は困難なので is_string_valid をテストフックで true 固定にして到達させる
        test_param_reset();
        memory_system_test_param_reset();

        s_choco_string_test.is_string_valid_test_enable = true;
        s_choco_string_test.string_valid_result = true;

        struct choco_string fake_dst;
        struct choco_string fake_src;
        memset(&fake_dst, 0, sizeof(fake_dst));
        memset(&fake_src, 0, sizeof(fake_src));

        fake_dst.len = SIZE_MAX - 2;
        fake_dst.capacity = 1;
        fake_dst.buffer = (char*)0x1;

        fake_src.len = 2;
        fake_src.capacity = 1;
        fake_src.buffer = (char*)0x1;

        const choco_string_result_t ret = choco_string_concat((const choco_string_t*)&fake_src, (choco_string_t*)&fake_dst);
        assert(CHOCO_STRING_OVERFLOW == ret);

        test_param_reset(); // フック解除
    }
    {
        // string_->len == 0 -> SUCCESS (no-op、メモリ確保しない)
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* src = NULL;
        choco_string_result_t ret = choco_string_default_create(&src);
        assert(CHOCO_STRING_SUCCESS == ret);

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        // ここで次のメモリ確保を失敗させても、concatは確保しないので成功することを固定化
        memory_system_test_param_set(0);

        const size_t old_len = dst->len;
        const size_t old_cap = dst->capacity;
        char* const old_ptr = dst->buffer;

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        // dst不変
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_reset();
        choco_string_destroy(&dst);
        choco_string_destroy(&src);
        assert(NULL == dst);
        assert(NULL == src);
    }
    {
        // (dst_len_new + 1) <= dst_->capacity -> in-place concat (メモリ確保なし)
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = buffer_resize(16, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(16 == dst->capacity);
        assert(0 == dst->len);

        ret = choco_string_copy_from_c_string(dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(16 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        choco_string_t* src = NULL;
        ret = choco_string_create_from_c_string(&src, "aa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(2 == src->len);

        // in-placeなので、次のメモリ確保失敗をセットしても成功する
        memory_system_test_param_set(0);

        const size_t old_cap = dst->capacity;
        char* const old_ptr = dst->buffer;

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbbaa"));

        memory_system_test_param_reset();
        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);
    }
    {
        // capacity不足 -> 再確保経由でSUCCESS (dst_->len != 0 かつ dst_->capacity != 0 の枝を踏む)
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb"); // len=3 cap=4
        assert(CHOCO_STRING_SUCCESS == ret);

        choco_string_t* src = NULL;
        ret = choco_string_create_from_c_string(&src, "aa"); // len=2
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbbaa"));

        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);
    }
    {
        // capacity不足 -> 再確保経由でSUCCESS (dst_->len == 0 かつ dst_->capacity == 0 の枝を踏む)
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_default_create(&dst); // len=0 cap=0
        assert(CHOCO_STRING_SUCCESS == ret);

        choco_string_t* src = NULL;
        ret = choco_string_create_from_c_string(&src, "aa"); // len=2
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(2 == dst->len);
        assert(3 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aa"));

        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);
    }
    {
        // 再確保側で string_malloc 失敗 -> CHOCO_STRING_NO_MEMORY、dst不変
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb"); // cap=4
        assert(CHOCO_STRING_SUCCESS == ret);

        choco_string_t* src = NULL;
        ret = choco_string_create_from_c_string(&src, "aa"); // 要再確保
        assert(CHOCO_STRING_SUCCESS == ret);

        // dst不変確認用
        const size_t old_len = dst->len;
        const size_t old_cap = dst->capacity;
        char* const old_ptr = dst->buffer;

        // 次のメモリ確保(string_malloc)を失敗させる
        memory_system_test_param_set(0);

        ret = choco_string_concat(src, dst);
        assert(CHOCO_STRING_NO_MEMORY == ret);

        // 不変確認
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_reset();
        choco_string_destroy(&src);
        choco_string_destroy(&dst);
        assert(NULL == src);
        assert(NULL == dst);
    }
}

static void NO_COVERAGE test_choco_string_concat_from_c_string(void) {
    {
        // dst_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        const choco_string_result_t ret = choco_string_concat_from_c_string("a", NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
    }
    {
        // string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat_from_c_string(NULL, dst);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // 壊れた dst_ -> CHOCO_STRING_DATA_CORRUPTED（bufferは参照しない壊し方）
        test_param_reset();
        memory_system_test_param_reset();

        struct choco_string corrupted_dst;
        memset(&corrupted_dst, 0, sizeof(corrupted_dst));
        corrupted_dst.len = 0;
        corrupted_dst.capacity = 0;
        corrupted_dst.buffer = (char*)0x1; // capacity==0なのにbuffer!=NULL

        const choco_string_result_t ret = choco_string_concat_from_c_string("a", (choco_string_t*)&corrupted_dst);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        corrupted_dst.buffer = NULL; // 念のため
    }
    {
        // overflow -> CHOCO_STRING_OVERFLOW
        // ※実サイズでの再現が困難なので is_string_valid をテストフックで true 固定にして到達させる
        test_param_reset();
        memory_system_test_param_reset();

        s_choco_string_test.is_string_valid_test_enable = true;
        s_choco_string_test.string_valid_result = true;

        struct choco_string fake_dst;
        memset(&fake_dst, 0, sizeof(fake_dst));
        fake_dst.len = SIZE_MAX - 2;   // SIZE_MAX - len - 1 == 1
        fake_dst.capacity = 1;
        fake_dst.buffer = (char*)0x1;

        const choco_string_result_t ret = choco_string_concat_from_c_string("aa", (choco_string_t*)&fake_dst); // src_len=2
        assert(CHOCO_STRING_OVERFLOW == ret);

        test_param_reset(); // フック解除
    }
    {
        // src_len == 0 -> SUCCESS（no-op、メモリ確保しない）
        // 次のメモリ確保を失敗させても、concat_from_c_stringは確保しないので成功することを固定化
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);

        const size_t old_len = dst->len;
        const size_t old_cap = dst->capacity;
        char* const old_ptr = dst->buffer;

        memory_system_test_param_set(0);

        ret = choco_string_concat_from_c_string("", dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        // dst不変
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_reset();
        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // (dst_len_new + 1) <= dst_->capacity -> in-place concat（メモリ確保なし）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = buffer_resize(16, dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(16 == dst->capacity);

        ret = choco_string_copy_from_c_string(dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(0 == strcmp(dst->buffer, "bbb"));

        // in-placeなので、次のメモリ確保失敗をセットしても成功する
        memory_system_test_param_set(0);

        const size_t old_cap = dst->capacity;
        char* const old_ptr = dst->buffer;

        ret = choco_string_concat_from_c_string("aa", dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbbaa"));

        memory_system_test_param_reset();
        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // capacity不足 -> 再確保経由でSUCCESS（dst_->len != 0 かつ dst_->capacity != 0 の枝を踏む）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb"); // len=3 cap=4
        assert(CHOCO_STRING_SUCCESS == ret);

        char* const old_ptr = dst->buffer;

        ret = choco_string_concat_from_c_string("aa", dst); // len=2 -> dst_len_new=5 -> 要再確保
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbbaa"));
        assert(old_ptr != dst->buffer); // 再確保でポインタが変わる（旧領域はまだ確保中に新規確保するため同一にならない想定）

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // capacity不足 -> 再確保経由でSUCCESS（dst_->len == 0 かつ dst_->capacity == 0 の枝を踏む）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_default_create(&dst); // len=0 cap=0 buffer=NULL
        assert(CHOCO_STRING_SUCCESS == ret);

        ret = choco_string_concat_from_c_string("aa", dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(2 == dst->len);
        assert(3 == dst->capacity);
        assert(NULL != dst->buffer);
        assert(0 == strcmp(dst->buffer, "aa"));

        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
    {
        // 再確保側で string_malloc 失敗 -> CHOCO_STRING_NO_MEMORY、dst不変
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* dst = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&dst, "bbb"); // cap=4
        assert(CHOCO_STRING_SUCCESS == ret);

        // dst不変確認用
        const size_t old_len = dst->len;
        const size_t old_cap = dst->capacity;
        char* const old_ptr = dst->buffer;

        // 次のメモリ確保(string_malloc)を失敗させる（concat_from_c_string内のtmp_buffer確保）
        memory_system_test_param_set(0);

        ret = choco_string_concat_from_c_string("aa", dst); // 要再確保
        assert(CHOCO_STRING_NO_MEMORY == ret);

        // 不変確認
        assert(old_len == dst->len);
        assert(old_cap == dst->capacity);
        assert(old_ptr == dst->buffer);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_reset();
        choco_string_destroy(&dst);
        assert(NULL == dst);
    }
}

static void NO_COVERAGE test_choco_string_length(void) {
    {
        // string_ == NULL -> 0
        test_param_reset();
        memory_system_test_param_reset();

        const size_t len = choco_string_length(NULL);
        assert(0 == len);
    }
    {
        // 空文字列(len==0) -> 0（非NULL分岐の return string_->len を通す）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        choco_string_result_t ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(0 == string->len);

        const size_t len = choco_string_length(string);
        assert(0 == len);

        choco_string_destroy(&string);
        assert(NULL == string);
    }
    {
        // "aaa"(len==3) -> 3（非NULL分岐の return string_->len を別値でも確認）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(3 == string->len);

        const size_t len = choco_string_length(string);
        assert(3 == len);

        choco_string_destroy(&string);
        assert(NULL == string);
    }
}

static void NO_COVERAGE test_choco_string_c_str(void) {
    {
        // string_ == NULL -> ""
        test_param_reset();
        memory_system_test_param_reset();

        const char* c_ptr = choco_string_c_str(NULL);
        assert(0 == strcmp(c_ptr, ""));
    }
    {
        // string_ != NULL かつ buffer == NULL -> ""
        // （非NULL分岐に入ってから、NULL buffer 分岐を通す）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        choco_string_result_t ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(NULL == string->buffer);
        assert(0 == string->len);
        assert(0 == string->capacity);

        const char* c_ptr = choco_string_c_str(string);
        assert(0 == strcmp(c_ptr, ""));

        choco_string_destroy(&string);
        assert(NULL == string);
    }
    {
        // string_ != NULL かつ buffer != NULL -> buffer内容
        // （非NULL分岐に入ってから、非NULL buffer 分岐を通す）
        test_param_reset();
        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(NULL != string->buffer);
        assert(3 == string->len);
        assert(4 == string->capacity);
        assert(0 == strcmp(string->buffer, "aaa"));

        const char* c_ptr = choco_string_c_str(string);
        assert(0 == strcmp(c_ptr, "aaa"));

        choco_string_destroy(&string);
        assert(NULL == string);
    }
}

static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* s = rslt_to_str(CHOCO_STRING_SUCCESS);
        assert(0 == strcmp(s, s_rslt_str_success));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_DATA_CORRUPTED);
        assert(0 == strcmp(s, s_rslt_str_data_corrupted));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_BAD_OPERATION);
        assert(0 == strcmp(s, s_rslt_str_bad_operation));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_NO_MEMORY);
        assert(0 == strcmp(s, s_rslt_str_no_memory));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT);
        assert(0 == strcmp(s, s_rslt_str_invalid_argument));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_RUNTIME_ERROR);
        assert(0 == strcmp(s, s_rslt_str_runtime_error));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_UNDEFINED_ERROR);
        assert(0 == strcmp(s, s_rslt_str_undefined_error));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_OVERFLOW);
        assert(0 == strcmp(s, s_rslt_str_overflow));
    }
    {
        const char* s = rslt_to_str(CHOCO_STRING_LIMIT_EXCEEDED);
        assert(0 == strcmp(s, s_rslt_str_limit_exceeded));
    }
    {
        // default: 未定義値を与えて default 分岐を通す
        const choco_string_result_t invalid = (choco_string_result_t)0x7fffffff;
        const char* s = rslt_to_str(invalid);
        assert(0 == strcmp(s, s_rslt_str_undefined_error));
    }
}

static void NO_COVERAGE test_string_malloc(void) {
    {
        // out_ptr_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();

        choco_string_result_t ret = string_malloc(16, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        memory_system_test_param_reset();
    }
    {
        // *out_ptr_ != NULL -> CHOCO_STRING_INVALID_ARGUMENT（不変確認）
        memory_system_test_param_reset();

        void* p = (void*)0x1; // sentinel non-NULL
        choco_string_result_t ret = string_malloc(16, &p);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert((void*)0x1 == p);

        memory_system_test_param_reset();
    }
    {
        // memory_system_allocate -> MEMORY_SYSTEM_INVALID_ARGUMENT
        // -> CHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();
        memory_system_err_code_set(MEMORY_SYSTEM_INVALID_ARGUMENT);

        void* p = NULL;
        choco_string_result_t ret = string_malloc(16, &p);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert(NULL == p);

        memory_system_test_param_reset();
    }
    {
        // memory_system_allocate -> MEMORY_SYSTEM_NO_MEMORY
        // -> CHOCO_STRING_NO_MEMORY
        memory_system_test_param_reset();
        memory_system_err_code_set(MEMORY_SYSTEM_NO_MEMORY);

        void* p = NULL;
        choco_string_result_t ret = string_malloc(16, &p);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == p);

        memory_system_test_param_reset();
    }
    {
        // memory_system_allocate -> MEMORY_SYSTEM_LIMIT_EXCEEDED
        // -> CHOCO_STRING_LIMIT_EXCEEDED
        memory_system_test_param_reset();
        memory_system_err_code_set(MEMORY_SYSTEM_LIMIT_EXCEEDED);

        void* p = NULL;
        choco_string_result_t ret = string_malloc(16, &p);
        assert(CHOCO_STRING_LIMIT_EXCEEDED == ret);
        assert(NULL == p);

        memory_system_test_param_reset();
    }
    {
        // memory_system_allocate -> (未定義の戻り値)
        // -> CHOCO_STRING_UNDEFINED_ERROR
        memory_system_test_param_reset();
        memory_system_err_code_set((memory_system_result_t)0x7fffffff);

        void* p = NULL;
        choco_string_result_t ret = string_malloc(16, &p);
        assert(CHOCO_STRING_UNDEFINED_ERROR == ret);
        assert(NULL == p);

        memory_system_test_param_reset();
    }
    {
        // SUCCESS パス（強制を解除して実 allocation）
        memory_system_test_param_reset();

        // 前提：test_choco_string() で memory_system_create() 済みであること
        void* p = NULL;
        const size_t size = 32;

        choco_string_result_t ret = string_malloc(size, &p);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != p);

        memory_system_free(p, size, MEMORY_TAG_STRING);
        p = NULL;

        memory_system_test_param_reset();
    }
}

static void NO_COVERAGE test_buffer_reserve(void) {
    {
        // 分岐: reserve_test_enable == true による早期 return
        memory_system_test_param_reset();
        test_param_reset();

        s_choco_string_test.reserve_test_enable = true;
        s_choco_string_test.reserve_test_result = CHOCO_STRING_RUNTIME_ERROR;

        choco_string_t string = {0};
        choco_string_result_t ret = buffer_reserve(16, &string);

        assert(CHOCO_STRING_RUNTIME_ERROR == ret);
        // 早期 return のため、状態は不変のまま
        assert(0 == string.len);
        assert(0 == string.capacity);
        assert(NULL == string.buffer);

        s_choco_string_test.reserve_test_enable = false;
    }
    {
        // 分岐: size_ == 0 -> CHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();
        test_param_reset();

        choco_string_t string = {0};
        choco_string_result_t ret = buffer_reserve(0, &string);

        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert(0 == string.len);
        assert(0 == string.capacity);
        assert(NULL == string.buffer);
    }
    {
        // 分岐: string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();
        test_param_reset();

        choco_string_result_t ret = buffer_reserve(8, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
    }
    {
        // 分岐: string_->capacity != 0 -> CHOCO_STRING_BAD_OPERATION
        // ※この分岐では is_string_valid() に到達しない
        memory_system_test_param_reset();
        test_param_reset();

        char dummy_buf[2] = {'\0', '\0'};

        choco_string_t string = {0};
        string.len = 0;
        string.capacity = 2;          // capacity != 0 を作る
        string.buffer = dummy_buf;

        choco_string_result_t ret = buffer_reserve(8, &string);
        assert(CHOCO_STRING_BAD_OPERATION == ret);

        // 不変確認（この経路では更新されない）
        assert(0 == string.len);
        assert(2 == string.capacity);
        assert(dummy_buf == string.buffer);
    }
    {
        // 分岐: is_string_valid(string_) == false -> CHOCO_STRING_DATA_CORRUPTED
        // capacity==0 は満たしつつ、buffer!=NULL にして壊れた状態を作る
        memory_system_test_param_reset();
        test_param_reset();

        choco_string_t string = {0};
        string.len = 0;
        string.capacity = 0;
        string.buffer = (char*)0x1;  // non-NULL（is_string_valid はここで false を返す）

        choco_string_result_t ret = buffer_reserve(8, &string);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        // 不変確認
        assert(0 == string.len);
        assert(0 == string.capacity);
        assert((char*)0x1 == string.buffer);

        // 後続で誤って触らないように戻す（テスト上の安全策）
        string.buffer = NULL;
    }
    {
        // 分岐: string_malloc() 失敗 -> ret != SUCCESS 経由
        // memory_system_allocate の返り値を強制して string_malloc を失敗させる
        memory_system_test_param_reset();
        test_param_reset();

        memory_system_err_code_set(MEMORY_SYSTEM_NO_MEMORY);

        choco_string_t string = {0};
        choco_string_result_t ret = buffer_reserve(8, &string);

        assert(CHOCO_STRING_NO_MEMORY == ret);
        // 不変確認（失敗時は状態不変の設計）
        assert(0 == string.len);
        assert(0 == string.capacity);
        assert(NULL == string.buffer);

        // 強制設定を解除
        memory_system_test_param_reset();
    }
    {
        // 正常系: SUCCESS（バッファ確保＆全0初期化確認）
        memory_system_test_param_reset();
        test_param_reset();

        choco_string_t string = {0};
        choco_string_result_t ret = buffer_reserve(8, &string);

        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == string.len);
        assert(8 == string.capacity);
        assert(NULL != string.buffer);

        // memset(tmp_buffer, 0, size_) の確認（全バイト0）
        for(size_t i = 0; i < string.capacity; ++i) {
            assert(0 == (unsigned char)string.buffer[i]);
        }

        // 後始末（リークさせない）
        memory_system_free(string.buffer, string.capacity, MEMORY_TAG_STRING);
        string.buffer = NULL;
        string.capacity = 0;
        string.len = 0;
    }
}

static void NO_COVERAGE test_buffer_resize(void) {
    {
        // 分岐: resize_test_enable == true による早期 return
        memory_system_test_param_reset();
        test_param_reset();

        s_choco_string_test.resize_test_enable = true;
        s_choco_string_test.resize_test_result = CHOCO_STRING_RUNTIME_ERROR;

        choco_string_t string = {0};
        choco_string_result_t ret = buffer_resize(16, &string);

        assert(CHOCO_STRING_RUNTIME_ERROR == ret);
        // 早期 return のため不変
        assert(0 == string.len);
        assert(0 == string.capacity);
        assert(NULL == string.buffer);

        s_choco_string_test.resize_test_enable = false;
    }
    {
        // 分岐: string_ == NULL -> CHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();
        test_param_reset();

        choco_string_result_t ret = buffer_resize(8, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
    }
    {
        // 分岐: size_ == 0 -> CHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();
        test_param_reset();

        choco_string_t string = {0};
        choco_string_result_t ret = buffer_resize(0, &string);

        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        // 不変
        assert(0 == string.len);
        assert(0 == string.capacity);
        assert(NULL == string.buffer);
    }
    {
        // 分岐: is_string_valid(string_) == false -> CHOCO_STRING_DATA_CORRUPTED
        // capacity==0 なのに buffer!=NULL の壊れた状態を作る
        memory_system_test_param_reset();
        test_param_reset();

        choco_string_t string = {0};
        string.len = 0;
        string.capacity = 0;
        string.buffer = (char*)0x1;  // non-NULL（この時点で is_string_valid は false になる）

        choco_string_result_t ret = buffer_resize(8, &string);
        assert(CHOCO_STRING_DATA_CORRUPTED == ret);

        // 不変確認（この関数は失敗時に状態不変の設計）
        assert(0 == string.len);
        assert(0 == string.capacity);
        assert((char*)0x1 == string.buffer);

        // 後続で誤って触らないように戻す（テスト上の安全策）
        string.buffer = NULL;
    }
    {
        // 分岐: string_malloc() 失敗 -> CHOCO_STRING_NO_MEMORY
        // memory_system_allocate の返り値を強制して string_malloc を失敗させる
        memory_system_test_param_reset();
        test_param_reset();

        memory_system_err_code_set(MEMORY_SYSTEM_NO_MEMORY);

        choco_string_t string = {0}; // valid
        choco_string_result_t ret = buffer_resize(8, &string);

        assert(CHOCO_STRING_NO_MEMORY == ret);
        // 不変
        assert(0 == string.len);
        assert(0 == string.capacity);
        assert(NULL == string.buffer);

        // 強制設定解除
        memory_system_test_param_reset();
    }
    {
        // 正常系: 元バッファなし（string_->capacity == 0）-> free 分岐を通らない
        memory_system_test_param_reset();
        test_param_reset();

        choco_string_t string = {0}; // valid
        choco_string_result_t ret = buffer_resize(8, &string);

        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == string.len);
        assert(8 == string.capacity);
        assert(NULL != string.buffer);

        // memset(tmp_buffer, 0, size_) の確認（全バイト0）
        for(size_t i = 0; i < string.capacity; ++i) {
            assert(0 == (unsigned char)string.buffer[i]);
        }

        // 後始末（リーク防止）
        memory_system_free(string.buffer, string.capacity, MEMORY_TAG_STRING);
        string.buffer = NULL;
        string.capacity = 0;
        string.len = 0;
    }
    {
        // 正常系: 元バッファあり（string_->capacity != 0）-> free 分岐を通る
        memory_system_test_param_reset();
        test_param_reset();

        choco_string_t string = {0};

        // 旧バッファを memory_system_allocate で確保して「正しい既存状態」を作る
        void* old_buf_void = NULL;
        memory_system_result_t ret_mem = memory_system_allocate(4, MEMORY_TAG_STRING, &old_buf_void);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != old_buf_void);

        char* old_buf = (char*)old_buf_void;
        // memory_system_allocate は memset(0) 済みなので old_buf[0] == '\0' を満たす（len==0の正当状態）
        string.buffer = old_buf;
        string.capacity = 4;
        string.len = 0;

        choco_string_result_t ret = buffer_resize(8, &string);

        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == string.len);
        assert(8 == string.capacity);
        assert(NULL != string.buffer);
        assert(string.buffer != old_buf); // 新規バッファに切り替わったこと

        // 新バッファが全0初期化されていること
        for(size_t i = 0; i < string.capacity; ++i) {
            assert(0 == (unsigned char)string.buffer[i]);
        }

        // 後始末（新バッファのみ解放。旧バッファは buffer_resize 内で解放済み）
        memory_system_free(string.buffer, string.capacity, MEMORY_TAG_STRING);
        string.buffer = NULL;
        string.capacity = 0;
        string.len = 0;
    }
}

static void NO_COVERAGE test_is_string_valid(void) {
    {
        // 分岐: is_string_valid_test_enable == true の早期 return（true）
        test_param_reset();
        s_choco_string_test.is_string_valid_test_enable = true;
        s_choco_string_test.string_valid_result = true;

        choco_string_t s = {0};
        bool ok = is_string_valid(&s);
        assert(true == ok);

        s_choco_string_test.is_string_valid_test_enable = false;
    }
    {
        // 分岐: is_string_valid_test_enable == true の早期 return（false）
        test_param_reset();
        s_choco_string_test.is_string_valid_test_enable = true;
        s_choco_string_test.string_valid_result = false;

        choco_string_t s = {0};
        bool ok = is_string_valid(&s);
        assert(false == ok);

        s_choco_string_test.is_string_valid_test_enable = false;
    }

    // 以降はフラグ無効で、本体ロジックの全分岐を潰す
    {
        // (1) (SIZE_MAX - 1) < len -> false
        test_param_reset();

        choco_string_t s = {0};
        s.len = SIZE_MAX;      // (SIZE_MAX - 1) < SIZE_MAX が true
        s.capacity = 0;
        s.buffer = NULL;

        bool ok = is_string_valid(&s);
        assert(false == ok);
    }
    {
        // (2) capacity < (len + 1) && len != 0 -> false
        // ※以降の条件に行かずここで return する
        test_param_reset();

        choco_string_t s = {0};
        s.len = 5;
        s.capacity = 3;        // 3 < 6
        s.buffer = NULL;       // 参照されない

        bool ok = is_string_valid(&s);
        assert(false == ok);
    }
    {
        // (3) capacity == 0 && buffer != NULL -> false
        // (2) を回避するため len=0 にする
        test_param_reset();

        char dummy = 0;
        choco_string_t s = {0};
        s.len = 0;
        s.capacity = 0;
        s.buffer = &dummy;     // non-NULL

        bool ok = is_string_valid(&s);
        assert(false == ok);
    }
    {
        // (4) capacity != 0 && buffer == NULL -> false
        test_param_reset();

        choco_string_t s = {0};
        s.len = 0;
        s.capacity = 1;
        s.buffer = NULL;

        bool ok = is_string_valid(&s);
        assert(false == ok);
    }
    {
        // (5) len != 0 && buffer[len] != '\0' -> false
        // ※(2)を回避するため capacity == len+1 を満たす
        test_param_reset();

        char buf[4] = {'a','b','c','X'};  // buf[3] が終端ではない
        choco_string_t s = {0};
        s.len = 3;
        s.capacity = 4;
        s.buffer = buf;

        bool ok = is_string_valid(&s);
        assert(false == ok);
    }
    {
        // (6) len == 0 && capacity > 0 && buffer[0] != '\0' -> false
        test_param_reset();

        char buf[1] = {'X'};
        choco_string_t s = {0};
        s.len = 0;
        s.capacity = 1;
        s.buffer = buf;

        bool ok = is_string_valid(&s);
        assert(false == ok);
    }

    {
        // 正常系A: len>0 の妥当な文字列 -> true
        // (5) の false 側も踏める（buffer[len] == '\0'）
        test_param_reset();

        char buf[4] = {'a','b','c','\0'};
        choco_string_t s = {0};
        s.len = 3;
        s.capacity = 4;
        s.buffer = buf;

        bool ok = is_string_valid(&s);
        assert(true == ok);
    }
    {
        // 正常系B: len==0, capacity>0, buffer[0]=='\0' -> true
        // (6) の false 側（条件式が false）を踏んで最終 return true に到達
        test_param_reset();

        char buf[1] = {'\0'};
        choco_string_t s = {0};
        s.len = 0;
        s.capacity = 1;
        s.buffer = buf;

        bool ok = is_string_valid(&s);
        assert(true == ok);
    }
}

static void test_param_reset(void) {
    s_choco_string_test.reserve_test_enable = false;
    s_choco_string_test.reserve_test_result = CHOCO_STRING_SUCCESS;

    s_choco_string_test.resize_test_enable = false;
    s_choco_string_test.resize_test_result = CHOCO_STRING_SUCCESS;

    s_choco_string_test.is_string_valid_test_enable = false;
    s_choco_string_test.string_valid_result = false;

    s_choco_string_test.mock_strlen_test_enable = false;
    s_choco_string_test.mock_strlen_result = 0;
}
#endif
