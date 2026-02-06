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

#include "engine/renderer/renderer_core/renderer_types.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"

static const char* s_rslt_str_success = "SUCCESS";                            /**< 実行結果コードRENDERER_SUCCESSの文字列 */
static const char* s_rslt_str_invalid_argument = "INVALID_ARGUMENT";          /**< 実行結果コードRENDERER_INVALID_ARGUMENTの文字列 */
static const char* s_rslt_str_runtime_error = "RUNTIME_ERROR";                /**< 実行結果コードRENDERER_RUNTIME_ERRORの文字列 */
static const char* s_rslt_str_no_memory = "NO_MEMORY";                        /**< 実行結果コードRENDERER_NO_MEMORYの文字列 */
static const char* s_rslt_str_shader_compile_error = "SHADER_COMPILE_ERROR";  /**< 実行結果コードRENDERER_SHADER_COMPILE_ERRORの文字列 */
static const char* s_rslt_str_shader_link_error = "SHADER_LINK_ERROR";        /**< 実行結果コードRENDERER_SHADER_LINK_ERRORの文字列 */
static const char* s_rslt_str_undefined_error = "UNDEFINED_ERROR";            /**< 実行結果コードRENDERER_UNDEFINED_ERRORの文字列 */
static const char* s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";              /**< 実行結果コードRENDERER_LIMIT_EXCEEDEDの文字列 */
static const char* s_rslt_str_bad_operation = "BAD_OPERATION";                /**< 実行結果コードRENDERER_BAD_OPERATIONの文字列 */
static const char* s_rslt_str_data_corrupted = "DATA_CORRUPTED";              /**< 実行結果コードRENDERER_DATA_CORRUPTEDの文字列 */

const char* renderer_rslt_to_str(renderer_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_SUCCESS:
        return s_rslt_str_success;
    case RENDERER_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RENDERER_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case RENDERER_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RENDERER_SHADER_COMPILE_ERROR:
        return s_rslt_str_shader_compile_error;
    case RENDERER_SHADER_LINK_ERROR:
        return s_rslt_str_shader_link_error;
    case RENDERER_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RENDERER_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RENDERER_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RENDERER_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

#ifdef TEST_BUILD
void test_renderer_result_str(void) {
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_SUCCESS);
        assert(0 == strcmp(tmp, s_rslt_str_success));
    }
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT);
        assert(0 == strcmp(tmp, s_rslt_str_invalid_argument));
    }
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_RUNTIME_ERROR);
        assert(0 == strcmp(tmp, s_rslt_str_runtime_error));
    }
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_NO_MEMORY);
        assert(0 == strcmp(tmp, s_rslt_str_no_memory));
    }
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_SHADER_COMPILE_ERROR);
        assert(0 == strcmp(tmp, s_rslt_str_shader_compile_error));
    }
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_SHADER_LINK_ERROR);
        assert(0 == strcmp(tmp, s_rslt_str_shader_link_error));
    }
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_UNDEFINED_ERROR);
        assert(0 == strcmp(tmp, s_rslt_str_undefined_error));
    }
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_LIMIT_EXCEEDED);
        assert(0 == strcmp(tmp, s_rslt_str_limit_exceeded));
    }
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_BAD_OPERATION);
        assert(0 == strcmp(tmp, s_rslt_str_bad_operation));
    }
    {
        const char* tmp = renderer_rslt_to_str(RENDERER_DATA_CORRUPTED);
        assert(0 == strcmp(tmp, s_rslt_str_data_corrupted));
    }
    {
        const char* tmp = renderer_rslt_to_str(1000);
        assert(0 == strcmp(tmp, s_rslt_str_undefined_error));
    }
}
#endif
