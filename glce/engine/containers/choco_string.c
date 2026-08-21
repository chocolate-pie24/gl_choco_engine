/** @ingroup containers
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
static choco_string_result_t choco_string_mem_allocate(size_t size_, void** out_ptr_);
static choco_string_result_t buffer_reserve(size_t size_, choco_string_t* string_);
static choco_string_result_t buffer_resize(size_t size_, choco_string_t* string_);
static size_t mock_strlen(const char* str_);
static int mock_strcmp(const char *s1_, const char *s2_);

static bool is_valid_shallow(const choco_string_t* string_);

choco_string_result_t choco_string_default_create(choco_string_t** string_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* tmp_string = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_default_create", "string_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_default_create", "*string_")

    // Simulation.
    ret = choco_string_mem_allocate(sizeof(*tmp_string), (void**)&tmp_string);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_default_create(%s) - Failed to allocate memory for 'tmp_string'.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_string, 0, sizeof(*tmp_string));

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(tmp_string)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_default_create(%s) - Postcondition validation failed for 'tmp_string'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    *string_ = tmp_string;
    tmp_string = NULL;

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(NULL != tmp_string) {
        memory_system_free(tmp_string, sizeof(*tmp_string), MEMORY_TAG_STRING);
        tmp_string = NULL;
    }
    return ret;
}

choco_string_result_t choco_string_create_from_c_string(const char* src_, choco_string_t** string_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* tmp_string = NULL;
    size_t src_len = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_create_from_c_string", "src_")
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_create_from_c_string", "string_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_create_from_c_string", "*string_")

    // Simulation.
    ret = choco_string_default_create(&tmp_string);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_create_from_c_string(%s) - Failed to create temporary string.", rslt_to_str(ret));
        goto cleanup;
    }

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

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(tmp_string)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_create_from_c_string(%s) - Postcondition validation failed for 'tmp_string'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    *string_ = tmp_string;
    tmp_string = NULL;

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(NULL != tmp_string) {
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

choco_string_result_t choco_string_copy(const choco_string_t* src_, choco_string_t* dst_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy", "src_")
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy", "dst_")
#if defined(DEBUG_BUILD)
    if(!is_valid_shallow(src_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Precondition validation failed for 'src_'.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!is_valid_shallow(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Precondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
#if defined(TEST_BUILD)
    if(!choco_string_is_valid(src_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Precondition validation failed for 'src_'.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Precondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    if(src_ == dst_) {
        // 何もしない
    } else {
        if(0 == src_->len) {
            if(NULL != dst_->buffer) {
                dst_->buffer[0] = '\0';
            }
            dst_->len = 0;
        } else {
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
        }
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Postcondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

choco_string_result_t choco_string_copy_from_c_string(const char* src_, choco_string_t* dst_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    size_t src_len = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy_from_c_string", "src_")
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy_from_c_string", "dst_")
#if defined(DEBUG_BUILD)
    if(!is_valid_shallow(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Precondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
#if defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Precondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    src_len = mock_strlen(src_);
    if(0 == src_len) {
        if(NULL != dst_->buffer) {
            dst_->buffer[0] = '\0';
        }
        dst_->len = 0;
    } else {
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
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Postcondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
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
    if(string_ == dst_) {
        ret = CHOCO_STRING_BAD_OPERATION;
        ERROR_MESSAGE("choco_string_concat(%s) - provided dst_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD)
    if(!is_valid_shallow(string_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Precondition validation failed for 'string_'.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!is_valid_shallow(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Precondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
#if defined(TEST_BUILD)
    if(!choco_string_is_valid(string_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Precondition validation failed for 'string_'.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Precondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

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
            ret = choco_string_mem_allocate(dst_len_new + 1, (void**)&tmp_buffer);
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

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Postcondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
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
#if defined(DEBUG_BUILD)
    if(!is_valid_shallow(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Precondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
#if defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Precondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

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
            ret = choco_string_mem_allocate(dst_len_new + 1, (void**)&tmp_buffer);
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

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Postcondition validation failed for 'dst_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
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

bool choco_string_equal(const char* str1_, const char* str2_) {
    if(NULL == str1_ || NULL == str2_) {
        return false;
    }
    return (0 == mock_strcmp(str1_, str2_) ? true : false);
}

bool choco_string_substring_exists(const char* str_, const char* target_) {
    if(NULL == str_ || NULL == target_) {
        return false;
    }

    const char* result = strstr(str_, target_);
    return (NULL == result) ? false : true;
}

choco_string_result_t choco_string_key_value_key_get(const char* line_, choco_string_t* out_key_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    char* tmp_buff = NULL;
    size_t len = 0;
    size_t equal_index = 0;
    size_t start_index = 0;
    size_t buff_size = 0;
    bool equal_found = false;

    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_key_value_key_get", "line_")
    IF_ARG_NULL_GOTO_CLEANUP(out_key_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_key_value_key_get", "out_key_")
#if defined(DEBUG_BUILD)
    if(!is_valid_shallow(out_key_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_key_value_key_get(%s) - Precondition validation failed for 'out_key_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
#if defined(TEST_BUILD)
    if(!choco_string_is_valid(out_key_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_key_value_key_get(%s) - Precondition validation failed for 'out_key_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    len = mock_strlen(line_);
    for(size_t i = 0; i != len; ++i) {
        if('=' == line_[i]) {
            equal_found = true;
            equal_index = i;
            break;
        }
    }
    if(!equal_found || 0 == equal_index) {
        ret = CHOCO_STRING_BAD_OPERATION;
        // ファイルをロードし、xxx = yyyの行じゃなければ無視する処理を想定し、メッセージは出さない
        goto cleanup;
    }

    for(size_t i = 0; i != equal_index; ++i) {
        if(' ' == line_[i]) {
            start_index++;
        } else {
            break;
        }
    }
    if(equal_index == start_index) {    // =の前に有効文字がない
        ret = CHOCO_STRING_BAD_OPERATION;
        // ファイルをロードし、xxx = yyyの行じゃなければ無視する処理を想定し、メッセージは出さない
        goto cleanup;
    }

    buff_size = equal_index - start_index + 1;
    ret = choco_string_mem_allocate(buff_size, (void**)&tmp_buff);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_key_value_key_get(%s) - Failed to get key-value key. reason=tmp_buffer_allocate_failed, bytes=%zu", rslt_to_str(ret), buff_size);
        goto cleanup;
    }
    memset(tmp_buff, 0, buff_size);

    for(size_t i = start_index, j = 0; i != equal_index; ++i, ++j) {
        tmp_buff[j] = line_[i];
    }

    for(size_t i = (buff_size - 1); i != 0; --i) {
        if(' ' == tmp_buff[i - 1]) {
            tmp_buff[i - 1] = '\0';
        } else {
            break;
        }
    }

    ret = choco_string_copy_from_c_string(tmp_buff, out_key_);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_key_value_key_get(%s) - choco_string_copy_from_c_string failed.", rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(out_key_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_key_value_key_get(%s) - Postcondition validation failed for 'out_key_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(NULL != tmp_buff) {
        memory_system_free(tmp_buff, buff_size, MEMORY_TAG_STRING);
        tmp_buff = NULL;
    }
    return ret;
}

choco_string_result_t choco_string_key_value_value_get(const char* line_, choco_string_t* out_value_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    char* tmp_buff = NULL;
    size_t len = 0;
    size_t equal_index = 0;
    size_t buff_size = 0;
    bool equal_found = false;

    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_key_value_value_get", "line_")
    IF_ARG_NULL_GOTO_CLEANUP(out_value_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_key_value_value_get", "out_value_")
#if defined(DEBUG_BUILD)
    if(!is_valid_shallow(out_value_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_key_value_value_get(%s) - Precondition validation failed for 'out_value_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
#if defined(TEST_BUILD)
    if(!choco_string_is_valid(out_value_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_key_value_value_get(%s) - Precondition validation failed for 'out_value_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    len = mock_strlen(line_);
    for(size_t i = 0; i != len; ++i) {
        if('=' == line_[i]) {
            equal_found = true;
            equal_index = i;
            break;
        }
    }
    if(!equal_found || 0 == equal_index || (len - 1) == equal_index) {
        ret = CHOCO_STRING_BAD_OPERATION;
        // ファイルをロードし、xxx = yyyの行じゃなければ無視する処理を想定し、メッセージは出さない
        goto cleanup;
    }

    // =の後のスペースをtrim
    for(size_t i = (equal_index + 1); i != len; ++i) {
        if(' ' == line_[i]) {
            equal_index++;
        } else {
            break;
        }
    }
    if((len - 1) == equal_index) {
        ret = CHOCO_STRING_BAD_OPERATION;
        goto cleanup;
    }

    buff_size = len - equal_index;
    ret = choco_string_mem_allocate(buff_size, (void**)&tmp_buff);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_key_value_value_get(%s) - Failed to get key-value value. reason=tmp_buffer_allocate_failed, bytes=%zu", rslt_to_str(ret), buff_size);
        goto cleanup;
    }
    memset(tmp_buff, 0, buff_size);

    for(size_t i = (equal_index + 1), j = 0; i != len; ++i, ++j) {
        tmp_buff[j] = line_[i];
    }

    // 末尾のスペースをtrim
    for(size_t i = (len - equal_index - 1); i != 0; --i) {
        if(tmp_buff[i - 1] != ' ') {
            break;
        } else {
            tmp_buff[i - 1] = '\0';
        }
    }
    ret = choco_string_copy_from_c_string(tmp_buff, out_value_);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_key_value_value_get(%s) - choco_string_copy_from_c_string failed.", rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(out_value_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_key_value_value_get(%s) - Postcondition validation failed for 'out_value_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(NULL != tmp_buff) {
        memory_system_free(tmp_buff, buff_size, MEMORY_TAG_STRING);
        tmp_buff = NULL;
    }
    return ret;
}

bool choco_string_is_valid(const choco_string_t* string_) {
    if(!is_valid_shallow(string_)) {
        return false;
    } else {
        if(0 < string_->len) {
            for(size_t i = 0; i != string_->len; ++i) {
                if('\0' == string_->buffer[i]) {
                    return false;
                }
            }
        }
    }
    return true;
}

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ 文字列に変換する実行結果コード
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
 * @param[in] size_ 確保するメモリサイズ
 * @param[out] out_ptr_ 確保したメモリの先頭アドレス
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 下記のいずれか
 * - out_ptr_ == NULL
 * - *out_ptr_ != NULL
 * - memory_system_allocateの実行結果がMEMORY_SYSTEM_INVALID_ARGUMENT
 * @retval CHOCO_STRING_NO_MEMORY メモリ確保失敗
 * @retval CHOCO_STRING_LIMIT_EXCEEDED 以下のいずれか
 * - 割り当てサイズを割り当てた結果、mem_tag_allocatedがSIZE_MAX超過
 * - 割り当てサイズを割り当てた結果、total_allocatedがSIZE_MAX超過
 * @retval CHOCO_STRING_BAD_OPERATION メモリシステム未初期化
 * @retval CHOCO_STRING_SUCCESS メモリ確保に成功し、正常終了
 */
