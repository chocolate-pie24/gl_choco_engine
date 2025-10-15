/** @addtogroup core_event
 * @{
 *
 * @file mouse_event.h
 * @author chocolate-pie24
 * @brief 全プラットフォーム共通で使用可能なマウスボタン定義と、マウスイベントオブジェクト定義
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
#ifndef ENGINE_CORE_MOUSE_EVENT_H
#define ENGINE_CORE_MOUSE_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief マウスボタン種別リスト(中ボタンは多分使わないので省略した)
 *
 */
typedef enum {
    MOUSE_BUTTON_LEFT,  /**< マウス左ボタン */
    MOUSE_BUTTON_RIGHT, /**< マウス右ボタン */
} mouse_button_t;

/**
 * @brief マウスイベントオブジェクト
 *
 */
typedef struct mouse_event {
    int x;                  /**< イベントが発生した際のウィンドウ内でのマウス座標 */
    int y;                  /**< イベントが発生した際のウィンドウ内でのマウス座標 */
    mouse_button_t button;  /**< イベントが発生したマウスボタン */
    bool pressed;           /**< イベントが発生した際のマウスボタン状態(true: 押下 / false: 離した) */
} mouse_event_t;

#ifdef __cplusplus
}
#endif
#endif

/** @}*/
