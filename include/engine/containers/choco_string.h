/** @addtogroup container
 * @{
 *
 * @file choco_string.h
 * @author chocolate-pie24
 * @brief 文字列のコピー、生成の際のリソース管理を含めた文字列操作APIを提供する
 *
 * @note
 * choco_string_tオブジェクトは、内部データを隠蔽している \n
 * このため、choco_string_t型で変数を宣言することはできない \n
 * 使用の際は、choco_string_t*型で宣言すること
 *
 * @version 0.1
 * @date 2025-09-26
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 * @todo choco_string_move
 * @todo choco_string_create_from_string
 * @todo choco_string_clone(deep copy - copyとの違いをきちんとドキュメントに書く)
 */
#ifndef GLCE_ENGINE_CONTAINERS_CHOCO_STRING_H
#define GLCE_ENGINE_CONTAINERS_CHOCO_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

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
    CHOCO_STRING_NO_MEMORY,         /**< メモリ確保に失敗 */
    CHOCO_STRING_INVALID_ARGUMENT,  /**< 無効な引数 */
    CHOCO_STRING_UNDEFINED_ERROR,   /**< 未定義エラー */
} choco_string_result_t;

/**
 * @brief choco_string_tデフォルトコンストラクタ
 *
 * @note 文字列格納用内部バッファサイズを0で初期化したchoco_string_tオブジェクトを生成する
 *
 * @code{.c}
 * choco_string_t* string = NULL;   // 必ずNULLで初期化しておく
 * choco_string_result_t ret = choco_string_default_create(&string);
 *
 * // エラー処理
 *
 * choco_string_destroy(&string);
 * @endcode
 *
 * @param[out] string_ 生成対象オブジェクト
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - string_ == NULL
 * - *string_ != NULL
 * @retval CHOCO_STRING_NO_MEMORY        メモリ確保に失敗
 * @retval CHOCO_STRING_SUCCESS          choco_string_tの生成に成功し正常終了
 *
 * @see choco_string_create_from_char
 */
choco_string_result_t choco_string_default_create(choco_string_t** string_);

/**
 * @brief 引数で与えた文字列src_でchoco_string_tオブジェクトを初期化し、生成する
 *
 * @note
 * - 空文字列""が渡された場合は、文字列バッファがNULLで初期化されるため、choco_string_default_createと等価になる
 *
 * @code{.c}
 * choco_string_t* string = NULL;   // 必ずNULLで初期化しておく
 * choco_string_result_t ret = choco_string_create_from_char(&string, "abc");
 *
 * // エラー処理
 *
 * choco_string_destroy(&string);
 * @endcode
 *
 * @param string_ 生成対象オブジェクト
 * @param src_ 初期化文字列
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - string_ == NULL
 * - *string_ != NULL
 * - src_ == NULL
 * @retval CHOCO_STRING_NO_MEMORY        メモリ確保に失敗
 * @retval CHOCO_STRING_SUCCESS          choco_string_tの生成に成功し正常終了
 *
 * @see choco_string_default_create
 */
choco_string_result_t choco_string_create_from_char(choco_string_t** string_, const char* src_);

/**
 * @brief choco_string_tが管理しているメモリおよびchoco_string_t*のメモリを解放する
 *
 * @note
 * - 2重デストロイ許可
 * - string == NULLの場合はno-op
 * - *string == NULLの場合はno-op
 *
 * @code{.c}
 * choco_string_t* string = NULL;   // 必ずNULLで初期化しておく
 * choco_string_result_t ret = choco_string_create_from_char(&string, "abc");
 *
 * // エラー処理
 *
 * choco_string_destroy(&string);   // string = NULLになる
 * choco_string_destroy(&string);   // 2重デストロイ(何もしない)
 * @endcode
 *
 * @param string_ 破棄対象オブジェクト
 */
void choco_string_destroy(choco_string_t** string_);

