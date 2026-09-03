// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file bmp_loader.c
 * @author chocolate-pie24
 * @brief BMPファイルのロード処理を行うAPIの実装
 *
 * @details GLCEでは以下のBMPファイルをサポートする
 * - 非圧縮BMPファイル
 * - ピクセルのチャンネルカウントはRGB or RGBAのみ
 * - 画像の高さがint16_tに収まること
 * - 画像の幅がが0より大きく、かつint16_tに収まること
 *
 * @date 2026-05-14
 *
 * @todo 計算各所のオーバーフローチェック漏れ修正
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/resource/loaders/bmp_loader.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/buffer_utils/buffer_utils.h"

#include "engine/io_utils/fs_stream.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

// #define TEST_BUILD

/**
 * @brief 無効なBMPファイルの原因リスト
 *
 */
typedef enum {
    BMP_FILE_VALID,                 /**< BMPファイル有効 */
    BMP_FILE_INVALID_BF_TYPE,       /**< bfType異常 */
    BMP_FILE_INVALID_BF_RESERVED,   /**< bfReserved1(or2)異常 */
    BMP_FILE_INVALID_BF_SIZE,       /**< bfSize異常 */
    BMP_FILE_INVALID_BF_OFF_BITS,   /**< bfOffBits異常 */
    BMP_FILE_INVALID_BI_SIZE,       /**< biSize異常  */
    BMP_FILE_INVALID_BI_PLANES,     /**< biPlanes異常 */
    BMP_FILE_INVALID_COMPRESSION,   /**< biCompression異常 */
    BMP_FILE_INVALID_HEIGHT,        /**< biHeight異常 */
    BMP_FILE_INVALID_WIDTH,         /**< biWidth異常 */
    BMP_FILE_INVALID_CHANNEL_COUNT, /**< チャンネルカウント異常 */
    BMP_FILE_UNDEFINED,             /**< 未定義エラー */
} bmp_invalid_reason_t;

/**
 * @brief BMP FILEHEADER(14byte固定)
 * @note 参考: https://qiita.com/ImagingSolAkira/items/30fd3727afa3076b8050
 *
 */
typedef struct file_header {
    uint16_t bf_type;       /**< bfType: ファイルタイプ。必ず "BM" (0x4D42), offset = 0 */
    uint16_t bf_reserved1;  /**< bfReserved1: 予約領域 (通常 0), offset = 6 */
    uint16_t bf_reserved2;  /**< bfReserved2: 予約領域 (通常 0), offset = 8 */

    uint32_t bf_size;       /**< bfSize ファイル全体のサイズ (バイト), offset = 2 */
    uint32_t bf_off_bits;   /**< bfOffBits: ファイル先頭からピクセルデータの開始位置までのオフセット(バイト), offset = 10 */
} file_header_t;

/**
 * @brief BMP INFOHEADER(40byte固定)
 * @note 参考: https://qiita.com/ImagingSolAkira/items/30fd3727afa3076b8050
 *
 */
typedef struct info_header {
    uint32_t bi_size;               /**< biSize: このヘッダーのサイズ(バイト)。通常40(0x28), offset = 14  */
    int32_t bi_width;               /**< biWidth: 画像の幅(ピクセル), offset = 18 */
    int32_t bi_height;              /**< biHeight: 画像の高さ(ピクセル), 正の値: ボトムアップ形式(左下原点), 負の値: トップダウン形式(左上原点)  offset = 22 */
    uint32_t bi_compression;        /**< biCompression: 圧縮形式, offset = 30 */
    uint32_t bi_size_image;         /**< biSizeImage: ピクセルデータ部分のサイズ (バイト), offset = 34 */
    int32_t bi_x_pels_per_meter;    /**< biXPelsPerMeter: 水平解像度 (ピクセル/メートル)。通常 0, offset = 38 */
    int32_t bi_y_pels_per_meter;    /**< biYPelsPerMeter: 垂直解像度 (ピクセル/メートル)。通常 0, offset = 42 */
    uint32_t bi_clr_used;           /**< biClrUsed: カラーパレット内の色数, offset = 46 */
    uint32_t bi_clr_important;      /**< biClrImportant: 重要な色の数。0の場合、全ての色が重要, offset = 50 */

    uint16_t bi_planes;             /**< biPlanes: プレーン数(常に 1), offset = 26 */
    uint16_t bi_bit_count;          /**< biBitCount: 1ピクセルあたりのビット数(色深度)。1, 4, 8, 16, 24, 32 など, offset = 28 */
} info_header_t;

