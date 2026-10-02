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
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/buffer_utils/buffer_utils.h"

#include "engine/io_utils/fs_stream.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

/*
 * Module Internal Contract
 *
 * Trust Boundary:
 * - BMP Loader moduleはexternal BMP fileを読み込み、
 *   GLCE内部で利用可能なtexture pixel representationへ変換するresource trust boundaryである。
 * - fileから読み込んだraw byteおよびparse直後のheader情報はuntrusted external dataとして扱う。
 * - external BMP representationからGLCE内部representationへの昇格は、
 *   本module内で必要なformat validation、checked transformation、
 *   pixel normalizationがすべて成功した場合にのみ成立する。
 *
 * Header Representation:
 * - file_header_tおよびinfo_header_tはBMP file headerからparseしたprivate representationである。
 * - parse成功だけではheaderのsemantic validityは成立しない。
 * - header_is_valid()成功後は、本moduleが受理するBMP header representationとして
 *   必要なsemantic conditionが成立しているものとして扱う。
 * - header validation成功時点ではheader representationのみがtrustedとなり、
 *   pixel data領域の存在および内容までは確定しない。
 *
 * Pixel Layout Representation:
 * - pixel_layout_tはvalidated BMP headerから導出されるprivate processing contextである。
 * - pixel_layout_tは独立したexternal representationまたはpublic domain objectではなく、
 *   pixel loadおよびpixel normalizationを安全に実行するためのderived informationを保持する。
 * - pixel_layout_initialize_from_header()はchecked transformationとして、
 *   row stride、raw pixel data size、pixel buffer size、pixel offset、
 *   image dimensions、bit count、vertical flip requirementを導出する。
 * - pixel_layout_initialize_from_header()成功後のpixel_layout_tはvalid-by-constructionとして扱う。
 * - private helper間では、initializer成功によって成立済みの
 *   pixel_layout_t内部relationを信頼し、同一semanticを重複して再検証しない。
 *
 * Pixel Representation:
 * - BMP fileから読み込んだpixel bufferは、pixel_layout_tが示す
 *   BMP file representationとして扱う。
 * - normalization成功後のpixel dataはBMP row paddingを含まず、
 *   GLCEで使用するRGBまたはRGBA channel orderへ変換済みである。
 * - BMPがbottom-up representationである場合はvertical flipを行い、
 *   GLCE内部で使用するorientationへ正規化する。
 * - normalization済みpixel dataのchannel countは3または4である。
 *
 * Resource Metadata:
 * - normalization完了後、pixel layoutおよびnormalized pixel data sizeから
 *   texture_resource_info_tを構築する。
 * - texture_resource_info_tを構成する各fieldおよびfield間relationの
 *   semantic ownershipはResource Coreに属する。
 * - BMP Loader moduleはtexture resource metadata固有のsemanticを独自に再定義せず、
 *   validityの確認をtexture_resource_info_is_valid()へ委譲する。
 *
 * Ownership:
 * - pixel loadおよびnormalizationで生成するtemporary pixel storageは
 *   General AllocatorからGENERAL_ALLOCATOR_MEMORY_TAG_TEXTUREで確保する。
 * - module内部ではpublic outputへcommitするまでtemporary resourceとして管理する。
 * - public commit完了後のoutput pixel storageはBMP Loader moduleのinternal ownershipから外れる。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

/*
 * Module Validation Policy
 *
 * General:
 * - BMP Loader moduleはexternal resource trust boundaryであるため、
 *   external BMP dataから安全なGLCE内部representationを構築するために必要なvalidationを
 *   RELEASE_BUILDを含む通常実行経路で行う。
 *
 * Header Validation:
 * - header_load()はBMP header byte列を読み込み、private header representationへparseする。
 * - parse処理自身はbyte representationからfield値への変換を担当し、
 *   header semanticのvalidityは確定しない。
 * - header_is_valid()はparse済みfile_header_tおよびinfo_header_tについて、
 *   本moduleが受理するBMP header representationとしてのsemantic validityを確認する。
 * - header_is_valid()成功後はheader representationをtrustedとして扱い、
 *   downstream private helperで同じheader semanticを重複して再検証しない。
 *
 * Derived Layout Validation:
 * - pixel_layout_initialize_from_header()はvalidated headerを入力とする
 *   checked transformationである。
 * - width、height、bit countからrow layoutおよびpixel buffer sizeを導出する際に、
 *   size_t arithmeticで必要となるoverflow checkを行う。
 * - calculated pixel buffer rangeがBMP headerで宣言されたfile range内に収まることを確認する。
 * - transformationに成功したpixel_layout_tはvalid-by-constructionとして扱い、
 *   standalone canonical validatorは設けない。
 *
 * Operation-specific Validation:
 * - private pixel processing helperは、pixel_layout_tの成立済みinternal relationを
 *   一律に再検証しない。
 * - 各helperは自身のoperationが直接consumeするsemantic preconditionのみを検証する。
 * - 例としてpixel_bgr_to_rgb()は、自身が処理可能なpixel formatである
 *   24bit RGBまたは32bit RGBA representationであることを確認する。
 * - padding removalやvertical flipに必要なstride、raw_data_size、height等のrelationは、
 *   pixel_layout_initialize_from_header()成功によって成立済みのinternal contractとして扱う。
 *
 * Final Resource Validation:
 * - pixel normalization完了後、texture_resource_info_tをcandidate metadataとして構築する。
 * - texture_resource_info_t固有のsemantic validityは、
 *   semantic ownerであるtexture_resource_info_is_valid()へ委譲する。
 * - BMP Loader moduleではwidth、height、channel_count、pixel_data_size等の
 *   metadata semanticを重複して実装しない。
 *
 * Commit Eligibility:
 * - DEBUG_BUILD / TEST_BUILDではpublic outputへcommitする前のstable boundaryで
 *   texture_resource_info_is_valid()を実行し、
 *   candidate metadataにCommit eligibility validationを行う。
 * - Commit eligibility validationはexternal BMP dataを再度trust boundaryとして
 *   認証するためのものではなく、
 *   本moduleの変換処理によって構築されたcandidate representationが
 *   Resource Coreのsemantic contractを満たすことをdiagnosticとして確認するために行う。
 * - RELEASE_BUILDではautomatic Commit eligibility validationを行わない。
 *
 * Corruption Handling:
 * - external BMPのformat不整合や未対応representationは
 *   internal DATA_CORRUPTEDとは区別して扱う。
 * - DATA_CORRUPTEDが確定した場合はfail-stop ruleに従い、
 *   suspectなownership graphを辿る通常cleanupまたはresource releaseを行わない。
 *
 * Private Helper Validation:
 * - private helperでもpointer existenceなど、
 *   helper自身が安全にoperationを開始するために必要なlocal preconditionは検証する。
 * - 一方で、validated headerやvalid-by-constructionなpixel_layout_tについて、
 *   upstreamで成立済みのsemantic conditionを防御的に重複検証しない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

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

// pixel load, pixel normalize処理をやりやすくするためにheader情報を変換、コピーした構造体
typedef struct pixel_layout {
    size_t file_size;
    size_t pixel_size;

    size_t stride;
    size_t raw_data_size;
    uint32_t pixel_offset;

    int32_t width;
    int32_t height;
    uint16_t bit_count;

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
static resource_result_t pixel_normalize(const pixel_layout_t* pixel_layout_, const uint8_t* pixels_, uint8_t** out_pixels_, size_t* out_new_size_, uint8_t* out_channel_count_);
static resource_result_t pixel_bgr_to_rgb(const pixel_layout_t* pixel_layout_, uint8_t* pixels_, uint8_t* out_channel_count_);
static resource_result_t pixel_flip(const pixel_layout_t* pixel_layout_, uint8_t* pixels_);
static resource_result_t padding_remove(const pixel_layout_t* pixel_layout_, const uint8_t* src_pixels_, uint8_t** out_pixels_, size_t* out_new_size_);

// Utilities
static resource_result_t pixel_layout_initialize_from_header(const file_header_t* file_header_, const info_header_t* info_header_, pixel_layout_t* out_layout_);
static void file_header_copy(const file_header_t* src_, file_header_t* dst_);
static void info_header_copy(const info_header_t* src_, info_header_t* dst_);

// Validators
static bmp_invalid_reason_t header_is_valid(const file_header_t* file_header_, const info_header_t* info_header_);
static const char* invalid_reason_to_str(bmp_invalid_reason_t reason_);

resource_result_t bmp_loader_load(const char* fullpath_, texture_resource_info_t* out_resource_info_, uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    uint8_t* tmp_pixels = NULL;
    uint8_t* pixel_normalized = NULL;

    bmp_invalid_reason_t reason = BMP_FILE_UNDEFINED;
    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };
    pixel_layout_t pixel_layout = { 0 };
    texture_resource_info_t tmp_resource_info = { 0 };
    uint8_t tmp_channel_count = 0;
    size_t new_pixel_size = 0;

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

    // pixelロード
    ret = pixel_load(fullpath_, &pixel_layout, &tmp_pixels);   // 内部でtmp_pixelsのメモリが確保されるが、失敗時には解放される
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to load BMP pixel data.", resource_result_to_str(ret));
        goto cleanup;
    }

    // pixel normalize
    // GLCE内部フォーマットへ変換
    ret = pixel_normalize(&pixel_layout, tmp_pixels, &pixel_normalized, &new_pixel_size, &tmp_channel_count);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - pixel_normalize failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    tmp_resource_info.channel_count = tmp_channel_count;
    tmp_resource_info.height = pixel_layout.height;
    tmp_resource_info.pixel_data_size = new_pixel_size;
    tmp_resource_info.width = pixel_layout.width;
    //////////////////////////////////////////////////////////////////////////////////////////
    // pixel_normalizeが成功したため、以降はGLCE内部で利用可能なbmpであることが確定
    //////////////////////////////////////////////////////////////////////////////////////////

    // Commit eligibility.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!texture_resource_info_is_valid(&tmp_resource_info)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("bmp_loader_load(%s) - Commit eligibility validation failed for 'tmp_resource_info'.", resource_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    *out_pixels_ = pixel_normalized;
    *out_resource_info_ = tmp_resource_info;
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
static resource_result_t pixel_normalize(const pixel_layout_t* pixel_layout_, const uint8_t* pixels_, uint8_t** out_pixels_, size_t* out_new_size_, uint8_t* out_channel_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    size_t new_pixel_size = 0;
    uint8_t* new_pixels = NULL;
    uint8_t tmp_channel_count = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(pixel_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_normalize", "pixel_layout_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_normalize", "pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_normalize", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "pixel_normalize", "*out_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_new_size_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_normalize", "out_new_size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_normalize", "out_channel_count_")

    // Prepare.
    if(0 < pixel_layout_->height) {
        ret = padding_remove(pixel_layout_, pixels_, &new_pixels, &new_pixel_size); // 内部でnew_pixelsのメモリが確保されるが、失敗時には解放される
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("bmp_loader_load(%s) - Failed to remove BMP row padding.", resource_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = pixel_bgr_to_rgb(pixel_layout_, new_pixels, &tmp_channel_count);
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
    *out_channel_count_ = tmp_channel_count;
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

static resource_result_t pixel_bgr_to_rgb(const pixel_layout_t* pixel_layout_, uint8_t* pixels_, uint8_t* out_channel_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    size_t ii = 0;
    int32_t channel_count = 0;
    int32_t bit_count = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(pixel_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_bgr_to_rgb", "pixel_layout_")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_bgr_to_rgb", "pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_bgr_to_rgb", "out_channel_count_")
    if(24 != pixel_layout_->bit_count && 32 != pixel_layout_->bit_count) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("pixel_bgr_to_rgb(%s) - Provided pixel_layout_->bit_count is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }
    bit_count = pixel_layout_->bit_count;
    channel_count = bit_count / 8;

    // Commit.
    for(int32_t i = 0; i != pixel_layout_->height; ++i) {
        for(int32_t j = 0; j != pixel_layout_->width; ++j) {
            const uint8_t tmp = pixels_[ii];
            pixels_[ii] = pixels_[ii + 2];
            pixels_[ii + 2] = tmp;
            ii += (size_t)channel_count;
        }
    }
    *out_channel_count_ = (uint8_t)channel_count;

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
        for(size_t i = 0; i != pixel_layout_->raw_data_size; ++i) {
            uint8_t tmp = pixels_[i];
            pixels_[i] = pixels_[pixel_layout_->raw_data_size + i];
            pixels_[pixel_layout_->raw_data_size + i] = tmp;
        }
    } else {
        size_t back = (size_t)(pixel_layout_->height) - 1;
        const size_t to = (size_t)(pixel_layout_->height) / 2;
        for(size_t i = 0; i != to; ++i) {
            for(size_t j = 0; j != pixel_layout_->raw_data_size; ++j) {
                uint8_t tmp = pixels_[i * pixel_layout_->raw_data_size + j];
                pixels_[i * pixel_layout_->raw_data_size + j] = pixels_[back * pixel_layout_->raw_data_size + j];
                pixels_[back * pixel_layout_->raw_data_size + j] = tmp;
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

    uint8_t* new_pixels = NULL;
    uintptr_t src_addr = 0;
    uintptr_t dst_addr = 0;
    size_t new_pixel_size = 0;
    size_t height = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(pixel_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "pixel_layout_")
    IF_ARG_NULL_GOTO_CLEANUP(src_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "src_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_padding_remove", "*out_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(out_new_size_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "out_new_size_")

    // Prepare.
    height = (size_t)pixel_layout_->height;
    new_pixel_size = pixel_layout_->raw_data_size * height;    // 既にpixel_layout_の初期化でstride x heightの計算に成功しているため、オーバーフローチェックは不要(stride >= raw_data_size)
    ret_general_allocator = general_allocator_allocate(new_pixel_size, GENERAL_ALLOCATOR_MEMORY_TAG_TEXTURE, (void**)&new_pixels);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    for(size_t i = 0; i != height; ++i) {
        src_addr = (uintptr_t)(src_pixels_) + (i * pixel_layout_->stride);
        dst_addr = (uintptr_t)(new_pixels) + (i * pixel_layout_->raw_data_size);
        memcpy((void*)dst_addr, (void*)src_addr, pixel_layout_->raw_data_size);
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

// ============================================================
// Utilities
// ============================================================
static resource_result_t pixel_layout_initialize_from_header(const file_header_t* file_header_, const info_header_t* info_header_, pixel_layout_t* out_layout_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    size_t width = 0;
    size_t height = 0;
    size_t bit_count = 0;
    size_t stride = 0;
    size_t pixel_buffer_size = 0;
    size_t raw_data_size = 0;
    bool flip_require = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(file_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_layout_initialize_from_header", "file_header_")
    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_layout_initialize_from_header", "info_header_")
    IF_ARG_NULL_GOTO_CLEANUP(out_layout_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "pixel_layout_initialize_from_header", "out_layout_")

    // Prepare.
    width = (size_t)(info_header_->bi_width);
    flip_require = (0 < info_header_->bi_height) ? true : false;
    height = flip_require ? (size_t)(info_header_->bi_height) : (size_t)(-(int64_t)info_header_->bi_height);
    bit_count = (size_t)(info_header_->bi_bit_count);

    // stride(raw_data_size + padding)計算
    if((SIZE_MAX / width) < bit_count) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("pixel_layout_initialize_from_header(%s) - overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - 31) < (bit_count * width)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("pixel_layout_initialize_from_header(%s) - overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    stride = ((bit_count * width + 31) / 32) * 4;

    // raw_data_size計算
    raw_data_size = (width * bit_count + 7) / 8;

    if((SIZE_MAX / height) < stride) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("pixel_layout_initialize_from_header(%s) - overflow.", resource_result_to_str(ret));
        goto cleanup;
    }
    pixel_buffer_size = stride * height;

    if(pixel_buffer_size > ((size_t)file_header_->bf_size - (size_t)file_header_->bf_off_bits)) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        goto cleanup;
    }

    // Output.
    out_layout_->bit_count = info_header_->bi_bit_count;
    out_layout_->file_size = file_header_->bf_size;
    out_layout_->height = (int32_t)height;
    out_layout_->raw_data_size = raw_data_size;
    out_layout_->pixel_offset = file_header_->bf_off_bits;
    out_layout_->pixel_size = pixel_buffer_size;
    out_layout_->requires_vertical_flip = flip_require;
    out_layout_->stride = stride;
    out_layout_->width = (int32_t)width;

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
    } else if(1 != info_header_->bi_bit_count && 4 != info_header_->bi_bit_count && 8 != info_header_->bi_bit_count && 16 != info_header_->bi_bit_count && 24 != info_header_->bi_bit_count && 32 != info_header_->bi_bit_count) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP bit count");
        return BMP_FILE_INVALID_CHANNEL_COUNT;
    } else {
        return BMP_FILE_VALID;
    }
}
