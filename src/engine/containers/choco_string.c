/**
 *
 * @file choco_string.c
 * @author chocolate-pie24
 * @brief 文字列のコピー、生成の際のリソース管理を含めた文字列操作API内部実装
 *
 * @version 0.1
 * @date 2025-09-26
 *
 * @todo src_len + 1のオーバーフローチェックをしていない + テストケース追加
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <string.h>

#include "engine/containers/choco_string.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#ifdef TEST_BUILD
#include <assert.h>
static void test_choco_string_default_create(void);
static void test_choco_string_create_from_char(void);
static void test_choco_string_destroy(void);
static void test_choco_string_copy(void);
static void test_choco_string_copy_from_char(void);
static void test_choco_string_length(void);
static void test_choco_string_c_str(void);
#endif

struct choco_string {
    size_t len;         /**< 文字列長さ(終端文字は含まない) */
    size_t capacity;    /**< バッファサイズ */
    char* buffer;       /**< 文字列格納バッファ */
};

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

static const char* rslt_to_str(choco_string_result_t rslt_);
static choco_string_result_t string_malloc(size_t size_, void** out_ptr_);

// string_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
// *string_ != NULLでCHOCO_STRING_INVALID_ARGUMENT
// tmp_stringメモリ確保失敗でCHOCO_STRING_NO_MEMORY
choco_string_result_t choco_string_default_create(choco_string_t** string_) {
    choco_string_t* tmp_string = NULL;
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    // Preconditions.
    CHECK_ARG_NULL_GOTO_CLEANUP(string_, CHOCO_STRING_INVALID_ARGUMENT, "choco_string_default_create", "string_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*string_, CHOCO_STRING_INVALID_ARGUMENT, "choco_string_default_create", "*string_")

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
        if(NULL != tmp_string) {
            memory_system_free(tmp_string, sizeof(*tmp_string), MEMORY_TAG_STRING);
            tmp_string = NULL;
        }
    }
    return ret;
}

