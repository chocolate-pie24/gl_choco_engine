/** @addtogroup application
 * @{
 *
 * @file application.h
 * @author chocolate-pie24
 * @brief 最上位のオーケストレーション。サブシステム初期化、メインループ駆動、終了処理を提供
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 * @license MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_APPLICATION_APPLICATION_H
#define GLCE_APPLICATION_APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief アプリケーション実行結果コード定義
 *
 */
typedef enum {
    APPLICATION_SUCCESS = 0,        /**< アプリケーション成功 */
    APPLICATION_NO_MEMORY,          /**< メモリ不足 */
    APPLICATION_RUNTIME_ERROR,      /**< 実行時エラー */
    APPLICATION_INVALID_ARGUMENT,   /**< 引数異常 */
    APPLICATION_UNDEFINED_ERROR,    /**< 未定義エラー */
} application_result_t;

/**
 * @brief エンジンを構成する各サブシステムを初期化する
 *
 * @retval APPLICATION_RUNTIME_ERROR    アプリケーションがすでに初期化済み
 * @retval APPLICATION_NO_MEMORY        メモリ確保に失敗
 * @retval APPLICATION_UNDEFINED_ERROR  未定義のエラーが発生
 * @retval APPLICATION_INVALID_ARGUMENT サブシステム初期化に無効な引数を指定した
 * @retval APPLICATION_SUCCESS          エンジンおよびアプリケーションの初期化に成功し、正常終了
 */
application_result_t application_create(void);

/**
 * @brief エンジンを構成するサブシステムを停止し、アプリケーション終了する
 *
 */
void application_destroy(void);

/**
 * @brief アプリケーションメインループ
 *
 * @return application_result_t
 */
application_result_t application_run(void);

#ifdef __cplusplus
}
#endif
#endif

/** @}*/
