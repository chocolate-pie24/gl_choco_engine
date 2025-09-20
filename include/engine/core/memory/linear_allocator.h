/** @addtogroup core_memory
 * @{
 *
 * @file linear_allocator.h
 * @author chocolate-pie24
 * @brief linear_alloc_tオブジェクトの定義と関連APIの宣言
 *
 * @details
 * メモリアロケータの一つであるLinearAllocatorを実装。特徴は、
 *
 * - 各リソースの個別解放はできず、アロケータが管理するメモリ領域の一括解放しかできない
 * - 個別解放をしないことで各リソースごとの割り当て領域管理が不要で高速な割り当てが可能
 *
 * gl_choco_engineでは、起動時の各サブシステム用メモリの取得に使用する
 *
 * @note
 * linear_alloc_tオブジェクトは、内部データを隠蔽している \n
 * このため、linear_alloc_t型で変数を宣言することはできない \n
 * 使用の際は、linear_alloc_t*型で宣言すること
 *
 * @todo 必要であればmemory_systemへの統合を考える
 *
 * @version 0.1
 * @date 2025-09-16
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef GLCE_ENGINE_CORE_MEMORY_LINEAR_ALLOCATOR_H
#define GLCE_ENGINE_CORE_MEMORY_LINEAR_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief linear_allocator構造体前方宣言
 * @note 内部データ構造はlinear_allocator.cで定義し、外部からは隠蔽する
 */
typedef struct linear_alloc linear_alloc_t;

/**
 * @brief linear_allocator APIエラーコード
 *
 */
typedef enum {
    LINEAR_ALLOC_SUCCESS,           /**< 処理成功 */
    LINEAR_ALLOC_NO_MEMORY,         /**< メモリ不足 */
    LINEAR_ALLOC_INVALID_ARGUMENT,  /**< 無効な引数 */
} linear_alloc_err_t;

/**
 * @brief linear_alloc_tオブジェクトを生成する
 *
 * @note 使用の際の前提条件
 * - 引数allocator_はNULLで初期化したlinear_alloc_t*型変数のアドレスを渡すこと
 * - 引数capacity_は0より大きい値を使用すること
 *
 * 使用例:
 * @code
 * linear_alloc_t* alloc = NULL;    // 必ずNULL初期化をすること
 * linear_alloc_err_t ret = linear_allocator_create(&alloc, 128);   // 128byteの容量でアロケータを生成
 * // エラー処理
 * linear_allocator_destroy(&alloc);    // オブジェクトを破棄
 * @endcode
 *
 * @param[out] allocator_ linear_alloc_t*型オブジェクトへのポインタ(create内でsizeof(linear_alloc_t)のメモリを確保するためダブルポインタを使用)
 * @param[in] capacity_ アロケータ保有メモリ容量(byte)
 *
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT 引数allocator_ == NULL
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT 引数*allocator_ != NULL
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT 引数capacity_ == 0
 * @retval LINEAR_ALLOC_NO_MEMORY        メモリ確保失敗
 * @retval LINEAR_ALLOC_SUCCESS          オブジェクトの生成に成功し正常終了
 */
linear_alloc_err_t linear_allocator_create(linear_alloc_t** allocator_, size_t capacity_);

/**
 * @brief linear_alloc_tオブジェクトを破棄する
 *
 * @note 本APIを使用することで、linear_alloc_t*型変数が有しているメモリも破棄され、*allocator_にはNULLが代入される
 *
 * @note 下記の場合は何もしない
 * - allocator_がNULL(linear_allocator_destroy(NULL)のケース)
 * - *allocator_がNULL(2重destroyをしたケース)
 *
 * 使用例:
 * @code
 * linear_alloc_t* alloc = NULL;    // 必ずNULL初期化をすること
 * linear_alloc_err_t ret = linear_allocator_create(&alloc, 128);   // 128byteの容量でアロケータを生成
 * // エラー処理
 * linear_allocator_destroy(&alloc);    // オブジェクトを破棄(これでalloc == NULLになる)
 * @endcode
 *
 * @param[in,out] allocator_ linear_alloc_t*型オブジェクトへのポインタ(destroy内でallocator_が有しているメモリを解放するためダブルポインタを使用)
 */
void linear_allocator_destroy(linear_alloc_t** allocator_);

/**
 * @brief linear_allocatorを使用してメモリを割り当てる
 *
 *
 * @note 下記の場合は何もしない
 * - req_size_ == 0 または req_aling_ == 0(結果はLINEAR_ALLOC_SUCCESSでワーニングメッセージを出力)
 *
 * 使用例:
 * @code
 * linear_alloc_t* alloc = NULL;    // 必ずNULL初期化をすること
 * linear_alloc_err_t ret = linear_allocator_create(&alloc, 128);   // 128byteの容量でアロケータを生成
 * // エラー処理
 *
 * int* int_ptr = NULL;
 * ret = linear_allocator_allocate(alloc, sizeof(int * 8), alignof(int), &int_ptr);   // int型で8個分メモリ確保
 * // エラー処理
 *
 * linear_allocator_destroy(&alloc);    // オブジェクトを破棄(これでalloc == NULLになる, int_ptrの再利用は不可)
 * @endcode
 *
 * @param[in] allocator_ linear_alloc_t型オブジェクトへのポインタ
 * @param[in] req_size_ 割り当て要領(byte)
 * @param[in] req_align_ 割り当てるオブジェクトのアライメント要件
 * @param[out] out_ptr_ 割り当てたアドレスを格納する
 *
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT 引数allocator_ == NULL
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT 引数out_ptr_ == NULL
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT 引数*out_ptr != NULL
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT req_align_が2の冪乗ではない
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT 割り当て先頭アドレス+size_がUINTPTR_MAXの値を超過する
 * @retval LINEAR_ALLOC_NO_MEMORY        割り当てるメモリ領域がallocator_が保有しているメモリ領域に収まらない
 * @retval LINEAR_ALLOC_SUCCESS          メモリ割り当てに成功し正常終了
 */
linear_alloc_err_t linear_allocator_allocate(linear_alloc_t* allocator_, size_t req_size_, size_t req_align_, void** out_ptr_);

#ifdef __cplusplus
}
#endif
#endif

/*@}*/
