/** @ingroup fs_utils
 *
 * @file fs_utils.h
 * @author chocolate-pie24
 * @brief ファイル処理に関するユーティリティAPIを提供する
 *
 * @details
 * - ファイルのオープン/クローズ/バイト単位での読み取りについては @ref filessytem.h を参照のこと
 * - ファイルのオープン/クローズは呼び出し側のコード量を少なくするため,fs_utils_create, fs_utils_destroyにて自動的に行う.ユーザーがオープン/クローズ操作は行わない
 *
 * @version 0.1
 * @date 2025-12.26
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_IO_UTILS_FS_UTILS_FS_UTILS_H
#define GLCE_ENGINE_IO_UTILS_FS_UTILS_FS_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/containers/choco_string.h"

#include "engine/core/filesystem/filesystem.h"

typedef struct fs_utils fs_utils_t; /**< ファイルシステムユーティリティ内部状態管理構造体前方宣言 */

/**
 * @brief ファイルシステムユーティリティ実行結果コード定義
 *
 */
typedef enum {
    FS_UTILS_SUCCESS = 0,       /**< 実行結果コード: 成功 */
    FS_UTILS_INVALID_ARGUMENT,  /**< 実行結果コード: 無効な引数 */
    FS_UTILS_BAD_OPERATION,     /**< 実行結果コード: API誤用 */
    FS_UTILS_DATA_CORRUPTED,    /**< 実行結果コード: データ破損or未初期化 */
    FS_UTILS_NO_MEMORY,         /**< 実行結果コード: メモリ不足 */
    FS_UTILS_LIMIT_EXCEEDED,    /**< 実行結果コード: システム使用可能範囲上限超過 */
    FS_UTILS_OVERFLOW,          /**< 実行結果コード: 計算過程でオーバーフロー発生 */
    FS_UTILS_FILE_OPEN_ERROR,   /**< 実行結果コード: ファイルオープンエラー */
    FS_UTILS_RUNTIME_ERROR,     /**< 実行結果コード: 実行時エラー */
    FS_UTILS_UNDEFINED_ERROR,   /**< 実行結果コード: 想定していないエラーが発生 */
} fs_utils_result_t;

fs_utils_result_t fs_utils_create(const char* filepath_, const char* filename_, const char* extension_, filesystem_open_mode_t open_mode_, fs_utils_t** fs_utils_);
void fs_utils_destroy(fs_utils_t** fs_utils_);
fs_utils_result_t fs_utils_text_file_read(fs_utils_t* fs_utils_, choco_string_t* out_string_);

/**
 * @brief ファイルパス,ファイル名,拡張子の文字列からフルパス文字列を生成する
 *
 * @note 処理に失敗した場合,out_fullpath_の内部データは変更される可能性がある
 * @warning out_fullpath_は @ref choco_string_default_create @ref choco_string_create_from_c_string によって初期化されたインスタンスを渡すこと
 *
 * @param fs_utils_ fs_utils_t構造体インスタンスへのポインタ
 * @param out_fullpath_ フルパス格納先choco_string_t構造体インスタンスへのポインタ

 * @retval FS_UTILS_INVALID_ARGUMENT 以下のいずれか
 * - fs_utils_がNULL
 * - out_fullpath_がNULL
 * - filepathの文字列をコピーする際にchoco_string_copyがCHOCO_STRING_INVALID_ARGUMENTを返した
 * - filenameの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_INVALID_ARGUMENTを返した
 * - extensionの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_INVALID_ARGUMENTを返した
 * @retval FS_UTILS_DATA_CORRUPTED 以下のいずれか
 * - fs_utils_の内部状態が無効もしくは未初期化
 * - filepathの文字列をコピーする際にchoco_string_copyがCHOCO_STRING_DATA_CORRUPTEDを返した
 * - filenameの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_DATA_CORRUPTEDを返した
 * - extensionの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_DATA_CORRUPTEDを返した
 * @retval FS_UTILS_OVERFLOW 以下のいずれか
 * - filepathの文字列をコピーする際にchoco_string_copyがCHOCO_STRING_OVERFLOWを返した
 * - filenameの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_OVERFLOWを返した
 * - extensionの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_OVERFLOWを返した
 * @retval FS_UTILS_NO_MEMORY 以下のいずれか
 * - filepathの文字列をコピーする際にchoco_string_copyがCHOCO_STRING_NO_MEMORYを返した
 * - filenameの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_NO_MEMORYを返した
 * - extensionの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_NO_MEMORYを返した
 * @retval FS_UTILS_LIMIT_EXCEEDED 以下のいずれか
 * - filepathの文字列をコピーする際にchoco_string_copyがCHOCO_STRING_LIMIT_EXCEEDEDを返した
 * - filenameの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_LIMIT_EXCEEDEDを返した
 * - extensionの文字列をコピーする際にchoco_string_concatがCHOCO_STRING_LIMIT_EXCEEDEDを返した
 * @retval FS_UTILS_UNDEFINED_ERROR 処理過程において想定外のエラーコードを受け取った
 * @retval FS_UTILS_SUCCESS フルパス文字列の生成に成功し,正常終了
 */
fs_utils_result_t fs_utils_fullpath_get(fs_utils_t* fs_utils_, choco_string_t* out_fullpath_);

#ifdef __cplusplus
}
#endif
#endif
