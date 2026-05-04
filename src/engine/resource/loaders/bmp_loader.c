// TODO: 計算各所のoverflowチェック追加
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/resource/loaders/bmp_loader.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/buffer_utils/buffer_utils.h"
#include "engine/core/filesystem/filesystem.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

#ifdef TEST_BUILD
#include <assert.h>
#endif

typedef enum {
    BMP_FILE_VALID,
    BMP_FILE_INVALID_BF_TYPE,
    BMP_FILE_INVALID_BF_RESERVED,
    BMP_FILE_INVALID_BF_SIZE,
    BMP_FILE_INVALID_BF_OFF_BITS,
    BMP_FILE_INVALID_BI_SIZE,
    BMP_FILE_INVALID_BI_PLANES,
    BMP_FILE_INVALID_COMPRESSION,
    BMP_FILE_INVALID_HEIGHT,
    BMP_FILE_INVALID_WIDTH,
    BMP_FILE_INVALID_CHANNEL_COUNT,
    BMP_FILE_NOT_INITIALIZED,
    BMP_FILE_UNDEFINED,
} bmp_invalid_reason_t;

/**
 * @brief BITMAPファイルヘッダー(14byte固定)
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
 * @brief BITMAP情報ヘッダー(40byte固定)
 * @note 参考: https://qiita.com/ImagingSolAkira/items/30fd3727afa3076b8050
 *
 */
typedef struct info_header {
    uint32_t bi_size;               /**< biSize: この情報ヘッダーのサイズ(バイト)。通常40(0x28), offset = 14  */
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

struct bmp_loader {
    file_header_t file_header;
    info_header_t info_header;
    size_t padding;
    size_t stride;
    bool padding_removed;
    uint8_t* pixels;
};

static resource_result_t bmp_loader_pixel_bgr_to_rgb(bmp_loader_t* bmp_loader_);
static resource_result_t bmp_loader_pixel_flip(bmp_loader_t* bmp_loader_);
static resource_result_t bmp_loader_padding_remove(bmp_loader_t* bmp_loader_);

static resource_result_t header_load(const char* fullpath_, file_header_t* file_header_, info_header_t* info_header_);
static resource_result_t pixel_load(const char* fullpath_, const file_header_t* file_header_, info_header_t* info_header_, size_t stride_, uint8_t** out_pixels_);

static resource_result_t file_header_parse(char header_[54], file_header_t* file_header_);
static resource_result_t info_header_parse(char header_[54], info_header_t* info_header_);

static void file_header_copy(const file_header_t* src_, file_header_t* dst_);
static void info_header_copy(const info_header_t* src_, info_header_t* dst_);

static bmp_invalid_reason_t is_bmp_supported(const file_header_t* file_header_, const info_header_t* info_header_);
static const char* invalid_reason_to_str(bmp_invalid_reason_t reason_);

static const char* invalid_bmp_file_reason_valid = "valid BMP file";
static const char* invalid_bmp_file_reason_bf_type = "invalid bfType";
static const char* invalid_bmp_file_reason_bf_reserved = "invalid bfReserved field";
static const char* invalid_bmp_file_reason_bf_size = "invalid bfSize";
static const char* invalid_bmp_file_reason_bf_off_bits = "invalid bfOffBits";
static const char* invalid_bmp_file_reason_bi_size = "invalid biSize";
static const char* invalid_bmp_file_reason_bi_planes = "invalid biPlanes";
static const char* invalid_bmp_file_reason_compression = "invalid biCompression";
static const char* invalid_bmp_file_reason_height = "invalid biHeight";
static const char* invalid_bmp_file_reason_width = "invalid biWidth";
static const char* invalid_bmp_file_reason_channel_count = "unsupported biBitCount";
static const char* invalid_bmp_file_reason_not_initialized = "not initialized";
static const char* invalid_bmp_file_reason_undefined = "undefined";

resource_result_t bmp_loader_create(bmp_loader_t** bmp_loader_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    bmp_loader_t* tmp_loader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_create", "bmp_loader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*bmp_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_create", "*bmp_loader_")

    ret_mem = memory_system_allocate(sizeof(bmp_loader_t), MEMORY_TAG_TEXTURE, (void**)&tmp_loader);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("bmp_loader_create(%s) - Failed to allocate memory for tmp_loader.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_loader->file_header.bf_off_bits = 0;
    tmp_loader->file_header.bf_reserved1 = 0;
    tmp_loader->file_header.bf_reserved2 = 0;
    tmp_loader->file_header.bf_size = 0;
    tmp_loader->file_header.bf_type = 0;

    tmp_loader->info_header.bi_bit_count = 0;
    tmp_loader->info_header.bi_clr_important = 0;
    tmp_loader->info_header.bi_clr_used = 0;
    tmp_loader->info_header.bi_compression = 0;
    tmp_loader->info_header.bi_height = 0;
    tmp_loader->info_header.bi_planes = 0;
    tmp_loader->info_header.bi_size = 0;
    tmp_loader->info_header.bi_size_image = 0;
    tmp_loader->info_header.bi_width = 0;
    tmp_loader->info_header.bi_x_pels_per_meter = 0;
    tmp_loader->info_header.bi_y_pels_per_meter = 0;

    tmp_loader->padding = 0;
    tmp_loader->stride = 0;

    tmp_loader->padding_removed = false;

    tmp_loader->pixels = NULL;

    *bmp_loader_ = tmp_loader;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_loader) {
            memory_system_free(tmp_loader, sizeof(bmp_loader_t), MEMORY_TAG_TEXTURE);
            tmp_loader = NULL;
        }
    }
    return ret;
}

