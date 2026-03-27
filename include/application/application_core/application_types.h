/** @ingroup application
 *
 * @file application_types.h
 * @author chocolate-pie24
 * @brief アプリケーションレイヤー全体で使用されるデータ型を提供する
 *
 * @version 0.1
 * @date 2026-03-25
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_APPLICATION_APPLICATION_CORE_APPLICATION_TYPES_H
#define GLCE_APPLICATION_APPLICATION_CORE_APPLICATION_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/platform/platform_core/platform_types.h"   // for platform_type_t

#include "engine/renderer/renderer_core/renderer_types.h"   // for target_graphics_api_t

/**
 * @brief アプリケーション実行結果コード定義
 *
 */
typedef enum {
    APPLICATION_SUCCESS = 0,        /**< アプリケーション成功 */
    APPLICATION_NO_MEMORY,          /**< メモリ不足 */
    APPLICATION_RUNTIME_ERROR,      /**< 実行時エラー */
    APPLICATION_INVALID_ARGUMENT,   /**< 引数異常 */
    APPLICATION_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    APPLICATION_BAD_OPERATION,      /**< API誤用 */
    APPLICATION_OVERFLOW,           /**< 計算過程でオーバーフロー発生 */
    APPLICATION_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    APPLICATION_UNDEFINED_ERROR,    /**< 未定義エラー */
} application_result_t;

/**
 * @brief GLCEビルドコンフィグレーション値格納構造体
 *
 */
typedef struct app_build_config {
    platform_type_t selected_platform;              /**< 実行プラットフォーム選択 */
    target_graphics_api_t selected_graphics_api;    /**< 使用グラフィックスAPI選択 */
} app_build_config_t;

#ifdef __cplusplus
}
#endif
#endif
