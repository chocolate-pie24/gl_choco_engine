/** @ingroup choco_memory
 *
 * @file choco_memory.h
 * @author chocolate-pie24
 * @brief 不定期に発生するメモリ確保、解放に対応するメモリアロケータモジュールの定義
 *
 * @details メモリトラッキング機能も有する
 *
 * @details
 * 不定期に発生するメモリ確保要求に対するメモリ確保と、メモリトラッキング機能を提供する \n
 * メモリ確保は現状はmallocをラップしたAPIによって行う。将来的にはFreeListを実装予定 \n
 * メモリトラッキングは、メモリタグごとに確保されたメモリ量を管理する \n
 * メモリタグは @ref memory_tag_t を参照 \n
 * なお、本APIで確保されるメモリは、全てmax_align_tにアライメントされている
 *
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_CORE_MEMORY_CHOCO_MEMORY_H
#define GLCE_ENGINE_CORE_MEMORY_CHOCO_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifdef TEST_BUILD
#include <stdint.h>
#endif

/**
 * @brief メモリタグリスト
 *
 */
typedef enum {
    MEMORY_TAG_SYSTEM,      /**< メモリタグ: システム系 */
    MEMORY_TAG_STRING,      /**< メモリタグ: 文字列系 */
    MEMORY_TAG_RING_QUEUE,  /**< メモリタグ: リングキュー */
    MEMORY_TAG_RENDERER,    /**< メモリタグ: レンダラー */
    MEMORY_TAG_FILE_IO,     /**< メモリタグ: ファイルI/O */
    MEMORY_TAG_CAMERA,      /**< メモリタグ: カメラ */
    MEMORY_TAG_MAX,         /**< メモリタグカウント用max値 */
} memory_tag_t;

/**
 * @brief メモリシステム実行結果コードリスト
 *
 */
typedef enum {
    MEMORY_SYSTEM_SUCCESS = 0,      /**< メモリシステム成功 */
    MEMORY_SYSTEM_INVALID_ARGUMENT, /**< 無効な引数 */
    MEMORY_SYSTEM_RUNTIME_ERROR,    /**< 実行時エラー */
    MEMORY_SYSTEM_LIMIT_EXCEEDED,   /**< メモリ使用量管理システムの使用量が使用範囲上限を超過 */
    MEMORY_SYSTEM_NO_MEMORY,        /**< メモリ不足 */
} memory_system_result_t;

/**
 * @brief メモリシステムを起動する
 *
 * @note
 * memory_system_createでは、シングルトンで定義されたメモリシステム状態管理構造体インスタンスを初期化する
 * このため、メモリシステムが既に初期化済みであった場合はMEMORY_SYSTEM_RUNTIME_ERRORを返す
 * memory_system_createを再度実行する際には、memory_system_destroyを呼び出してから使用すること
 *
 * 使用例:
 * @code{.c}
 * memory_system_result_t ret = memory_system_create();   // メモリシステム内部状態管理構造体インスタンスが初期化される
 * @endcode
 *
 * @retval MEMORY_SYSTEM_RUNTIME_ERROR メモリシステムが既に初期化済み
 * @retval MEMORY_SYSTEM_NO_MEMORY     メモリシステム用のメモリ確保に失敗
 * @retval MEMORY_SYSTEM_SUCCESS       メモリシステムの初期化に成功し、正常終了
 *
 * @see memory_system_destroy
 */
memory_system_result_t memory_system_create(void);

/**
 * @brief メモリシステムを停止する
 *
 * @note
 * memory_system_destroyでは、シングルトンで定義されたメモリシステム状態管理構造体インスタンスのメモリを破棄する
 * このため、memory_system_destroyを呼び出した後で、memory_system_allocate, memory_system_freeを呼び出すことはできない
 * 再度memory_system_allocate, memory_system_freeを使用する際には、memory_system_createを実行してからにすること
 *
 * @note
 * - 2重destroyは許可する
 * - この関数を呼び出した時点でメモリシステムが管理しているメモリ使用量が0でない場合は、ワーニングメッセージを出力し、メモリシステムを破棄する
 *
 * 使用例:
 * @code{.c}
 * memory_system_result_t ret = memory_system_create();
 * memory_system_destroy();
 * memory_system_destroy(); // 2重destroyは許可
 * @endcode
 *
 * @see memory_system_create
 */
void memory_system_destroy(void);

