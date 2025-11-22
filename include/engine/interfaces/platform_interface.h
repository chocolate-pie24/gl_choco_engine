/** @ingroup platform_interface
 *
 * @file platform_interface.h
 * @author chocolate-pie24
 * @brief プラットフォームシステムのInterface構造体を提供する
 *
 * @details
 * ウィンドウ制御、マウス、キーボード処理を全プラットフォーム(x11, win32, glfw...)で共通化するために、
 * Strategyパターンを使用する。このレイヤーではStrategyパターンのinterfaceに相当する構造体インスタンスを提供する
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
#ifndef GLCE_ENGINE_INTERFACES_PLATFORM_INTERFACE_H
#define GLCE_ENGINE_INTERFACES_PLATFORM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/core/platform/platform_utils.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

/**< プラットフォーム内部状態管理構造体前方宣言(実体は各ソースファイルで定義) */
typedef struct platform_backend platform_backend_t;

/**
 * @brief 内部状態管理構造体のメモリ要件、メモリアライメント要件を取得する
 *
 * @note
 * - memory_requirement_ == NULL または alignment_requirement_ == NULLの場合は何もしない
 *
 * @param[out] memory_requirement_ platform_backend_tのメモリ要件
 * @param[out] alignment_requirement_ platform_backend_tのアライメント要件
 *
 */
typedef void (*pfn_platform_backend_preinit)(size_t* memory_requirement_, size_t* alignment_requirement_);

/**
 * @brief 内部状態管理構造体インスタンスメンバの初期化を行う
 *
 * @note
 * - platform_backend_自身のメモリは呼び出し側で確保する
 *
 * @param[in,out] platform_backend_ 初期化対象構造体インスタンス
 *
 * @retval PLATFORM_INVALID_ARGUMENT platform_backend_ == NULL
 * @retval PLATFORM_SUCCESS          初期化に成功し、正常終了
 * @retval その他                     各プラットフォーム実装依存
 */
typedef platform_result_t (*pfn_platform_backend_init)(platform_backend_t* platform_backend_);

/**
 * @brief 内部状態管理構造体インスタンスが保有するリソースを破棄する
 *
 * @note
 * - platform_backend_自身のメモリは呼び出し側で解放する
 * - platform_backend_ == NULLの場合は何もしない
 *
 * @param[in,out] platform_backend_ 破棄対象構造体インスタンス
 */
typedef void (*pfn_platform_backend_destroy)(platform_backend_t* platform_backend_);

/**
 * @brief ウィンドウを生成する
 *
 * @note
 * - window_label_の文字列は内部でdeep copyされるため、window_label_自身のメモリは呼び出し側で破棄すること
 *
 * @param[in,out] platform_backend_ 内部状態管理構造体インスタンス
 * @param[in] window_label_ ウィンドウ名称文字列
 * @param[in] window_width_ 初期状態のウィンドウ幅
 * @param[in] window_height_ 初期状態のウィンドウ高さ
 *
 * @retval PLATFORM_INVALID_ARGUMENT 以下のいずれか
 * - platform_backend_ == NULL
 * - window_label_ == NULL
 * - window_width_ == 0
 * - window_height_ == 0
 * @retval PLATFORM_NO_MEMORY        メモリ確保失敗
 * @retval PLATFORM_SUCCESS          ウィンドウの生成に成功し、正常終了
 * @retval その他                     プラットフォーム実装依存
 */
typedef platform_result_t (*pfn_platform_backend_window_create)(
    platform_backend_t* platform_backend_,
    const char* window_label_,
    int window_width_,
    int window_height_);

/**
 * @brief ウィンドウ、キーボード、マウスイベントを吸い上げ、各イベントをコールバック内で処理する
 *
 * @note
 * - ウィンドウクローズイベントについては、イベントキューが満杯時に破棄される可能性を考慮し、戻り値で返す
 *
 * @param[in,out] platform_backend_ 処理対象プラットフォーム内部状態管理オブジェクト
 * @param[in] window_event_callback ウィンドウイベント発生時用コールバック関数
 * @param[in] keyboard_event_callback キーボードイベント発生時用コールバック関数
 * @param[in] mouse_event_callback マウスイベント発生時用コールバック関数
 *
 * @retval PLATFORM_INVALID_ARGUMENT 以下のいずれか
 * - platform_backend_ == NULL または プラットフォーム内部状態管理オブジェクトが未初期化
 * - window_event_callback == NULL
 * - keyboard_event_callback == NULL
 * - mouse_event_callback == NULL
 * @retval PLATFORM_SUCCESS          イベントの吸い上げおよびイベントコールバックの処理に成功し、正常終了
 * @retval PLATFORM_WINDOW_CLOSE     ウィンドウクローズイベントが発生
 * @retval その他                     プラットフォーム実装依存
 */
typedef platform_result_t (*pfn_platform_backend_pump_messages)(
    platform_backend_t* platform_backend_,
    void (*window_event_callback)(const window_event_t* event_),
    void (*keyboard_event_callback)(const keyboard_event_t* event_),
    void (*mouse_event_callback)(const mouse_event_t* event_));

/**
 * @brief プラットフォーム処理共通化のための仮想関数テーブル(実装はsrc/platform/以下のソースファイルに格納)
 */
typedef struct platform_vtable {
    pfn_platform_backend_preinit        platform_backend_preinit;           /**< 関数ポインタ @ref pfn_platform_backend_preinit 参照 */
    pfn_platform_backend_init           platform_backend_init;              /**< 関数ポインタ @ref pfn_platform_backend_init 参照 */
    pfn_platform_backend_destroy        platform_backend_destroy;           /**< 関数ポインタ @ref pfn_platform_backend_destroy 参照 */
    pfn_platform_backend_window_create  platform_backend_window_create;     /**< 関数ポインタ @ref pfn_platform_backend_window_create 参照 */
    pfn_platform_backend_pump_messages  platform_backend_pump_messages;     /**< 関数ポインタ @ref pfn_platform_backend_pump_messages 参照 */
} platform_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
