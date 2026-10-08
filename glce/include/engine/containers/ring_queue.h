// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/**
 * @ingroup containers
 *
 * @file ring_queue.h
 * @author chocolate-pie24
 * @brief 固定された要素形式をFIFO順に保持するジェネリックリングキューを提供する
 *
 * @details
 * Ring Queue moduleは、生成時に指定された要素形式のdataをFIFO順に保持する`ring_queue_t`と、その基本操作を提供する。
 *
 * `ring_queue_t`は内部表現を公開しないopaque objectであり、callerは`ring_queue_t*`を通してmodule APIを利用する。
 *
 * @section ring_queue_boundary_contract Module Boundary Contract
 *
 * - `ring_queue_t`はopaque typeとして公開し、内部表現をmodule外部へ公開しない。
 * - Ring Queue moduleは、`ring_queue_t` object自身のstorageおよび
 *   queue内部で要素を保持するために必要なstorageのownershipを管理する。
 * - callerは`ring_queue_t` object自身のstorageまたはqueue内部storageを直接解放しない。
 * - module APIへ渡す`ring_queue_t*`は、Ring Queue moduleによって生成され、
 *   lifetime中にあるobjectを参照するものとする。
 *
 * - 一つの`ring_queue_t`が扱う要素のsizeおよびalignment requirementは
 *   object生成時に固定され、objectのlifetime中は変更しない。
 * - 一つのqueue内へ異なる要素形式を混在させない。
 * - Ring Queue moduleは要素dataをbyte sequenceとしてcopyし、
 *   callerが渡した要素storageへのpointerを保持しない。
 * - 要素data内部にpointer等が含まれる場合も、Ring Queue moduleがcopyするのは
 *   その値のrepresentationのみであり、そのpointerが参照するresourceのownershipは取得しない。
 *
 * - queueはFIFO semanticsを持つ。
 * - queueが最大要素数に達した状態で新しい要素を追加した場合、
 *   最古の要素を破棄し、新しい要素を格納する。
 * - queueへ格納可能な最大要素数はobject生成時に固定され、
 *   objectのlifetime中は変更しない。
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
    RING_QUEUE_LIMIT_EXCEEDED,      /**< システム使用可能範囲上限超過 */
    RING_QUEUE_BAD_OPERATION,       /**< API誤用 */
    RING_QUEUE_DATA_CORRUPTED,      /**< 内部データ破損 */
    RING_QUEUE_OVERFLOW,            /**< 計算過程のオーバーフロー */
    RING_QUEUE_EMPTY,               /**< リングキューが空 */
} ring_queue_result_t;

/**
 * @brief out_queue_のメモリを確保し、容量max_element_count_で初期化する
 *
 * @note 初期化されたリングキューに格納するデータのサイズとアライメント要件はelement_size_,element_align_で固定化される
 *
 * @param[in] max_element_count_ 要素を格納可能な最大個数
 * @param[in] element_size_ 格納する要素のサイズ
 * @param[in] element_align_ 格納する要素のアライメント要件(2のべき乗 かつ max_align_t以下でなければいけない)
 * @param[out] ring_queue_ 初期化対象構造体インスタンスへのダブルポインタ
 *
 * @retval RING_QUEUE_INVALID_ARGUMENT 以下のいずれか
 * - out_queue_ == NULL
 * - *out_queue_ != NULL
 * - 0 == max_element_count_
 * - 0 == element_size_
 * - element_align_が2の冪乗ではない
 * - element_align_がmax_align_tを超過
 * @retval RING_QUEUE_OVERFLOW 処理過程でオーバーフローが発生
 * @retval RING_QUEUE_NO_MEMORY メモリ不足によりメモリ確保失敗
 * @retval RING_QUEUE_LIMIT_EXCEEDED メモリ管理システムのリソースがシステム使用可能範囲上限を超過
 * @retval RING_QUEUE_BAD_OPERATION メモリシステム未初期化
 * @retval RING_QUEUE_SUCCESS 初期化に成功し、正常終了
 */
ring_queue_result_t ring_queue_create(size_t max_element_count_, size_t element_size_, size_t element_align_, ring_queue_t** out_queue_);

/**
 * @brief ring_queue_が管理しているメモリと自身のメモリを解放し、*queue_ = NULLにする
 *
 * @warning 内部データが破損している場合にはmemory_poolの破棄は行わず、メモリリークとなる
 *
 * @note
 * - 2重デストロイ許可
 * - queue_ == NULLの場合はno-op
 * - *queue_ == NULLの場合はno-op
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
 * @param queue_ メモリ破棄対象構造体インスタンスへのダブルポインタ
 */
void ring_queue_destroy(ring_queue_t** queue_);

/**
 * @brief ring_queue_にdata_をpushする
 *
 * @note キューが満杯だった場合は以下の動作となる、
 * - ワーニングメッセージを出力する(DEBUG_BUILD,TEST_BUILD時のみ)
 * - 最古のデータを捨てて新しいデータを格納する(返り値はRING_QUEUE_SUCCESS)
 *
 * @param[in] data_ 格納データへのポインタ
 * @param[in] element_size_ 格納データサイズ(create時と異なる型ではないかをチェックするため)
 * @param[in] element_align_ 格納データアライメント要件(create時と異なる型ではないかをチェックするため)
 * @param[in,out] queue_ データをpushするリングキュー構造体インスタンスへのポインタ
 *
 * @retval RING_QUEUE_INVALID_ARGUMENT 以下のいずれか
 * - queue_ == NULL
 * - data_ == NULL
 * - データ格納キューが未初期化
 * - element_size_がring_queue_createを実行した時の値と異なる
 * - element_align_がring_queue_createを実行した時の値と異なる
 * @retval RING_QUEUE_SUCCESS          データの格納に成功し、正常終了(キューが満杯で古いデータを捨てて新しいデータを格納した場合でも成功となる)
 */
ring_queue_result_t ring_queue_push(const void* data_, size_t element_size_, size_t element_align_, ring_queue_t* queue_);

/**
 * @brief ring_queue_からout_data_にデータをpopする
 *
 * @param[in] element_size_ 格納データサイズ(create時と異なる型ではないかをチェックするため)
 * @param[in] element_align_ 格納データアライメント要件(create時と異なる型ではないかをチェックするため)
 * @param[in,out] queue_ データをpopするリングキュー構造体インスタンスへのポインタ
 * @param[out] out_data_ popしたデータの格納先アドレス
 *
 * @retval RING_QUEUE_INVALID_ARGUMENT 以下のいずれか
 * - queue_ == NULL
 * - out_data_ == NULL
 * - データ格納キューが未初期化
 * - element_size_がring_queue_createを実行した時の値と異なる
 * - element_align_がring_queue_createを実行した時の値と異なる
 * @retval RING_QUEUE_EMPTY            ring_queueが空
 * @retval RING_QUEUE_SUCCESS          データの取得に成功し、正常終了
 */
ring_queue_result_t ring_queue_pop(size_t element_size_, size_t element_align_, ring_queue_t* queue_, void* out_data_);

/**
 * @brief リングキューが空かを判定する
 *
 * @note
 * - 引数で与えたring_queue_がNULLの場合は何もせず、true(=空)を返す
 *
 * @param queue_ 判定対象リングキュー
 *
 * @return true リングキューが空
 * @return false リングキューが空ではない
 */
bool ring_queue_is_empty(const ring_queue_t* queue_);

bool ring_queue_is_valid(const ring_queue_t* queue_);

#ifdef __cplusplus
}
#endif
#endif
