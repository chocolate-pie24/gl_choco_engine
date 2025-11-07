/** @ingroup linear_allocator
 *
 * @file linear_allocator.h
 * @author chocolate-pie24
 * @brief サブシステム等、ライフサイクルが固定で、個別のメモリ開放が不要なメモリ確保に対応するリニアアロケータモジュールの定義
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
 * linear_alloc_t構造体は、内部データを隠蔽している \n
 * このため、linear_alloc_t型で変数を宣言することはできない \n
 * 使用の際は、linear_alloc_t*型で宣言すること
 *
 * @todo linear_allocator_reset追加
 *
 * @version 0.1
 * @date 2025-09-16
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
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
 * @brief linear_allocator実行結果コードリスト
 *
 */
typedef enum {
    LINEAR_ALLOC_SUCCESS = 0,       /**< 処理成功 */
    LINEAR_ALLOC_NO_MEMORY,         /**< メモリ不足 */
    LINEAR_ALLOC_INVALID_ARGUMENT,  /**< 無効な引数 */
} linear_allocator_result_t;

/**
 * @brief リニアアロケータオブジェクトのメモリ確保のため、メモリアライメント要件とメモリ容量を取得する
 *
 * @note
 * - リニアアロケータのメモリ確保には、メモリシステムを使用する
 * - リニアアロケータモジュールがメモリシステムに依存しないよう、上位層にてリニアアロケータのメモリを確保するため本APIを使用する
 * - memory_requirement_ == NULLまたは、align_requirement_ == NULLの場合は何もしない
 * - 本APIを使用した後、リニアアロケータのメモリを確保し、linear_allocator_initを使用して初期化する
 *
 * 使用例:
 * @code{.c}
 * size_t memory_requirement = 0;   // メモリ使用量格納先
 * size_t align_requirement = 0;    // メモリアライメント要件格納先
 * linear_allocator_preinit(&memory_requirement, &align_requirement);
 * @endcode

 * @param[out] memory_requirement_ メモリ使用量格納先
 * @param[out] align_requirement_ メモリアライメント要件格納先
 *
 * @see memory_system_allocate
 * @see linear_allocator_init
 */
void linear_allocator_preinit(size_t* memory_requirement_, size_t* align_requirement_);

/**
 * @brief リニアアロケータオブジェクトを初期化する
 *
 * @note
 * - リニアアロケータオブジェクトは自身のメモリを破棄するAPIを持たない
 * - リニアアロケータオブジェクト自体の破棄は上位層で行う
 *
 * 使用例:
 * @code{.c}
 * linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
 * memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
 *
 * linear_alloc_t* linear_alloc = NULL;       // リニアアロケータ
 * void* linear_alloc_pool = NULL;            // リニアアロケータメモリプール先頭アドレス
 * size_t linear_alloc_pool_size = 1 * KIB;   // リニアアロケータメモリプール容量(1KiB)
 *
 * size_t linear_alloc_mem_req = 0;
 * size_t linear_alloc_align_req = 0;
 * linear_allocator_preinit(&linear_alloc_mem_req, &linear_alloc_align_req);
 *
 * // リニアアロケータの自体のメモリを確保
 * ret_mem = memory_system_allocate(linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&linear_alloc);
 *
 * // リニアアロケータのメモリプールメモリを確保
 * ret_mem = memory_system_allocate(linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &linear_alloc_pool);
 *
 * // リニアアロケータ初期化
 * ret_linear = linear_allocator_init(linear_alloc, linear_alloc_pool_size, linear_alloc_pool);
 *
 * // リニアアロケータメモリプールメモリ破棄
 * memory_system_free(linear_alloc_pool, linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
 *
 * // リニアアロケータメモリ破棄
 * memory_system_free(linear_alloc, linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
 * @endcode
 *
 * @param[in,out] allocator_ リニアアロケータオブジェクトアドレス
 * @param[in] capacity_ メモリプール容量(byte)
 * @param[in] memory_pool_ メモリプールアドレス
 *
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT 以下のいずれか
 * - 引数allocator_ == NULL
 * - 引数memory_pool_ == NULL
 * - 引数capacity_ == 0
 * @retval LINEAR_ALLOC_SUCCESS          リニアアロケータの初期化に成功し、正常終了
 *
 * @see linear_allocator_preinit
 * @see memory_system_allocate
 * @see memory_system_free
 */