void bmp_loader_destroy(bmp_loader_t** bmp_loader_) {
    if(NULL == bmp_loader_) {
        return;
    }
    if(NULL == *bmp_loader_) {
        return;
    }
    if(NULL != (*bmp_loader_)->pixels) {
        memory_system_free((*bmp_loader_)->pixels, (*bmp_loader_)->info_header.bi_size_image, MEMORY_TAG_TEXTURE);
        (*bmp_loader_)->pixels = NULL;
    }

    memory_system_free(*bmp_loader_, sizeof(bmp_loader_t), MEMORY_TAG_TEXTURE);
    *bmp_loader_ = NULL;
}

// pixelロード処理以降でエラーが発生した場合にはbmp_loader_の値は不変とはならない
resource_result_t bmp_loader_load(const char* fullpath_, bmp_loader_t* bmp_loader_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    bmp_invalid_reason_t valid_bmp = BMP_FILE_NOT_INITIALIZED;

    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };
    uint8_t* tmp_pixels = NULL;
    size_t width = 0;
    size_t bit_count = 0;
    size_t stride = 0;
    size_t padding = 0;

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_load", "bmp_loader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(bmp_loader_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_load", "bmp_loader_->pixels")

    ret = header_load(fullpath_, &tmp_file_header, &tmp_info_header);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to load BMP header.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    valid_bmp = is_bmp_supported(&tmp_file_header, &tmp_info_header);
    if(BMP_FILE_NOT_INITIALIZED == valid_bmp) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("bmp_loader_load(%s) - bmp_loader is not initialized.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_UNDEFINED == valid_bmp) {
        ret = RESOURCE_UNDEFINED_ERROR;
        ERROR_MESSAGE("bmp_loader_load(%s) - Undefined error.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_VALID != valid_bmp) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_load(%s) - Unsupported BMP file. reason = '%s'", resource_rslt_to_str(ret), invalid_reason_to_str(valid_bmp));
        goto cleanup;
    }

    width = (size_t)(tmp_info_header.bi_width);
    bit_count = (size_t)(tmp_info_header.bi_bit_count);

    // 現状ではINT16_MAXがサイズの上限なので不要だが、将来的な拡張のためにチェックをいれる
    if((SIZE_MAX / width) < bit_count) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to calculate BMP row stride: bit_count * width would overflow. width=%zu, bit_count=%zu", resource_rslt_to_str(ret), width, bit_count);
        goto cleanup;
    }
    if((SIZE_MAX - 31) < (bit_count * width)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to calculate BMP row stride: row bit count alignment overflow. row_bits=%zu", resource_rslt_to_str(ret), bit_count * width);
        goto cleanup;
    }
    if((SIZE_MAX / 4) < ((bit_count * width + 31) / 32)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to calculate BMP row stride: aligned stride byte count overflow. aligned_units=%zu", resource_rslt_to_str(ret), (bit_count * width + 31) / 32);
        goto cleanup;
    }

    stride = ((bit_count * width + 31) / 32) * 4;
    padding = stride - (bit_count * width / 8);
    ret = pixel_load(fullpath_, &tmp_file_header, &tmp_info_header, stride, &tmp_pixels);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to load BMP pixel data.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    file_header_copy(&tmp_file_header, &bmp_loader_->file_header);
    info_header_copy(&tmp_info_header, &bmp_loader_->info_header);
    bmp_loader_->pixels = tmp_pixels;

    bmp_loader_->padding = padding;
    bmp_loader_->stride = stride;

    ret = bmp_loader_padding_remove(bmp_loader_);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to remove BMP row padding.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = bmp_loader_pixel_bgr_to_rgb(bmp_loader_);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to convert BGR to RGB.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = bmp_loader_pixel_flip(bmp_loader_);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to flip BMP pixel data vertically.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// bmp_loader_->pixelsの管理権限をout_pixels_へ移譲
