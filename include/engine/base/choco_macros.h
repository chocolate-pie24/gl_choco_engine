/** @ingroup choco_macros
 *
 * @file choco_macros.h
 * @author chocolate-pie24
 * @brief 全レイヤーで使用される共通マクロ定義
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_BASE_CHOCO_MACROS_H
#define GLCE_ENGINE_BASE_CHOCO_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "choco_message.h"

#ifdef __clang__
  /**
   * @brief テスト関数をカバレッジ計測対象外とするためのマクロ定義(clang使用時のみ)
   *
   */
  #define NO_COVERAGE __attribute__((no_profile_instrument_function))
#else
  #define NO_COVERAGE
#endif

/**
 * @brief KiB定義(=1024)
 *
 */
#define KIB ((size_t)(1ULL << 10))

/**
 * @brief MiB定義(=1024 * 1024)
 *
 */
#define MIB ((size_t)(1ULL << 20))

/**
 * @brief GiB定義(=1024 * 1024 * 1024)
 *
 */
#define GIB ((size_t)(1ULL << 30))

/**
 * @brief 2の冪乗かをチェックする
 *
 * @retval true  2の冪乗
 * @retval false 2の冪乗ではない
 */
#define IS_POWER_OF_TWO(val_) ( ((size_t)(val_) != 0u) && ( (((size_t)(val_) & ((size_t)(val_) - 1u)) == 0u)))

/**
 * @brief 引数ptr_がNULLであればret_コードを出力し、cleanupにジャンプする
 *
 * @note エラーメッセージはINVALID_ARGUMENTで、function_name_とvariable_name_をエラーメッセージに出力する
 *
 * @param[in] ptr_ NULL判定対象変数
 * @param[in] return_valiable_ この関数を呼び出す関数のリターン変数
 * @param[in] rslt_code_ 実行結果コード
 * @param[in] rslt_str_ 実行結果コードの文字列
 * @param[in] function_name_ このマクロを使用する関数名称
 * @param[in] variable_name_ NULL判定対象変数名
 *
 * @code{.c}
 * #define ERROR_CODE 1
 * #define SUCCESS 0
 * int func(void) {
 *      int result = ERROR_CODE;
 *      int* a = NULL;
 *      IF_ARG_NULL_GOTO_CLEANUP(a, result, ERROR_CODE, "error_code", "func", "a")  // NULLなのでcleanupに飛ぶ
 *      result = SUCCESS;
 * cleanup:
 *      return result;
 * }
 * @endcode
 */
#define IF_ARG_NULL_GOTO_CLEANUP(ptr_, return_valiable_, rslt_code_, rslt_str_, function_name_, variable_name_) \
    if(NULL == ptr_) { \
        ERROR_MESSAGE("%s(%s) - Argument %s requires a valid pointer.", function_name_, rslt_str_, variable_name_); \
        return_valiable_ = rslt_code_; \
        goto cleanup;  \
    } \

/**
 * @brief 引数ptr_がNULLでなければret_コードを出力し、cleanupにジャンプする
 *
 * @note エラーメッセージはINVALID_ARGUMENTで、function_name_とvariable_name_をエラーメッセージに出力する
 */
#define IF_ARG_NOT_NULL_GOTO_CLEANUP(ptr_, ret_, rslt_str_, function_name_, variable_name_) \
    if(NULL != ptr_) { \
        ERROR_MESSAGE("%s(%s) - Argument %s requires a null pointer.", function_name_, rslt_str_, variable_name_); \
        ret = ret_; \
        goto cleanup;  \
    } \

/**
 * @brief 引数ptr_がNULLであればret_コードを出力し、cleanupにジャンプする
 *
 * @note エラーメッセージはNO_MEMORYで、function_name_とvariable_name_をエラーメッセージに出力する
 */
#define IF_ALLOC_FAIL_GOTO_CLEANUP(ptr_, ret_, function_name_, variable_name_) \
    if(NULL == ptr_) { \
        ERROR_MESSAGE("%s(NO_MEMORY) - Failed to allocate %s memory.", function_name_, variable_name_); \
        ret = ret_; \
        goto cleanup;  \
    } \

/**
 * @brief 引数is_valid_がfalseであればret_コードを出力し、cleanupにジャンプする
 *
 * @note エラーメッセージはINVALID_ARGUMENTで、function_name_とvariable_name_をエラーメッセージに出力する
 */
#define IF_ARG_FALSE_GOTO_CLEANUP(is_valid_, ret_, rslt_str_, function_name_, variable_name_) \
    if(!(is_valid_)) { \
        ERROR_MESSAGE("%s(%s) - Argument %s is not valid.", function_name_, rslt_str_, variable_name_); \
        ret = ret_; \
        goto cleanup;  \
    } \

#ifdef __cplusplus
}
#endif
#endif
