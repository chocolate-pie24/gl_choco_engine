// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_MEMORY_GENERAL_ALLOCATOR_GENERAL_ALLOCATOR_H
#define GLCE_ENGINE_MEMORY_GENERAL_ALLOCATOR_GENERAL_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct general_allocator general_allocator_t;

typedef enum {
    GENERAL_ALLOCATOR_SUCCESS = 0,
    GENERAL_ALLOCATOR_DATA_CORRUPTED,
    GENERAL_ALLOCATOR_BAD_OPERATION,
    GENERAL_ALLOCATOR_INVALID_ARGUMENT,
    GENERAL_ALLOCATOR_NO_MEMORY,
    GENERAL_ALLOCATOR_OVERFLOW,
    GENERAL_ALLOCATOR_UNDEFINED_ERROR,
} general_allocator_result_t;

typedef enum {
    GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM = 0,  /**< メモリタグ: システム系 */
    GENERAL_ALLOCATOR_MEMORY_TAG_STRING,      /**< メモリタグ: 文字列系 */
    GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE,  /**< メモリタグ: リングキュー */
    GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER,    /**< メモリタグ: レンダラー */
    GENERAL_ALLOCATOR_MEMORY_TAG_FILE_IO,     /**< メモリタグ: ファイルI/O */
    GENERAL_ALLOCATOR_MEMORY_TAG_CAMERA,      /**< メモリタグ: カメラシステム */
    GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE,     /**< メモリタグ: テクスチャ */
    GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY,    /**< メモリタグ: ジオメトリ */
    GENERAL_ALLOCATOR_MEMORY_TAG_MAX,         /**< メモリタグカウント用max値 */
} general_allocator_memory_tag_t;

general_allocator_result_t general_allocator_create(void);

void general_allocator_destroy(void);

general_allocator_result_t general_allocator_allocate(size_t allocation_size_, general_allocator_memory_tag_t memory_tag_, void** out_ptr_);

void general_allocator_free(void** ptr_, general_allocator_memory_tag_t memory_tag_);

bool general_allocator_is_valid(void);

#ifdef __cplusplus
}
#endif
#endif