static resource_result_t bmp_loader_pixel_bgr_to_rgb(const info_header_t* info_header_, uint8_t* pixels_);
static resource_result_t bmp_loader_pixel_flip(const info_header_t* info_header_, uint8_t* pixels_);
static resource_result_t bmp_loader_padding_remove(const info_header_t* info_header_, size_t stride_, size_t padding_, const uint8_t* src_pixels_, uint8_t** dst_pixels_, size_t* out_new_size_);

static resource_result_t header_load(const char* fullpath_, file_header_t* file_header_, info_header_t* info_header_);
static resource_result_t pixel_load(const char* fullpath_, const file_header_t* file_header_, info_header_t* info_header_, size_t stride_, uint8_t** out_pixels_);

static resource_result_t file_header_parse(const char header_[54], file_header_t* file_header_);
static resource_result_t info_header_parse(const char header_[54], info_header_t* info_header_);

static void file_header_copy(const file_header_t* src_, file_header_t* dst_);
static void info_header_copy(const info_header_t* src_, info_header_t* dst_);

static bmp_invalid_reason_t is_bmp_supported(const file_header_t* file_header_, const info_header_t* info_header_);
static const char* invalid_reason_to_str(bmp_invalid_reason_t reason_);

static const char* const invalid_bmp_file_reason_valid = "valid BMP file";                      /**< 無効なBMPファイルの原因文字列(BMPファイル有効) */
static const char* const invalid_bmp_file_reason_bf_type = "invalid bfType";                    /**< 無効なBMPファイルの原因文字列(bfType異常) */
static const char* const invalid_bmp_file_reason_bf_reserved = "invalid bfReserved field";      /**< 無効なBMPファイルの原因文字列(bfReserved異常) */
static const char* const invalid_bmp_file_reason_bf_size = "invalid bfSize";                    /**< 無効なBMPファイルの原因文字列(bfSize異常) */
static const char* const invalid_bmp_file_reason_bf_off_bits = "invalid bfOffBits";             /**< 無効なBMPファイルの原因文字列(bfOffBits異常) */
static const char* const invalid_bmp_file_reason_bi_size = "invalid biSize";                    /**< 無効なBMPファイルの原因文字列(biSize異常) */
static const char* const invalid_bmp_file_reason_bi_planes = "invalid biPlanes";                /**< 無効なBMPファイルの原因文字列(biPlanes異常) */
static const char* const invalid_bmp_file_reason_compression = "invalid biCompression";         /**< 無効なBMPファイルの原因文字列(biCompression異常) */
static const char* const invalid_bmp_file_reason_height = "invalid biHeight";                   /**< 無効なBMPファイルの原因文字列(biHeight異常) */
static const char* const invalid_bmp_file_reason_width = "invalid biWidth";                     /**< 無効なBMPファイルの原因文字列(biWidth異常) */
static const char* const invalid_bmp_file_reason_channel_count = "unsupported biBitCount";      /**< 無効なBMPファイルの原因文字列(biBitCount異常) */
static const char* const invalid_bmp_file_reason_undefined = "undefined";                       /**< 無効なBMPファイルの原因文字列(不明な異常) */

