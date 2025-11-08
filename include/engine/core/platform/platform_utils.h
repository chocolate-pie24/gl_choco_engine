/** @ingroup platform_utils
 *
 * @file platform_utils.h
 * @author chocolate-pie24
 * @brief プラットフォームシステムで共通に使用されるデータ型を提供する
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
#ifndef GLCE_ENGINE_CORE_PLATFORM_PLATFORM_UTILS_H
#define GLCE_ENGINE_CORE_PLATFORM_PLATFORM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief プラットフォームシステム実行結果コード定義
 *
 */
typedef enum {
    PLATFORM_SUCCESS = 0,       /**< 実行結果: 成功 */
    PLATFORM_INVALID_ARGUMENT,  /**< 実行結果: 無効な引数が与えられてエラー */
    PLATFORM_RUNTIME_ERROR,     /**< 実行結果: 実行時エラー */
    PLATFORM_NO_MEMORY,         /**< 実行結果: メモリ不足エラー */
    PLATFORM_UNDEFINED_ERROR,   /**< 実行結果: 未定義エラー */
    PLATFORM_WINDOW_CLOSE,      /**< 実行結果: ウィンドウクローズ(これは絶対に落としてはいけないイベントであるため、イベントキューには入れない(キューが満杯時に捨てられる可能性があるため)) */
} platform_result_t;

/**
 * @brief ウィンドウ、キーボード、マウスシステム処理のプラットフォーム種別定義
 *
 */
typedef enum {
    PLATFORM_USE_GLFW,  /**< プラットフォーム: GLFW */
} platform_type_t;

#ifdef __cplusplus
}
#endif
#endif
