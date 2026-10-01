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
#include "engine/resource/loaders/bmp_loader.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/buffer_utils/buffer_utils.h"

#include "engine/io_utils/fs_stream.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

// ============================================================
// Private Type Definitions
// ============================================================
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

// GLCEではbit_count == 24(RGB), 32(RGBA)以外には対応しないため、bit_countは保存しない
typedef struct pixel_layout {
    size_t file_size;
    size_t pixel_size;

    size_t stride;
    size_t padding;
    uint32_t pixel_offset;

    size_t width;
    size_t height;
    uint8_t channel_count;

    bool requires_vertical_flip;
} pixel_layout_t;

// ============================================================
// Private Constants
// ============================================================
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

// ============================================================
// Private Function Declarations
// ============================================================
// Loading helpers
static resource_result_t header_load(const char* fullpath_, file_header_t* out_file_header_, info_header_t* out_info_header_);
static resource_result_t pixel_load(const char* fullpath_, const pixel_layout_t* pixel_layout_, uint8_t** out_pixels_);

// Parsing helpers
static resource_result_t file_header_parse(const char header_[54], file_header_t* file_header_);
static resource_result_t info_header_parse(const char header_[54], info_header_t* info_header_);

// Pixel normalization helpers
static resource_result_t pixel_normalize(const pixel_layout_t* pixel_layout_, const uint8_t* pixels_, uint8_t** out_pixels_, size_t* out_new_size_);
static resource_result_t pixel_bgr_to_rgb(const pixel_layout_t* pixel_layout_, uint8_t* pixels_);
static resource_result_t pixel_flip(const pixel_layout_t* pixel_layout_, uint8_t* pixels_);
static resource_result_t padding_remove(const pixel_layout_t* pixel_layout_, const uint8_t* src_pixels_, uint8_t** out_pixels_, size_t* out_new_size_);

// Utilities
static resource_result_t pixel_layout_initialize_from_header(const file_header_t* file_header_, const info_header_t* info_header_, pixel_layout_t* out_layout_);
static void file_header_copy(const file_header_t* src_, file_header_t* dst_);
static void info_header_copy(const info_header_t* src_, info_header_t* dst_);

// Validators
static bmp_invalid_reason_t header_is_valid(const file_header_t* file_header_, const info_header_t* info_header_);
static bool pixel_layout_is_valid(const pixel_layout_t* pixel_layout_);
static bmp_invalid_reason_t is_bmp_supported(const file_header_t* file_header_, const info_header_t* info_header_);
static const char* invalid_reason_to_str(bmp_invalid_reason_t reason_);

resource_result_t bmp_loader_load(const char* fullpath_, texture_resource_info_t* out_resource_info_, uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };
    uint8_t* tmp_pixels = NULL;
    uint8_t* pixel_normalized = NULL;
    size_t new_pixel_size = 0;
    bmp_invalid_reason_t reason = BMP_FILE_UNDEFINED;

    pixel_layout_t pixel_layout = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_resource_info_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "out_resource_info_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_load", "*out_pixels_")
    if('\0' == fullpath_[0]) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("bmp_loader_load(%s) - Provided fullpath_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    // ヘッダロード
    ret = header_load(fullpath_, &tmp_file_header, &tmp_info_header);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to load BMP header.", resource_result_to_str(ret));
        goto cleanup;
    }
    reason = header_is_valid(&tmp_file_header, &tmp_info_header);
    if(BMP_FILE_VALID != reason) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_load(%s) - Invalid file header. reason = %s", resource_result_to_str(ret), invalid_reason_to_str(reason));
        goto cleanup;
    }
    //////////////////////////////////////////////////////////////////////////////////////////
    // header_is_validが成功したため、BMPのヘッダフォーマットが正しいことが確定(ピクセルデータ部は未確定)
    //////////////////////////////////////////////////////////////////////////////////////////

    // ヘッダからpixel_layoutを計算
    ret = pixel_layout_initialize_from_header(&tmp_file_header, &tmp_info_header, &pixel_layout);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - pixel_layout_initialize_from_header failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    if(!pixel_layout_is_valid(&pixel_layout)) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_load(%s) - pixel layout is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }
    //////////////////////////////////////////////////////////////////////////////////////////
    // pixel_layout_is_validが成功したため、以降はglce内部で利用可能なbmpであることが確定
    //////////////////////////////////////////////////////////////////////////////////////////

    // pixelロード
    ret = pixel_load(fullpath_, &pixel_layout, &tmp_pixels);   // 内部でtmp_pixelsのメモリが確保されるが、失敗時には解放される
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to load BMP pixel data.", resource_result_to_str(ret));
        goto cleanup;
    }

    // pixel normalize
    ret = pixel_normalize(&pixel_layout, tmp_pixels, &pixel_normalized, &new_pixel_size);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - pixel_normalize failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    *out_pixels_ = pixel_normalized;
    out_resource_info_->channel_count = pixel_layout.channel_count;
    out_resource_info_->height = (uint16_t)pixel_layout.height;
    out_resource_info_->pixel_data_size = new_pixel_size;
    out_resource_info_->width = (uint16_t)pixel_layout.width;
    pixel_normalized = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != pixel_normalized) {
            general_allocator_free((void**)&pixel_normalized, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        }
        general_allocator_free((void**)&tmp_pixels, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
    }

    return ret;
}

