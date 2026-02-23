#ifdef TEST_BUILD
#include <string.h>
#include <assert.h>
#endif

#include "engine/base/choco_macros.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/platform/platform_core/platform_err_utils.h"
#include "engine/platform/platform_core/platform_types.h"

static const char* const s_rslt_str_success = "SUCCESS";                    /**< プラットフォームAPI実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< プラットフォームAPI実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< プラットフォームAPI実行結果コード(実行時エラー)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< プラットフォームAPI実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< プラットフォームAPI実行結果コード(メモリ破損)に対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< プラットフォームAPI実行結果コード(API誤用)に対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< プラットフォームAPI実行結果コード(オーバーフロー)に対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< プラットフォームAPI実行結果コード(システム使用可能範囲上限超過)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< プラットフォームAPI実行結果コード(未定義エラー)に対応する文字列 */
static const char* const s_rslt_str_window_close = "WINDOW_CLOSE";          /**< プラットフォームAPI実行結果コード(ウィンドウクローズ)に対応する文字列 */

#ifdef TEST_BUILD
static void NO_COVERAGE test_platform_rslt_to_str(void);
static void NO_COVERAGE test_platform_rslt_convert_choco_string(void);
static void NO_COVERAGE test_platform_rslt_convert_linear_alloc(void);
#endif

const char* platform_rslt_to_str(platform_result_t rslt_) {
    switch(rslt_) {
    case PLATFORM_SUCCESS:
        return s_rslt_str_success;
    case PLATFORM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case PLATFORM_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case PLATFORM_NO_MEMORY:
        return s_rslt_str_no_memory;
    case PLATFORM_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case PLATFORM_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case PLATFORM_OVERFLOW:
        return s_rslt_str_overflow;
    case PLATFORM_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case PLATFORM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case PLATFORM_WINDOW_CLOSE:
        return s_rslt_str_window_close;
    default:
        return s_rslt_str_undefined_error;
    }
}

platform_result_t platform_rslt_convert_choco_string(choco_string_result_t rslt_) {
    switch(rslt_) {
    case CHOCO_STRING_SUCCESS:
        return PLATFORM_SUCCESS;
    case CHOCO_STRING_NO_MEMORY:
        return PLATFORM_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return PLATFORM_INVALID_ARGUMENT;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return PLATFORM_UNDEFINED_ERROR;
    case CHOCO_STRING_DATA_CORRUPTED:
        return PLATFORM_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return PLATFORM_BAD_OPERATION;
    case CHOCO_STRING_RUNTIME_ERROR:
        return PLATFORM_RUNTIME_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return PLATFORM_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return PLATFORM_LIMIT_EXCEEDED;
    default:
        return PLATFORM_UNDEFINED_ERROR;
    }
}

platform_result_t platform_rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return PLATFORM_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return PLATFORM_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return PLATFORM_INVALID_ARGUMENT;
    default:
        return PLATFORM_UNDEFINED_ERROR;
    }
}

#ifdef TEST_BUILD
void test_platform_err_utils(void) {
    test_platform_rslt_to_str();
    test_platform_rslt_convert_choco_string();
    test_platform_rslt_convert_linear_alloc();
}

static void NO_COVERAGE test_platform_rslt_to_str(void) {
    {
        const char* test = platform_rslt_to_str(PLATFORM_SUCCESS);
        assert(0 == strcmp(s_rslt_str_success, test));
    }
    {
        const char* test = platform_rslt_to_str(PLATFORM_DATA_CORRUPTED);
        assert(0 == strcmp(s_rslt_str_data_corrupted, test));
    }
    {
        const char* test = platform_rslt_to_str(PLATFORM_BAD_OPERATION);
        assert(0 == strcmp(s_rslt_str_bad_operation, test));
    }
    {
        const char* test = platform_rslt_to_str(PLATFORM_OVERFLOW);
        assert(0 == strcmp(s_rslt_str_overflow, test));
    }
    {
        const char* test = platform_rslt_to_str(PLATFORM_LIMIT_EXCEEDED);
        assert(0 == strcmp(s_rslt_str_limit_exceeded, test));
    }
    {
        const char* test = platform_rslt_to_str(PLATFORM_NO_MEMORY);
        assert(0 == strcmp(s_rslt_str_no_memory, test));
    }
    {
        const char* test = platform_rslt_to_str(PLATFORM_RUNTIME_ERROR);
        assert(0 == strcmp(s_rslt_str_runtime_error, test));
    }
    {
        const char* test = platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT);
        assert(0 == strcmp(s_rslt_str_invalid_argument, test));
    }
    {
        const char* test = platform_rslt_to_str(PLATFORM_UNDEFINED_ERROR);
        assert(0 == strcmp(s_rslt_str_undefined_error, test));
    }
    {
        const char* test = platform_rslt_to_str(PLATFORM_WINDOW_CLOSE);
        assert(0 == strcmp(s_rslt_str_window_close, test));
    }
    {
        const char* test = platform_rslt_to_str(100);
        assert(0 == strcmp(s_rslt_str_undefined_error, test));
    }
}

static void NO_COVERAGE test_platform_rslt_convert_choco_string(void) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    ret = platform_rslt_convert_choco_string(CHOCO_STRING_SUCCESS);
    assert(PLATFORM_SUCCESS == ret);

    ret = platform_rslt_convert_choco_string(CHOCO_STRING_NO_MEMORY);
    assert(PLATFORM_NO_MEMORY == ret);

    ret = platform_rslt_convert_choco_string(CHOCO_STRING_INVALID_ARGUMENT);
    assert(PLATFORM_INVALID_ARGUMENT == ret);

    ret = platform_rslt_convert_choco_string(CHOCO_STRING_UNDEFINED_ERROR);
    assert(PLATFORM_UNDEFINED_ERROR == ret);

    ret = platform_rslt_convert_choco_string(CHOCO_STRING_DATA_CORRUPTED);
    assert(PLATFORM_DATA_CORRUPTED == ret);

    ret = platform_rslt_convert_choco_string(CHOCO_STRING_BAD_OPERATION);
    assert(PLATFORM_BAD_OPERATION == ret);

    ret = platform_rslt_convert_choco_string(CHOCO_STRING_RUNTIME_ERROR);
    assert(PLATFORM_RUNTIME_ERROR == ret);

    ret = platform_rslt_convert_choco_string(CHOCO_STRING_OVERFLOW);
    assert(PLATFORM_OVERFLOW == ret);

    ret = platform_rslt_convert_choco_string(CHOCO_STRING_LIMIT_EXCEEDED);
    assert(PLATFORM_LIMIT_EXCEEDED == ret);

    ret = platform_rslt_convert_choco_string(100);
    assert(PLATFORM_UNDEFINED_ERROR == ret);
}

static void NO_COVERAGE test_platform_rslt_convert_linear_alloc(void) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    ret = platform_rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(PLATFORM_SUCCESS == ret);

    ret = platform_rslt_convert_linear_alloc(LINEAR_ALLOC_NO_MEMORY);
    assert(PLATFORM_NO_MEMORY == ret);

    ret = platform_rslt_convert_linear_alloc(LINEAR_ALLOC_INVALID_ARGUMENT);
    assert(PLATFORM_INVALID_ARGUMENT == ret);

    ret = platform_rslt_convert_linear_alloc(100);
    assert(PLATFORM_UNDEFINED_ERROR == ret);
}

#endif