resource_result_t bmp_loader_load(const char* fullpath_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, size_t* out_pixel_data_size_, uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    bmp_invalid_reason_t valid_bmp = BMP_FILE_UNDEFINED;

    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };
    uint8_t* tmp_pixels = NULL;
    uint8_t* formatted_pixels = NULL;
    size_t formatted_size = 0;
    size_t bit_count = 0;
    size_t stride = 0;
    size_t padding = 0;

    size_t tmp_width = 0;
    size_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_width_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "out_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_height_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "out_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "out_channel_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixel_data_size_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "out_pixel_data_size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_load", "*out_pixels_")
    if('\0' == fullpath_[0]) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("bmp_loader_load(%s) - Provided fullpath_ is not valid.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = header_load(fullpath_, &tmp_file_header, &tmp_info_header);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to load BMP header.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    valid_bmp = is_bmp_supported(&tmp_file_header, &tmp_info_header);
    if(BMP_FILE_UNDEFINED == valid_bmp) {
        ret = RESOURCE_UNDEFINED_ERROR;
        ERROR_MESSAGE("bmp_loader_load(%s) - Undefined error.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_VALID != valid_bmp) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_load(%s) - Unsupported BMP file. reason = '%s'", resource_rslt_to_str(ret), invalid_reason_to_str(valid_bmp));
        goto cleanup;
    }

    tmp_width = (size_t)(tmp_info_header.bi_width);
    bit_count = (size_t)(tmp_info_header.bi_bit_count);

    // 現状ではINT16_MAXがサイズの上限なので不要だが、将来的な拡張のためにチェックをいれる
    if((SIZE_MAX / tmp_width) < bit_count) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to calculate BMP row stride: bit_count * width would overflow. width=%zu, bit_count=%zu", resource_rslt_to_str(ret), tmp_width, bit_count);
        goto cleanup;
    }
    if((SIZE_MAX - 31) < (bit_count * tmp_width)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to calculate BMP row stride: row bit count alignment overflow. row_bits=%zu", resource_rslt_to_str(ret), bit_count * tmp_width);
        goto cleanup;
    }

    stride = ((bit_count * tmp_width + 31) / 32) * 4;
    padding = stride - (bit_count * tmp_width / 8);
    ret = pixel_load(fullpath_, &tmp_file_header, &tmp_info_header, stride, &tmp_pixels);   // 内部でtmp_pixelsのメモリが確保されるが、失敗時には解放される
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to load BMP pixel data.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if(0 < padding) {
        ret = bmp_loader_padding_remove(&tmp_info_header, stride, padding, tmp_pixels, &formatted_pixels, &formatted_size); // 内部でformatted_pixelsのメモリが確保されるが、失敗時には解放される
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("bmp_loader_load(%s) - Failed to remove BMP row padding.", resource_rslt_to_str(ret));
            goto cleanup;
        }
        memory_system_free(tmp_pixels, tmp_info_header.bi_size_image, MEMORY_TAG_TEXTURE);
        tmp_pixels = NULL;
        tmp_pixels = formatted_pixels;
        formatted_pixels = NULL;
        tmp_info_header.bi_size_image = (uint32_t)formatted_size;
    }

    ret = bmp_loader_pixel_bgr_to_rgb(&tmp_info_header, tmp_pixels);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to convert BGR to RGB.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = bmp_loader_pixel_flip(&tmp_info_header, tmp_pixels);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to flip BMP pixel data vertically.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_height = (tmp_info_header.bi_height > 0) ? (size_t)tmp_info_header.bi_height : (size_t)(-1 * tmp_info_header.bi_height);
    if(24 == tmp_info_header.bi_bit_count) {
        tmp_channel_count = 3;
    } else if(32 == tmp_info_header.bi_bit_count) {
        tmp_channel_count = 4;
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_load(%s) - Unsupported BMP bit count.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *out_width_ = (uint16_t)tmp_width;
    *out_height_ = (uint16_t)tmp_height;
    *out_channel_count_ = tmp_channel_count;
    *out_pixel_data_size_ = tmp_info_header.bi_size_image;
    *out_pixels_ = tmp_pixels;
    tmp_pixels = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != formatted_pixels && 0 != formatted_size) {
        memory_system_free(formatted_pixels, formatted_size, MEMORY_TAG_TEXTURE);
        formatted_pixels = NULL;
    }
    if(NULL != tmp_pixels) {
        memory_system_free(tmp_pixels, tmp_info_header.bi_size_image, MEMORY_TAG_TEXTURE);
        tmp_pixels = NULL;
    }
    return ret;
}

/**
 * @brief BMPのピクセルデータをBGRからRGBに変換する
 *
 * @warning この関数は必ずpadding除去後に実行すること
 * @note 処理に失敗した場合は、pixels_の状態は不変
 *
 * @param[in] info_header_ INFOHEADER構造体インスタンスへのポインタ
 * @param[in,out] pixels_ 変換対象ピクセルデータ配列
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - info_header_ == NULL
 * - pixels_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - info_header_->bi_width == 0
 * - info_header_->bi_height == 0
 * @retval RESOURCE_UNSUPPORTED_FILE サポート対象外のBMPファイル
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 *
 * @todo TODO: paddingが除去されていないpixels_が渡された場合、メモリアクセス違反となるため、padding除去済みフラグを引数に追加するか、ピクセルバッファサイズを引数に追加する
 */
