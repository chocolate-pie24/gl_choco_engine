// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup core
 *
 * @file filesystem.h
 * @author chocolate-pie24
 * @brief ファイルシステムモジュールAPIの提供
 *
 * @details ファイルシステムモジュールは、ファイルI/Oについて最も基本的なAPIを提供する。
 * そのため、1行単位の読み込みや、ファイル全体の読み込みといった処理は提供しない。
 * これらの処理には可変長文字列バッファのリソース管理が必要で、choco_stringモジュールを使用したい。
 * choco_stringモジュールを使用するとなると、containersレイヤーよりも上層にfilesystemを位置づける必要がある。
 * 一方で、ファイルI/Oについての基本的な処理はcoreレイヤーに置きたい。このため、高度な処理と基本的な処理を分け、基本的な処理はcore/filesystemに置くことにする。
 * なお、高度な処理は、io_utils/fs_utilsに格納する。
 *
 * @date 2025-12-23
 *
 */
#ifndef GLCE_ENGINE_CORE_FILESYSTEM_FILESYSTEM_H
#define GLCE_ENGINE_CORE_FILESYSTEM_FILESYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/core/file_io/fs_types.h"

/**
 * @brief ファイルシステムモジュール内部状態管理構造体前方宣言(内部データ構造は外部非公開)
 *
 */
typedef struct filesystem filesystem_t;

/**
 * @brief ファイルシステムモジュールの実行結果コード定義
 *
 */
typedef enum {
    FILESYSTEM_SUCCESS = 0,         /**< 実行結果コード: 成功 */
    FILESYSTEM_INVALID_ARGUMENT,    /**< 実行結果コード: 無効な引数 */
    FILESYSTEM_RUNTIME_ERROR,       /**< 実行結果コード: 実行時エラー */
    FILESYSTEM_NO_MEMORY,           /**< 実行結果コード: メモリ不足 */
    FILESYSTEM_FILE_OPEN_ERROR,     /**< 実行結果コード: ファイルオープン失敗 */
    FILESYSTEM_FILE_CLOSE_ERROR,    /**< 実行結果コード: ファイルクローズ失敗 */
    FILESYSTEM_UNDEFINED_ERROR,     /**< 実行結果コード: 未定義エラー */
    FILESYSTEM_LIMIT_EXCEEDED,      /**< 実行結果コード: システムリソースが使用可能範囲を超過 */
    FILESYSTEM_BAD_OPERATION,       /**< 実行結果コード: API誤用 */
    FILESYSTEM_DATA_CORRUPTED,      /**< 実行結果コード: 内部データ破損 */
    FILESYSTEM_EOF,                 /**< 実行結果コード: ファイル読み取りEOF */
} filesystem_result_t;

/**
 * @brief filesystem_t構造体インスタンスを生成し、初期化する
 *
 * @note filesystem_は下記の状態で初期化される
 * - ファイルオープンモード: FILESYSTEM_MODE_NONE
 * - ファイルハンドル(FILE*): NULL
 *
 * @note 生成したインスタンスは @ref filesystem_destroy を使用して破棄する
 *
 * @code{.c}
 * filesystem_t* filesystem = NULL;
 * filesystem_result_t ret = filesystem_create(&filesystem);
 * // エラー処理
 * @endcode
 *
 * @param[out] filesystem_ ファイルシステム内部状態管理構造体へのダブルポインタ
 *
 * @retval FILESYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - filesystem_がNULL
 * - *filesystem_が非NULL
 * - memory_system_allocateがMEMORY_SYSTEM_INVALID_ARGUMENTを返した(これより前の処理で弾かれるため、基本的には起こり得ない)
 * @retval FILESYSTEM_NO_MEMORY メモリ不足によりfilesystem_tのメモリ確保に失敗
 * @retval FILESYSTEM_LIMIT_EXCEEDED メモリシステムの管理変数がシステム使用可能範囲を超過
 * @retval FILESYSTEM_UNDEFINED_ERROR 未定義のエラーが発生
 * @retval FILESYSTEM_BAD_OPERATION メモリシステム未初期化
 * @retval FILESYSTEM_SUCCESS filesystem_のメモリ確保と初期化に成功し、正常終了
 */