linear_allocator_result_t linear_allocator_init(linear_alloc_t* allocator_, size_t capacity_, void* memory_pool_);

/**
 * @brief linear_allocatorを使用してメモリを割り当てる
 *
 *
 * @note 下記の場合は何もしない
 * - req_size_ == 0 または req_align_ == 0(結果はLINEAR_ALLOC_SUCCESSでワーニングメッセージを出力)
 *
 * @warning
 * - req_align_ == 0 または req_size_ == 0でワーニング出力し何もしない
 *
 * 使用例:
 * @code{.c}
 * linear_allocator_result_t ret_linear = LINEAR_ALLOC_INVALID_ARGUMENT;
 * memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
 *
 * linear_alloc_t* linear_alloc = NULL;       // リニアアロケータ
 * void* linear_alloc_pool = NULL;            // リニアアロケータメモリプール先頭アドレス
 * size_t linear_alloc_pool_size = 1 * KIB;   // リニアアロケータメモリプール容量(1kib)
 *
 * size_t linear_alloc_mem_req = 0;
 * size_t linear_alloc_align_req = 0;
 * linear_allocator_preinit(&linear_alloc_mem_req, &linear_alloc_align_req);
 *
 * // リニアアロケータの自体のメモリを確保
 * ret_mem = memory_system_allocate(linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&linear_alloc);
 *
 * // リニアアロケータのメモリプールメモリを確保
 * ret_mem = memory_system_allocate(linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &linear_alloc_pool);
 *
 * // リニアアロケータ初期化
 * ret_linear = linear_allocator_init(linear_alloc, linear_alloc_pool_size, linear_alloc_pool);
 *
 * // メモリ割り当て
 * void* ptr = NULL;
 * ret_linear = linear_allocator_allocate(linear_alloc, 128, 8, &ptr);  // ptrにアライメント8バイト, 容量128バイトでメモリ割り当て
 * @endcode
 *
 * @param[in] allocator_ linear_alloc_t型オブジェクトへのポインタ
 * @param[in] req_size_ 割り当て容量(byte)
 * @param[in] req_align_ 割り当てるオブジェクトのアライメント要件
 * @param[out] out_ptr_ 割り当てたアドレスを格納する
 *
 * @retval LINEAR_ALLOC_INVALID_ARGUMENT 以下のいずれか
 * - allocator_ == NULL
 * - out_ptr_ == NULL
 * - *out_ptr_ != NULL
 * - req_align_が2の冪乗ではない
 * - メモリを割り当てた場合、割り当て開始アドレスの値がオーバーフロー
 * - メモリ割り当て先頭アドレス+割り当てサイズの値がオーバーフロー
 * @retval LINEAR_ALLOC_NO_MEMORY        メモリを割り当てた場合、メモリプール内に収まらない
 * @retval LINEAR_ALLOC_SUCCESS 以下のいずれか
 * - req_align_ == 0 または req_size_ == 0でワーニング出力し何もしない
 * - メモリ割り当てに成功し正常終了
 *
 */
linear_allocator_result_t linear_allocator_allocate(linear_alloc_t* allocator_, size_t req_size_, size_t req_align_, void** out_ptr_);

#ifdef TEST_BUILD
/**
 * @brief テスト専用API(メモリ確保に失敗させる)
 *
 * @param malloc_fail_n_ 何回目のメモリ確保で失敗させるかを指定(1回目 = 0を指定)
 */
void linear_allocator_malloc_fail_set(size_t malloc_fail_n_);

/**
 * @brief テスト専用API(テスト設定値をリセットする)
 *
 */
void linear_allocator_malloc_fail_reset(void);
#endif

#ifdef __cplusplus
}
#endif
#endif

/** @}*/
