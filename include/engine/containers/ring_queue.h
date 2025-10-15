/** @addtogroup containers_ring_queue
 * @{
 *
 * @file ring_queue.h
 * @author chocolate-pie24
 * @brief ジェネリック型のリングキューオブジェクト定義と関連APIを提供する
 *
 * @note
 * ring_queue_tオブジェクトは、内部データを隠蔽している \n
 * このため、ring_queue_t型で変数を宣言することはできない \n
 * 使用の際は、ring_queue_t*型で宣言すること
 *
 * @note
 * ring_queue_tに格納できるデータには、下記の制約を設ける。
 * この制約を設けることにより、バッファが満杯になった時の処理を簡便化することができる。
 * また、内部でmemory_systemを使用したメモリアロケーションを行うが、
 * memory_systemのメモリアロケーションで取得するメモリは、全てmax_align_tでアライメントされている必要がある。
 * - データのアライメント要件は2のべき乗でなければいけない
 * - データのアライメント要件はmax_align_t以下でなければいけない
 *
 * @note
 * ring_queue_tはジェネリック型のデータを格納可能であるが、複数のデータ型を混在して格納することはできない
 *
 * @version 0.1
 * @date 2025-10-14
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_CONTAINERS_RING_QUEUE_H
#define GLCE_ENGINE_CONTAINERS_RING_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief ring_queue_t前方宣言
 *
 */
typedef struct ring_queue ring_queue_t;

/**
 * @brief リングキューAPI実行結果コードリスト
 *
 */
typedef enum {
    RING_QUEUE_SUCCESS = 0,         /**< 処理成功 */
    RING_QUEUE_INVALID_ARGUMENT,    /**< 無効な引数 */
    RING_QUEUE_NO_MEMORY,           /**< メモリ不足 */
    RING_QUEUE_RUNTIME_ERROR,       /**< 実行時エラー */
    RING_QUEUE_UNDEFINED_ERROR,     /**< 未定義エラー */
    RING_QUEUE_EMPTY,               /**< リングキューが空 */
} ring_queue_result_t;

/**
 * @brief ring_queue_tオブジェクトのメモリを確保し、初期化を行いリングキューを使用可能な状態にする
 *
 * 使用例:
 * @code{.c}
 * ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
 * ring_queue_t* ring_queue = NULL;
 *
 * // int型のデータを格納するリングキュー初期化処理(格納要素数は8)
 * ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
 * @endcode
 *
 * @param[in] max_element_count_ 要素を格納可能な最大個数
 * @param[in] element_size_ 格納する要素のサイズ
 * @param[in] element_align_ 格納する要素のアライメント要件(2のべき乗 かつ max_align_t以下でなければいけない)
 * @param[out] ring_queue_ 初期化対象オブジェクト
 *
 * @retval RING_QUEUE_INVALID_ARGUMENT 以下のいずれか
 * - ring_queue_ == NULL
 * - *ring_queue_ != NULL
 * - 0 == max_element_count_
 * - 0 == element_size_
 * - element_align_が2の冪乗ではない
 * - element_align_がmax_align_tを超過
 * - 処理過程でオーバーフローが発生
 * @retval RING_QUEUE_NO_MEMORY        メモリ確保失敗
 * @retval RING_QUEUE_SUCCESS          初期化に成功し、正常終了
 */
ring_queue_result_t ring_queue_create(size_t max_element_count_, size_t element_size_, size_t element_align_, ring_queue_t** ring_queue_);

/**
 * @brief ring_queue_tオブジェクトが管理しているメモリと、ring_queue_t自身のメモリを破棄する
 *
 * @note
 * ring_queue_destroyは2重デストロイを許可する
 *
 * 使用例:
 * @code{.c}
 * ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
 * ring_queue_t* ring_queue = NULL;
 *
 * // int型のデータを格納するリングキュー初期化処理(格納要素数は8)
 * ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
 *
 * ring_queue_destroy(&ring_queue); // ring_queue = NULLになる
 * ring_queue_destroy(&ring_queue); // 2重デストロイ許可
 * @endcode
 *
 * @param ring_queue_ メモリ破棄対象オブジェクト
 */
void ring_queue_destroy(ring_queue_t** ring_queue_);

