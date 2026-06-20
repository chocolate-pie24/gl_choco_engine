#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_CORE_RESOURCE_REGISTRY_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_CORE_RESOURCE_REGISTRY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_GEOMETRY_ID (-1)

typedef enum {
    RESOURCE_REGISTRY_SUCCESS = 0,        /**< 処理成功 */
    RESOURCE_REGISTRY_NO_MEMORY,          /**< メモリ不足 */
    RESOURCE_REGISTRY_RUNTIME_ERROR,      /**< 実行時エラー */
    RESOURCE_REGISTRY_INVALID_ARGUMENT,   /**< 引数異常 */
    RESOURCE_REGISTRY_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    RESOURCE_REGISTRY_BAD_OPERATION,      /**< API誤用 */
    RESOURCE_REGISTRY_OVERFLOW,           /**< 計算過程でオーバーフロー発生 */
    RESOURCE_REGISTRY_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    RESOURCE_REGISTRY_UNDEFINED_ERROR,    /**< 未定義エラー */
} resource_registry_result_t;

#ifdef __cplusplus
}
#endif
#endif
