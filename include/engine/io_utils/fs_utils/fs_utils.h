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

/**
 * @brief fs_utils_t構造体インスタンスのメモリを確保し初期化する(指定されたファイルのオープンも行う)
 *
 * @warning 引数の値は以下に注意する
 * - filepath_の末尾には必ず'/'をつける
 * - extensionは必ず'.'から始めること
 *
 * @code{.c}
 * fs_utils_t* fs_utils = NULL;
 * fs_utils_result_t ret = fs_utils_create("path/to/", "filename", ".txt", FILESYSTEM_MODE_READ, &fs_utils);    // path/to/filename.txtがオープンされる
 * // エラー処理
 * @endcode
 *
 * @param filepath_ ファイルパス
 * @param filename_ ファイル名
 * @param extension_ 拡張子(拡張子がない場合はNULLを指定する)
 * @param open_mode_ ファイルオープンモード @ref filesystem_open_mode_t
 * @param fs_utils_ 初期化対象構造体インスタンスへのダブルポインタ
 *
 * @retval FS_UTILS_INVALID_ARGUMENT 以下のいずれか
 * - filepath_ == NULL
 * - filename_ == NULL
 * - fs_utils_ == NULL
 * - *fs_utils_ != NULL
 * - open_mode_ == FILESYSTEM_MODE_NONE
 * - メモリシステム未初期化
 * @retval FS_UTILS_LIMIT_EXCEEDED メモリシステムのシステム使用可能範囲上限を超過
 * @retval FS_UTILS_NO_MEMORY メモリ割り当て失敗
 * @retval FS_UTILS_OVERFLOW 文字列が長すぎてオーバーフロー
 * @retval FS_UTILS_UNDEFINED_ERROR 想定していないエラー(バグorエラー処理漏れ)
 * @retval FS_UTILS_DATA_CORRUPTED データメモリ破損,API誤用,初期化漏れ
 * @retval FS_UTILS_FILE_OPEN_ERROR ファイルオープンエラー
 * @retval FS_UTILS_RUNTIME_ERROR 既にオープン済みのファイルハンドル(初期化済みのハンドルは引数チェックで弾かれるため起こり得ない。発生したらバグ)
 * @retval FS_UTILS_SUCCESS メモリ確保と初期化に成功し、正常終了
 */
fs_utils_result_t fs_utils_create(const char* filepath_, const char* filename_, const char* extension_, filesystem_open_mode_t open_mode_, fs_utils_t** fs_utils_);

/**
 * @brief fs_utils_のメモリを解放する
 *
 * @note
 * - 2重デストロイを許可する
 * - filesystem_destroy内でファイルがクローズされる
 *
 * @code{.c}
 * fs_utils_t* fs_utils = NULL;
 * fs_utils_result_t ret = fs_utils_create("path/to/", "filename", ".txt", FILESYSTEM_MODE_READ, &fs_utils);    // path/to/filename.txtがオープンされる
 * // エラー処理
 *
 * fs_utils_destroy(&fs_utils);
 * fs_utils_destroy(&fs_utils); // 2重デストロイ許可
 * @endcode
 *
 * @param fs_utils_ メモリ解放対象構造体インスタンスへのダブルポインタ
 */
void fs_utils_destroy(fs_utils_t** fs_utils_);

/**
 * @ref fs_utils_create で指定したファイルについて、ファイルの中身を全て読み込む
 *
 * @warning 内部ではchoco_string_concat_from_c_stringを使用して文字列を連結する。各文字列処理には終端文字による判定処理が存在する。
 * ここで、バイナリファイルには終端文字(0)が普通に含まれるため、バイナリファイルの読み込みには使用してはいけない。
 *
 * @code{.c}
 * fs_utils_t* fs_utils = NULL;
 * fs_utils_result_t ret = fs_utils_create("path/to/", "filename", ".txt", FILESYSTEM_MODE_READ, &fs_utils);    // path/to/filename.txtがオープンされる
 * // エラー処理
 *
 * choco_string_t* string = NULL;
 * choco_string_result_t ret_str = choco_string_default_create(&string);
 * // エラー処理
 *
 * ret = fs_utils_text_file_read(fs_utils, string); // path/to/filename.txtの中身が全て読み込まれ、stringに格納される
 * // エラー処理
 * @endcode
 *
 * @param fs_utils_ fs_utils_t構造体インスタンスへのポインタ
 * @param out_string_ 読み込んだ文字列の格納先
 *
 * @retval FS_UTILS_INVALID_ARGUMENT 以下のいずれか
 * - fs_utils_ == NULL
 * - out_string_ == NULL
 * @retval FS_UTILS_DATA_CORRUPTED データメモリ破損,API誤用,初期化漏れ
 * @retval FS_UTILS_BAD_OPERATION 読み込み用ではないファイルオープンモードが渡された
 * @retval FS_UTILS_RUNTIME_ERROR ファイル読み込み中にエラーが発生
 * @retval FS_UTILS_SUCCESS ファイルの読み込みに成功し、正常終了
 */
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

#ifdef TEST_BUILD
// 引数で与えた実行結果コードを強制的に出力させる
void fs_utils_fail_enable(fs_utils_result_t result_code_);
void fs_utils_fail_disable(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
