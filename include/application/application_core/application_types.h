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

typedef struct app_build_config {
    platform_type_t selected_platform;
    target_graphics_api_t selected_graphics_api;
} app_build_config_t;

#ifdef __cplusplus
}
#endif
#endif
