#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/resources/loaders/bmp_loader.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/buffer_utils/buffer_utils.h"
#include "engine/core/filesystem/filesystem.h"

#include "engine/containers/choco_string.h"

#include "engine/resources/resources_core/resource_types.h"

// TODO:
// - [] resource_core/resource_types.hの実行結果コードをresource_result_tにしてレイヤーで共通利用する。
// - [] bgr_to_rgbでパディングを考慮する
// - [] flip実装
// - [] load処理にbgr_to_rgb、flip、パディング除去追加
// - [] texture.cにloaderを反映する

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
    uint8_t* pixels;
};

static texture_result_t header_load(const char* fullpath_, file_header_t* file_header_, info_header_t* info_header_);
static texture_result_t pixel_load(const char* fullpath_, const file_header_t* file_header_, const info_header_t* info_header_, uint8_t** out_pixels_);
static texture_result_t file_header_parse(char header_[54], file_header_t* file_header_);
static texture_result_t info_header_parse(char header_[54], info_header_t* info_header_);
static void file_header_copy(const file_header_t* src_, file_header_t* dst_);
static void info_header_copy(const info_header_t* src_, info_header_t* dst_);

static bool file_header_valid_check(const file_header_t* file_header_);
static bool info_header_valid_check(const info_header_t* info_header_);

// TODO: resources_core/texture_err_utilsに移す
static const char* s_rslt_str_success = "SUCCESS";                      /**< 実行結果コード文字列: 正常終了 */
static const char* s_rslt_str_no_memory = "NO_MEMORY";                  /**< 実行結果コード文字列: メモリ不足 */
static const char* s_rslt_str_runtime_error = "RUNTIME_ERROR";          /**< 実行結果コード文字列: 実行時エラー */
static const char* s_rslt_str_invalid_argument = "INVALID_ARGUMENT";    /**< 実行結果コード文字列: 引数異常 */
static const char* s_rslt_str_data_corrupted = "DATA_CORRUPTED";        /**< 実行結果コード文字列: メモリ破壊 */
static const char* s_rslt_str_bad_operation = "BAD_OPERATION";          /**< 実行結果コード文字列: API誤用、未初期化 */
static const char* s_rslt_str_overflow = "OVERFLOW";                    /**< 実行結果コード文字列: 計算過程でオーバーフロー発生 */
static const char* s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";        /**< 実行結果コード文字列: システム使用可能範囲上限超過 */
static const char* s_rslt_str_file_open_error = "FILE_OPEN_ERROR";      /**< 実行結果コード文字列: ファイルオープンエラー */
static const char* s_rslt_str_file_read_error = "FILE_READ_ERROR";      /**< 実行結果コード文字列: ファイル読み込みエラー */
static const char* s_rslt_str_file_unsupported = "FILE_UNSUPPORTED";    /**< 実行結果コード文字列: 未対応のファイル形式エラー */
static const char* s_rslt_str_undefined_error = "UNDEFINED_ERROR";      /**< 実行結果コード文字列: 未定義エラー */

static const char* rslt_to_str(texture_result_t rslt_); // TODO: resources_core/texture_err_utilsに移す
static texture_result_t memory_system_result_convert(memory_system_result_t result_);   // TODO: resources_core/texture_err_utilsに移す
static texture_result_t filesystem_result_convert(filesystem_result_t result_); // TODO: resources_core/texture_err_utilsに移す

