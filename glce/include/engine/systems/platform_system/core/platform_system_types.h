// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_CORE_PLATFORM_SYSTEM_TYPES_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_SYSTEM_CORE_PLATFORM_SYSTEM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct window_event window_event_t;
typedef struct keyboard_event keyboard_event_t;
typedef struct mouse_event mouse_event_t;

/**
 * @brief プラットフォームシステム実行結果コード定義
 *
 */
typedef enum {
    PLATFORM_SYSTEM_SUCCESS = 0,       /**< 実行結果: 成功 */
    PLATFORM_SYSTEM_INVALID_ARGUMENT,  /**< 実行結果: 無効な引数が与えられてエラー */
    PLATFORM_SYSTEM_RUNTIME_ERROR,     /**< 実行結果: 実行時エラー */
    PLATFORM_SYSTEM_NO_MEMORY,         /**< 実行結果: メモリ不足エラー */
    PLATFORM_SYSTEM_DATA_CORRUPTED,    /**< 実行結果: メモリ破損, API誤用, データ未初期化 */
    PLATFORM_SYSTEM_BAD_OPERATION,     /**< 実行結果: API誤用 */
    PLATFORM_SYSTEM_OVERFLOW,          /**< 実行結果: 計算過程でオーバーフロー発生 */
    PLATFORM_SYSTEM_LIMIT_EXCEEDED,    /**< 実行結果: システムで使用可能な上限範囲を超過 */
    PLATFORM_SYSTEM_UNDEFINED_ERROR,   /**< 実行結果: 未定義エラー */
} platform_system_result_t;

typedef struct platform_event_view {
    bool window_close_requested;

    size_t window_event_count;
    const window_event_t* window_events;

    size_t keyboard_event_count;
    const keyboard_event_t* keyboard_events;

    size_t mouse_event_count;
    const mouse_event_t* mouse_events;
} platform_event_view_t;

bool platform_event_view_is_valid(const platform_event_view_t* event_view_);

#ifdef __cplusplus
}
#endif
#endif
