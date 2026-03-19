/** @ingroup filesystem
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
 * なお、高度な処理は、io_utils/fs_utils/fs_utilsに格納する。
 *
 * @version 0.1
 * @date 2025-12-23
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_CORE_FILESYSTEM_FILESYSTEM_H
#define GLCE_ENGINE_CORE_FILESYSTEM_FILESYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

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
    FILESYSTEM_EOF,                 /**< 実行結果コード: ファイル読み取りEOF */
} filesystem_result_t;

/**
 * @brief ファイルオープンモードリスト
 *
 */
typedef enum {
    FILESYSTEM_MODE_NONE = 0,               /**< オープンモード: デフォルト(未オープン) */
    FILESYSTEM_MODE_READ,                   /**< オープンモード: 読み取り */
    FILESYSTEM_MODE_WRITE,                  /**< オープンモード: 書き込み */
    FILESYSTEM_MODE_APPEND,                 /**< オープンモード: 追記 */
    FILESYSTEM_MODE_READ_PLUS,              /**< オープンモード: 読み書き可(既存ファイルの内容は消さない、ファイルがなければ失敗) */
    FILESYSTEM_MODE_WRITE_PLUS,             /**< オープンモード: 読み書き可(新規作成or既存ファイルの中身を消去) */
    FILESYSTEM_MODE_APPEND_PLUS,            /**< オープンモード: 読み書き可(既存ファイルがあれば追記、ファイルがなければ新規作成) */
    FILESYSTEM_MODE_READ_BINARY,            /**< オープンモード: 読み取り(バイナリファイル) */
    FILESYSTEM_MODE_WRITE_BINARY,           /**< オープンモード: 書き込み(バイナリファイル) */
    FILESYSTEM_MODE_APPEND_BINARY,          /**< オープンモード: 追記(バイナリファイル) */
    FILESYSTEM_MODE_READ_PLUS_BINARY,       /**< オープンモード: 読み書き可(既存ファイルの内容は消さない、ファイルがなければ失敗)(バイナリファイル) */
    FILESYSTEM_MODE_WRITE_PLUS_BINARY,      /**< オープンモード: 読み書き可(新規作成or既存ファイルの中身を消去)(バイナリファイル) */
    FILESYSTEM_MODE_APPEND_PLUS_BINARY,     /**< オープンモード: 読み書き可(既存ファイルがあれば追記、ファイルがなければ新規作成)(バイナリファイル) */
} filesystem_open_mode_t;

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
 * @param filesystem_ ファイルシステム内部状態管理構造体へのダブルポインタ
 *
 * @retval FILESYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - filesystem_がNULL
 * - *filesystem_が非NULL
 * - memory_system_allocateがMEMORY_SYSTEM_INVALID_ARGUMENTを返した(これより前の処理で弾かれるため、基本的には起こり得ない)
 * @retval FILESYSTEM_NO_MEMORY メモリ不足によりfilesystem_tのメモリ確保に失敗
 * @retval FILESYSTEM_LIMIT_EXCEEDED メモリシステムの管理変数がシステム使用可能範囲を超過
 * @retval FILESYSTEM_UNDEFINED_ERROR 未定義のエラーが発生
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
 * @code{.c}
 * filesystem_t* filesystem = NULL;
 * filesystem_result_t ret = filesystem_create(&filesystem);
 * // エラー処理
 *
 * ret = filesystem_open(filesystem, "/path/to/file", FILESYSTEM_MODE_READ);
 * // エラー処理
 * @endcode
 *
 * @param filesystem_ オープン対象ファイルシステムモジュール構造体インスタンスへのポインタ
 * @param fullpath_ オープンするファイルのフルパス
 * @param mode_ ファイルオープンモード @ref filesystem_open_mode_t
 *
 * @retval FILESYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - filesystem_がNULL
 * - fullpath_がNULL
 * - mode_が未定義の値
 * @retval FILESYSTEM_RUNTIME_ERROR 既にオープン済のファイルハンドルが渡された
 * @retval FILESYSTEM_FILE_OPEN_ERROR ファイルオープン失敗
 * @retval FILESYSTEM_SUCCESS ファイルオープンに成功し、正常終了
 *
 * @todo 既にオープン済のファイルハンドルが渡された場合の実行結果コードをBAD_OPERATIONに変更する
 */
filesystem_result_t filesystem_open(filesystem_t* filesystem_, const char* fullpath_, filesystem_open_mode_t mode_);

