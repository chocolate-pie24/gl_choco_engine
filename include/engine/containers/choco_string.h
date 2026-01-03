/** @ingroup choco_string
 *
 * @file choco_string.h
 * @author chocolate-pie24
 * @brief 文字列を格納するコンテナモジュールAPIの定義
 *
 * @details 文字列比較や文字列連結等の文字列処理機能も提供する
 *
 * @note
 * choco_string_t構造体は、内部データを隠蔽している \n
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
 */
#ifndef GLCE_ENGINE_CONTAINERS_CHOCO_STRING_H
#define GLCE_ENGINE_CONTAINERS_CHOCO_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

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
    CHOCO_STRING_DATA_CORRUPTED,    /**< 内部データ整合異常 */
    CHOCO_STRING_BAD_OPERATION,     /**< API誤用 */
    CHOCO_STRING_NO_MEMORY,         /**< メモリ確保に失敗 */
    CHOCO_STRING_INVALID_ARGUMENT,  /**< 無効な引数 */
    CHOCO_STRING_RUNTIME_ERROR,     /**< 実行時エラー */
    CHOCO_STRING_UNDEFINED_ERROR,   /**< 未定義エラー */
    CHOCO_STRING_OVERFLOW,          /**< 計算過程でオーバーフロー発生 */
    CHOCO_STRING_LIMIT_EXCEEDED,    /**< システム使用可能範囲上限超過 */
} choco_string_result_t;

/**
 * @brief string_のメモリを確保し,バッファ容量0,文字列長さ0で初期化する
 *
 * @code{.c}
 * choco_string_t* string = NULL;
 * choco_string_result_t ret = choco_string_default_create(&string);
 * // エラー処理
 * @endcode
 *
 * @param string_ 初期化対象文字列コンテナ
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - string_ == NULL
 * - *string_ != NULL
 * @retval CHOCO_STRING_NO_MEMORY メモリ確保失敗
 * @retval CHOCO_STRING_LIMIT_EXCEEDED メモリ管理システムの管理変数が使用可能範囲を超過
 * @retval CHOCO_STRING_SUCCESS 初期化に成功し,正常終了
 */
choco_string_result_t choco_string_default_create(choco_string_t** string_);

/**
 * @brief string_のメモリを確保し,src_文字列で初期化する
 *
 * @note string_が管理する文字列バッファはsrc_の文字列長さ+1(終端文字)のサイズで初期化される
 *
 * @code{.c}
 * choco_string_t* string = NULL;
 * choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaa");
 * // エラー処理
 * @endcode
 *
 * @param string_ 初期化対象文字列コンテナ
 * @param src_ 初期化文字列
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - string_ == NULL
 * - *string_ != NULL
 * - src_ == NULL
 * @retval CHOCO_STRING_NO_MEMORY メモリ確保失敗
 * @retval CHOCO_STRING_LIMIT_EXCEEDED メモリ管理システムの管理変数が使用可能範囲を超過
 * @retval CHOCO_STRING_OVERFLOW src_の文字列長さ+1(終端文字)がsize_tの最大値を超過
 * @retval CHOCO_STRING_SUCCESS 初期化に成功し,正常終了
 * @warning 上記以外のエラーは,テストなどで意図的に発生させない限り起こり得ないエラーで確実にバグ
 */
choco_string_result_t choco_string_create_from_c_string(choco_string_t** string_, const char* src_);

/**
 * @brief choco_string_tが管理しているメモリおよびchoco_string_t*のメモリを解放し,*string_=NULLにする
 *
 * @note
 * - 2重デストロイ許可
 * - string_ == NULLの場合はno-op
 * - *string_ == NULLの場合はno-op
 *
 * @code{.c}
 * choco_string_t* string = NULL;   // 必ずNULLで初期化しておく
 * choco_string_result_t ret = choco_string_create_from_c_string(&string, "abc");
 *
 * // エラー処理
 *
 * choco_string_destroy(&string);   // string = NULLになる
 * choco_string_destroy(&string);   // 2重デストロイ(何もしない)
 * @endcode
 *
 * @param string_ 破棄対象構造体インスタンス
 */
void choco_string_destroy(choco_string_t** string_);