filesystem_result_t filesystem_create(filesystem_t** filesystem_);

/**
 * @brief filesystem_が管理しているメモリと自身のメモリを解放し、*filesystem_=NULLにする
 *
 * @note
 * - 2重デストロイ許可
 * - filesystem_ == NULLの場合はno-op
 * - *filesystem_ == NULLの場合はno-op
 * - open済のファイルハンドルを持つ構造体インスタンスが渡された場合は、@ref filesystem_close によるクローズ処理を行ってからメモリを解放する。
 * なお、クローズ処理は、エラーが発生した場合でも正常終了した場合でも、共にファイルハンドルが再利用不可となる。
 * そのため、filesystem_destroyでは、クローズ処理の成否に関わらず、メモリを解放する(ただしワーニングメッセージを出力する)。
 *
 * @code{.c}
 * filesystem_t* filesystem = NULL;
 * filesystem_result_t ret = filesystem_create(&filesystem);
 * // エラー処理
 *
 * filesystem_destroy(&filesystem); // filesystem == NULLになる
 * filesystem_destroy(&filesystem); // 2重デストロイ許可
 * @endcode
 *
 * @param filesystem_ メモリ解放対象構造体インスタンスへのダブルポインタ
 */
void filesystem_destroy(filesystem_t** filesystem_);

/**
 * @brief filesystem_が保持するファイルハンドルをオープンする
 *
 * @param[in] fullpath_ オープンするファイルのフルパス
 * @param[in] mode_ ファイルオープンモード @ref fs_open_mode_t
 * @param[in,out] filesystem_ オープン対象ファイルシステムモジュール構造体インスタンスへのポインタ
 *
 * @retval FILESYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - filesystem_がNULL
 * - fullpath_がNULL
 * - mode_が未定義の値またはFILESYSTEM_MODE_NONE
 * @retval FILESYSTEM_RUNTIME_ERROR 既にオープン済のファイルハンドルが渡された
 * @retval FILESYSTEM_FILE_OPEN_ERROR ファイルオープン失敗
 * @retval FILESYSTEM_SUCCESS ファイルオープンに成功し、正常終了
 *
 * @todo 既にオープン済のファイルハンドルが渡された場合の実行結果コードをBAD_OPERATIONに変更する
 */
filesystem_result_t filesystem_open(const char* fullpath_, fs_open_mode_t mode_, filesystem_t* filesystem_);

/**
 * @brief filesystem_が保持するファイルハンドルをクローズする
 *
 * @note
 * - FILESYSTEM_FILE_CLOSE_ERRORまたはFILESYSTEM_SUCCESSとなった場合、file_handleはNULL, modeはFILESYSTEM_MODE_NONEにリセットされる。
 * - ファイルハンドルクローズには標準ライブラリのfcloseを使用する。
 * fcloseに失敗する事例として、NASとの接続断等によりファイルの変更内容のフラッシュに失敗した場合がある。
 * この場合、クローズ後のファイルハンドルは再利用不可となりFILESYSTEM_FILE_CLOSE_ERRORを返す。
 *
 * @param[in,out] filesystem_ クローズ対象構造体インスタンスへのポインタ
 *
 * @retval FILESYSTEM_INVALID_ARGUMENT filesystem_がNULL
 * @retval FILESYSTEM_RUNTIME_ERROR 既にクローズ済のファイルハンドルが渡された
 * @retval FILESYSTEM_FILE_CLOSE_ERROR ファイルハンドルのクローズに失敗
 * @retval FILESYSTEM_SUCCESS ファイルハンドルのクローズに成功し、正常終了
 *
 * @todo 既にクローズ済のファイルハンドルが渡された場合の実行結果コードをBAD_OPERATIONに変更する
 */
filesystem_result_t filesystem_close(filesystem_t* filesystem_);

filesystem_result_t filesystem_byte_read(filesystem_t* filesystem_, size_t read_bytes_, size_t* result_n_, char* buffer_);

bool filesystem_is_valid(const filesystem_t* filesystem_);

#ifdef __cplusplus
}
#endif
#endif