texture_result_t bmp_loader_create(bmp_loader_t** bmp_loader_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    bmp_loader_t* tmp_loader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_create", "bmp_loader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*bmp_loader_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_create", "*bmp_loader_")

    ret_mem = memory_system_allocate(sizeof(bmp_loader_t), MEMORY_TAG_TEXTURE, (void**)&tmp_loader);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = memory_system_result_convert(ret_mem);
        ERROR_MESSAGE("bmp_loader_create(%s) - Failed to allocate memory for tmp_loader.", rslt_to_str(ret));
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

    tmp_loader->pixels = NULL;

    *bmp_loader_ = tmp_loader;

    ret = TEXTURE_SUCCESS;

cleanup:
    if(TEXTURE_SUCCESS != ret) {
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

texture_result_t bmp_loader_load(const char* fullpath_, bmp_loader_t* bmp_loader_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };
    uint8_t* tmp_pixels = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_load", "bmp_loader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(bmp_loader_->pixels, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "bmp_loader_load", "bmp_loader_->pixels")

    ret = header_load(fullpath_, &tmp_file_header, &tmp_info_header);
    if(TEXTURE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to load bmp header.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = pixel_load(fullpath_, &tmp_file_header, &tmp_info_header, &tmp_pixels);
    if(TEXTURE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_loader_load(%s) - Failed to load bmp pixels.", rslt_to_str(ret));
        goto cleanup;
    }

    file_header_copy(&tmp_file_header, &bmp_loader_->file_header);
    info_header_copy(&tmp_info_header, &bmp_loader_->info_header);
    bmp_loader_->pixels = tmp_pixels;

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

// bmp_loader_->pixelsの管理権限をout_pixels_へ移譲
texture_result_t bmp_loader_pixel_move(bmp_loader_t* bmp_loader_, uint8_t** out_pixels_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_pixel_move", "bmp_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_pixel_move", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_pixel_move", "*out_pixels_")
    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_->pixels, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "bmp_loader_pixel_move", "bmp_loader_->pixels")

    *out_pixels_ = bmp_loader_->pixels;
    bmp_loader_->pixels = NULL;

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

texture_result_t bmp_loader_bmp_size_get(const bmp_loader_t* bmp_loader_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_bmp_size_get", "bmp_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(width_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_bmp_size_get", "width_")
    IF_ARG_NULL_GOTO_CLEANUP(height_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_bmp_size_get", "height_")
    IF_ARG_NULL_GOTO_CLEANUP(channel_count_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_bmp_size_get", "channel_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != bmp_loader_->info_header.bi_width, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "bmp_loader_bmp_size_get", "bmp_loader_->info_header.bi_width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != bmp_loader_->info_header.bi_height, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "bmp_loader_bmp_size_get", "bmp_loader_->info_header.bi_height")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != bmp_loader_->info_header.bi_bit_count, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "bmp_loader_bmp_size_get", "bmp_loader_->info_header.bi_bit_count")

    tmp_width = bmp_loader_->info_header.bi_width;
    tmp_height = bmp_loader_->info_header.bi_height;
    if(24 == bmp_loader_->info_header.bi_bit_count) {
        tmp_channel_count = 3;
    } else if(32 == bmp_loader_->info_header.bi_bit_count) {
        tmp_channel_count = 4;
    } else {
        ret = TEXTURE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("bmp_loader_bmp_size_get(%s) - Unsupported file type.", rslt_to_str(ret));
        return ret;
    }

    *width_ = tmp_width;
    *height_ = tmp_height;
    *channel_count_ = tmp_channel_count;

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

static texture_result_t bmp_loader_pixel_bgr_to_rgb(bmp_loader_t* bmp_loader_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "bmp_loader_pixel_bgr_to_rgb", "bmp_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(bmp_loader_->pixels, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "bmp_loader_pixel_bgr_to_rgb", "bmp_loader_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != bmp_loader_->info_header.bi_bit_count, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "bmp_loader_pixel_bgr_to_rgb", "bmp_loader_->info_header.bi_bit_count")

    if(24 == bmp_loader_->info_header.bi_bit_count || 32 == bmp_loader_->info_header.bi_bit_count) {
        const size_t count = (24 == bmp_loader_->info_header.bi_bit_count) ? 3 : 4;
        for(size_t i = 0, ii = 0; i != (size_t)(bmp_loader_->info_header.bi_width * bmp_loader_->info_header.bi_height); ++i, ii += count) {
            const uint8_t tmp = bmp_loader_->pixels[ii];
            bmp_loader_->pixels[ii] = bmp_loader_->pixels[ii + 2];
            bmp_loader_->pixels[ii + 2] = tmp;
        }
    } else {
        ret = TEXTURE_UNSUPPORTED_FILE;
        goto cleanup;
    }
    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

static texture_result_t bmp_loader_pixel_flip(void) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    //　後で実装

    return ret;
}

static texture_result_t header_load(const char* fullpath_, file_header_t* file_header_, info_header_t* info_header_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;

    filesystem_t* filesystem = NULL;
    size_t read_size = 0;
    char header_buf[54] = { 0 };

    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "header_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(file_header_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "header_load", "file_header_")
    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "header_load", "info_header_")

    ret_fs = filesystem_create(&filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("header_load(%s) - Failed to create filesystem.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs = filesystem_open(fullpath_, FILESYSTEM_MODE_READ, filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("header_load(%s) - Failed to open bmp file(%s).", rslt_to_str(ret), fullpath_);
        goto cleanup;
    }

    ret_fs = filesystem_byte_read(54, filesystem, &read_size, header_buf);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("header_load(%s) - Failed to read bmp file header.", rslt_to_str(ret));
        goto cleanup;
    } else if(54 != read_size) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("header_load(%s) - Invalid bmp file format(header size: %d).", rslt_to_str(ret), read_size);
        goto cleanup;
    }

    ret = file_header_parse(header_buf, &tmp_file_header);
    if(TEXTURE_SUCCESS != ret) {
        ERROR_MESSAGE("header_load(%s) - Failed to parse bmp file header.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = info_header_parse(header_buf, &tmp_info_header);
    if(TEXTURE_SUCCESS != ret) {
        ERROR_MESSAGE("header_load(%s) - Failed to parse bmp info header.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs = filesystem_close(filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("header_load(%s) - Failed to close bmp file.", rslt_to_str(ret));
        goto cleanup;
    }

    filesystem_destroy(&filesystem);

    file_header_copy(&tmp_file_header, file_header_);
    info_header_copy(&tmp_info_header, info_header_);

    ret = TEXTURE_SUCCESS;

cleanup:
    if(TEXTURE_SUCCESS != ret) {
        filesystem_destroy(&filesystem);
    }
    return ret;
}

static texture_result_t pixel_load(const char* fullpath_, const file_header_t* file_header_, const info_header_t* info_header_, uint8_t** out_pixels_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;

    filesystem_t* filesystem = NULL;
    uint8_t* tmp_buffer = NULL;
    uint8_t* tmp_pixels = NULL;
    size_t read_size_all = 0;

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "pixel_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(file_header_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "pixel_load", "file_header_")
    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "pixel_load", "info_header_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "pixel_load", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "pixel_load", "*out_pixels_")

    ret_mem = memory_system_allocate(file_header_->bf_size, MEMORY_TAG_TEXTURE, (void**)&tmp_buffer);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = memory_system_result_convert(ret_mem);
        ERROR_MESSAGE("pixel_load(%s) - Failed to allocate memory for tmp_buffer.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs = filesystem_create(&filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("pixel_load(%s) - Failed to create filesystem.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs = filesystem_open(fullpath_, FILESYSTEM_MODE_READ, filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("pixel_load(%s) - Failed to open bmp file(%s).", rslt_to_str(ret), fullpath_);
        goto cleanup;
    }

    ret_fs = filesystem_byte_read(file_header_->bf_size, filesystem, &read_size_all, (char*)tmp_buffer);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("pixel_load(%s) - Failed to read bmp file(%s).", rslt_to_str(ret), fullpath_);
        goto cleanup;
    } else if(file_header_->bf_size != read_size_all) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("pixel_load(%s) - Invalid file size.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_mem = memory_system_allocate(info_header_->bi_size_image, MEMORY_TAG_TEXTURE, (void**)&tmp_pixels);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = memory_system_result_convert(ret_mem);
        ERROR_MESSAGE("pixel_load(%s) - Failed to allocate memory for tmp_pixels.", rslt_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != info_header_->bi_size_image; ++i) {
        tmp_pixels[i] = tmp_buffer[i + file_header_->bf_off_bits];
    }

    filesystem_destroy(&filesystem);
    memory_system_free(tmp_buffer, file_header_->bf_size, MEMORY_TAG_TEXTURE);
    tmp_buffer = NULL;

    *out_pixels_ = tmp_pixels;
    ret = TEXTURE_SUCCESS;

cleanup:
    if(TEXTURE_SUCCESS != ret) {
        filesystem_destroy(&filesystem);
        if(NULL != tmp_buffer) {
            memory_system_free(tmp_buffer, file_header_->bf_size, MEMORY_TAG_TEXTURE);
            tmp_buffer = NULL;
        }
        if(NULL != tmp_pixels) {
            memory_system_free(tmp_pixels, info_header_->bi_size_image, MEMORY_TAG_TEXTURE);
            tmp_pixels = NULL;
        }
    }
    return ret;
}

static texture_result_t file_header_parse(char header_[54], file_header_t* file_header_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;
    file_header_t tmp_header = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(file_header_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "file_header_parse", "file_header_");

    tmp_header.bf_type = buffer_utils_le_uint16_t_get(header_);
    tmp_header.bf_size = buffer_utils_le_uint32_t_get(header_ + 2);
    tmp_header.bf_reserved1 = buffer_utils_le_uint16_t_get(header_ + 6);
    tmp_header.bf_reserved2 = buffer_utils_le_uint16_t_get(header_ + 8);
    tmp_header.bf_off_bits = buffer_utils_le_uint32_t_get(header_ + 10);

    if(!file_header_valid_check(&tmp_header)) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("file_header_parse(%s) - bmp file header is broken.", rslt_to_str(ret));
        goto cleanup;
    }
    file_header_copy(&tmp_header, file_header_);

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

static texture_result_t info_header_parse(char header_[54], info_header_t* info_header_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;
    info_header_t tmp_header = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(info_header_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "info_header_parse", "info_header_");

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

    if(!info_header_valid_check(&tmp_header)) {
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("info_header_parse(%s) - bmp info header is broken.", rslt_to_str(ret));
        goto cleanup;
    }
    info_header_copy(&tmp_header, info_header_);

    ret = TEXTURE_SUCCESS;

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

static bool file_header_valid_check(const file_header_t* file_header_) {
    if(NULL == file_header_) {
        return false;
    }

    // bf_type != "BM"
    if(0x4D42 != file_header_->bf_type) {
        DEBUG_MESSAGE("file_header_valid_check - file_header->bf_type is not valid(%d).", file_header_->bf_type);
        return false;
    }
    if(0 != file_header_->bf_reserved1 || 0 != file_header_->bf_reserved2) {
        DEBUG_MESSAGE("file_header_valid_check - file_header->bf_reserved1,2 is not valid(%d, %d).", file_header_->bf_reserved1, file_header_->bf_reserved2);
        return false;
    }
    if(54 >= file_header_->bf_size) {
        DEBUG_MESSAGE("file_header_valid_check - file_header->bf_size is not valid(%d).", file_header_->bf_size);
        return false;
    }
    return true;
}

static bool info_header_valid_check(const info_header_t* info_header_) {
    if(NULL == info_header_) {
        return false;
    }
    if(40 != info_header_->bi_size) {
        DEBUG_MESSAGE("info_header_valid_check - info_header->bi_size is not valid(%d).", info_header_->bi_size);
        return false;
    }
    return true;
}

// TODO: resources_core/texture_err_utilsに移す
static const char* rslt_to_str(texture_result_t rslt_) {
    switch(rslt_) {
    case TEXTURE_SUCCESS:
        return s_rslt_str_success;
    case TEXTURE_NO_MEMORY:
        return s_rslt_str_no_memory;
    case TEXTURE_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case TEXTURE_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case TEXTURE_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case TEXTURE_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case TEXTURE_OVERFLOW:
        return s_rslt_str_overflow;
    case TEXTURE_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case TEXTURE_FILE_OPEN_ERROR:
        return s_rslt_str_file_open_error;
    case TEXTURE_FILE_READ_ERROR:
        return s_rslt_str_file_read_error;
    case TEXTURE_UNSUPPORTED_FILE:
        return s_rslt_str_file_unsupported;
    case TEXTURE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

static texture_result_t memory_system_result_convert(memory_system_result_t result_) {
    switch(result_) {
    case MEMORY_SYSTEM_SUCCESS:
        return TEXTURE_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return TEXTURE_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return TEXTURE_RUNTIME_ERROR;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return TEXTURE_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return TEXTURE_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return TEXTURE_NO_MEMORY;
    default:
        return TEXTURE_UNDEFINED_ERROR;
    }
}

static texture_result_t filesystem_result_convert(filesystem_result_t result_) {
    switch(result_) {
    case FILESYSTEM_SUCCESS:
        return TEXTURE_SUCCESS;
    case FILESYSTEM_INVALID_ARGUMENT:
        return TEXTURE_INVALID_ARGUMENT;
    case FILESYSTEM_RUNTIME_ERROR:
        return TEXTURE_RUNTIME_ERROR;
    case FILESYSTEM_NO_MEMORY:
        return TEXTURE_NO_MEMORY;
    case FILESYSTEM_FILE_OPEN_ERROR:
        return TEXTURE_FILE_OPEN_ERROR;
    case FILESYSTEM_FILE_CLOSE_ERROR:
        return TEXTURE_UNDEFINED_ERROR;
    case FILESYSTEM_UNDEFINED_ERROR:
        return TEXTURE_UNDEFINED_ERROR;
    case FILESYSTEM_LIMIT_EXCEEDED:
        return TEXTURE_LIMIT_EXCEEDED;
    case FILESYSTEM_BAD_OPERATION:
        return TEXTURE_BAD_OPERATION;
    case FILESYSTEM_EOF:
        return TEXTURE_FILE_READ_ERROR;
    }
}
