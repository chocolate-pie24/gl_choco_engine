// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup containers
 *
 * @file choco_string.h
 * @author chocolate-pie24
 * @brief 文字列を所有するコンテナと、その基本操作を提供する
 *
 * @details
 * Choco String moduleは、動的に確保された文字列storageを所有する
 * `choco_string_t`と、文字列の生成、複製、連結、参照等に関する機能を提供する。
 *
 * `choco_string_t`は内部表現を公開しないopaque objectであり、
 * callerは`choco_string_t*`を通してmodule APIを利用する。
 *
 * @section choco_string_boundary_contract Module Boundary Contract
 *
 * - `choco_string_t`はopaque typeとして公開し、内部表現をmodule外部へ公開しない。
 * - Choco String moduleは、`choco_string_t` object自身のstorageおよび
 *   objectが所有する文字列storageのallocation / releaseを管理する。
 * - callerは`choco_string_t` object自身のstorageまたはobjectが所有する
 *   文字列storageを直接解放しない。
 * - module APIへ渡す`choco_string_t*`は、Choco String moduleによって生成され、lifetime中にあるobjectを参照するものとする。
 * - `choco_string_t`が表す文字列の長さには終端NULを含めない。
 * - `choco_string_t`が表す文字列は、文字列長に対応する位置に終端NULを持ち、それより前の文字列領域にはNULを含まない。
 * - sourceとして受け取るC stringはborrowとして扱い、
 *   Choco String moduleはそのstorageのownershipを取得しない。
 * - sourceとして受け取るC stringへのpointerはobject内部へ保持せず、
 *   必要な文字列dataはChoco String moduleが所有するstorageへcopyする。
 * - sourceとして受け取るC stringは、終端NULを持つ有効なC stringとして扱う。
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

choco_string_result_t choco_string_default_create(choco_string_t** out_string_);

choco_string_result_t choco_string_create_from_c_string(const char* src_, choco_string_t** out_string_);

void choco_string_destroy(choco_string_t** string_);

choco_string_result_t choco_string_copy(const choco_string_t* src_, choco_string_t* dst_);

choco_string_result_t choco_string_copy_from_c_string(const char* src_, choco_string_t* dst_);

choco_string_result_t choco_string_concat(const choco_string_t* string_, choco_string_t* dst_);

choco_string_result_t choco_string_concat_from_c_string(const char* string_, choco_string_t* dst_);

size_t choco_string_length(const choco_string_t* string_);

const char* choco_string_c_str(const choco_string_t* string_);

bool choco_string_is_equal(const char* str1_, const char* str2_);

bool choco_string_substring_exists(const char* str_, const char* target_);

bool choco_string_is_valid(const choco_string_t* string_);

#ifdef __cplusplus
}
#endif
#endif
