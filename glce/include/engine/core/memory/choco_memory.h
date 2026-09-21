// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup core
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
 * @date 2025-09-20
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
    MEMORY_TAG_CAMERA,      /**< メモリタグ: カメラシステム */
    MEMORY_TAG_TEXTURE,     /**< メモリタグ: テクスチャ */
    MEMORY_TAG_GEOMETRY,    /**< メモリタグ: ジオメトリ */
    MEMORY_TAG_MAX,         /**< メモリタグカウント用max値 */
} memory_tag_t;

/**
 * @brief メモリシステム実行結果コードリスト
 *
 */
typedef enum {
    MEMORY_SYSTEM_SUCCESS = 0,      /**< メモリシステム成功 */
    MEMORY_SYSTEM_INVALID_ARGUMENT, /**< 無効な引数 */
    MEMORY_SYSTEM_LIMIT_EXCEEDED,   /**< メモリ使用量管理システムの使用量が使用範囲上限を超過 */
    MEMORY_SYSTEM_BAD_OPERATION,    /**< メモリシステムAPI誤用 */
    MEMORY_SYSTEM_NO_MEMORY,        /**< メモリ不足 */
} memory_system_result_t;

memory_system_result_t choco_memory_create(void);

void choco_memory_destroy(void);

memory_system_result_t choco_memory_allocate(size_t size_, memory_tag_t mem_tag_, void** out_ptr_);

void choco_memory_free(void* ptr_, size_t size_, memory_tag_t mem_tag_);

void memory_system_report(void);

#ifdef __cplusplus
}
#endif
#endif