// string_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
// *string_ != NULLでCHOCO_STRING_INVALID_ARGUMENT
// src_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
// src_文字列が""でbuffer = NULL, len = 0, capacity = 0(choco_string_default_createと同等の動作)
// tmp_stringメモリ確保失敗でCHOCO_STRING_NO_MEMORY(choco_string_default_create内で判定される)
// tmp_string->bufferメモリ確保失敗でCHOCO_STRING_NO_MEMORY
choco_string_result_t choco_string_create_from_char(choco_string_t** string_, const char* src_) {
    choco_string_t* tmp_string = NULL;
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    size_t src_len = 0;

    // Preconditions.
    CHECK_ARG_NULL_GOTO_CLEANUP(string_, CHOCO_STRING_INVALID_ARGUMENT, "choco_string_create_from_char", "string_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*string_, CHOCO_STRING_INVALID_ARGUMENT, "choco_string_create_from_char", "*string_")
    CHECK_ARG_NULL_GOTO_CLEANUP(src_, CHOCO_STRING_INVALID_ARGUMENT, "choco_string_create_from_char", "src_")

    // Simulation.
    ret = string_malloc(sizeof(*tmp_string), (void**)&tmp_string);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_create_from_char(%s) - Failed to allocate memory for 'tmp_string'.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_string, 0, sizeof(*tmp_string));

    src_len = strlen(src_);
    if(0 != src_len) {
        ret = string_malloc(src_len + 1, (void**)&tmp_string->buffer);
        if(CHOCO_STRING_SUCCESS != ret) {
            ERROR_MESSAGE("choco_string_create_from_char(%s) - Failed to allocate memory for 'tmp_string->buffer'.", rslt_to_str(ret));
            goto cleanup;
        }
        memset(tmp_string->buffer, 0, src_len + 1);
        tmp_string->len = src_len;
        tmp_string->capacity = src_len + 1;
        memcpy(tmp_string->buffer, src_, src_len + 1);
    }

    // Commit.
    *string_ = tmp_string;
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_SUCCESS != ret) {
        if(NULL != tmp_string) {
            if(NULL != tmp_string->buffer) {
                memory_system_free(tmp_string->buffer, src_len + 1, MEMORY_TAG_STRING);
                tmp_string->buffer = NULL;
            }
            memory_system_free(tmp_string, sizeof(*tmp_string), MEMORY_TAG_STRING);
            tmp_string = NULL;
        }
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

// dst_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
// src_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
// tmp_bufferメモリ確保失敗でCHOCO_STRING_NO_MEMORY
// src_->len == 0でdst_->bufferをクリアし、dst_->lenを0にする(CHOCO_STRING_SUCCESS)
// テストケース dst->capacity >= (src_->len + 1)のケース(CHOCO_STRING_SUCCESS)
// テストケース dst->capacity < (src_->len + 1)のケース(CHOCO_STRING_SUCCESS)
// テストケース dst_->capacity == 0かつsrc_が空文字列(CHOCO_STRING_SUCCESS)
choco_string_result_t choco_string_copy(choco_string_t* dst_, const choco_string_t* src_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    char* tmp_buffer = NULL;

    // Preconditions.
    CHECK_ARG_NULL_GOTO_CLEANUP(dst_, CHOCO_STRING_INVALID_ARGUMENT, "choco_string_copy", "dst_")
    CHECK_ARG_NULL_GOTO_CLEANUP(src_, CHOCO_STRING_INVALID_ARGUMENT, "choco_string_copy", "src_")

    // Commit.
    if(0 == src_->len) {
        if(NULL != dst_->buffer) {
            memset(dst_->buffer, 0, dst_->len + 1);
        }
        dst_->len = 0;
        ret = CHOCO_STRING_SUCCESS;
        goto cleanup;
    }

    if(dst_->capacity >= (src_->len + 1)) {
        // Commit.
        memcpy(dst_->buffer, src_->buffer, src_->len + 1);  // 終端文字を含めてコピー
        dst_->len = src_->len;
    } else {
        // Simulation.
        ret = string_malloc(src_->len + 1, (void**)&tmp_buffer);
        if(CHOCO_STRING_SUCCESS != ret) {
            ERROR_MESSAGE("choco_string_copy(%s) - Failed to allocate tmp_buffer memory.", rslt_to_str(ret));
            goto cleanup;
        }
        memset(tmp_buffer, 0, src_->len + 1);
        memcpy(tmp_buffer, src_->buffer, src_->len + 1);  // 終端文字を含めてコピー
        // Commit.
        if(NULL != dst_->buffer) {
            memory_system_free(dst_->buffer, dst_->capacity, MEMORY_TAG_STRING);
            dst_->buffer = NULL;
        }
        dst_->buffer = tmp_buffer;
        dst_->len = src_->len;
        dst_->capacity = src_->len + 1;
    }
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_SUCCESS != ret) {
        if(NULL != tmp_buffer) {
            memory_system_free(tmp_buffer, src_->len + 1, MEMORY_TAG_STRING);
            tmp_buffer = NULL;
        }
    }
    return ret;
}

// dst_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
// src_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
// tmp_bufferメモリ確保失敗でCHOCO_STRING_NO_MEMORY
// src_->len == 0でdst_->bufferをクリアし、dst_->lenを0にする(CHOCO_STRING_SUCCESS)
// テストケース dst_->capacity >= (src_len + 1)のケース(CHOCO_STRING_SUCCESS)
// テストケース dst_->capacity < (src_len + 1)のケース(CHOCO_STRING_SUCCESS)
// テストケース dst_->capacity == 0かつsrc_が空文字列(CHOCO_STRING_SUCCESS)
choco_string_result_t choco_string_copy_from_char(choco_string_t* dst_, const char* src_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    char* tmp_buffer = NULL;
    size_t src_len = 0;

    // Preconditions.
    CHECK_ARG_NULL_GOTO_CLEANUP(dst_, CHOCO_STRING_INVALID_ARGUMENT, "choco_string_copy_from_char", "dst_")
    CHECK_ARG_NULL_GOTO_CLEANUP(src_, CHOCO_STRING_INVALID_ARGUMENT, "choco_string_copy_from_char", "src_")
    src_len = strlen(src_);
    if(0 == src_len) {
        if(NULL != dst_->buffer) {
            memset(dst_->buffer, 0, dst_->len + 1);
        }
        dst_->len = 0;
        ret = CHOCO_STRING_SUCCESS;
        goto cleanup;
    }

    // Simulation, Commit.
    if(dst_->capacity >= (src_len + 1)) {
        memcpy(dst_->buffer, src_, src_len + 1);
        dst_->len = src_len;
    } else {
        ret = string_malloc(src_len + 1, (void**)&tmp_buffer);
        if(CHOCO_STRING_SUCCESS != ret) {
            ERROR_MESSAGE("choco_string_copy_from_char(%s) - Failed to allocate tmp_buffer memory.", rslt_to_str(ret));
            goto cleanup;
        }
        memset(tmp_buffer, 0, src_len + 1);
        memcpy(tmp_buffer, src_, src_len + 1);

        if(NULL != dst_->buffer) {
            memory_system_free(dst_->buffer, dst_->capacity, MEMORY_TAG_STRING);
            dst_->buffer = NULL;
        }
        dst_->len = src_len;
        dst_->capacity = src_len + 1;
        dst_->buffer = tmp_buffer;
    }
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_SUCCESS != ret) {
        if(NULL != tmp_buffer) {
            memory_system_free(tmp_buffer, src_len + 1, MEMORY_TAG_STRING);
            tmp_buffer = NULL;
        }
    }
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
    case CHOCO_STRING_NO_MEMORY:
        return s_rslt_str_no_memory;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