/**
 * @brief ring_queue_tにデータをpushする
 *
 * @note
 * - 本APIでは、キューが満杯だった際にpushすると、Ring queue is full; overwriting the oldest element.のメッセージを出力し、古いデータを捨てて新しいデータを格納する
 * - なお、上記メッセージはデバッグビルド時のみ出力される
 * - 満杯で古いデータを捨てた場合でも、返り値はRING_QUEUE_SUCCESSとなる
 *
 * 使用例:
 * @code{.c}
 * ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
 * ring_queue_t* ring_queue = NULL;
 *
 * // int型のデータを格納するリングキュー初期化処理(格納要素数は8)
 * ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
 *
 * int a = 0;
 * ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
 *
 * ring_queue_destroy(&ring_queue); // ring_queue = NULLになる
 * ring_queue_destroy(&ring_queue); // 2重デストロイ許可
 * @endcode
 *
 * @param ring_queue_ データをpushするリングキューオブジェクト
 * @param data_ 格納データへのポインタ
 * @param element_size_ 格納データサイズ(create時と異なる型ではないかをチェックするため)
 * @param element_align_ 格納データアライメント要件(create時と異なる型ではないかをチェックするため)
 *
 * @retval RING_QUEUE_INVALID_ARGUMENT 以下のいずれか
 * - ring_queue_ == NULL
 * - data_ == NULL
 * - データ格納バッファが未初期化
 * - element_size_がring_queue_createを実行した時の値と異なる
 * - element_align_がring_queue_createを実行した時の値と異なる
 * @retval RING_QUEUE_SUCCESS          データの格納に成功し、正常終了(バッファが満杯で古いデータを捨てて新しいデータを格納した場合でも成功となる)
 */
ring_queue_result_t ring_queue_push(ring_queue_t* ring_queue_, const void* data_, size_t element_size_, size_t element_align_);

/**
 * @brief ring_queue_tからデータをpopする
 *
 * 使用例:
 * @code{.c}
 * ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
 * ring_queue_t* ring_queue = NULL;
 *
 * // int型のデータを格納するリングキュー初期化処理(格納要素数は8)
 * ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
 *
 * int a = 10;
 * ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
 *
 * int b = 0;
 * ret = ring_queue_pop(ring_queue, &b, sizeof(int), alignof(int)); // b = 10になる
 *
 * ring_queue_destroy(&ring_queue); // ring_queue = NULLになる
 * ring_queue_destroy(&ring_queue); // 2重デストロイ許可
 * @endcode
 *
 * @param ring_queue_ データをpopするリングキューオブジェクト
 * @param data_ popしたデータの格納先アドレス
 * @param element_size_ 格納データサイズ(create時と異なる型ではないかをチェックするため)
 * @param element_align_ 格納データアライメント要件(create時と異なる型ではないかをチェックするため)
 *
 * @retval RING_QUEUE_INVALID_ARGUMENT 以下のいずれか
 * - ring_queue_ == NULL
 * - data_ == NULL
 * - データ格納バッファが未初期化
 * - element_size_がring_queue_createを実行した時の値と異なる
 * - element_align_がring_queue_createを実行した時の値と異なる
 * @retval RING_QUEUE_EMPTY            ring_queueが空
 * @retval RING_QUEUE_SUCCESS          データの取得に成功し、正常終了
 */
ring_queue_result_t ring_queue_pop(ring_queue_t* ring_queue_, void* data_, size_t element_size_, size_t element_align_);

/**
 * @brief リングキューが空かを判定する
 *
 * @note
 * - 引数で与えたring_queue_がNULLの場合は何もせず、true(=空)を返す
 *
 * 使用例:
 * @code{.c}
 * ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;
 * ring_queue_t* ring_queue = NULL;
 *
 * // int型のデータを格納するリングキュー初期化処理(格納要素数は8)
 * ret = ring_queue_create(8, sizeof(int), alignof(int), &ring_queue);
 *
 * bool empty = false;
 * empty = ring_queue_empty(ring_queue);   // empty == true
 *
 * int a = 10;
 * ret = ring_queue_push(ring_queue, &a, sizeof(int), alignof(int));
 *
 * empty = ring_queue_empty(ring_queue);   // empty == false
 *
 * int b = 0;
 * ret = ring_queue_pop(ring_queue, &b, sizeof(int), alignof(int)); // b = 10になる
 *
 * empty = ring_queue_empty(ring_queue);   // empty == true
 *
 * ring_queue_destroy(&ring_queue); // ring_queue = NULLになる
 * ring_queue_destroy(&ring_queue); // 2重デストロイ許可
 * @endcode
 *
 * @param ring_queue_ 判定対象リングキュー
 *
 * @return true リングキューが空
 * @return false リングキューが空ではない
 */
bool ring_queue_empty(const ring_queue_t* ring_queue_);

#ifdef __cplusplus
}
#endif
#endif

/** @}*/
