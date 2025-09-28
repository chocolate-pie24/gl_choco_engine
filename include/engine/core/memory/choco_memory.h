/** @addtogroup core_memory
 * @{
 *
 * @file choco_memory.h
 * @author chocolate-pie24
 * @brief memory_system_tオブジェクトの定義と関連APIの宣言
 *
 * @details
 * 不定期に発生するメモリ確保要求に対するメモリ確保と、メモリトラッキング機能を提供する \n
 * メモリ確保は現状はmallocをラップしたAPIによって行う。将来的にはFreeListを実装予定 \n
 * メモリトラッキングは、メモリータグごとに確保されたメモリ量を管理する \n
 * メモリータグは @ref memory_tag_t を参照 \n
 *
 * @note
 * memory_system_tオブジェクトは、内部データを隠蔽している \n
 * このため、memory_system_t型で変数を宣言することはできない \n
 * 使用の際は、memory_system_t*型で宣言すること
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef GLCE_ENGINE_CORE_MEMORY_CHOCO_MEMORY_H
#define GLCE_ENGINE_CORE_MEMORY_CHOCO_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief メモリータグリスト
 *
 */
typedef enum {
    MEMORY_TAG_SYSTEM,  /**< メモリータグ: システム系 */
    MEMORY_TAG_STRING,  /**< メモリータグ: 文字列系 */
    MEMORY_TAG_MAX,     /**< メモリータグカウント用max値 */
} memory_tag_t;

/**
 * @brief メモリーシステムエラーコードリスト
 *
 */
typedef enum {
    MEMORY_SYSTEM_SUCCESS,          /**< メモリーシステム成功 */
    MEMORY_SYSTEM_INVALID_ARGUMENT, /**< 無効な引数 */
    MEMORY_SYSTEM_RUNTIME_ERROR,    /**< 実行時エラー */
    MEMORY_SYSTEM_NO_MEMORY,        /**< メモリ不足 */
} memory_sys_err_t;

/**
 * @brief memory_system構造体前方宣言
 * @note 内部データ構造はchoco_memory.cで定義し、外部からは隠蔽する
 */
typedef struct memory_system memory_system_t;

/**
 * @brief memory_system_tのメモリ確保のため、memory_system_tの必要メモリ量とメモリアライメント要件を取得する
 *
 * @note
 * 下記の理由から本APIと @ref memory_system_init() を使用した2段階でのシステムの初期化が必要
 * - memory_system_tのメモリは @ref linear_alloc_t を使用して外部で確保する
 * - @ref choco_memory.h は @ref linear_alloc_t には依存しない
 * - memory_system_tの内部構造を隠蔽しているため、外部システムはmemory_system_tのメモリ要件、メモリアライメント要件を知らない \n
 * なお、memory_systemは起動時から終了時まで存在し続けることを想定している \n
 * このため、不定期解放が必要なメモリアロケーションではなく、linear_allocatorによるメモリ確保を行った
 *
 * 使用例:
 * @code
 * size_t memory = 0;
 * size_t align = 0;
 * memory_system_preinit(&memory, &aling);
 * @endcode
 *
 * @param[out] memory_requirement_ memory_system_tに必要なメモリ量格納先
 * @param[out] alignment_requirement_ memory_system_tのメモリアライメント要件
 *
 * @see memory_system_init
 */
void memory_system_preinit(size_t* const memory_requirement_, size_t* const alignment_requirement_);

/**
 * @brief memory_system_preinitで取得したmemory_requirement, alignment_requirementを元に外部で確保したメモリを私、initで内部データを初期化する
 *
 * @note
 * 本APIを使用する前に必ず @ref memory_system_preinit() を実行し、memory_system_のメモリを確保してから実行すること
 *
 * 使用例:
 * @code
 * // memory_system_tオブジェクトメモリを割り当てるためのlinear_alloc_tオブジェクトの作成
 * linear_alloc_t* linear_allocator = NULL;
 * linear_alloc_err_t ret_linear_alloc = linear_allocator_create(&linear_allocator, 1 * KIB);   // 1Kibでメモリ確保
 * if(LINEAR_ALLOC_NO_MEMORY == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
 *      // エラー処理
 * }
 *
 * // memory_system_tのメモリを割り当て
 * size_t memory = 0;
 * size_t align = 0;
 * memory_system_preinit(&memory, &align);
 * void* memory_system_ptr = NULL;
 * linear_alloc_err_t ret_memory_system_allocate = linear_allocator_allocate(linear_allocator, memory, align, &memory_system_ptr);
 * if(LINEAR_ALLOC_NO_MEMORY == ret_memory_system_allocate) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_memory_system_allocate) {
 *      // エラー処理
 * }
 *
 * // memory_system初期化
 * memory_sys_err_t ret_memory_system_init = memory_system_init(memory_system_ptr);
 * if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_memory_system_init) {
 *      // エラー処理
 * }
 * @endcode
 *
 * @param[in,out] memory_system_ 初期化対象オブジェクト
 *
 * @retval MEMORY_SYSTEM_INVALID_ARGUMENT memory_system_がNULL
 * @retval MEMORY_SYSTEM_SUCCESS memory_system_の初期化に成功し正常終了
 *
 * @see memory_system_preinit
 * @see linear_allocator_create
 * @see linear_allocator_allocate
 */
