// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup containers
 *
 * @file choco_string.h
 * @author chocolate-pie24
 * @brief 文字列を格納するコンテナモジュールAPIの定義
 *
 * @details 文字列比較や文字列連結等の文字列処理機能も提供する
 *
 * @note
 * choco_string_t構造体は、内部データを隠蔽している。
 * このため、choco_string_t型で変数を宣言することはできない。
 * 使用の際は、choco_string_t*型で宣言すること
 *
 * @date 2025-09-26
 *
 */
#ifndef GLCE_ENGINE_CONTAINERS_CHOCO_STRING_H
#define GLCE_ENGINE_CONTAINERS_CHOCO_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief choco_string_t前方宣言
 *
 */
typedef struct choco_string choco_string_t;

/**
 * @brief 文字列API実行結果コードリスト
 */
typedef enum {
    CHOCO_STRING_SUCCESS = 0,       /**< 処理成功 */
    CHOCO_STRING_DATA_CORRUPTED,    /**< 内部データ整合異常 */
    CHOCO_STRING_BAD_OPERATION,     /**< API誤用 */
    CHOCO_STRING_NO_MEMORY,         /**< メモリ確保に失敗 */
    CHOCO_STRING_INVALID_ARGUMENT,  /**< 無効な引数 */
    CHOCO_STRING_RUNTIME_ERROR,     /**< 実行時エラー */
    CHOCO_STRING_UNDEFINED_ERROR,   /**< 未定義エラー */
    CHOCO_STRING_OVERFLOW,          /**< 計算過程でオーバーフロー発生 */
    CHOCO_STRING_LIMIT_EXCEEDED,    /**< システム使用可能範囲上限超過 */
} choco_string_result_t;

choco_string_result_t choco_string_default_create(choco_string_t** string_);

choco_string_result_t choco_string_create_from_c_string(const char* src_, choco_string_t** string_);

void choco_string_destroy(choco_string_t** string_);

choco_string_result_t choco_string_copy(const choco_string_t* src_, choco_string_t* dst_);

choco_string_result_t choco_string_copy_from_c_string(const char* src_, choco_string_t* dst_);

choco_string_result_t choco_string_concat(const choco_string_t* string_, choco_string_t* dst_);

choco_string_result_t choco_string_concat_from_c_string(const char* string_, choco_string_t* dst_);

size_t choco_string_length(const choco_string_t* string_);

const char* choco_string_c_str(const choco_string_t* string_);

bool choco_string_equal(const char* str1_, const char* str2_);

bool choco_string_substring_exists(const char* str_, const char* target_);

choco_string_result_t choco_string_key_value_key_get(const char* line_, choco_string_t* out_key_);

choco_string_result_t choco_string_key_value_value_get(const char* line_, choco_string_t* out_value_);

bool choco_string_is_valid(const choco_string_t* string_);

#ifdef __cplusplus
}
#endif
#endif