static resource_result_t bmp_loader_pixel_bgr_to_rgb(const info_header_t* info_header_, uint8_t* pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    size_t channel_count = 0;
    size_t width = 0;
    size_t height = 0;
    size_t ii = 0;

    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_pixel_bgr_to_rgb", "info_header_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_pixel_bgr_to_rgb", "pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != info_header_->bi_width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_pixel_bgr_to_rgb", "info_header_->bi_width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != info_header_->bi_height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_pixel_bgr_to_rgb", "info_header_->bi_height")

    if(24 == info_header_->bi_bit_count) {
        channel_count = 3;
    } else if(32 == info_header_->bi_bit_count) {
        channel_count = 4;
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_pixel_bgr_to_rgb(%s) - Unsupported BMP bit count.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    width = (size_t)(info_header_->bi_width);
    height = (0 < info_header_->bi_height) ? (size_t)info_header_->bi_height : (size_t)(-1 * (int64_t)(info_header_->bi_height));
    for(size_t i = 0; i != height; ++i) {
        for(size_t j = 0; j != width; ++j) {
            const uint8_t tmp = pixels_[ii];
            pixels_[ii] = pixels_[ii + 2];
            pixels_[ii + 2] = tmp;
            ii += channel_count;
        }
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief GLCEの画像は左上原点とするため、左下原点の画像を左上原点に直す
 *
 * @warning この関数は必ずpadding除去後に実行すること
 * @note 既に左上原点の場合は何もしないでRESOURCE_SUCCESSを返す
 * @note 処理に失敗した場合は、pixels_の状態は不変
 *
 * @param[in] info_header_ INFOHEADER構造体インスタンスへのポインタ
 * @param[in,out] pixels_ 座標原点変換対象ピクセル配列
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - info_header_ == NULL
 * - pixels_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - info_header_->bi_width == 0
 * - info_header_->bi_height == 0
 * @retval RESOURCE_UNSUPPORTED_FILE RGB、RGBA以外のBMPファイル
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 *
 * @todo TODO: paddingが除去されていないpixels_が渡された場合、メモリアクセス違反となるため、padding除去済みフラグを引数に追加するか、ピクセルバッファサイズを引数に追加する
 */
static resource_result_t bmp_loader_pixel_flip(const info_header_t* info_header_, uint8_t* pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    size_t channel_count = 0;
    size_t width = 0;
    size_t height = 0;

    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_pixel_flip", "info_header_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_pixel_flip", "pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != info_header_->bi_width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_pixel_flip", "info_header_->bi_width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != info_header_->bi_height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_pixel_flip", "info_header_->bi_height")

    if(24 == info_header_->bi_bit_count) {
        channel_count = 3;
    } else if(32 == info_header_->bi_bit_count) {
        channel_count = 4;
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_pixel_flip(%s) - Unsupported BMP bit count.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 > info_header_->bi_height) {
        ret = RESOURCE_SUCCESS;
        goto cleanup;
    }

    width = (size_t)(info_header_->bi_width);
    height = (size_t)(info_header_->bi_height);

    if(1 == height) {
        // flip不要なので何もしない
    } else if(2 == height) {
        const size_t width_count = width * channel_count;
        for(size_t i = 0; i != width_count; ++i) {
            uint8_t tmp = pixels_[i];
            pixels_[i] = pixels_[width_count + i];
            pixels_[width_count + i] = tmp;
        }
    } else {
        size_t back = height - 1;
        const size_t to = height / 2;
        const size_t width_count = width * channel_count;
        for(size_t i = 0; i != to; ++i) {
            for(size_t j = 0; j != width_count; ++j) {
                uint8_t tmp = pixels_[i * width_count + j];
                pixels_[i * width_count + j] = pixels_[back * width_count + j];
                pixels_[back * width_count + j] = tmp;
            }
            back = back - 1;
        }
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief 読み込んだピクセルデータからpaddingを除去する
 *
 * @warning 24bit BMPデータ専用, 32bitではpaddingが発生しないため、呼び出し対象外で、RESOURCE_BAD_OPERATIONを返す
 * @note 処理に失敗した場合、out引数は不変
 *
 * @param[in] info_header_ INFOHEADER構造体インスタンスへのポインタ
 * @param[in] stride_ BMPファイルの各行のサイズ(byte)
 * @param[in] padding_ パディングサイズ
 * @param[in] src_pixels_ padding除去前のピクセルデータ
 * @param[out] dst_pixels_ padding除去後のピクセルデータ
 * @param[out] out_new_size_ padding除去後のピクセルデータサイズ(byte)
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - info_header_ == NULL
 * - src_pixels_ == NULL
 * - dst_pixels_ == NULL
 * - out_new_size_ == NULL
 * @retval RESOURCE_BAD_OPERATION 以下のいずれか
 * - stride_ == 0
 * - padding_ == 0
 * - info_header_->bi_width == 0
 * - info_header_->bi_height == 0
 * - info_header_->bi_bit_count != 24
 * - メモリシステム未初期化
 * @retval RESOURCE_OVERFLOW 計算過程でオーバーフローが発生
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステムの使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t bmp_loader_padding_remove(const info_header_t* info_header_, size_t stride_, size_t padding_, const uint8_t* src_pixels_, uint8_t** dst_pixels_, size_t* out_new_size_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    uint8_t* new_pixel = NULL;
    size_t new_size = 0;
    size_t width = 0;
    const size_t channel_count = 3;
    size_t height = 0;

    size_t ii = 0;
    size_t ii_new = 0;

    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "info_header_")
    IF_ARG_NULL_GOTO_CLEANUP(src_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "src_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(dst_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "dst_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*dst_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "*dst_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_new_size_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "out_new_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != stride_, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_padding_remove", "stride_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != padding_, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_padding_remove", "padding_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != info_header_->bi_width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_padding_remove", "info_header_->bi_width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != info_header_->bi_height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_padding_remove", "info_header_->bi_height")
    // 32bit BMPでは通常padding = 0なので、BAD_OPERATION(24, 32以外はnot supportedでis_bmp_supportedで弾かれる)
    IF_ARG_FALSE_GOTO_CLEANUP(24 == info_header_->bi_bit_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_padding_remove", "info_header_->bi_bit_count")

    width = (size_t)(info_header_->bi_width);
    height = (0 < info_header_->bi_height) ? (size_t)info_header_->bi_height : (size_t)(-1 * (int64_t)(info_header_->bi_height));

    // NOTE: 現状はサイズがint16_tなのでオーバーフローにはならないが、将来の拡張のために入れておく
    if((SIZE_MAX / height) < width) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - Failed to calculate BMP output pixel count: width * height would overflow. width=%zu, height=%zu", resource_rslt_to_str(ret), width, height);
        goto cleanup;
    }
    if((SIZE_MAX / channel_count) < (width * height)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - Failed to calculate BMP output image size: pixel_count * channel_count would overflow. pixel_count=%zu, channel_count=%zu", resource_rslt_to_str(ret), width * height, channel_count);
        goto cleanup;
    }

    new_size = width * height * channel_count;
    if(new_size > UINT32_MAX) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - BMP output image size exceeds uint32_t range. output_size=%zu, limit=%u", resource_rslt_to_str(ret), new_size, UINT32_MAX);
        goto cleanup;
    }
    ret_mem = memory_system_allocate(new_size, MEMORY_TAG_TEXTURE, (void**)&new_pixel);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - Failed to allocate memory for new_pixel.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != height; ++i) {
        for(size_t j = 0; j != width; ++j) {
            for(size_t k = 0; k != channel_count; ++k) {
                new_pixel[ii_new + k] = src_pixels_[ii + k];
            }
            ii_new += channel_count;
            ii += channel_count;
        }
        ii += padding_;
    }

    *dst_pixels_ = new_pixel;
    *out_new_size_ = new_size;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != new_pixel) {
            memory_system_free(new_pixel, new_size, MEMORY_TAG_TEXTURE);
            new_pixel = NULL;
        }
    }
    return ret;
}

static resource_result_t header_load(const char* fullpath_, file_header_t* file_header_, info_header_t* info_header_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;
    size_t read_size = 0;
    char header_buf[54] = { 0 };

    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "header_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(file_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "header_load", "file_header_")
    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "header_load", "info_header_")

    ret_fs_stream = fs_stream_create(&fs_stream, fullpath_, FS_OPEN_MODE_READ_BINARY);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_rslt_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("header_load(%s) - fs_stream_create failed.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_stream = fs_stream_byte_read(fs_stream, 54, &read_size, header_buf);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_rslt_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("header_load(%s) - Failed to read BMP file header.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(54 != read_size) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("header_load(%s) - Invalid BMP file format: header size is invalid. header size = %zu", resource_rslt_to_str(ret), read_size);
        goto cleanup;
    }

    ret = file_header_parse(header_buf, &tmp_file_header);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("header_load(%s) - Failed to parse BMP file header.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    ret = info_header_parse(header_buf, &tmp_info_header);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("header_load(%s) - Failed to parse BMP info header.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    // close失敗はfs_stream_destroy内のERROR_MESSAGEを出力するのみとし、エラー処理は行わない
    fs_stream_destroy(&fs_stream, NULL);

    file_header_copy(&tmp_file_header, file_header_);
    info_header_copy(&tmp_info_header, info_header_);

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        fs_stream_destroy(&fs_stream, NULL);
    }
    return ret;
}

