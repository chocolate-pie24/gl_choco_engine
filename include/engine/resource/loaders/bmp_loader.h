/** @ingroup resource
 *
 * @file bmp_loader.h
 * @author chocolate-pie24
 * @brief BMPファイルのロード処理を行うAPIの定義
 *
 * @details 以下のファイルをサポートする
 * - 非圧縮BMPファイル
 * - ピクセルのチャンネルカウントはRGB or RGBAのみ
 * - 画像の高さがint16_tに収まること
 * - 画像の幅がが0より大きく、かつint16_tに収まること
 *
 * @version 0.1
 * @date 2026-05-14
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_ENGINE_RESOURCE_LOADERS_BMP_LOADER_H
#define GLCE_ENGINE_RESOURCE_LOADERS_BMP_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/resource/resource_core/resource_types.h"

typedef struct bmp_loader bmp_loader_t; /**< BMPローダー内部状態管理構造体前方宣言 */

/**
 * @brief bmp_loader_t構造体インスタンスのメモリを確保し、フィールドを0, NULL, falseで初期化する
 *
 * @note 処理に失敗した場合、bmp_loader_の状態は不変
 *
 * @param[out] bmp_loader_ メモリ確保、初期化対象構造体インスタンスへのダブルポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - bmp_loader_ == NULL
 * - *bmp_loader_ != NULL
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステムの使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t bmp_loader_create(bmp_loader_t** bmp_loader_);

/**
 * @brief bmp_loader_が保持するメモリを解放し、自身のメモリをNULLで初期化する
 *
 * @note 2重destroy許可で、bmp_loader_ == NULL or *bmp_loader_ == NULLの場合は何もしない
 *
 * @param[in,out] bmp_loader_ リソース解放対象構造体インスタンスへのダブルポインタ
 */
void bmp_loader_destroy(bmp_loader_t** bmp_loader_);

/**
 * @brief BMPファイルをロードする
 *
 * @note ロード後のピクセルデータは以下の特徴を持つ
 * - BGR -> RGBの順でピクセルデータが格納される
 * - 24bit(RGB)形式の場合でもpaddingは0で密にデータが格納される
 * - 画像原点は左上を基準とする
 *
 * @note 処理に失敗した場合、bmp_loader_の状態は不変
 *
 * @param[in] fullpath_ BMPファイルフルパス
 * @param[in,out] bmp_loader_ ロードデータ格納先構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - fullpath_ == NULL
 * - bmp_loader_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - bmp_loader_->pixels != NULL
 * - メモリシステム未初期化
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステムの使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_FILE_OPEN_ERROR ファイルオープン失敗
 * @retval RESOURCE_FILE_CLOSE_ERROR ファイルクローズ失敗
 * @retval RESOURCE_UNDEFINED_ERROR 未定義エラーが発生
 * @retval RESOURCE_FILE_READ_ERROR ヘッダまたはピクセル情報の読み込みに失敗
 * @retval RESOURCE_UNSUPPORTED_FILE サポート対象外のBMPファイル(DEBUG_BUILD or TEST_BUILDで詳細なログが出力される)
 * @retval RESOURCE_OVERFLOW 計算過程でオーバーフロー発生
 * @retval RESOURCE_DATA_CORRUPTED ピクセル読み込みサイズ異常またはヘッダ情報破損
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t bmp_loader_load(const char* fullpath_, bmp_loader_t* bmp_loader_);

/**
 * @brief bmp_loader_が保有するピクセルデータメモリの所有権をout_pixels_に委譲し、自身のピクセルデータはNULLにする
 *
 * @note 委譲後のbmp_loader_->pixelsは再使用不可で、destroyをしてから再度bmp_loader_create -> bmp_loader_loadを行うこと
 *
 * @note 処理に失敗した場合、out_pixels_の状態は不変
 *
 * @param[in,out] bmp_loader_ 委譲元のピクセルデータを保持する構造体インスタンスへのポインタ
 * @param[out] out_pixels_ メモリ委譲先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - bmp_loader_ == NULL
 * - out_pixels_ == NULL
 * - *out_pixels_ != NULL
 * @retval RESOURCE_BAD_OPERATION bmp_loader_->pixels == NULL
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t bmp_loader_pixel_move(bmp_loader_t* bmp_loader_, uint8_t** out_pixels_);

/**
 * @brief BMPファイルのサイズ情報を取得する
 *
 * @note 処理に失敗した場合はout引数の状態は不変
 * @note 画像原点が左上でheightが負の場合は絶対値を返す
 *
 * @param[in] bmp_loader_ BMPローダー構造体インスタンスへのポインタ
 * @param[out] width_ 画像の幅情報格納先
 * @param[out] height_ 画像の高さ情報格納先
 * @param[out] channel_count_ 画像のチャンネルカウント(RGB or RGBA)格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - bmp_loader_ == NULL
 * - width_ == NULL
 * - height_ == NULL
 * - channel_count_ == NULL
 * @retval RESOURCE_BAD_OPERATION 画像が未ロード状態
 * @retval RESOURCE_UNDEFINED_ERROR 不明なエラーが発生
 * @retval RESOURCE_UNSUPPORTED_FILE サポート対象外のBMPファイル(DEBUG_BUILD or TEST_BUILDで詳細なログが出力される)
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t bmp_loader_bmp_size_get(const bmp_loader_t* bmp_loader_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_);

#ifdef __cplusplus
}
#endif
#endif