resource_result_t bmp_loader_pixel_move(bmp_loader_t* bmp_loader_, uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_pixel_move", "bmp_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_pixel_move", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_pixel_move", "*out_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_pixel_move", "bmp_loader_->pixels")

    *out_pixels_ = bmp_loader_->pixels;
    bmp_loader_->pixels = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// GLCEの画像は左上原点を基準とするためheightは基本的に負の値となるが、絶対値を取って返す
resource_result_t bmp_loader_bmp_size_get(const bmp_loader_t* bmp_loader_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    bmp_invalid_reason_t valid_bmp = BMP_FILE_NOT_INITIALIZED;

    int32_t tmp_width = 0;
    int32_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_bmp_size_get", "bmp_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(width_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_bmp_size_get", "width_")
    IF_ARG_NULL_GOTO_CLEANUP(height_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_bmp_size_get", "height_")
    IF_ARG_NULL_GOTO_CLEANUP(channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_bmp_size_get", "channel_count_")

    valid_bmp = is_bmp_supported(&bmp_loader_->file_header, &bmp_loader_->info_header);
    if(BMP_FILE_NOT_INITIALIZED == valid_bmp) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("bmp_loader_bmp_size_get(%s) - bmp_loader is not initialized.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_UNDEFINED == valid_bmp) {
        ret = RESOURCE_UNDEFINED_ERROR;
        ERROR_MESSAGE("bmp_loader_bmp_size_get(%s) - Undefined error.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_VALID != valid_bmp) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_bmp_size_get(%s) - Unsupported BMP file. reason = '%s'", resource_rslt_to_str(ret), invalid_reason_to_str(valid_bmp));
        goto cleanup;
    }

    tmp_width = bmp_loader_->info_header.bi_width;
    tmp_height = (bmp_loader_->info_header.bi_height > 0) ? bmp_loader_->info_header.bi_height : -1 * bmp_loader_->info_header.bi_height;

    if(24 == bmp_loader_->info_header.bi_bit_count) {
        tmp_channel_count = 3;
    } else if(32 == bmp_loader_->info_header.bi_bit_count) {
        tmp_channel_count = 4;
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_bmp_size_get(%s) - Unsupported BMP bit count.", resource_rslt_to_str(ret));
        return ret;
    }

    *width_ = tmp_width;
    *height_ = tmp_height;
    *channel_count_ = tmp_channel_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// この関数はpadding除去後に実行する
static resource_result_t bmp_loader_pixel_bgr_to_rgb(bmp_loader_t* bmp_loader_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    bmp_invalid_reason_t valid_bmp = BMP_FILE_NOT_INITIALIZED;

    size_t channel_count = 0;
    size_t width = 0;
    size_t height = 0;
    size_t ii = 0;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_pixel_bgr_to_rgb", "bmp_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_pixel_bgr_to_rgb", "bmp_loader_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(bmp_loader_->padding_removed, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_pixel_bgr_to_rgb", "bmp_loader_->padding_removed")

    valid_bmp = is_bmp_supported(&bmp_loader_->file_header, &bmp_loader_->info_header);
    if(BMP_FILE_NOT_INITIALIZED == valid_bmp) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("bmp_loader_pixel_bgr_to_rgb(%s) - bmp_loader is not initialized.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_UNDEFINED == valid_bmp) {
        ret = RESOURCE_UNDEFINED_ERROR;
        ERROR_MESSAGE("bmp_loader_pixel_bgr_to_rgb(%s) - Undefined error.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_VALID != valid_bmp) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_pixel_bgr_to_rgb(%s) - Unsupported BMP file. reason = '%s'", resource_rslt_to_str(ret), invalid_reason_to_str(valid_bmp));
        goto cleanup;
    }

    if(24 == bmp_loader_->info_header.bi_bit_count) {
        channel_count = 3;
    } else if(32 == bmp_loader_->info_header.bi_bit_count) {
        channel_count = 4;
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_pixel_bgr_to_rgb(%s) - Unsupported BMP bit count.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    width = (size_t)(bmp_loader_->info_header.bi_width);
    height = (0 < bmp_loader_->info_header.bi_height) ? bmp_loader_->info_header.bi_height : (size_t)(-1 * (int64_t)(bmp_loader_->info_header.bi_height));
    for(size_t i = 0; i != height; ++i) {
        for(size_t j = 0; j != width; ++j) {
            const uint8_t tmp = bmp_loader_->pixels[ii];
            bmp_loader_->pixels[ii] = bmp_loader_->pixels[ii + 2];
            bmp_loader_->pixels[ii + 2] = tmp;
            ii += channel_count;
        }
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// 左下原点の画像を左上原点に直す(GLCEは左上原点の画像を基準にする)
// この関数は必ずpadding除去後に行うこと！！
static resource_result_t bmp_loader_pixel_flip(bmp_loader_t* bmp_loader_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    bmp_invalid_reason_t valid_bmp = BMP_FILE_NOT_INITIALIZED;

    size_t channel_count = 0;
    size_t width = 0;
    size_t height = 0;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_pixel_flip", "bmp_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_pixel_flip", "bmp_loader_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(bmp_loader_->padding_removed, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_pixel_flip", "bmp_loader_->padding_removed")

    valid_bmp = is_bmp_supported(&bmp_loader_->file_header, &bmp_loader_->info_header);
    if(BMP_FILE_NOT_INITIALIZED == valid_bmp) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("bmp_loader_pixel_flip(%s) - bmp_loader is not initialized.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_UNDEFINED == valid_bmp) {
        ret = RESOURCE_UNDEFINED_ERROR;
        ERROR_MESSAGE("bmp_loader_pixel_flip(%s) - Undefined error.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_VALID != valid_bmp) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_pixel_flip(%s) - Unsupported BMP file. reason = '%s'", resource_rslt_to_str(ret), invalid_reason_to_str(valid_bmp));
        goto cleanup;
    }

    if(24 == bmp_loader_->info_header.bi_bit_count) {
        channel_count = 3;
    } else if(32 == bmp_loader_->info_header.bi_bit_count) {
        channel_count = 4;
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_pixel_flip(%s) - Unsupported BMP bit count.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 > bmp_loader_->info_header.bi_height) {
        ret = RESOURCE_SUCCESS;
        goto cleanup;
    }

    width = (size_t)(bmp_loader_->info_header.bi_width);
    height = (size_t)(bmp_loader_->info_header.bi_height);

    if(1 == height) {
        // flip不要なので何もしない
    } else if(2 == height) {
        const size_t width_count = width * channel_count;
        for(size_t i = 0; i != width_count; ++i) {
            uint8_t tmp = bmp_loader_->pixels[i];
            bmp_loader_->pixels[i] = bmp_loader_->pixels[width_count + i];
            bmp_loader_->pixels[width_count + i] = tmp;
        }
    } else {
        size_t back = height - 1;
        const size_t to = height / 2;
        const size_t width_count = width * channel_count;
        for(size_t i = 0; i != to; ++i) {
            for(size_t j = 0; j != width_count; ++j) {
                uint8_t tmp = bmp_loader_->pixels[i * width_count + j];
                bmp_loader_->pixels[i * width_count + j] = bmp_loader_->pixels[back * width_count + j];
                bmp_loader_->pixels[back * width_count + j] = tmp;
            }
            back = back - 1;
        }
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

static resource_result_t bmp_loader_padding_remove(bmp_loader_t* bmp_loader_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    bmp_invalid_reason_t valid_bmp = BMP_FILE_NOT_INITIALIZED;

    uint8_t* new_pixel = NULL;
    size_t new_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_loader_padding_remove", "bmp_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_padding_remove", "bmp_loader_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != bmp_loader_->stride, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_loader_padding_remove", "bmp_loader_->stride")

    valid_bmp = is_bmp_supported(&bmp_loader_->file_header, &bmp_loader_->info_header);
    if(BMP_FILE_NOT_INITIALIZED == valid_bmp) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - bmp_loader is not initialized.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_UNDEFINED == valid_bmp) {
        ret = RESOURCE_UNDEFINED_ERROR;
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - Undefined error.", resource_rslt_to_str(ret));
        goto cleanup;
    } else if(BMP_FILE_VALID != valid_bmp) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - Unsupported BMP file. reason = '%s'", resource_rslt_to_str(ret), invalid_reason_to_str(valid_bmp));
        goto cleanup;
    }

    if(24 == bmp_loader_->info_header.bi_bit_count || 32 == bmp_loader_->info_header.bi_bit_count) {
        const size_t width = (size_t)(bmp_loader_->info_header.bi_width);
        const size_t bit_count = (size_t)(bmp_loader_->info_header.bi_bit_count);
        const size_t channel_count = (24 == bit_count) ? 3 : 4;
        const size_t height = (0 < bmp_loader_->info_header.bi_height) ? bmp_loader_->info_header.bi_height : (size_t)(-1 * (int64_t)(bmp_loader_->info_header.bi_height));

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

        size_t ii = 0;
        size_t ii_new = 0;
        for(size_t i = 0; i != height; ++i) {
            for(size_t j = 0; j != width; ++j) {
                for(size_t k = 0; k != channel_count; ++k) {
                    new_pixel[ii_new + k] = bmp_loader_->pixels[ii + k];
                }
                ii_new += channel_count;
                ii += channel_count;
            }
            ii += bmp_loader_->padding;
        }
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_padding_remove(%s) - Provided BMP file is not supported.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    memory_system_free(bmp_loader_->pixels, bmp_loader_->info_header.bi_size_image, MEMORY_TAG_TEXTURE);
    bmp_loader_->pixels = NULL;
    bmp_loader_->pixels = new_pixel;
    bmp_loader_->info_header.bi_size_image = (uint32_t)new_size;
    bmp_loader_->padding_removed = true;

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
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;

    filesystem_t* filesystem = NULL;
    size_t read_size = 0;
    char header_buf[54] = { 0 };

    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "header_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(file_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "header_load", "file_header_")
    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "header_load", "info_header_")

    ret_fs = filesystem_create(&filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = resource_rslt_convert_filesystem(ret_fs);
        ERROR_MESSAGE("header_load(%s) - Failed to create filesystem.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs = filesystem_open(fullpath_, FILESYSTEM_MODE_READ, filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = resource_rslt_convert_filesystem(ret_fs);
        ERROR_MESSAGE("header_load(%s) - Failed to open BMP file(%s).", resource_rslt_to_str(ret), fullpath_);
        goto cleanup;
    }

    ret_fs = filesystem_byte_read(54, filesystem, &read_size, header_buf);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = resource_rslt_convert_filesystem(ret_fs);
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

    ret_fs = filesystem_close(filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = resource_rslt_convert_filesystem(ret_fs);
        ERROR_MESSAGE("header_load(%s) - Failed to close BMP file.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    filesystem_destroy(&filesystem);

    file_header_copy(&tmp_file_header, file_header_);
    info_header_copy(&tmp_info_header, info_header_);

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        filesystem_destroy(&filesystem);
    }
    return ret;
}

// bi_size_imageは自前で計算するため非const
static resource_result_t pixel_load(const char* fullpath_, const file_header_t* file_header_, info_header_t* info_header_, size_t stride_, uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;

    filesystem_t* filesystem = NULL;
    uint8_t* tmp_buffer = NULL;
    uint8_t* tmp_pixels = NULL;
    size_t read_size_all = 0;
    size_t pixel_buffer_size = 0;

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

    ret_fs = filesystem_create(&filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = resource_rslt_convert_filesystem(ret_fs);
        ERROR_MESSAGE("pixel_load(%s) - Failed to create filesystem.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs = filesystem_open(fullpath_, FILESYSTEM_MODE_READ, filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = resource_rslt_convert_filesystem(ret_fs);
        ERROR_MESSAGE("pixel_load(%s) - Failed to open BMP file(%s).", resource_rslt_to_str(ret), fullpath_);
        goto cleanup;
    }

    ret_fs = filesystem_byte_read(file_header_->bf_size, filesystem, &read_size_all, (char*)tmp_buffer);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = resource_rslt_convert_filesystem(ret_fs);
        ERROR_MESSAGE("pixel_load(%s) - Failed to read BMP file(%s).", resource_rslt_to_str(ret), fullpath_);
        goto cleanup;
    } else if(file_header_->bf_size != read_size_all) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("pixel_load(%s) - Invalid file size.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    // NOTE: info_header_->bi_size_imageはツールによっては信用できない値が入るので、strideとheightから自前で計算する
    size_t height = (0 < info_header_->bi_height) ? (size_t)(info_header_->bi_height) : (size_t)(-1 * (int64_t)info_header_->bi_height);
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
    info_header_->bi_size_image = (uint32_t)pixel_buffer_size;

    if((SIZE_MAX - pixel_buffer_size) < file_header_->bf_off_bits) {
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

    filesystem_destroy(&filesystem);
    memory_system_free(tmp_buffer, file_header_->bf_size, MEMORY_TAG_TEXTURE);
    tmp_buffer = NULL;

    *out_pixels_ = tmp_pixels;
    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        filesystem_destroy(&filesystem);
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

static resource_result_t file_header_parse(char header_[54], file_header_t* file_header_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    file_header_t tmp_header = { 0 };

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

static resource_result_t info_header_parse(char header_[54], info_header_t* info_header_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    info_header_t tmp_header = { 0 };

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
    } else if(0 == info_header_->bi_bit_count || 0 == info_header_->bi_height || 0 == info_header_->bi_width) {
        DEBUG_MESSAGE("is_bmp_supported - Invalid BMP dimensions or bit count: width, height, and bit_count must be non-zero. width=%d, height=%d, bit_count=%u", info_header_->bi_width, info_header_->bi_height, info_header_->bi_bit_count);
        ret = BMP_FILE_NOT_INITIALIZED;
    } else if(INT16_MIN > info_header_->bi_height || INT16_MAX < info_header_->bi_height) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP height: height must be within int16_t-compatible range. height=%d, min=%d, max=%d", info_header_->bi_height, INT16_MIN, INT16_MAX);
        ret = BMP_FILE_INVALID_HEIGHT;
    } else if(info_header_->bi_width < 0 || INT16_MAX < info_header_->bi_width) {
        DEBUG_MESSAGE("is_bmp_supported - Unsupported BMP width: width must be positive and within int16_t-compatible range. width=%d, max=%d", info_header_->bi_width, INT16_MAX);
        ret = BMP_FILE_INVALID_WIDTH;
    } else if(24 != info_header_->bi_bit_count && 32 != info_header_->bi_bit_count) {
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
    case BMP_FILE_NOT_INITIALIZED:
        return invalid_bmp_file_reason_not_initialized;
    case BMP_FILE_UNDEFINED:
        return invalid_bmp_file_reason_undefined;
    default:
        return invalid_bmp_file_reason_undefined;
    }
}
