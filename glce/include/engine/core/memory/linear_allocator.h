// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup core
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
 * @date 2025-09-16
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

void linear_allocator_preinit(size_t* memory_requirement_, size_t* align_requirement_);

linear_allocator_result_t linear_allocator_initialize(linear_alloc_t* allocator_, size_t capacity_, void* memory_pool_);

linear_allocator_result_t linear_allocator_allocate(linear_alloc_t* allocator_, size_t req_size_, size_t req_align_, void** out_ptr_);

#ifdef __cplusplus
}
#endif
#endif