static choco_string_result_t string_malloc(size_t size_, void** out_ptr_) {
    void* tmp_ptr = NULL;
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    ret_mem = memory_system_allocate(size_, MEMORY_TAG_STRING, &tmp_ptr);
    if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_mem) {
        ret = CHOCO_STRING_INVALID_ARGUMENT;
        goto cleanup;
    } else if(MEMORY_SYSTEM_NO_MEMORY == ret_mem) {
        ret = CHOCO_STRING_NO_MEMORY;
        goto cleanup;
    } else if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = CHOCO_STRING_UNDEFINED_ERROR;
        goto cleanup;
    }
    *out_ptr_ = tmp_ptr;
    ret = CHOCO_STRING_SUCCESS;
cleanup:
    return ret;
}

#ifdef TEST_BUILD
void test_choco_string(void) {
    memory_system_create();
    memory_system_report();

    test_choco_string_default_create();
    memory_system_report();

    test_choco_string_create_from_char();
    memory_system_report();

    test_choco_string_destroy();
    memory_system_report();

    test_choco_string_copy();
    memory_system_report();

    test_choco_string_copy_from_char();
    memory_system_report();

    test_choco_string_length();
    memory_system_report();

    test_choco_string_c_str();
    memory_system_report();

    memory_system_destroy();
}

static void NO_COVERAGE test_choco_string_default_create(void) {
    {
        // string_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();

        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        ret = choco_string_default_create(NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
    }
    {
        // *string_ != NULLでCHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();

        // 正常系テスト
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(0 == string->capacity);
        assert(0 == string->len);
        assert(NULL == string->buffer);

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&string);
        assert(NULL == string);
    }
    {
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        // tmp_stringメモリ確保失敗でCHOCO_STRING_NO_MEMORY
        memory_system_test_param_set(0);    // 1回目のメモリ確保で失敗

        choco_string_t* string = NULL;
        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        memory_system_test_param_reset();
    }
}

