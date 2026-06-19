#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_PIPELINES_RENDERER_PIPELINE_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_PIPELINES_RENDERER_PIPELINE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RENDERER_PIPELINE_SUCCESS = 0,        /**< 処理成功 */
    RENDERER_PIPELINE_NO_MEMORY,          /**< メモリ不足 */
    RENDERER_PIPELINE_RUNTIME_ERROR,      /**< 実行時エラー */
    RENDERER_PIPELINE_INVALID_ARGUMENT,   /**< 引数異常 */
    RENDERER_PIPELINE_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    RENDERER_PIPELINE_BAD_OPERATION,      /**< API誤用 */
    RENDERER_PIPELINE_OVERFLOW,           /**< 計算過程でオーバーフロー発生 */
    RENDERER_PIPELINE_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    RENDERER_PIPELINE_FILE_OPEN_ERROR,    /**< ファイルオープン失敗 */
    RENDERER_PIPELINE_FILE_READ_ERROR,    /**< ファイル読み込み失敗 */
    RENDERER_PIPELINE_FILE_CLOSE_ERROR,   /**< ファイルクローズ失敗 */
    RENDERER_PIPELINE_UNSUPPORTED_FILE,   /**< 未対応ファイル形式 */
    RENDERER_PIPELINE_UNDEFINED_ERROR,    /**< 未定義エラー */
} renderer_pipeline_result_t;

#ifdef __cplusplus
}
#endif
#endif