/**
 * @brief BMPファイルのヘッダ以降のピクセルデータを読み込む
 *
 * @note 引数info_header_はbi_size_imageを更新するため非const(画像変換ツールによっては正しく情報が設定されていない場合があるため)
 * @note 処理に失敗した場合、out引数は不変
 *
 * @param[in] fullpath_ BMPファイルのフルパス
 * @param[in] file_header_ FILEHEADER構造体インスタンスへのポインタ
 * @param[in,out] info_header_ INFOHEADER構造体インスタンスへのポインタ
 * @param[in] stride_ BMPファイルの各行のサイズ(byte)
 * @param[out] out_pixels_ 読み込んだピクセルデータの格納先(メモリは本関数内で確保する)
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - fullpath_ == NULL
 * - file_header_ == NULL
 * - info_header_ == NULL
 * - out_pixels_ == NULL
 * - *out_pixels_ != NULL
 * - stride_ == 0
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステムの使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_FILE_OPEN_ERROR ファイルオープン失敗
 * @retval RESOURCE_UNDEFINED_ERROR 未定義エラーが発生
 * @retval RESOURCE_FILE_READ_ERROR ピクセル読み込み失敗
 * @retval RESOURCE_DATA_CORRUPTED ピクセル読み込みサイズ異常
 * @retval RESOURCE_OVERFLOW 計算過程でオーバーフロー発生
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t pixel_load(const char* fullpath_, const file_header_t* file_header_, info_header_t* info_header_, size_t stride_, uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;
    uint8_t* tmp_buffer = NULL;
    uint8_t* tmp_pixels = NULL;
    size_t read_size_all = 0;
    size_t pixel_buffer_size = 0;
    size_t height = 0;

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(file_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_load", "file_header_")
    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_load", "info_header_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_load", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_load", "*out_pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != stride_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_load", "stride_")

    ret_mem = memory_system_allocate(file_header_->bf_size, MEMORY_TAG_TEXTURE, (void**)&tmp_buffer);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("pixel_load(%s) - Failed to allocate memory for tmp_buffer.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_stream = fs_stream_create(&fs_stream, fullpath_, FS_OPEN_MODE_READ_BINARY);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_rslt_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("pixel_load(%s) - fs_stream_create failed.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_stream = fs_stream_byte_read(fs_stream, file_header_->bf_size, &read_size_all, (char*)tmp_buffer);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_rslt_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("pixel_load(%s) - Failed to read BMP file(%s).", resource_rslt_to_str(ret), fullpath_);
        goto cleanup;
    } else if(file_header_->bf_size != read_size_all) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("pixel_load(%s) - Invalid file size.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    // NOTE: info_header_->bi_size_imageはツールによっては信用できない値が入るので、strideとheightから自前で計算する
    height = (0 < info_header_->bi_height) ? (size_t)(info_header_->bi_height) : (size_t)(-1 * (int64_t)info_header_->bi_height);
    if(SIZE_MAX / height < stride_) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("pixel_load(%s) - Failed to calculate BMP source pixel buffer size: stride * height would overflow. stride=%zu, height=%zu", resource_rslt_to_str(ret), stride_, height);
        goto cleanup;
    }
    pixel_buffer_size = stride_ * height;
    if(pixel_buffer_size > UINT32_MAX) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("pixel_load(%s) - BMP source pixel buffer size exceeds uint32_t range. pixel_buffer_size=%zu, limit=%u", resource_rslt_to_str(ret), pixel_buffer_size, UINT32_MAX);
        goto cleanup;
    }

    if((SIZE_MAX - pixel_buffer_size) < file_header_->bf_off_bits) {
        // NOTE: bf_off_bitsとpixel_buffer_sizeはuint32_tに収まるようになっているため、ここは通らないためカバレッジは100にならない。許容する。
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("pixel_load(%s) - BMP pixel data range overflow: bfOffBits + pixel_buffer_size would overflow. bfOffBits=%u, pixel_buffer_size=%zu", resource_rslt_to_str(ret), file_header_->bf_off_bits, pixel_buffer_size);
        goto cleanup;
    }
    if((file_header_->bf_off_bits + pixel_buffer_size) > file_header_->bf_size) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("pixel_load(%s) - Invalid BMP pixel data range: pixel data extends beyond file size. bfOffBits=%u, pixel_buffer_size=%zu, bfSize=%u", resource_rslt_to_str(ret), file_header_->bf_off_bits, pixel_buffer_size, file_header_->bf_size);
        goto cleanup;
    }

    ret_mem = memory_system_allocate(pixel_buffer_size, MEMORY_TAG_TEXTURE, (void**)&tmp_pixels);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("pixel_load(%s) - Failed to allocate memory for tmp_pixels.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != pixel_buffer_size; ++i) {
        tmp_pixels[i] = tmp_buffer[i + file_header_->bf_off_bits];
    }

    // close失敗はfs_stream_destroyないのERROR_MESSAGEを出力するのみとし、エラー処理は行わない
    fs_stream_destroy(&fs_stream, NULL);
    memory_system_free(tmp_buffer, file_header_->bf_size, MEMORY_TAG_TEXTURE);
    tmp_buffer = NULL;

    info_header_->bi_size_image = (uint32_t)pixel_buffer_size;
    *out_pixels_ = tmp_pixels;
    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        fs_stream_destroy(&fs_stream, NULL);
        if(NULL != tmp_buffer) {
            memory_system_free(tmp_buffer, file_header_->bf_size, MEMORY_TAG_TEXTURE);
            tmp_buffer = NULL;
        }
        if(NULL != tmp_pixels) {
            memory_system_free(tmp_pixels, pixel_buffer_size, MEMORY_TAG_TEXTURE);
            tmp_pixels = NULL;
        }
    }
    return ret;
}

/**
 * @brief FILEHEADER情報文字列をパースし、構造体にパラメータを格納する
 *
 * @param[in] header_ ヘッダ情報文字列(FILEHEADER + INFOHEADER)
 * @param[out] file_header_ FILEHEADER情報格納先構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - header_ == NULL
 * - file_header_ == NULL
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t file_header_parse(const char header_[54], file_header_t* file_header_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    file_header_t tmp_header = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "file_header_parse", "header_")
    IF_ARG_NULL_GOTO_CLEANUP(file_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "file_header_parse", "file_header_")

    tmp_header.bf_type = buffer_utils_le_uint16_t_get(header_);
    tmp_header.bf_size = buffer_utils_le_uint32_t_get(header_ + 2);
    tmp_header.bf_reserved1 = buffer_utils_le_uint16_t_get(header_ + 6);
    tmp_header.bf_reserved2 = buffer_utils_le_uint16_t_get(header_ + 8);
    tmp_header.bf_off_bits = buffer_utils_le_uint32_t_get(header_ + 10);

    file_header_copy(&tmp_header, file_header_);

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief INFOHEADER情報文字列をパースし、構造体にパラメータを格納する
 *
 * @param[in] header_ ヘッダ情報文字列(FILEHEADER + INFOHEADER)
 * @param[out] info_header_ INFOHEADER情報格納先構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - header_ == NULL
 * - info_header_ == NULL
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t info_header_parse(const char header_[54], info_header_t* info_header_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    info_header_t tmp_header = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "info_header_parse", "header_")
    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "info_header_parse", "info_header_")

    tmp_header.bi_size = buffer_utils_le_uint32_t_get(header_ + 14);
    tmp_header.bi_width = buffer_utils_le_int32_t_get(header_ + 18);
    tmp_header.bi_height = buffer_utils_le_int32_t_get(header_ + 22);
    tmp_header.bi_planes = buffer_utils_le_uint16_t_get(header_ + 26);
    tmp_header.bi_bit_count = buffer_utils_le_uint16_t_get(header_ + 28);
    tmp_header.bi_compression = buffer_utils_le_uint32_t_get(header_ + 30);
    tmp_header.bi_size_image = buffer_utils_le_uint32_t_get(header_ + 34);
    tmp_header.bi_x_pels_per_meter = buffer_utils_le_int32_t_get(header_ + 38);
    tmp_header.bi_y_pels_per_meter = buffer_utils_le_int32_t_get(header_ + 42);
    tmp_header.bi_clr_used = buffer_utils_le_uint32_t_get(header_ + 46);
    tmp_header.bi_clr_important = buffer_utils_le_uint32_t_get(header_ + 50);

    info_header_copy(&tmp_header, info_header_);

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief FILEHEADER情報をコピーする
 *
 * @note src_ == NULL or dst_ == NULLの場合は何もしない
 *
 * @param[in] src_ コピー元ヘッダ
 * @param[out] dst_ コピー先ヘッダ
 */
