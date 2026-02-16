/** @ingroup event_window
 *
 * @file window_event.h
 * @author chocolate-pie24
 * @brief 全プラットフォーム共通で使用可能なウィンドウイベント定義と、ウィンドウイベント構造体定義
 *
 * @version 0.1
 * @date 2025-10-14
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_CORE_EVENT_WINDOW_EVENT_H
#define GLCE_ENGINE_CORE_EVENT_WINDOW_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ウィンドウイベント種別リスト
 *
 */
typedef enum {
    WINDOW_EVENT_RESIZE,    /**< ウィンドウイベント: ウィンドウサイズ変化 */
} window_event_code_t;

/**
 * @brief ウィンドウイベント付随情報
 *
 */
typedef struct window_event_args {
    int window_width;               /**< イベントが発生した際のウィンドウ幅 */
    int window_height;              /**< イベントが発生した際のウィンドウ高さ */
    int framebuffer_width;          /**< イベントが発生した際のフレームバッファサイズ(幅) */
    int framebuffer_height;         /**< イベントが発生した際のフレームバッファサイズ(高さ) */
} window_event_args_t;

/**
 * @brief ウィンドウイベント構造体
 *
 */
typedef struct window_event {
    window_event_code_t event_code; /**< ウィンドウイベント種別 */
    window_event_args_t event_args; /**< イベント付随情報 */
} window_event_t;

#ifdef __cplusplus
}
#endif
#endif