static void NO_COVERAGE test_choco_string_create_from_char(void) {
    {
        // string_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        ret = choco_string_create_from_char(NULL, "aaa");
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
    }
    {
        // *string_ != NULLでCHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();

        // 正常系テスト
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(0 == string->capacity);
        assert(0 == string->len);
        assert(NULL == string->buffer);

        ret = choco_string_create_from_char(&string, "aaa");
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&string);
        assert(NULL == string);
    }
    {
        // src_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        ret = choco_string_create_from_char(&string, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
        assert(NULL == string);
    }
    {
        // src_文字列が""でbuffer = NULL, len = 0, capacity = 0(choco_string_default_createと同等の動作)
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        ret = choco_string_create_from_char(&string, "");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL == string->buffer);
        assert(0 == string->len);
        assert(0 == string->capacity);

        choco_string_destroy(&string);
    }
    {
        // tmp_stringメモリ確保失敗でCHOCO_STRING_NO_MEMORY
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        memory_system_test_param_set(0);    // 1回目のメモリ確保で失敗

        choco_string_t* string = NULL;
        ret = choco_string_create_from_char(&string, "aaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        memory_system_test_param_reset();
    }
    {
        // tmp_string->bufferメモリ確保失敗でCHOCO_STRING_NO_MEMORY
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        memory_system_test_param_set(1);    // 2回目のメモリ確保で失敗

        choco_string_t* string = NULL;
        ret = choco_string_create_from_char(&string, "aaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(NULL == string);

        memory_system_test_param_reset();
    }
    {
        // 正常系
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        memory_system_test_param_reset();

        choco_string_t* string = NULL;
        ret = choco_string_create_from_char(&string, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(3 == string->len);
        assert(4 == string->capacity);
        assert(0 == strcmp(string->buffer, "aaa"));

        choco_string_destroy(&string);
    }
}

static void NO_COVERAGE test_choco_string_destroy(void) {
    {
        choco_string_destroy(NULL); // 最初のgoto cleanupを通ること
    }
    {
        choco_string_t* string = NULL;
        choco_string_destroy(&string);  // 2回目のgoto cleanupを通ること
    }
    {
        // *string_ != NULLでCHOCO_STRING_INVALID_ARGUMENT
        memory_system_test_param_reset();

        // 正常系テスト
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* string = NULL;
        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != string);
        assert(0 == string->capacity);
        assert(0 == string->len);
        assert(NULL == string->buffer);

        choco_string_destroy(&string);  // 全てのfreeを通ること
        assert(NULL == string);
        choco_string_destroy(&string);  // 2重デストロイ
    }
}

static void NO_COVERAGE test_choco_string_copy(void) {
    {
        // dst_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        ret = choco_string_create_from_char(&src, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == src->len);
        assert(4 == src->capacity);
        assert(0 == strcmp(src->buffer, "aaa"));

        ret = choco_string_copy(NULL, src);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&src);
    }
    {
        // src_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaa"));

        ret = choco_string_copy(dst, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
    }
    {
        // src_->len == 0でdst_->bufferをクリアし、dst_->lenを0にする(CHOCO_STRING_SUCCESS)
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        ret = choco_string_default_create(&src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == src->len);
        assert(0 == src->capacity);
        assert(NULL == src->buffer);

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaa"));

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strlen(dst->buffer));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
    {
        // テストケース dst->capacity >= (src_->len + 1)のケース
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        ret = choco_string_create_from_char(&src, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == src->len);
        assert(4 == src->capacity);
        assert(0 == strcmp(src->buffer, "aaa"));

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "bbbbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbbbb"));

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, src->buffer));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
    {
        // テストケース dst->capacity < (src_->len + 1)のケース(CHOCO_STRING_SUCCESS)
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        ret = choco_string_create_from_char(&src, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == src->len);
        assert(6 == src->capacity);
        assert(0 == strcmp(src->buffer, "aaaaa"));

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, src->buffer));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
    {
        // dst_->capacity == 0かつsrc_が空文字列
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != dst);
        assert(0 == dst->capacity);
        assert(0 == dst->len);
        assert(NULL == dst->buffer);

        choco_string_t* src = NULL;
        ret = choco_string_default_create(&src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != src);
        assert(0 == src->capacity);
        assert(0 == src->len);
        assert(NULL == src->buffer);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != dst);
        assert(0 == dst->capacity);
        assert(0 == dst->len);
        assert(NULL == dst->buffer);

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
    {
        // malloc失敗
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* src = NULL;
        ret = choco_string_create_from_char(&src, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == src->len);
        assert(6 == src->capacity);
        assert(0 == strcmp(src->buffer, "aaaaa"));

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_set(0);    // 1回目のメモリ確保で失敗

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_reset();

        choco_string_destroy(&src);
        choco_string_destroy(&dst);
    }
    {
        // dst->buffer = NULL, dst->capacity = 0の状態でaaaを割り当て
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        choco_string_t* src = NULL;
        ret = choco_string_create_from_char(&src, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == src->len);
        assert(6 == src->capacity);
        assert(0 == strcmp(src->buffer, "aaaaa"));

        choco_string_t* dst = NULL;
        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != dst);
        assert(0 == dst->capacity);
        assert(0 == dst->len);
        assert(NULL == dst->buffer);

        ret = choco_string_copy(dst, src);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaaaa"));

        choco_string_destroy(&dst);
        choco_string_destroy(&src);
    }
}