// ============================================================
// Loading helpers
// ============================================================
static resource_result_t header_load(const char* fullpath_, file_header_t* out_file_header_, info_header_t* out_info_header_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;
    size_t read_size = 0;
    char header_buf[54] = { 0 };

    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "header_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_file_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "header_load", "out_file_header_")
    IF_ARG_NULL_GOTO_CLEANUP(out_info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "header_load", "out_info_header_")

    // Prepare.
    ret_fs_stream = fs_stream_create(&fs_stream, fullpath_, FS_OPEN_MODE_READ_BINARY);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_result_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("header_load(%s) - fs_stream_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    // ヘッダ読み込み
    ret_fs_stream = fs_stream_byte_read(fs_stream, 54, &read_size, header_buf);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_result_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("header_load(%s) - Failed to read BMP file header.", resource_result_to_str(ret));
        goto cleanup;
    } else if(54 != read_size) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("header_load(%s) - Invalid BMP file format: header size is invalid. header size = %zu", resource_result_to_str(ret), read_size);
        goto cleanup;
    }

    // ヘッダparse
    ret = file_header_parse(header_buf, &tmp_file_header);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("header_load(%s) - Failed to parse BMP file header.", resource_result_to_str(ret));
        goto cleanup;
    }
    ret = info_header_parse(header_buf, &tmp_info_header);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("header_load(%s) - Failed to parse BMP info header.", resource_result_to_str(ret));
        goto cleanup;
    }

    // close失敗はfs_stream_destroy内のERROR_MESSAGEを出力するのみとし、エラー処理は行わない
    fs_stream_destroy(&fs_stream, NULL);

    // Output.
    file_header_copy(&tmp_file_header, out_file_header_);
    info_header_copy(&tmp_info_header, out_info_header_);

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret && RESOURCE_DATA_CORRUPTED != ret) {
        fs_stream_destroy(&fs_stream, NULL);
    }
    return ret;
}