/**
 * @brief 容量size_のメモリを確保し、mem_tag_で指定されたメモリタグのメモリ使用量を更新する
 *
 * @note
 * - 割り当ての際にはmemory_tag_tを指定することで、各メモリタグごとの合計割り当てサイズと総メモリ割り当てサイズをトラッキングする
 * - 本関数を使用する前に、memory_system_createでメモリシステムの初期化を行うこと
 * - max_align_tでアラインされたメモリが割り当てられる
 *
 * @param[in] size_ 割り当てサイズ
 * @param[in] mem_tag_ メモリタグ
 * @param[out] out_ptr_ 割り当てたメモリ格納先(ダブルポインタを渡す)
 *
 * 使用例:
 * @code{.c}
 * memory_system_result_t ret = memory_system_create(); // メモリシステム初期化
 * // エラー処理
 *
 * // メモリ割り当て
 * void* ptr = NULL;
 * ret = memory_system_allocate(128, MEMORY_TAG_SYSTEM, &ptr); // 128バイト割り当て
 * // エラー処理
 * @endcode
 *
 * @retval MEMORY_SYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - メモリシステム未初期化
 * - out_ptr_ == NULL
 * - *out_ptr_ != NULL
 * - mem_tag_ >= MEMORY_TAG_MAX
 * @retval MEMORY_SYSTEM_LIMIT_EXCEEDED 以下のいずれか
 * - 割り当てサイズを割り当てた結果、mem_tag_allocatedがSIZE_MAX超過
 * - 割り当てサイズを割り当てた結果、total_allocatedがSIZE_MAX超過
 * @retval MEMORY_SYSTEM_NO_MEMORY        メモリ割り当て失敗
 * @retval MEMORY_SYSTEM_SUCCESS          size_ == 0または割り当てに成功し正常終了
 *
 * @see memory_tag_t
 * @see memory_system_create
 */
memory_system_result_t memory_system_allocate(size_t size_, memory_tag_t mem_tag_, void** out_ptr_);

/**
 * @brief ptr_が保持する領域のメモリを解放し、mem_tag_で指定されたメモリタグのメモリ使用量を更新する
 *
 * @note 引数にはvoid*型を渡しており、ptr_のメモリ開放後、NULLをセットすることはできない。
 * この仕様は、標準ライブラリのfree()の仕様に合わせた。なので、呼び出し側でメモリの解放後、NULLをセットすること。
 *
 * @note
 * - メモリシステムが未初期化の場合はワーニングを出力し、何もしない
 * - NULL == ptr_でワーニングを出力し、何もしない
 * - mem_tag_ >= MEMORY_TAG_MAXでワーニングを出力し、何もしない
 * - mem_tag_allocatedがマイナスとなる量をfreeしようとするとワーニングを出力し、何もしない
 * - total_allocatedがマイナスとなる量をfreeしようとするとワーニングを出力し、何もしない
 *
 *
 * 使用例:
 * @code{.c}
 * memory_system_result_t ret = memory_system_create(); // メモリシステム初期化
 *
 * // メモリ割り当て
 * void* ptr = NULL;
 * ret = memory_system_allocate(128, MEMORY_TAG_SYSTEM, &ptr); // 128バイト割り当て
 * // エラー処理
 *
 * // メモリ解放
 * memory_system_free(ptr, 128, MEMORY_TAG_SYSTEM);
 * ptr = NULL;
 * @endcode
 *
 * @param[in] ptr_ 解放メモリアドレス
 * @param[in] size_ 解放サイズ
 * @param[in] mem_tag_ メモリタグ
 *
 * @see memory_tag_t
 */
void memory_system_free(void* ptr_, size_t size_, memory_tag_t mem_tag_);

/**
 * @brief メモリシステムが管理しているメモリ使用量状態を標準出力に出力する
 *
 * @note
 * - メモリシステムが未初期化の場合はワーニングを出力し、何もしない
 *
 * 使用例:
 * @code{.c}
 * memory_system_result_t ret = memory_system_create(); // メモリシステム初期化
 *
 * // メモリ割り当て
 * void* ptr = NULL;
 * ret = memory_system_allocate(128, MEMORY_TAG_SYSTEM, &ptr); // 128バイト割り当て
 *
 * // メモリ割り当て状態をレポート
 * memory_system_report();
 * @endcode
 *
 */
void memory_system_report(void);

#ifdef TEST_BUILD
/**
 * @brief テスト用に何回目の呼び出しでtest_mallocを失敗させるかの回数を指定する
 *
 * @note
 * - 失敗した場合はtest_mallocはNULLを返す
 * - 1回目で失敗させる場合はmalloc_fail_n_ = 0とする
 *
 * @param malloc_fail_n_ 何回目のtest_mallocで失敗させるかを指定する
 */
void memory_system_test_param_set(int32_t malloc_fail_n_);

/**
 * @brief テスト用に、メモリーシステムの実行結果コードを指定値に固定する
 *
 * @note この関数を実行した後は、memory_system_test_param_resetが呼ばれるまで効果が継続する
 *
 * @param result_code_ 出力実行結果コード
 */
void memory_system_rslt_code_set(memory_system_result_t result_code_);

/**
 * @brief メモリシステムのテスト時専用構造体のフィールドを初期化する
 *
 */
void memory_system_test_param_reset(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
