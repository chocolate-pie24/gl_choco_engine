/** @ingroup view
 *
 * @file view_types.h
 * @author chocolate-pie24
 * @brief ビューレイヤー全体で使用されるデータ型を提供する
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_VIEW_VIEW_TYPES_H
#define GLCE_ENGINE_VIEW_VIEW_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VIEW_SUCCESS = 0,           /**< 処理成功 */
    VIEW_INVALID_ARGUMENT,      /**< 無効な引数 */
    VIEW_RUNTIME_ERROR,         /**< 実行時エラー */
    VIEW_NO_MEMORY,             /**< メモリ不足 */
    VIEW_LIMIT_EXCEEDED,        /**< メモリシステム使用可能範囲超過 */
    VIEW_BAD_OPERATION,         /**< API誤用 */
    VIEW_DATA_CORRUPTED,        /**< メモリ破損,未初期化 */
    VIEW_UNDEFINED_ERROR,       /**< 不明なエラー */
} view_result_t;

typedef enum {
    CAMERA_TYPE_FLIGHT_CAMERA = 0,  /**< 空間内自由飛行カメラ */
} camera_type_t;

#ifdef __cplusplus
}
#endif
#endif