/**
 * @brief src_が管理する文字列をdst_が管理するバッファにコピーする
 *
 * @note dst_のバッファサイズにより下記の動作をする
 * - dst_のバッファサイズがsrc_の文字列長さ+1(終端文字)よりも小さい -> dst_のバッファサイズをsrc_の文字列長さ+1(終端文字)に拡張
 * - dst_のバッファサイズがsrc_の文字列長さ+1(終端文字)よりも大きい -> dst_のバッファサイズは変更せず,文字列をコピー
 * - dst_のバッファサイズとsrc_の文字列長さ+1(終端文字)が等しい -> dst_のバッファサイズは変更せず,文字列をコピー
 *
 * @code{.c}
 * choco_string_t* dst = NULL;
 * choco_string_result_t ret = choco_string_default_create(&dst);
 * // エラー処理
 *
 * choco_string_t* src = NULL;
 * ret = choco_string_create_from_c_string(&src, "aaa");
 * // エラー処理
 *
 * ret = choco_string_copy(dst, src);   // dstのバッファにaaa+'\0'がコピーされる
 * // エラー処理
 * @endcode
 *
 * @param dst_ コピー先文字列コンテナ
 * @param src_ コピー元文字列コンテナ
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - dst_ == NULL
 * - src_ == NULL
 * @retval CHOCO_STRING_DATA_CORRUPTED 以下のいずれか
 * - dst_内部データが破損(アドレスへの不正アクセス等により発生)
 * - src_内部データが破損(アドレスへの不正アクセス等により発生)
 * @retval CHOCO_STRING_OVERFLOW src_の文字列長さ+1(終端文字)がsize_tの最大値を超過
 * @retval CHOCO_STRING_NO_MEMORY メモリ確保失敗
 * @retval CHOCO_STRING_LIMIT_EXCEEDED メモリ管理システムの管理変数が使用可能範囲を超過
 * @retval CHOCO_STRING_SUCCESS コピーに成功し,正常終了
 * @warning 上記以外のエラーは,テストなどで意図的に発生させない限り起こり得ないエラーで確実にバグ
 */
choco_string_result_t choco_string_copy(choco_string_t* dst_, const choco_string_t* src_);

/**
 * @brief char*による文字列によってdst_に文字列をコピーする
 *
 * @note dst_のバッファサイズにより下記の動作をする
 * - dst_のバッファがsrc_の文字列長さ+1(終端文字)よりも小さい -> dst_のバッファサイズをsrc_の文字列長さ+1(終端文字)に拡張
 * - dst_のバッファがsrc_の文字列長さ+1(終端文字)よりも大きい -> dst_のバッファサイズは変更せず,文字列をコピー
 * - dst_のバッファとsrc_のバッファサイズが等しい -> dst_のバッファサイズは変更せず,文字列をコピー
 *
 * @code{.c}
 * choco_string_t* dst = NULL;
 * choco_string_result_t ret = choco_string_default_create(&dst);
 * // エラー処理
 *
 * ret = choco_string_copy_from_c_string(dst, "aaa");   // dstのバッファにaaa+'\0'がコピーされる
 * // エラー処理
 * @endcode
 *
 * @param dst_ コピー先文字列コンテナ
 * @param src_ コピー元文字列
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - dst_がNULL
 * - src_がNULL
 * @retval CHOCO_STRING_DATA_CORRUPTED dst_の内部データが破損(アドレスへの不正アクセス等により発生)
 * @retval CHOCO_STRING_OVERFLOW src_の文字列長さ+1(終端文字)がsize_tの上限を超過
 * @retval CHOCO_STRING_NO_MEMORY メモリ確保失敗
 * @retval CHOCO_STRING_LIMIT_EXCEEDED メモリ管理システムの管理変数が使用可能範囲を超過
 * @retval CHOCO_STRING_SUCCESS コピーに成功し,正常終了
 * @warning 上記以外のエラーは,テストなどで意図的に発生させない限り起こり得ないエラーで確実にバグ
 */
choco_string_result_t choco_string_copy_from_c_string(choco_string_t* dst_, const char* src_);

