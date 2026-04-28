#ifndef GLCE_ENGINE_RESOURCES_RESOURCES_CORE_RESOURCE_TYPES_H
#define GLCE_ENGINE_RESOURCES_RESOURCES_CORE_RESOURCE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TEXTURE_SUCCESS = 0,        /**< アプリケーション成功 */
    TEXTURE_NO_MEMORY,          /**< メモリ不足 */
    TEXTURE_RUNTIME_ERROR,      /**< 実行時エラー */
    TEXTURE_INVALID_ARGUMENT,   /**< 引数異常 */
    TEXTURE_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    TEXTURE_BAD_OPERATION,      /**< API誤用 */
    TEXTURE_OVERFLOW,           /**< 計算過程でオーバーフロー発生 */
    TEXTURE_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    TEXTURE_FILE_OPEN_ERROR,    /**< ファイルオープン失敗 */
    TEXTURE_FILE_READ_ERROR,    /**< ファイル読み込み失敗 */
    TEXTURE_UNSUPPORTED_FILE,   /**< 未対応ファイル形式 */
    TEXTURE_UNDEFINED_ERROR,    /**< 未定義エラー */
} texture_result_t;

#ifdef __cplusplus
}
#endif
#endif