memory_sys_err_t memory_system_init(memory_system_t* memory_system_);

/**
 * @brief memory_system_の内部状態を解放し初期化する
 *
 * @note 本APIは内部状態のみの破棄であるため、memory_system_本体のメモリは解放されない
 *
 * 使用例:
 * @code
 * // memory_system_tオブジェクトメモリを割り当てるためのlinear_alloc_tオブジェクトの作成
 * linear_alloc_t* linear_allocator = NULL;
 * linear_alloc_err_t ret_linear_alloc = linear_allocator_create(&linear_allocator, 1 * KIB);   // 1Kibでメモリ確保
 * if(LINEAR_ALLOC_NO_MEMORY == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
 *      // エラー処理
 * }
 *
 * // memory_system_tのメモリを割り当て
 * size_t memory = 0;
 * size_t align = 0;
 * memory_system_preinit(&memory, &align);
 * void* memory_system_ptr = NULL;
 * linear_alloc_err_t ret_memory_system_allocate = linear_allocator_allocate(linear_allocator, memory, align, &memory_system_ptr);
 * if(LINEAR_ALLOC_NO_MEMORY == ret_memory_system_allocate) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_memory_system_allocate) {
 *      // エラー処理
 * }
 *
 * // memory_system初期化
 * memory_sys_err_t ret_memory_system_init = memory_system_init(memory_system_ptr);
 * if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_memory_system_init) {
 *      // エラー処理
 * }
 *
 * // オブジェクトの破棄
 * memory_system_destroy(memory_system_ptr);
 * linear_allocator_destroy(&linear_allocator);    // linear_allocatorを使用してメモリを割り当てたため、linear_allocatorのみの破棄で良い
 * @endcode
 *
 * @param[in,out] memory_system_ 初期化対象オブジェクト
 *
 * @see memory_system_preinit
 * @see linear_allocator_create
 * @see linear_allocator_allocate
 * @see linear_allocator_destroy
 */
void memory_system_destroy(memory_system_t* memory_system_);

/**
 * @brief memory_system_を使用してメモリを割り当てる
 *
 * @note 割り当ての際にはmemory_tag_tを指定することで、各メモリータグごとの合計割り当てサイズと総メモリ割り当てサイズをトラッキングする
 *
 * @param[in,out] memory_system_ memory_system_tハンドル
 * @param[in] size_ 割り当てサイズ
 * @param[in] mem_tag_ メモリータグ
 * @param[out] out_ptr_ 割り当てたメモリ
 *
 * 使用例:
 * @code
 * // memory_system_tオブジェクトメモリを割り当てるためのlinear_alloc_tオブジェクトの作成
 * linear_alloc_t* linear_allocator = NULL;
 * linear_alloc_err_t ret_linear_alloc = linear_allocator_create(&linear_allocator, 1 * KIB);   // 1Kibでメモリ確保
 * if(LINEAR_ALLOC_NO_MEMORY == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
 *      // エラー処理
 * }
 *
 * // memory_system_tのメモリを割り当て
 * size_t memory = 0;
 * size_t align = 0;
 * memory_system_preinit(&memory, &align);
 * void* memory_system_ptr = NULL;
 * linear_alloc_err_t ret_memory_system_allocate = linear_allocator_allocate(linear_allocator, memory, align, &memory_system_ptr);
 * if(LINEAR_ALLOC_NO_MEMORY == ret_memory_system_allocate) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_memory_system_allocate) {
 *      // エラー処理
 * }
 *
 * // memory_system初期化
 * memory_sys_err_t ret_memory_system_init = memory_system_init(memory_system_ptr);
 * if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_memory_system_init) {
 *      // エラー処理
 * }
 *
 * // メモリ割り当て
 * void* ptr = NULL;
 * memory_sys_err_t ret_allocate = memory_system_allocate(memory_system_ptr, 128, MEMORY_TAG_SYSTEM, &ptr); // 128byte割り当て
 *
 * // オブジェクトの破棄
 * memory_system_destroy(memory_system_ptr);
 * linear_allocator_destroy(&linear_allocator);    // linear_allocatorを使用してメモリを割り当てたため、linear_allocatorのみの破棄で良い
 * @endcode
 *
 * @retval MEMORY_SYSTEM_INVALID_ARGUMENT memory_system_ == NULL
 * @retval MEMORY_SYSTEM_INVALID_ARGUMENT out_ptr == NULL
 * @retval MEMORY_SYSTEM_INVALID_ARGUMENT *out_ptr != NULL
 * @retval MEMORY_SYSTEM_INVALID_ARGUMENT mem_tag_ >= MEMORY_TAG_MAX
 * @retval MEMORY_SYSTEM_INVALID_ARGUMENT 割り当てサイズを割り当てた結果、mem_tag_allocatedがSIZE_MAX超過
 * @retval MEMORY_SYSTEM_INVALID_ARGUMENT 割り当てサイズを割り当てた結果、total_allocatedがSIZE_MAX超過
 * @retval MEMORY_SYSTEM_SUCCESS          size_t == 0または割り当てに成功し正常終了
 *
 * @see memory_tag_t
 */