/**
 * @brief filesystem_が保持するファイルハンドルをクローズする
 *
 * @note
 * - FILESYSTEM_FILE_CLOSE_ERRORまたはFILESYSTEM_SUCCESSとなった場合、file_handleはNULL, modeはFILESYSTEM_MODE_NONEにリセットされる。
 * - ファイルハンドルクローズには標準ライブラリのfcloseを使用する。
 * fcloseに失敗する事例として、NASとの接続断等によりファイルの変更内容のフラッシュに失敗した場合がある。
 * この場合、クローズ後のファイルハンドルは再利用不可となりFILESYSTEM_FILE_CLOSE_ERRORを返す。
 *
 * @code{.c}
 * filesystem_t* filesystem = NULL;
 * filesystem_result_t ret = filesystem_create(&filesystem);
 * // エラー処理
 *
 * ret = filesystem_open(filesystem, "/path/to/file", FILESYSTEM_MODE_READ);
 * // エラー処理
 *
 * ret = filesystem_close(filesystem);
 * // エラー処理
 * @endcode
 *
 * @param filesystem_ クローズ対象構造体インスタンスへのポインタ
 *
 * @retval FILESYSTEM_INVALID_ARGUMENT filesystem_がNULL
 * @retval FILESYSTEM_RUNTIME_ERROR 既にクローズ済のファイルハンドルが渡された
 * @retval FILESYSTEM_FILE_CLOSE_ERROR ファイルハンドルのクローズに失敗
 * @retval FILESYSTEM_SUCCESS ファイルハンドルのクローズに成功し、正常終了
 *
 * @todo 既にクローズ済のファイルハンドルが渡された場合の実行結果コードをBAD_OPERATIONに変更する
 */
filesystem_result_t filesystem_close(filesystem_t* filesystem_);

/**
 * @brief ファイルからバイト単位でデータを読み込む
 *
 * @note
 * - 本APIは、成功した場合のみ引数のポインタにデータを書き込むのではなく、失敗した場合でもデータが書き込まれる。
 * これは、ロールバックするためには本API内部でread_bytes_サイズの一時バッファを確保しなければならず、パフォーマンスが低下するため、
 * readの結果は引数のbuffer_に直接書き込むことにする。このため、返り値がエラーとなった場合にはbuffer_の中身を利用してはいけない。
 * なお、result_n_については、エラー発生時は値に0が代入される。
 * - ファイルが末尾に到達し、指定したバイト数に満たないバイト数を読み込んだ場合でも、FILESYSTEM_SUCCESSを出力する。
 * このため、呼び出し側は必ず実行結果コードと合わせて実際に読み込んだバイト数を見て処理を行うこと。
 * - 本APIを使用するためには、下記のいずれかのモードでfilesystem_openを行ったファイルハンドルを使用すること。
 *   - FILESYSTEM_MODE_READ
 *   - FILESYSTEM_MODE_READ_PLUS
 *   - FILESYSTEM_MODE_WRITE_PLUS
 *   - FILESYSTEM_MODE_APPEND_PLUS
 *   - FILESYSTEM_MODE_READ_BINARY
 *   - FILESYSTEM_MODE_READ_PLUS_BINARY
 *   - FILESYSTEM_MODE_WRITE_PLUS_BINARY
 *   - FILESYSTEM_MODE_APPEND_PLUS_BINARY
 *
 * @code{.c}
 * filesystem_t* filesystem = NULL;
 * filesystem_result_t ret = filesystem_create(&filesystem);
 * // エラー処理
 *
 * ret = filesystem_open(filesystem, "/path/to/file", FILESYSTEM_MODE_READ);
 * // エラー処理
 *
 * size_t read_size = 64;
 * size_t result = 0;
 * char buffer[128] = { 0 };
 * ret = filesystem_byte_read(filesystem, read_size, &result, buffer);
 * if(FILESYSTEM_SUCCESS == ret) {
 *     if(result == read_size) {
 *         // 正常読み込み
 *     } else {
 *         // ファイル末尾に到達し、指定バイト数未満を読み込み
 *     }
 * } else {
 *     // エラー処理
 * }
 * @endcode
 *
 * @param filesystem_ 読み込み対象ファイルハンドルを持つ構造体インスタンスへのポインタ
 * @param read_bytes_ 読み込みバイト数
 * @param result_n_ 実際に読み込みに成功したバイト数
 * @param buffer_ データ格納先バッファ(バッファサイズはread_bytes_以上であること)
 *
 * @retval FILESYSTEM_INVALID_ARGUMENT 以下のいずれか
 * - filesystem_がNULL
 * - result_n_がNULL
 * - buffer_がNULL
 * - read_bytes_が0
 * @retval FILESYSTEM_RUNTIME_ERROR 以下のいずれか
 * - 無効なファイルハンドル(== NULL)が渡された
 * - ファイル読み込みでエラーが発生
 * - ファイルオープンモードが読み込み可能モードではない(本APIのnoteを参照)
 * @retval FILESYSTEM_EOF 読み込んだ結果EOFで読み取りバイト数ゼロ
 * @retval FILESYSTEM_SUCCESS 以下のいずれか
 * - 読み込んだ結果EOFとなり指定バイト数未満を読み込み
 * - 指定したバイト数の読み込みに成功し、正常終了
 */
filesystem_result_t filesystem_byte_read(filesystem_t* filesystem_, size_t read_bytes_, size_t* result_n_, char* buffer_);

/**
 * @brief ファイルオープンモードを文字列に変換する
 *
 * @note 不明なモードが入力された場合は文字列"undefined"が返される
 *
 * @param mode_ ファイルオープンモード
 * @return const char* オープンモード文字列
 */
const char* filesystem_open_mode_c_str(filesystem_open_mode_t mode_);

#ifdef __cplusplus
}
#endif
#endif