static void file_header_copy(const file_header_t* src_, file_header_t* dst_) {
    if(NULL == src_ || NULL == dst_) {
        return;
    }
    dst_->bf_off_bits = src_->bf_off_bits;
    dst_->bf_reserved1 = src_->bf_reserved1;
    dst_->bf_reserved2 = src_->bf_reserved2;
    dst_->bf_size = src_->bf_size;
    dst_->bf_type = src_->bf_type;
}

/**
 * @brief INFOHEADER情報をコピーする
 *
 * @note src_ == NULL or dst_ == NULLの場合は何もしない
 *
 * @param[in] src_ コピー元ヘッダ
 * @param[out] dst_ コピー先ヘッダ
 */
static void info_header_copy(const info_header_t* src_, info_header_t* dst_) {
    if(NULL == src_ || NULL == dst_) {
        return;
    }
    dst_->bi_bit_count = src_->bi_bit_count;
    dst_->bi_clr_important = src_->bi_clr_important;
    dst_->bi_clr_used = src_->bi_clr_used;
    dst_->bi_compression = src_->bi_compression;
    dst_->bi_height = src_->bi_height;
    dst_->bi_planes = src_->bi_planes;
    dst_->bi_size = src_->bi_size;
    dst_->bi_size_image = src_->bi_size_image;
    dst_->bi_width = src_->bi_width;
    dst_->bi_x_pels_per_meter = src_->bi_x_pels_per_meter;
    dst_->bi_y_pels_per_meter = src_->bi_y_pels_per_meter;
}

