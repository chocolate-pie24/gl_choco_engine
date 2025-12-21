/** @ingroup renderer_core
 *
 * @file renderer_err_utils.c
 * @author chocolate-pie24
 * @brief レンダラーレイヤー全体で使用されるエラー処理関連APIの実装
 *
 * @version 0.1
 * @date 2025-12.19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifdef TEST_BUILD
#include <string.h>
#include <assert.h>
#endif

#include "engine/renderer/renderer_base/renderer_types.h"

#include "engine/renderer/renderer_core/renderer_err_utils.h"

static const char* s_result_str_success = "SUCCESS";                    /**< 実行結果コードRENDERER_SUCCESSの文字列 */
static const char* s_result_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コードRENDERER_INVALID_ARGUMENTの文字列 */
static const char* s_result_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コードRENDERER_RUNTIME_ERRORの文字列 */
static const char* s_result_str_no_memory = "NO_MEMORY";                /**< 実行結果コードRENDERER_NO_MEMORYの文字列 */
static const char* s_result_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コードRENDERER_UNDEFINED_ERRORの文字列 */

const char* renderer_result_to_str(renderer_result_t result_) {
    const char* ret_str;
    switch(result_) {
    case RENDERER_SUCCESS:
        ret_str = s_result_str_success;
        break;
    case RENDERER_INVALID_ARGUMENT:
        ret_str = s_result_str_invalid_argument;
        break;
    case RENDERER_RUNTIME_ERROR:
        ret_str = s_result_str_runtime_error;
        break;
    case RENDERER_NO_MEMORY:
        ret_str = s_result_str_no_memory;
        break;
    case RENDERER_UNDEFINED_ERROR:
        ret_str = s_result_str_undefined_error;
        break;
    default:
        ret_str = s_result_str_undefined_error;
        break;
    }
    return ret_str;
}

#ifdef TEST_BUILD
void test_renderer_result_str(void) {
    {
        const char* tmp = renderer_result_to_str(RENDERER_SUCCESS);
        assert(0 == strcmp(tmp, s_result_str_success));
    }
    {
        const char* tmp = renderer_result_to_str(RENDERER_INVALID_ARGUMENT);
        assert(0 == strcmp(tmp, s_result_str_invalid_argument));
    }
    {
        const char* tmp = renderer_result_to_str(RENDERER_RUNTIME_ERROR);
        assert(0 == strcmp(tmp, s_result_str_runtime_error));
    }
    {
        const char* tmp = renderer_result_to_str(RENDERER_NO_MEMORY);
        assert(0 == strcmp(tmp, s_result_str_no_memory));
    }
    {
        const char* tmp = renderer_result_to_str(RENDERER_UNDEFINED_ERROR);
        assert(0 == strcmp(tmp, s_result_str_undefined_error));
    }
    {
        const char* tmp = renderer_result_to_str(1000);
        assert(0 == strcmp(tmp, s_result_str_undefined_error));
    }
}
#endif
