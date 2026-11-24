// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup base
 *
 * @file choco_macros.h
 * @author chocolate-pie24
 * @brief 全レイヤーで使用される共通マクロ定義
 *
 * @date 2025-09-20
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
  /**
   * @brief clang使用時以外はカバレッジ計測対象外マクロは無効
   *
   */
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

#define IF_ARG_NULL_GOTO_CLEANUP(ptr_, ret_, code_) \
    do { \
        if(NULL == (ptr_)) { \
            ERROR_MESSAGE("%s(%s) - %s must not be NULL.", __func__, #code_, #ptr_); \
            (ret_) = (code_); \
            goto cleanup; \
        } \
    } while(0)

#define IF_ARG_NOT_NULL_GOTO_CLEANUP(ptr_, ret_, code_) \
    do { \
        if(NULL != (ptr_)) { \
            ERROR_MESSAGE("%s(%s) - %s must be NULL.", \
                          __func__, #code_, #ptr_); \
            (ret_) = (code_); \
            goto cleanup; \
        } \
    } while(0)

/**
 * @brief 引数ptr_がNULLであればret_コードを出力し、cleanupにジャンプする
 *
 * @note エラーメッセージはNO_MEMORYで、function_name_とvariable_name_をエラーメッセージに出力する
 *
 * @param[in] ptr_ NULL判定対象変数
 * @param[in] return_variable_ この関数を呼び出す関数のリターン変数
 * @param[in] result_code_ 実行結果コード
 * @param[in] function_name_ このマクロを使用する関数名称
 * @param[in] variable_name_ NULL判定対象変数名
 *
 * @code{.c}
 * #define ERROR_CODE 1
 * #define SUCCESS 0
 * int func(void) {
 *      int result = ERROR_CODE;
 *      int* a = NULL;
 *      IF_ALLOC_FAIL_GOTO_CLEANUP(a, result, ERROR_CODE, "func", "a")  // NULLなのでcleanupに飛ぶ
 *      result = SUCCESS;
 * cleanup:
 *      return result;
 * }
 * @endcode
 */
#define IF_ALLOC_FAIL_GOTO_CLEANUP(ptr_, return_variable_, result_code_, function_name_, variable_name_) \
    do { \
        if(NULL == ptr_) { \
            ERROR_MESSAGE("%s(NO_MEMORY) - Failed to allocate %s memory.", function_name_, variable_name_); \
            return_variable_ = result_code_; \
            goto cleanup;  \
        } \
    } while(0); \

// token_を文字列に変換する
// 例: STRINGIFY(APPLICATION_INVALID_ARGUMENT) -> "APPLICATION_INVALID_ARGUMENT"
#define STRINGIFY(token_) #token_

#ifdef __cplusplus
}
#endif
#endif