static void NO_COVERAGE test_choco_string_copy_from_char(void) {
    {
        // dst_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        ret = choco_string_copy_from_char(NULL, "aaa");
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);
    }
    {
        // src_ == NULLでCHOCO_STRING_INVALID_ARGUMENT
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaa"));

        ret = choco_string_copy_from_char(dst, NULL);
        assert(CHOCO_STRING_INVALID_ARGUMENT == ret);

        choco_string_destroy(&dst);
    }
    {
        // src_->len == 0でdst_->bufferをクリアし、dst_->lenを0にする(CHOCO_STRING_SUCCESS)

        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaa"));

        ret = choco_string_copy_from_char(dst, "");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strlen(dst->buffer));

        choco_string_destroy(&dst);
    }
    {
        // テストケース dst->capacity >= (src_->len + 1)のケース
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "bbbbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbbbb"));

        ret = choco_string_copy_from_char(dst, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaa"));

        choco_string_destroy(&dst);
    }
    {
        // テストケース dst->capacity < (src_len + 1)のケース(CHOCO_STRING_SUCCESS)
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        ret = choco_string_copy_from_char(dst, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaaaa"));

        choco_string_destroy(&dst);
    }
    {
        // dst_->capacity == 0かつsrc_が空文字列
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != dst);
        assert(0 == dst->capacity);
        assert(0 == dst->len);
        assert(NULL == dst->buffer);

        ret = choco_string_copy_from_char(dst, "");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != dst);
        assert(0 == dst->capacity);
        assert(0 == dst->len);
        assert(NULL == dst->buffer);

        choco_string_destroy(&dst);
    }
    {
        // malloc失敗
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
        choco_string_t* dst = NULL;
        ret = choco_string_create_from_char(&dst, "bbb");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_set(0);    // 1回目のメモリ確保で失敗

        ret = choco_string_copy_from_char(dst, "aaaaa");
        assert(CHOCO_STRING_NO_MEMORY == ret);
        assert(3 == dst->len);
        assert(4 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "bbb"));

        memory_system_test_param_reset();
        choco_string_destroy(&dst);
    }
    {
        // dst->buffer = NULL, dst->capacity = 0の状態でaaaを割り当て
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        choco_string_t* dst = NULL;
        ret = choco_string_default_create(&dst);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(NULL != dst);
        assert(0 == dst->capacity);
        assert(0 == dst->len);
        assert(NULL == dst->buffer);

        ret = choco_string_copy_from_char(dst, "aaaaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(5 == dst->len);
        assert(6 == dst->capacity);
        assert(0 == strcmp(dst->buffer, "aaaaa"));

        choco_string_destroy(&dst);
    }
}

static void NO_COVERAGE test_choco_string_length(void) {
    {
        // ""(length == 0)
        choco_string_t* string = NULL;
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == string->len);
        assert(0 == string->capacity);
        assert(NULL == string->buffer);

        const size_t len = choco_string_length(string);
        assert(0 == len);
        choco_string_destroy(&string);
    }
    {
        // "aaa"(length == 3)
        choco_string_t* string = NULL;
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        ret = choco_string_create_from_char(&string, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == string->len);
        assert(4 == string->capacity);
        assert(0 == strcmp(string->buffer, "aaa"));

        const size_t len = choco_string_length(string);
        assert(3 == len);
        choco_string_destroy(&string);
    }
    {
        const size_t len = choco_string_length(NULL);
        assert(0 == len);
    }
}

static void NO_COVERAGE test_choco_string_c_str(void) {
    {
        // ""(length == 0)
        choco_string_t* string = NULL;
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        ret = choco_string_default_create(&string);
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(0 == string->len);
        assert(0 == string->capacity);
        assert(NULL == string->buffer);

        const char* c_ptr = choco_string_c_str(string);
        assert(0 == strcmp(c_ptr, ""));

        choco_string_destroy(&string);
    }
    {
        // "aaa"(length == 3)
        choco_string_t* string = NULL;
        choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

        ret = choco_string_create_from_char(&string, "aaa");
        assert(CHOCO_STRING_SUCCESS == ret);
        assert(3 == string->len);
        assert(4 == string->capacity);
        assert(0 == strcmp(string->buffer, "aaa"));

        const char* c_ptr = choco_string_c_str(string);
        assert(0 == strcmp(c_ptr, "aaa"));

        choco_string_destroy(&string);
    }
    {
        const char* c_ptr = choco_string_c_str(NULL);
        assert(0 == strcmp(c_ptr, ""));
    }
}
#endif
