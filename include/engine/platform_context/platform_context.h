/** @ingroup platform_context
 *
 * @file platform_context.h
 * @author chocolate-pie24
 * @brief プラットフォームシステムのStrategy Contextモジュールを提供する
 *
 * @details
 * ウィンドウ制御、マウス、キーボード処理を全プラットフォーム(X Window System, win32, glfw...)で共通化するために、
 * Strategyパターンを使用する。このレイヤーではStrategyパターンのContextに相当する構造体インスタンスを提供する
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
#ifndef GLCE_ENGINE_PLATFORM_CONTEXT_PLATFORM_CONTEXT_H
#define GLCE_ENGINE_PLATFORM_CONTEXT_PLATFORM_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/platform/platform_utils.h"

#include "engine/core/memory/linear_allocator.h"

/**
 * @brief プラットフォームコンテキスト構造体前方宣言
 *
 */
typedef struct platform_context platform_context_t;

/**
 * @brief プラットフォームStrategyパターンを初期化する
 *
 * @note
 * - allocator_は初期化済みのリニアアロケータを渡すこと
 *
 * 使用例:
 * @code{.c}
 * linear_alloc_t* linear_alloc = NULL;
 * // リニアアロケータ初期化
 *
 * platform_context_t* platform_context = NULL;
 * platform_result_t ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
 * // エラー処理
 * @endcode
 *
 * @param[in,out] allocator_ メモリ確保用リニアアロケータ
 * @param[in] platform_type_ プラットフォーム種別
 * @param[out] out_platform_context_ プラットフォームコンテキスト構造体インスタンス
 *
 * @retval PLATFORM_INVALID_ARGUMENT 以下のいずれか
 * - allocator_ == NULL
 * - out_platform_context_ == NULL
 * - *out_platform_context_ != NULL
 * - platform_type_が無効値
 * @retval PLATFORM_RUNTIME_ERROR vtable取得失敗
 * @retval PLATFORM_SUCCESS 初期化およびメモリ確保に成功し、正常終了
 * @retval 上記以外 リニアアロケータによるメモリ確保失敗( @ref linear_allocator_allocate )
 *
 * @see linear_allocator_init
 * @see linear_allocator_allocate
 */
platform_result_t platform_initialize(linear_alloc_t* allocator_, platform_type_t platform_type_, platform_context_t** out_platform_context_);

/**
 * @brief プラットフォームStrategyパターンのContext構造体インスタンスの内部データをクリアする
 *
 * @note
 * - platform_context_のメモリはlinear_allocatorによって確保しているため、内部メモリおよび自身のメモリは呼び出し側で行うこと
 *
 * 使用例:
 * @code{.c}
 * linear_alloc_t* linear_alloc = NULL;
 * // リニアアロケータ初期化
 *
 * platform_context_t* platform_context = NULL;
 * platform_result_t ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
 * // エラー処理
 *
 * platform_destroy(platform_context);
 *
 * // リニアアロケータによるメモリ破棄
 * @endcode
 *
 * @param[in,out] platform_context_ クリア対象構造体インスタンス
 */
void platform_destroy(platform_context_t* platform_context_);

/**
 * @brief プラットフォームに応じたウィンドウ生成処理を行う
 *
 * @note
 * - window_label_はplatform_window_create内部でdeep copyするため、呼び出し側でメモリを破棄すること
 *
 * 使用例:
 * @code{.c}
 * linear_alloc_t* linear_alloc = NULL;
 * // リニアアロケータ初期化
 *
 * platform_context_t* platform_context = NULL;
 * platform_result_t ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
 * // エラー処理
 *
 * ret = platform_window_create(platform_context, "test_window", 1024, 768);
 * // エラー処理
 *
 * platform_destroy(platform_context);
 *
 * // リニアアロケータによるメモリ破棄
 * @endcode
 *
 * @param platform_context_ プラットフォームStrategy Context構造体インスタンス
 * @param window_label_ ウィンドウラベル
 * @param window_width_ ウィンドウ幅
 * @param window_height_ ウィンドウ高さ
 *
 * @retval PLATFORM_INVALID_ARGUMENT 以下のいずれか
 * - platform_context_ == NULL
 * - platform_context_->vtable
 * - platform_context_->backend
 * - window_label_ == NULL
 * - window_width_ == 0
 * - window_height_ == 0
 * @retval PLATFORM_SUCCESS ウィンドウ生成に成功し、正常終了
 * @retval 上記以外 各プラットフォーム実装依存
 *
 */
platform_result_t platform_window_create(platform_context_t* platform_context_, const char* window_label_, int window_width_, int window_height_);

#ifdef __cplusplus
}
#endif
#endif