/**
 * @brief dst_の文字列の末尾にstring_の文字列を連結する
 *
 * @note
 * - 自己連結(dst_にdst_を連結する)ことは禁止する(内部バッファ管理を簡便にするため)
 * - string_が管理する文字列が""の場合は何もしない
 * - dst_が管理するバッファの容量が足りない場合は,新規にバッファを取得し直す
 *
 * @code{.c}
 * choco_string_t* string = NULL;
 * choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaa");
 * // エラー処理
 *
 * choco_string_t* dst = NULL;
 * ret = choco_string_create_from_c_string(&dst, "bbb");
 * // エラー処理
 *
 * ret = choco_string_concat(string, dst);   // dstの文字列はbbbaaaとなる
 * // エラー処理
 * @endcode
 *
 * @param string_ 連結元文字列
 * @param dst_ 連結先文字列
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - dst_ == NULL
 * - string_ == NULL
 * @retval CHOCO_STRING_BAD_OPERATION 連結先文字列と連結元文字列の先頭アドレスが等しい(自己連結は禁止する)
 * @retval CHOCO_STRING_DATA_CORRUPTED 以下のいずれか
 * - string_の内部データが破損(アドレスへの不正アクセス等により発生)
 * - dst_の内部データが破損(アドレスへの不正アクセス等により発生)
 * @retval CHOCO_STRING_OVERFLOW 連結後の文字列長さがsize_tの上限を超過
 * @retval CHOCO_STRING_NO_MEMORY メモリ確保失敗
 * @retval CHOCO_STRING_LIMIT_EXCEEDED メモリ管理システムの管理変数が使用可能範囲を超過
 * @retval CHOCO_STRING_SUCCESS 文字列の連結に成功し,正常終了
 */
choco_string_result_t choco_string_concat(const choco_string_t* string_, choco_string_t* dst_);

/**
 * @brief dst_の文字列の末尾にstring_(const char*)の文字列を連結する
 *
 * @note
 * - string_が""の場合は何もしない
 * - dst_が管理するバッファの容量が足りない場合は,新規にバッファを取得し直す
 *
 * @code{.c}
 * choco_string_t* dst = NULL;
 * ret = choco_string_create_from_c_string(&dst, "bbb");
 * // エラー処理
 *
 * ret = choco_string_concat_from_c_string("aaa", dst);   // dstの文字列はbbbaaaとなる
 * // エラー処理
 * @endcode
 *
 * @param string_ 連結元文字列
 * @param dst_ 連結先文字列
 *
 * @retval CHOCO_STRING_INVALID_ARGUMENT 以下のいずれか
 * - dst_ == NULL
 * - string_ == NULL
 *
 * @retval CHOCO_STRING_DATA_CORRUPTED dst_の内部データが破損(アドレスへの不正アクセス等により発生)
 * @retval CHOCO_STRING_OVERFLOW 連結後の文字列長さがsize_tの上限を超過
 * @retval CHOCO_STRING_NO_MEMORY メモリ確保失敗
 * @retval CHOCO_STRING_LIMIT_EXCEEDED メモリ管理システムの管理変数が使用可能範囲を超過
 * @retval CHOCO_STRING_SUCCESS 文字列の連結に成功し,正常終了
 */
choco_string_result_t choco_string_concat_from_c_string(const char* string_, choco_string_t* dst_);

/**
 * @brief string_が管理する文字列の長さを取得する(終端文字を含まない長さが返される)
 *
 * @note
 * - string_がNULLまたはstring_内部管理バッファサイズが0の場合は0を返す
 *
 * @code{.c}
 * choco_string_t* string = NULL;
 * choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaaaa");
 *
 * // エラー処理
 *
 * const size_t len = choco_string_length(string);  // len == 5
 *
 * choco_string_destroy(&string);
 * @endcode
 *
 * @param string_ 文字列長さ取得元構造体インスタンス
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
 * choco_string_result_t ret = choco_string_create_from_c_string(&string, "aaaaa");
 *
 * // エラー処理
 *
 * const char* c_ptr = choco_string_c_str(string);
 * fprintf(stdout, "%s\n", c_ptr);  // 標準出力にaaaaaが出力される
 *
 * choco_string_destroy(&string);
 * @endcode
 *
 * @param string_ 文字列先頭アドレス取得元構造体インスタンス
 * @return const char* 文字列先頭アドレス
 */
const char* choco_string_c_str(const choco_string_t* string_);

#ifdef __cplusplus
}
#endif
#endif
