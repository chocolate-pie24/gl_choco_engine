/** @ingroup camera_system
 *
 * @file camera_types.h
 * @author chocolate-pie24
 * @brief カメラシステムレイヤー全体で使用されるデータ型を提供する
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
#ifndef GLCE_ENGINE_CAMERA_SYSTEM_CAMERA_CORE_CAMERA_TYPES_H
#define GLCE_ENGINE_CAMERA_SYSTEM_CAMERA_CORE_CAMERA_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CAMERA_SUCCESS = 0,           /**< 処理成功 */
    CAMERA_INVALID_ARGUMENT,      /**< 無効な引数 */
    CAMERA_RUNTIME_ERROR,         /**< 実行時エラー */
    CAMERA_NO_MEMORY,             /**< メモリ不足 */
    CAMERA_LIMIT_EXCEEDED,        /**< システム使用可能範囲超過 */
    CAMERA_BAD_OPERATION,         /**< API誤用 */
    CAMERA_DATA_CORRUPTED,        /**< メモリ破損,未初期化 */
    CAMERA_UNDEFINED_ERROR,       /**< 不明なエラー */
} camera_result_t;

typedef enum {
    CAMERA_TYPE_FLIGHT_CAMERA = 0,  /**< 空間内自由飛行カメラ */
} camera_type_t;

#ifdef __cplusplus
}
#endif
#endif