static choco_string_result_t choco_string_mem_allocate(size_t size_, void** out_ptr_) {
    void* tmp_ptr = NULL;
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_mem_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_mem_allocate", "*out_ptr_")

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
    case MEMORY_SYSTEM_BAD_OPERATION:
        ret = CHOCO_STRING_BAD_OPERATION;
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

    IF_ARG_FALSE_GOTO_CLEANUP(size_ > 0, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_reserve", "size_")
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_reserve", "string_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == string_->capacity, ret, CHOCO_STRING_BAD_OPERATION, rslt_to_str(CHOCO_STRING_BAD_OPERATION), "buffer_reserve", "string_->capacity")

    ret = choco_string_mem_allocate(size_, (void**)&tmp_buffer);
    if(CHOCO_STRING_SUCCESS != ret) {
        goto cleanup;
    }
    memset(tmp_buffer, 0, size_);
    string_->buffer = tmp_buffer;
    string_->len = 0;
    string_->capacity = size_;

    ret = CHOCO_STRING_SUCCESS;

cleanup:
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

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_resize", "string_")
    IF_ARG_FALSE_GOTO_CLEANUP(size_ > 0, ret, CHOCO_STRING_INVALID_ARGUMENT, rslt_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_resize", "size_")

    // Simulation.
    ret = choco_string_mem_allocate(size_, (void**)&tmp_buffer);
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
    return ret;
}

static size_t NO_COVERAGE mock_strlen(const char* str_) {
    return strlen(str_);
}

static int NO_COVERAGE mock_strcmp(const char *s1_, const char *s2_) {
    return strcmp(s1_, s2_);
}

static bool is_valid_shallow(const choco_string_t* string_) {
    if(NULL == string_) {
        return false;
    }
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
