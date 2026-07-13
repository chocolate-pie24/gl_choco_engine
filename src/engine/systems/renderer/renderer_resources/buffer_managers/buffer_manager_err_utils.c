#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_err_utils.h"

#include "engine/systems/renderer/renderer_core/allocators/range_free_list.h"

#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_types.h"

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コードBUFFER_MANAGER_SUCCESSの文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コードBUFFER_MANAGER_INVALID_ARGUMENTの文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コードBUFFER_MANAGER_RUNTIME_ERRORの文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コードBUFFER_MANAGER_LIMIT_EXCEEDEDの文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コードBUFFER_MANAGER_NO_MEMORYの文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コードBUFFER_MANAGER_DATA_CORRUPTEDの文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コードBUFFER_MANAGER_BAD_OPERATIONの文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< 実行結果コードBUFFER_MANAGER_OVERFLOWの文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コードBUFFER_MANAGER_UNDEFINED_ERRORの文字列 */

const char* buffer_manager_rslt_to_str(buffer_manager_result_t rslt_) {
    switch(rslt_) {
    case BUFFER_MANAGER_SUCCESS:
        return s_rslt_str_success;
    case BUFFER_MANAGER_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case BUFFER_MANAGER_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case BUFFER_MANAGER_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case BUFFER_MANAGER_NO_MEMORY:
        return s_rslt_str_no_memory;
    case BUFFER_MANAGER_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case BUFFER_MANAGER_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case BUFFER_MANAGER_OVERFLOW:
        return s_rslt_str_overflow;
    case BUFFER_MANAGER_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

buffer_manager_result_t buffer_manager_rslt_convert_range_free_list(range_free_list_result_t rslt_) {
    switch(rslt_) {
    case RANGE_FREE_LIST_SUCCESS:
        return BUFFER_MANAGER_SUCCESS;
    case RANGE_FREE_LIST_INVALID_ARGUMENT:
        return BUFFER_MANAGER_INVALID_ARGUMENT;
    case RANGE_FREE_LIST_LIMIT_EXCEEDED:
        return BUFFER_MANAGER_LIMIT_EXCEEDED;
    case RANGE_FREE_LIST_NO_MEMORY:
        return BUFFER_MANAGER_NO_MEMORY;
    case RANGE_FREE_LIST_DATA_CORRUPTED:
        return BUFFER_MANAGER_DATA_CORRUPTED;
    case RANGE_FREE_LIST_BAD_OPERATION:
        return BUFFER_MANAGER_BAD_OPERATION;
    case RANGE_FREE_LIST_OVERFLOW:
        return BUFFER_MANAGER_OVERFLOW;
    case RANGE_FREE_LIST_UNDEFINED_ERROR:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    default:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    }
}

renderer_result_t buffer_manager_rslt_convert_renderer(renderer_result_t rslt_) {
    switch(rslt_) {
    case RENDERER_SUCCESS:
        return BUFFER_MANAGER_SUCCESS;
    case RENDERER_INVALID_ARGUMENT:
        return BUFFER_MANAGER_INVALID_ARGUMENT;
    case RENDERER_RUNTIME_ERROR:
        return BUFFER_MANAGER_RUNTIME_ERROR;
    case RENDERER_NO_MEMORY:
        return BUFFER_MANAGER_NO_MEMORY;
    case RENDERER_SHADER_COMPILE_ERROR:
        return BUFFER_MANAGER_UNDEFINED_ERROR;  // buffer_managerはシェーダーのコンパイル / リンクは行わないのでUNDEFINED_ERRORに変換
    case RENDERER_SHADER_LINK_ERROR:
        return BUFFER_MANAGER_UNDEFINED_ERROR;  // buffer_managerはシェーダーのコンパイル / リンクは行わないのでUNDEFINED_ERRORに変換
    case RENDERER_LIMIT_EXCEEDED:
        return BUFFER_MANAGER_LIMIT_EXCEEDED;
    case RENDERER_BAD_OPERATION:
        return BUFFER_MANAGER_BAD_OPERATION;
    case RENDERER_DATA_CORRUPTED:
        return BUFFER_MANAGER_DATA_CORRUPTED;
    case RENDERER_OVERFLOW:
        return BUFFER_MANAGER_OVERFLOW;
    case RENDERER_UNDEFINED_ERROR:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    default:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    }
}

memory_system_result_t buffer_manager_rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return BUFFER_MANAGER_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return BUFFER_MANAGER_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return BUFFER_MANAGER_RUNTIME_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return BUFFER_MANAGER_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return BUFFER_MANAGER_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return BUFFER_MANAGER_NO_MEMORY;
    default:
        return BUFFER_MANAGER_UNDEFINED_ERROR;
    }
}