static bmp_invalid_reason_t is_bmp_supported(const file_header_t* file_header_, const info_header_t* info_header_) {
    if(NULL == file_header_ || NULL == info_header_) {
        DEBUG_MESSAGE("BMP validation failed: file_header or info_header is NULL. file_header=%p, info_header=%p", file_header_, info_header_);
        return BMP_FILE_UNDEFINED;
    }

    bmp_invalid_reason_t ret = BMP_FILE_VALID;
    if(0x4D42 != file_header_->bf_type) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP signature: expected=0x4D42('BM'), actual=0x%04X", file_header_->bf_type);
        ret = BMP_FILE_INVALID_BF_TYPE;
    } else if(0 != file_header_->bf_reserved1 || 0 != file_header_->bf_reserved2) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP reserved fields: expected bfReserved1=0 and bfReserved2=0, actual bfReserved1=%u, bfReserved2=%u", file_header_->bf_reserved1, file_header_->bf_reserved2);
        ret = BMP_FILE_INVALID_BF_RESERVED;
    } else if(54 >= file_header_->bf_size) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP file size: bfSize must be greater than BMP header size. bfSize=%u", file_header_->bf_size);
        ret = BMP_FILE_INVALID_BF_SIZE;
    } else if(54 > file_header_->bf_off_bits) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP pixel data offset: bfOffBits must be at least 54 for BITMAPINFOHEADER. bfOffBits=%u, minimum=54", file_header_->bf_off_bits);
        ret = BMP_FILE_INVALID_BF_OFF_BITS;
    } else if(file_header_->bf_size <= file_header_->bf_off_bits) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP pixel data offset: bfOffBits must be smaller than bfSize. bfOffBits=%u, bfSize=%u", file_header_->bf_off_bits, file_header_->bf_size);
        ret = BMP_FILE_INVALID_BF_OFF_BITS;
    } else if(40 != info_header_->bi_size) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported DIB header size: only BITMAPINFOHEADER(40 bytes) is supported. biSize=%u", info_header_->bi_size);
        ret = BMP_FILE_INVALID_BI_SIZE;
    } else if(1 != info_header_->bi_planes) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP plane count: biPlanes must be 1. biPlanes=%u", info_header_->bi_planes);
        ret = BMP_FILE_INVALID_BI_PLANES;
    } else if(0 != info_header_->bi_compression) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP compression: only BI_RGB(0) is supported. biCompression=%u", info_header_->bi_compression);
        ret = BMP_FILE_INVALID_COMPRESSION;
    } else if(0 == info_header_->bi_height || INT16_MIN > info_header_->bi_height || INT16_MAX < info_header_->bi_height) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP height: height must be within int16_t-compatible range. height=%d, min=%d, max=%d", info_header_->bi_height, INT16_MIN, INT16_MAX);
        ret = BMP_FILE_INVALID_HEIGHT;
    } else if(0 == info_header_->bi_width || info_header_->bi_width < 0 || INT16_MAX < info_header_->bi_width) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP width: width must be positive and within int16_t-compatible range. width=%d, max=%d", info_header_->bi_width, INT16_MAX);
        ret = BMP_FILE_INVALID_WIDTH;
    } else if(0 == info_header_->bi_bit_count || (24 != info_header_->bi_bit_count && 32 != info_header_->bi_bit_count)) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP bit count: only 24-bit and 32-bit BMP files are supported. biBitCount=%u", info_header_->bi_bit_count);
        ret = BMP_FILE_INVALID_CHANNEL_COUNT;
    } else {
        ret = BMP_FILE_VALID;
    }

    return ret;
}

