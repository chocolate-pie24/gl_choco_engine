/** @addtogroup event_keyboard
 * @{
 *
 * @file keyboard_event.h
 * @author chocolate-pie24
 * @brief 全プラットフォーム共通で使用可能なキーコード定義と、キーボードイベントオブジェクト定義
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
#ifndef ENGINE_CORE_KEYBOARD_EVENT_H
#define ENGINE_CORE_KEYBOARD_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief 全プラットフォームで共通化するためのキーコード一覧定義
 *
 * @note
 * - プラットフォーム固有のキーはプラットフォーム層で本列挙子にマッピングする
 */
typedef enum {
    KEY_1,              /**< key: '1' */
    KEY_2,              /**< key: '2' */
    KEY_3,              /**< key: '3' */
    KEY_4,              /**< key: '4' */
    KEY_5,              /**< key: '5' */
    KEY_6,              /**< key: '6' */
    KEY_7,              /**< key: '7' */
    KEY_8,              /**< key: '8' */
    KEY_9,              /**< key: '9' */
    KEY_0,              /**< key: '0' */
    KEY_A,              /**< key: 'a' */
    KEY_B,              /**< key: 'b' */
    KEY_C,              /**< key: 'c' */
    KEY_D,              /**< key: 'd' */
    KEY_E,              /**< key: 'e' */
    KEY_F,              /**< key: 'f' */
    KEY_G,              /**< key: 'g' */
    KEY_H,              /**< key: 'h' */
    KEY_I,              /**< key: 'i' */
    KEY_J,              /**< key: 'j' */
    KEY_K,              /**< key: 'k' */
    KEY_L,              /**< key: 'l' */
    KEY_M,              /**< key: 'm' */
    KEY_N,              /**< key: 'n' */
    KEY_O,              /**< key: 'o' */
    KEY_P,              /**< key: 'p' */
    KEY_Q,              /**< key: 'q' */
    KEY_R,              /**< key: 'r' */
    KEY_S,              /**< key: 's' */
    KEY_T,              /**< key: 't' */
    KEY_U,              /**< key: 'u' */
    KEY_V,              /**< key: 'v' */
    KEY_W,              /**< key: 'w' */
    KEY_X,              /**< key: 'x' */
    KEY_Y,              /**< key: 'y' */
    KEY_Z,              /**< key: 'z' */
    KEY_RIGHT,          /**< key: '右矢印キー' */
    KEY_LEFT,           /**< key: '左矢印キー' */
    KEY_UP,             /**< key: '上矢印キー' */
    KEY_DOWN,           /**< key: '下矢印キー' */
    KEY_LEFT_SHIFT,     /**< key: '左シフトキー' */
    KEY_SPACE,          /**< key: 'スペース' */
    KEY_SEMICOLON,      /**< key: 'セミコロン' */
    KEY_MINUS,          /**< key: 'マイナス' */
    KEY_F1,             /**< key: 'F1' */
    KEY_F2,             /**< key: 'F2' */
    KEY_F3,             /**< key: 'F3' */
    KEY_F4,             /**< key: 'F4' */
    KEY_F5,             /**< key: 'F5' */
    KEY_F6,             /**< key: 'F6' */
    KEY_F7,             /**< key: 'F7' */
    KEY_F8,             /**< key: 'F8' */
    KEY_F9,             /**< key: 'F9' */
    KEY_F10,            /**< key: 'F10' */
    KEY_F11,            /**< key: 'F11' */
    KEY_F12,            /**< key: 'F12' */
    KEY_CODE_MAX,       /**< キーコード最大数 */
} keycode_t;

/**
 * @brief キーボードイベント構造体
 *
 */
typedef struct keyboard_event {
    keycode_t key;  /**< イベントが発生したキー */
    bool pressed;   /**< イベント(キー押下: true / キーを離した: false) */
} keyboard_event_t;

#ifdef __cplusplus
}
#endif
#endif

/** @}*/