memory_sys_err_t memory_system_allocate(memory_system_t* memory_system_, size_t size_, memory_tag_t mem_tag_, void** out_ptr_);

/**
 * @brief memory_system_を使用してメモリを解放する
 *
 * 使用例:
 * @code
 * // memory_system_tオブジェクトメモリを割り当てるためのlinear_alloc_tオブジェクトの作成
 * linear_alloc_t* linear_allocator = NULL;
 * linear_alloc_err_t ret_linear_alloc = linear_allocator_create(&linear_allocator, 1 * KIB);   // 1Kibでメモリ確保
 * if(LINEAR_ALLOC_NO_MEMORY == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
 *      // エラー処理
 * }
 *
 * // memory_system_tのメモリを割り当て
 * size_t memory = 0;
 * size_t align = 0;
 * memory_system_preinit(&memory, &align);
 * void* memory_system_ptr = NULL;
 * linear_alloc_err_t ret_memory_system_allocate = linear_allocator_allocate(linear_allocator, memory, align, &memory_system_ptr);
 * if(LINEAR_ALLOC_NO_MEMORY == ret_memory_system_allocate) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_memory_system_allocate) {
 *      // エラー処理
 * }
 *
 * // memory_system初期化
 * memory_sys_err_t ret_memory_system_init = memory_system_init(memory_system_ptr);
 * if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_memory_system_init) {
 *      // エラー処理
 * }
 *
 * // メモリ割り当て
 * void* ptr = NULL;
 * memory_sys_err_t ret_allocate = memory_system_allocate(memory_system_ptr, 128, MEMORY_TAG_SYSTEM, &ptr); // 128byte割り当て
 *
 * // メモリ解放
 * memory_system_free(memory_system_ptr, ptr, 128, MEMORY_TAG_SYSTEM);
 *
 * // オブジェクトの破棄
 * memory_system_destroy(memory_system_ptr);
 * linear_allocator_destroy(&linear_allocator);    // linear_allocatorを使用してメモリを割り当てたため、linear_allocatorのみの破棄で良い
 * @endcode
 *
 * @param[in,out] memory_system_ memory_system_tハンドル
 * @param ptr_ 解放メモリアドレス
 * @param size_ 解放サイズ
 * @param mem_tag_ メモリータグ
 *
 * @see memory_tag_t
 */
void memory_system_free(memory_system_t* memory_system_, void* ptr_, size_t size_, memory_tag_t mem_tag_);

/**
 * @brief memory_system_が管理しているメモリ確保状態を標準出力に出力する
 *
 * 使用例:
 * @code
 * // memory_system_tオブジェクトメモリを割り当てるためのlinear_alloc_tオブジェクトの作成
 * linear_alloc_t* linear_allocator = NULL;
 * linear_alloc_err_t ret_linear_alloc = linear_allocator_create(&linear_allocator, 1 * KIB);   // 1Kibでメモリ確保
 * if(LINEAR_ALLOC_NO_MEMORY == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_linear_alloc) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
 *      // エラー処理
 * }
 *
 * // memory_system_tのメモリを割り当て
 * size_t memory = 0;
 * size_t align = 0;
 * memory_system_preinit(&memory, &align);
 * void* memory_system_ptr = NULL;
 * linear_alloc_err_t ret_memory_system_allocate = linear_allocator_allocate(linear_allocator, memory, align, &memory_system_ptr);
 * if(LINEAR_ALLOC_NO_MEMORY == ret_memory_system_allocate) {
 *      // エラー処理
 * } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_memory_system_allocate) {
 *      // エラー処理
 * }
 *
 * // memory_system初期化
 * memory_sys_err_t ret_memory_system_init = memory_system_init(memory_system_ptr);
 * if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_memory_system_init) {
 *      // エラー処理
 * }
 *
 * // メモリ割り当て
 * void* ptr = NULL;
 * memory_sys_err_t ret_allocate = memory_system_allocate(memory_system_ptr, 128, MEMORY_TAG_SYSTEM, &ptr); // 128byte割り当て
 *
 * // メモリ割り当て状態をレポート
 * memory_system_report(memory_system_ptr);
 *
 * // メモリ解放
 * memory_system_free(memory_system_ptr, ptr, 128, MEMORY_TAG_SYSTEM);
 *
 * // オブジェクトの破棄
 * memory_system_destroy(memory_system_ptr);
 * linear_allocator_destroy(&linear_allocator);    // linear_allocatorを使用してメモリを割り当てたため、linear_allocatorのみの破棄で良い
 * @endcode
 *
 * @param memory_system_ 出力対象memory_system_tハンドル
 */
void memory_system_report(const memory_system_t* memory_system_);

#ifdef __cplusplus
}
#endif
#endif

/*@}*/