static const char* invalid_reason_to_str(bmp_invalid_reason_t reason_) {
    switch(reason_) {
    case BMP_FILE_VALID:
        return invalid_bmp_file_reason_valid;
    case BMP_FILE_INVALID_BF_TYPE:
        return invalid_bmp_file_reason_bf_type;
    case BMP_FILE_INVALID_BF_RESERVED:
        return invalid_bmp_file_reason_bf_reserved;
    case BMP_FILE_INVALID_BF_SIZE:
        return invalid_bmp_file_reason_bf_size;
    case BMP_FILE_INVALID_BF_OFF_BITS:
        return invalid_bmp_file_reason_bf_off_bits;
    case BMP_FILE_INVALID_BI_SIZE:
        return invalid_bmp_file_reason_bi_size;
    case BMP_FILE_INVALID_BI_PLANES:
        return invalid_bmp_file_reason_bi_planes;
    case BMP_FILE_INVALID_COMPRESSION:
        return invalid_bmp_file_reason_compression;
    case BMP_FILE_INVALID_HEIGHT:
        return invalid_bmp_file_reason_height;
    case BMP_FILE_INVALID_WIDTH:
        return invalid_bmp_file_reason_width;
    case BMP_FILE_INVALID_CHANNEL_COUNT:
        return invalid_bmp_file_reason_channel_count;
    case BMP_FILE_UNDEFINED:
        return invalid_bmp_file_reason_undefined;
    default:
        return invalid_bmp_file_reason_undefined;
    }
}
