/** @ingroup resource
 *
 * @file texture.h
 * @author chocolate-pie24
 * @brief テクスチャCPU側リソース管理モジュールAPI定義
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
#ifndef GLCE_ENGINE_RESOURCE_TEXTURE_TEXTURE_H
#define GLCE_ENGINE_RESOURCE_TEXTURE_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"

typedef struct texture texture_t; /**< テクスチャCPU側リソース内部状態管理構造体前方宣言 */

/**
 * @brief texture_tのメモリを確保し、テクスチャ名称を初期化する(名称以外のフィールドは0, NULLで初期化)
 *
 * @note 処理に失敗した場合、texture_の状態は不変
 *
 * @param[in] name_ テクスチャ名称
 * @param[out] texture_ メモリ確保、初期化対象テクスチャ構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - name_ == NULL
 * - texture_ == NULL
 * - *texture_ == NULL
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_OVERFLOW 計算過程でオーバーフロー発生(名称文字列が長すぎる)
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t texture_create(const char* name_, texture_t** texture_);

/**
 * @brief texture_が保持するリソースと、自身のメモリを解放し、NULLで初期化する
 *
 * @note 2重destroyを許可し、texture_ == NULLまたは*texture_ == NULLの場合は何もしない
 * @warning データ破損により、ピクセルデータリソースの破棄に失敗した場合は、処理を続行しtexture_自身のメモリを解放する(この場合ピクセルデータは再利用不可でメモリリークとなる)
 *
 * @param[in,out] texture_ リソース解放対象構造体インスタンスへのダブルポインタ
 */
void texture_destroy(texture_t** texture_);

/**
 * @brief テクスチャピクセルデータをロードする
 *
 * @note 以下のテクスチャ名称は特殊(テスト用テクスチャ)で、filepath_, extension_はNULLでok
 * - test_texture_red
 * - test_texture_green
 * - test_texture_blue
 * @note filepath_の末尾には'/'を付加すること
 * @note extension_の先頭には'.'を付加すること
 *
 * @param texture_ ロード対象テクスチャ名称を格納するテクスチャ構造体インスタンスへのポインタ
 * @param filepath_ ファイルパス
 * @param extension_ ファイル拡張子
 *
 * @retval RESOURCE_INVALID_ARGUMENT texture_ == NULL
 * @retval RESOURCE_DATA_CORRUPTED 以下のいずれか
 * - texture_->name == NULL
 * - 計算過程でのデータ破損
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - texture_->pixels != NULL
 * - texture_->channel_count != 0
 * - texture_->width != 0
 * - texture_->height != 0
 * - メモリシステム未初期化
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステムの使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ割り当て失敗
 * @retval RESOURCE_OVERFLOW 計算過程でオーバーフロー
 * @retval RESOURCE_FILE_OPEN_ERROR ファイルオープン失敗
 * @retval RESOURCE_FILE_CLOSE_ERROR ファイルクローズ失敗
 * @retval RESOURCE_FILE_READ_ERROR BMPファイルのヘッダまたはピクセルデータの読み込みに失敗
 * @retval RESOURCE_UNSUPPORTED_FILE サポート対象外のBMPファイル(DEBUG_BUILD or TEST_BUILDで詳細なログが出力される)
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t texture_pixel_load(texture_t* texture_, const char* filepath_, const char* extension_);

/**
 * @brief texture_が保持するピクセルデータのメモリを解放し、テクスチャ情報をNULL, 0で初期化する
 *
 * @note 処理に失敗した場合、texture_の状態は不変
 *
 * @param[in,out] texture_ メモリ解放、初期化対象構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT texture_ == NULL
 * @retval RESOURCE_DATA_CORRUPTED texture_->name == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - texture_->channel_count == 0
 * - texture_->height == 0
 * - texture_->width
 * - texture_->pixels == NULL
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t texture_pixel_unload(texture_t* texture_);

/**
 * @brief texture_が保持するピクセルデータへのポインタを取得する(委譲ではない)
 *
 * @note getではなく、moveにして所有権を委譲しても良いかもしれないが、GPUへのアップロードが終わった以降に使う可能性があるかもしれないので、当面はgetにする
 * @note 処理に失敗した場合、out_pixels_の状態は不変
 *
 * @param[in] texture_ ピクセルデータを保持するテクスチャ構造体インスタンスへのポインタ
 * @param[out] out_pixels_ ピクセルデータへのポインタ格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - texture_ == NULL
 * - out_pixels_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - texture_->channel_count == 0
 * - texture_->pixels == NULL
 * - texture_->width == 0
 * - texture_->height == 0
 * @retval RESOURCE_DATA_CORRUPTED texture_->name == NULL
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t texture_pixel_get(const texture_t* texture_, uint8_t** out_pixels_);

/**
 * @brief テクスチャのピクセルサイズを取得する
 *
 * @note 処理に失敗した場合、out引数の状態は不変
 *
 * @param[in] texture_ ピクセルデータを保持するテクスチャ構造体インスタンスへのポインタ
 * @param[out] width_ テクスチャの幅格納先
 * @param[out] height_ テクスチャの高さ格納先
 * @param[out] channel_count_ テクスチャのチャンネルカウント(RGB or RGBA)格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - texture_ == NULL
 * - width_ == NULL
 * - height_ == NULL
 * - channel_count_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - texture_->channel_count == 0
 * - texture_->pixels == NULL
 * - texture_->width == 0
 * - texture_->height == 0
 * @retval RESOURCE_DATA_CORRUPTED texture_->name
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
resource_result_t texture_pixel_size_get(const texture_t* texture_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_);

/**
 * @brief texture_が保持するテクスチャ名称をchar*型文字列で取得する
 *
 * @note texture_ == NULLまたはtexture_->name == NULLの場合はNULLが返される
 * @note texture_->nameがNULLの場合は、データ破損(createで必ず初期化されるため)のため、ワーニングメッセージを出力する
 *
 * @param[in] texture_ テクスチャ名称取得元構造体インスタンスへのポインタ
 *
 * @return const char* テクスチャ名称文字列
 */
const char* texture_name_get(const texture_t* texture_);

#ifdef __cplusplus
}
#endif
#endif
