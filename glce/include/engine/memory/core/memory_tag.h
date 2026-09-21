// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_MEMORY_CORE_MEMORY_TAG_H
#define GLCE_ENGINE_MEMORY_CORE_MEMORY_TAG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief メモリタグリスト
 *
 */
typedef enum {
    MEMORY_TAG_SYSTEM = 0,  /**< メモリタグ: システム系 */
    MEMORY_TAG_STRING,      /**< メモリタグ: 文字列系 */
    MEMORY_TAG_RING_QUEUE,  /**< メモリタグ: リングキュー */
    MEMORY_TAG_RENDERER,    /**< メモリタグ: レンダラー */
    MEMORY_TAG_FILE_IO,     /**< メモリタグ: ファイルI/O */
    MEMORY_TAG_CAMERA,      /**< メモリタグ: カメラシステム */
    MEMORY_TAG_TEXTURE,     /**< メモリタグ: テクスチャ */
    MEMORY_TAG_GEOMETRY,    /**< メモリタグ: ジオメトリ */
    MEMORY_TAG_MAX,         /**< メモリタグカウント用max値 */
} memory_tag_t;

bool memory_tag_is_valid(memory_tag_t memory_tag_);

#ifdef __cplusplus
}
#endif
#endif
