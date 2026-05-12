#ifndef GLCE_ENGINE_RESOURCE_RESOURCE_CORE_RESOURCE_TYPES_H
#define GLCE_ENGINE_RESOURCE_RESOURCE_CORE_RESOURCE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RESOURCE_SUCCESS = 0,        /**< 処理成功 */
    RESOURCE_NO_MEMORY,          /**< メモリ不足 */
    RESOURCE_RUNTIME_ERROR,      /**< 実行時エラー */
    RESOURCE_INVALID_ARGUMENT,   /**< 引数異常 */
    RESOURCE_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    RESOURCE_BAD_OPERATION,      /**< API誤用 */
    RESOURCE_OVERFLOW,           /**< 計算過程でオーバーフロー発生 */
    RESOURCE_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    RESOURCE_FILE_OPEN_ERROR,    /**< ファイルオープン失敗 */
    RESOURCE_FILE_READ_ERROR,    /**< ファイル読み込み失敗 */
    RESOURCE_FILE_CLOSE_ERROR,   /**< ファイルクローズ失敗 */
    RESOURCE_UNSUPPORTED_FILE,   /**< 未対応ファイル形式 */
    RESOURCE_UNDEFINED_ERROR,    /**< 未定義エラー */
} resource_result_t;

#ifdef __cplusplus
}
#endif
#endif