static resource_result_t pixel_load(const char* fullpath_, const pixel_layout_t* pixel_layout_, uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;
    uint8_t* tmp_buffer = NULL;
    uint8_t* tmp_pixels = NULL;
    size_t read_size_all = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(pixel_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_load", "pixel_layout_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_load", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "pixel_load", "*out_pixels_")
    // fullpath_, pixel_layout_は上位関数でvalidation済みであること

    ret_general_allocator = general_allocator_allocate(pixel_layout_->file_size, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&tmp_buffer);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("pixel_load(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_stream = fs_stream_create(&fs_stream, fullpath_, FS_OPEN_MODE_READ_BINARY);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_result_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("pixel_load(%s) - fs_stream_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_stream = fs_stream_byte_read(fs_stream, pixel_layout_->file_size, &read_size_all, (char*)tmp_buffer);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_result_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("pixel_load(%s) - Failed to read BMP file(%s).", resource_result_to_str(ret), fullpath_);
        goto cleanup;
    } else if(pixel_layout_->file_size != read_size_all) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("pixel_load(%s) - Invalid file size.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(pixel_layout_->pixel_size, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&tmp_pixels);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("pixel_load(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != pixel_layout_->pixel_size; ++i) {
        tmp_pixels[i] = tmp_buffer[i + pixel_layout_->pixel_offset];
    }

    // close失敗はfs_stream_destroyないのERROR_MESSAGEを出力するのみとし、エラー処理は行わない
    fs_stream_destroy(&fs_stream, NULL);
    general_allocator_free((void**)&tmp_buffer, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);

    *out_pixels_ = tmp_pixels;
    tmp_pixels = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != tmp_buffer) {
            general_allocator_free((void**)&tmp_buffer, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        }
        if(NULL != fs_stream) {
            fs_stream_destroy(&fs_stream, NULL);
        }
        if(NULL != tmp_pixels) {
            general_allocator_free((void**)&tmp_pixels, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        }
    }
    return ret;
}

// ============================================================
// Parsing helpers
// ============================================================
/**
 * @brief FILEHEADER情報文字列をパースし、構造体にパラメータを格納する
 *
 * @param[in] header_ ヘッダ情報文字列(FILEHEADER + INFOHEADER)
 * @param[out] out_file_header_ FILEHEADER情報格納先構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - header_ == NULL
 * - out_file_header_ == NULL
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t file_header_parse(const char header_[54], file_header_t* out_file_header_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    file_header_t tmp_header = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "file_header_parse", "header_")
    IF_ARG_NULL_GOTO_CLEANUP(out_file_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "file_header_parse", "out_file_header_")

    // Prepare.
    tmp_header.bf_type = buffer_utils_le_uint16_t_get(header_);
    tmp_header.bf_size = buffer_utils_le_uint32_t_get(header_ + 2);
    tmp_header.bf_reserved1 = buffer_utils_le_uint16_t_get(header_ + 6);
    tmp_header.bf_reserved2 = buffer_utils_le_uint16_t_get(header_ + 8);
    tmp_header.bf_off_bits = buffer_utils_le_uint32_t_get(header_ + 10);

    // Output.
    file_header_copy(&tmp_header, out_file_header_);

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief INFOHEADER情報文字列をパースし、構造体にパラメータを格納する
 *
 * @param[in] header_ ヘッダ情報文字列(FILEHEADER + INFOHEADER)
 * @param[out] out_info_header_ INFOHEADER情報格納先構造体インスタンスへのポインタ
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - header_ == NULL
 * - out_info_header_ == NULL
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t info_header_parse(const char header_[54], info_header_t* out_info_header_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    info_header_t tmp_header = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "info_header_parse", "header_")
    IF_ARG_NULL_GOTO_CLEANUP(out_info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "info_header_parse", "out_info_header_")

    // Prepare.
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

    // Output.
    info_header_copy(&tmp_header, out_info_header_);

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Pixel normalization helpers
// ============================================================
static resource_result_t pixel_normalize(const pixel_layout_t* pixel_layout_, const uint8_t* pixels_, uint8_t** out_pixels_, size_t* out_new_size_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    size_t new_pixel_size = 0;
    uint8_t* new_pixels = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(pixel_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_normalize", "pixel_layout_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_normalize", "pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_normalize", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "pixel_normalize", "*out_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_new_size_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_normalize", "out_new_size_")

    // Prepare.
    if(0 < pixel_layout_->height) {
        ret = padding_remove(pixel_layout_, pixels_, &new_pixels, &new_pixel_size); // 内部でnew_pixelsのメモリが確保されるが、失敗時には解放される
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("bmp_loader_load(%s) - Failed to remove BMP row padding.", resource_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = pixel_bgr_to_rgb(pixel_layout_, new_pixels);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to convert BGR to RGB.", resource_result_to_str(ret));
        goto cleanup;
    }

    if(pixel_layout_->requires_vertical_flip) {
        ret = pixel_flip(pixel_layout_, new_pixels);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("bmp_loader_load(%s) - Failed to flip BMP pixel data vertically.", resource_result_to_str(ret));
            goto cleanup;
        }
    }

    // Output.
    *out_pixels_ = new_pixels;
    *out_new_size_ = new_pixel_size;
    new_pixels = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != new_pixels) {
            general_allocator_free((void**)&new_pixels, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        }
    }
    return ret;
}

static resource_result_t pixel_bgr_to_rgb(const pixel_layout_t* pixel_layout_, uint8_t* pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    size_t ii = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(pixel_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_bgr_to_rgb", "pixel_layout_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_bgr_to_rgb", "pixels_")

    // Commit.
    for(size_t i = 0; i != pixel_layout_->height; ++i) {
        for(size_t j = 0; j != pixel_layout_->width; ++j) {
            const uint8_t tmp = pixels_[ii];
            pixels_[ii] = pixels_[ii + 2];
            pixels_[ii + 2] = tmp;
            ii += pixel_layout_->channel_count;
        }
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

static resource_result_t pixel_flip(const pixel_layout_t* pixel_layout_, uint8_t* pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(pixel_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_flip", "pixel_layout_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_flip", "pixels_")

    // Commit.
    if(1 == pixel_layout_->height) {
        // flip不要なので何もしない
    } else if(2 == pixel_layout_->height) {
        const size_t width_count = pixel_layout_->width * pixel_layout_->channel_count;
        for(size_t i = 0; i != width_count; ++i) {
            uint8_t tmp = pixels_[i];
            pixels_[i] = pixels_[width_count + i];
            pixels_[width_count + i] = tmp;
        }
    } else {
        size_t back = pixel_layout_->height - 1;
        const size_t to = pixel_layout_->height / 2;
        const size_t width_count = pixel_layout_->width * pixel_layout_->channel_count;
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

static resource_result_t padding_remove(const pixel_layout_t* pixel_layout_, const uint8_t* src_pixels_, uint8_t** out_pixels_, size_t* out_new_size_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    uint8_t* new_pixel = NULL;
    size_t new_size = 0;

    size_t ii = 0;
    size_t ii_new = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(pixel_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "pixel_layout_")
    IF_ARG_NULL_GOTO_CLEANUP(src_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "src_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_padding_remove", "*out_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_new_size_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "out_new_size_")
    if((SIZE_MAX / pixel_layout_->height) < pixel_layout_->width) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("padding_remove(%s) - overflow", resource_result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX / pixel_layout_->channel_count) < (pixel_layout_->width * pixel_layout_->height)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("padding_remove(%s) - overflow", resource_result_to_str(ret));
        goto cleanup;
    }
    new_size = pixel_layout_->width * pixel_layout_->height * pixel_layout_->channel_count;
    if(new_size > UINT32_MAX) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("padding_remove(%s) - overflow", resource_result_to_str(ret), new_size);
        goto cleanup;
    }

    // Prepare.
    ret_general_allocator = general_allocator_allocate(new_size, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&new_pixel);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    for(size_t i = 0; i != pixel_layout_->height; ++i) {
        for(size_t j = 0; j != pixel_layout_->width; ++j) {
            for(size_t k = 0; k != pixel_layout_->channel_count; ++k) {
                new_pixel[ii_new + k] = src_pixels_[ii + k];
            }
            ii_new += pixel_layout_->channel_count;
            ii += pixel_layout_->channel_count;
        }
        ii += pixel_layout_->padding;
    }

    // Output.
    *out_pixels_ = new_pixel;
    *out_new_size_ = new_size;
    new_pixel = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != new_pixel) {
            general_allocator_free((void**)&new_pixel, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE);
        }
    }

    return ret;
}

// ============================================================
// Utilities
// ============================================================
static resource_result_t pixel_layout_initialize_from_header(const file_header_t* file_header_, const info_header_t* info_header_, pixel_layout_t* out_layout_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    uint8_t channel_count = 0;
    size_t width = 0;
    size_t height = 0;
    size_t stride = 0;
    size_t padding = 0;
    size_t pixel_buffer_size = 0;
    bool flip_require = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(file_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_layout_initialize_from_header", "file_header_")
    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_layout_initialize_from_header", "info_header_")
    IF_ARG_NULL_GOTO_CLEANUP(out_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_layout_initialize_from_header", "out_layout_")

    // Prepare.
    channel_count = (uint8_t)(info_header_->bi_bit_count / 8); // GLCEでは1, 4bit countは扱わないため切り捨て
    width = (size_t)(info_header_->bi_width);
    flip_require = (0 < info_header_->bi_height) ? true : false;
    height = flip_require ? (size_t)(info_header_->bi_height) : (size_t)(info_header_->bi_height * -1);

    if((SIZE_MAX / width) < info_header_->bi_bit_count) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("pixel_layout_initialize_from_header(%s) - overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - 31) < (info_header_->bi_bit_count * width)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("pixel_layout_initialize_from_header(%s) - overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    stride = ((info_header_->bi_bit_count * width + 31) / 32) * 4;
    padding = stride - (info_header_->bi_bit_count * width / 8);

    if((SIZE_MAX / height) < stride) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("pixel_layout_initialize_from_header(%s) - overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    pixel_buffer_size = stride * height;

    // Output.
    out_layout_->channel_count = channel_count;
    out_layout_->file_size = file_header_->bf_size;
    out_layout_->height = height;
    out_layout_->padding = padding;
    out_layout_->pixel_offset = file_header_->bf_off_bits;
    out_layout_->pixel_size = pixel_buffer_size;
    out_layout_->requires_vertical_flip = flip_require;
    out_layout_->stride = stride;
    out_layout_->width = width;

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

// ============================================================
// Validators
// ============================================================
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

static bmp_invalid_reason_t header_is_valid(const file_header_t* file_header_, const info_header_t* info_header_) {
    if(NULL == file_header_ || NULL == info_header_) {
        DEBUG_MESSAGE("BMP header validation failed: file_header or info_header is NULL. file_header=%p, info_header=%p", file_header_, info_header_);
        return BMP_FILE_UNDEFINED;
    }

    if(0x4D42 != file_header_->bf_type) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP signature: expected=0x4D42('BM'), actual=0x%04X", file_header_->bf_type);
        return BMP_FILE_INVALID_BF_TYPE;
    } else if(0 != file_header_->bf_reserved1 || 0 != file_header_->bf_reserved2) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP reserved fields: expected bfReserved1=0 and bfReserved2=0, actual bfReserved1=%u, bfReserved2=%u", file_header_->bf_reserved1, file_header_->bf_reserved2);
        return BMP_FILE_INVALID_BF_RESERVED;
    } else if(54 >= file_header_->bf_size) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP file size: bfSize must be greater than BMP header size. bfSize=%u", file_header_->bf_size);
        return BMP_FILE_INVALID_BF_SIZE;
    } else if(54 > file_header_->bf_off_bits) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP pixel data offset: bfOffBits must be at least 54 for BITMAPINFOHEADER. bfOffBits=%u, minimum=54", file_header_->bf_off_bits);
        return BMP_FILE_INVALID_BF_OFF_BITS;
    } else if(file_header_->bf_size <= file_header_->bf_off_bits) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP pixel data offset: bfOffBits must be smaller than bfSize. bfOffBits=%u, bfSize=%u", file_header_->bf_off_bits, file_header_->bf_size);
        return BMP_FILE_INVALID_BF_OFF_BITS;
    } else if(40 != info_header_->bi_size) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported DIB header size: only BITMAPINFOHEADER(40 bytes) is supported. biSize=%u", info_header_->bi_size);
        return BMP_FILE_INVALID_BI_SIZE;
    } else if(1 != info_header_->bi_planes) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP plane count: biPlanes must be 1. biPlanes=%u", info_header_->bi_planes);
        return BMP_FILE_INVALID_BI_PLANES;
    } else if(0 != info_header_->bi_compression) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP compression: only BI_RGB(0) is supported. biCompression=%u", info_header_->bi_compression);
        return BMP_FILE_INVALID_COMPRESSION;
    } else if(0 == info_header_->bi_height) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP height");
        return BMP_FILE_INVALID_HEIGHT;
    } else if(0 == info_header_->bi_width || info_header_->bi_width < 0) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP width");
        return BMP_FILE_INVALID_WIDTH;
    } else if(0 == info_header_->bi_bit_count) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP bit count");
        return BMP_FILE_INVALID_CHANNEL_COUNT;
    } else {
        return BMP_FILE_VALID;
    }
}

static bool pixel_layout_is_valid(const pixel_layout_t* pixel_layout_) {
    if(NULL == pixel_layout_) {
        return false;
    }
    if(3 != pixel_layout_->channel_count && 4 != pixel_layout_->channel_count) {
        return false;
    }
    if((SIZE_MAX / pixel_layout_->height) < pixel_layout_->width) {
        return false;
    }
    if((SIZE_MAX / pixel_layout_->channel_count) < (pixel_layout_->width * pixel_layout_->height)) {
        return false;
    }
    return true;
}