/**
 * @brief src_が管理する文字列をdst_にコピーする
 *
 * @note
 * - dst_のバッファサイズがsrc_以上の場合は、dst_のバッファサイズは変更されない
 * - dst_のバッファサイズがsrc_よりも小さい場合は、dst_のバッファは破棄され、新規にメモリが確保される
 * - src_が管理する文字列が空文字列だった場合は、dst_が管理する文字列を空文字列とし、バッファサイズは変更されない
 * - src_が管理する文字列が空文字列かつ、dst_が管理する文字列が空文字列の場合は何もしない
 *
 * @code{.c}
 * choco_string_t* src = NULL;
 * choco_string_result_t ret = choco_string_create_from_char(&src, "aaaaa");
 *
 * // エラー処理
 *
 * choco_string_t* dst = NULL;
 * ret = choco_string_create_from_char(&dst, "bbb");
 *
 * // エラー処理
 *
 * ret = choco_string_copy(dst, src);   // dstの文字列バッファがaaaaaになる(バッファサイズは拡張される)
 *
 * // エラー処理
 *
 * choco_string_destroy(&dst);
 * choco_string_destroy(&src);
 * @endcode
 *
 * @param dst_ コピー先オブジェクト
 * @param src_ コピー元オブジェクト
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - dst_ == NULL
 * - src_ == NULL
 * @retval CHOCO_STRING_NO_MEMORY        メモリ確保失敗
 * @retval CHOCO_STRING_SUCCESS          文字列のコピーに成功し、正常終了
 */
choco_string_result_t choco_string_copy(choco_string_t* dst_, const choco_string_t* src_);

/**
 * @brief 文字列src_をdst_にコピーする
 *
 * @note
 * - dst_のバッファサイズがsrc_以上の場合は、dst_のバッファサイズは変更されない
 * - dst_のバッファサイズがsrc_よりも小さい場合は、dst_のバッファは破棄され、新規にメモリが確保される
 * - src_が管理する文字列が空文字列だった場合は、dst_が管理する文字列を空文字列とし、バッファサイズは変更されない
 * - src_が管理する文字列が空文字列かつ、dst_が管理する文字列が空文字列の場合は何もしない
 *
 * @code{.c}
 * choco_string_t* src = NULL;
 * choco_string_result_t ret = choco_string_default_create(&dst);
 *
 * // エラー処理
 *
 * ret = choco_string_copy_from_char(dst, "aaaaa");   // dstの文字列バッファがaaaaaになる(バッファサイズは拡張される)
 *
 * // エラー処理
 *
 * choco_string_destroy(&dst);
 * @endcode
 *
 * @param dst_ コピー先オブジェクト
 * @param src_ コピー元文字列
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - dst_ == NULL
 * - src_ == NULL
 * @retval CHOCO_STRING_NO_MEMORY        メモリ確保失敗
 * @retval CHOCO_STRING_SUCCESS          文字列のコピーに成功し、正常終了
 */
choco_string_result_t choco_string_copy_from_char(choco_string_t* dst_, const char* src_);

/**
 * @brief string_が管理する文字列の長さを取得する(終端文字を含まない長さが返される)
 *
 * @note
 * - string_がNULLまたはstring_内部管理バッファサイズが0の場合は0を返す
 *
 * @code{.c}
 * choco_string_t* string = NULL;
 * choco_string_result_t ret = choco_string_create_from_char(&string, "aaaaa");
 *
 * // エラー処理
 *
 * const size_t len = choco_string_length(string);  // len == 5
 *
 * choco_string_destroy(&string);
 * @endcode
 *
 * @param string_ 文字列長さ取得元オブジェクト
 * @return size_t 文字列長さ
 */
size_t choco_string_length(const choco_string_t* string_);

/**
 * @brief string_が管理する文字列の先頭アドレスを取得する
 *
 * @note
 * - string_がNULLまたは内部管理バッファがNULLで空の文字列を返す
 *
 * @code{.c}
 * choco_string_t* string = NULL;
 * choco_string_result_t ret = choco_string_create_from_char(&string, "aaaaa");
 *
 * // エラー処理
 *
 * const char* c_ptr = choco_string_c_str(string);
 * fprintf(stdout, "%s\n", c_ptr);  // 標準出力にaaaaaが出力される
 *
 * choco_string_destroy(&string);
 * @endcode
 *
 * @param string_ 文字列先頭アドレス取得元オブジェクト
 * @return const char* 文字列先頭アドレス
 */
const char* choco_string_c_str(const choco_string_t* string_);

#ifdef __cplusplus
}
#endif
#endif

/** @}*/
