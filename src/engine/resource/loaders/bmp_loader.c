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

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

// #define TEST_BUILD

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

static const char* const invalid_bmp_file_reason_valid = "valid BMP file";
static const char* const invalid_bmp_file_reason_bf_type = "invalid bfType";
static const char* const invalid_bmp_file_reason_bf_reserved = "invalid bfReserved field";
static const char* const invalid_bmp_file_reason_bf_size = "invalid bfSize";
static const char* const invalid_bmp_file_reason_bf_off_bits = "invalid bfOffBits";
static const char* const invalid_bmp_file_reason_bi_size = "invalid biSize";
static const char* const invalid_bmp_file_reason_bi_planes = "invalid biPlanes";
static const char* const invalid_bmp_file_reason_compression = "invalid biCompression";
static const char* const invalid_bmp_file_reason_height = "invalid biHeight";
static const char* const invalid_bmp_file_reason_width = "invalid biWidth";
static const char* const invalid_bmp_file_reason_channel_count = "unsupported biBitCount";
static const char* const invalid_bmp_file_reason_not_initialized = "not initialized";
static const char* const invalid_bmp_file_reason_undefined = "undefined";

#ifdef TEST_BUILD
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "test_controller.h"

#include "engine/resource/loaders/test_bmp_loader.h"

#include "engine/core/filesystem/test_filesystem.h"
#include "engine/core/memory/test_choco_memory.h"

// bmp_loader用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_bmp_loader_create;         /**< bmp_loader_create()テスト設定 */
static test_call_control_t s_test_config_bmp_loader_load;           /**< bmp_loader_load()テスト設定 */
static test_call_control_t s_test_config_bmp_loader_pixel_move;     /**< bmp_loader_pixel_move()テスト設定 */
static test_call_control_t s_test_config_bmp_loader_bmp_size_get;   /**< bmp_loader_bmp_size_get()テスト設定 */

// プライベート関数テスト設定
static test_call_control_t s_test_config_bmp_loader_pixel_bgr_to_rgb;   /**< bmp_loader_pixel_bgr_to_rgb()テスト設定 */
static test_call_control_t s_test_config_bmp_loader_pixel_flip;         /**< bmp_loader_pixel_flip()テスト設定 */
static test_call_control_t s_test_config_bmp_loader_padding_remove;     /**< bmp_loader_padding_remove()テスト設定 */
static test_call_control_t s_test_config_header_load;                   /**< header_load()テスト設定 */
static test_call_control_t s_test_config_pixel_load;                    /**< pixel_load()テスト設定 */
static test_call_control_t s_test_config_file_header_parse;             /**< file_header_parse()テスト設定 */
static test_call_control_t s_test_config_info_header_parse;             /**< info_header_parse()テスト設定 */
static test_call_control_t s_test_config_is_bmp_supported;              /**< is_bmp_supported()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_bmp_loader_create(void);
static void test_bmp_loader_destroy(void);
static void test_bmp_loader_load(void);
static void test_bmp_loader_pixel_move(void);
static void test_bmp_loader_bmp_size_get(void);
static void test_bmp_loader_pixel_bgr_to_rgb(void);
static void test_bmp_loader_pixel_flip(void);
static void test_bmp_loader_padding_remove(void);
static void test_header_load(void);
static void test_pixel_load(void);
static void test_file_header_parse(void);
static void test_info_header_parse(void);
static void test_file_header_copy(void);
static void test_info_header_copy(void);
static void test_is_bmp_supported(void);
static void test_invalid_reason_to_str(void);

// テスト用ヘルパー関数
static void test_bmp_loader_valid_header_make(file_header_t* file_header_, info_header_t* info_header_);
static void test_bmp_file_write(const char* filepath_, const uint8_t* data_, size_t size_);
static void test_bmp_file_2x2_24bit_bottom_up_write(const char* filepath_);
static void test_bmp_file_2x2_24bit_top_down_write(const char* filepath_);
static void test_bmp_file_2x2_32bit_bottom_up_write(const char* filepath_);
#endif

resource_result_t bmp_loader_create(bmp_loader_t** bmp_loader_) {
#ifdef TEST_BUILD
    s_test_config_bmp_loader_create.call_count++;
    if(s_test_config_bmp_loader_create.fail_on_call != 0) {
        if(s_test_config_bmp_loader_create.call_count == s_test_config_bmp_loader_create.fail_on_call) {
            return (resource_result_t)s_test_config_bmp_loader_create.forced_result;
        }
    }
#endif
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

resource_result_t bmp_loader_load(const char* fullpath_, bmp_loader_t* bmp_loader_) {
#ifdef TEST_BUILD
    s_test_config_bmp_loader_load.call_count++;
    if(s_test_config_bmp_loader_load.fail_on_call != 0) {
        if(s_test_config_bmp_loader_load.call_count == s_test_config_bmp_loader_load.fail_on_call) {
            return (resource_result_t)s_test_config_bmp_loader_load.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    bmp_invalid_reason_t valid_bmp = BMP_FILE_NOT_INITIALIZED;

    file_header_t tmp_file_header = { 0 };
    info_header_t tmp_info_header = { 0 };
    uint8_t* tmp_pixels = NULL;
    uint8_t* formatted_pixels = NULL;
    size_t formatted_size = 0;
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
        tmp_info_header.bi_size_image = formatted_size;
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

    file_header_copy(&tmp_file_header, &bmp_loader_->file_header);
    info_header_copy(&tmp_info_header, &bmp_loader_->info_header);
    bmp_loader_->padding = 0;
    bmp_loader_->stride = stride - padding;
    bmp_loader_->padding_removed = true;
    bmp_loader_->pixels = tmp_pixels;

    tmp_pixels = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != formatted_pixels && 0 != formatted_size) {
            memory_system_free(formatted_pixels, formatted_size, MEMORY_TAG_TEXTURE);
            formatted_pixels = NULL;
        }
        if(NULL != tmp_pixels) {
            memory_system_free(tmp_pixels, tmp_info_header.bi_size_image, MEMORY_TAG_TEXTURE);
            tmp_pixels = NULL;
        }
    }
    return ret;
}

// bmp_loader_->pixelsの管理権限をout_pixels_へ移譲
resource_result_t bmp_loader_pixel_move(bmp_loader_t* bmp_loader_, uint8_t** out_pixels_) {
#ifdef TEST_BUILD
    s_test_config_bmp_loader_pixel_move.call_count++;
    if(s_test_config_bmp_loader_pixel_move.fail_on_call != 0) {
        if(s_test_config_bmp_loader_pixel_move.call_count == s_test_config_bmp_loader_pixel_move.fail_on_call) {
            return (resource_result_t)s_test_config_bmp_loader_pixel_move.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_bmp_loader_bmp_size_get.call_count++;
    if(s_test_config_bmp_loader_bmp_size_get.fail_on_call != 0) {
        if(s_test_config_bmp_loader_bmp_size_get.call_count == s_test_config_bmp_loader_bmp_size_get.fail_on_call) {
            return (resource_result_t)s_test_config_bmp_loader_bmp_size_get.forced_result;
        }
    }
#endif
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
// is_bmp_supportedを通過したinfo_header_を与えること
static resource_result_t bmp_loader_pixel_bgr_to_rgb(const info_header_t* info_header_, uint8_t* pixels_) {
#ifdef TEST_BUILD
    s_test_config_bmp_loader_pixel_bgr_to_rgb.call_count++;
    if(s_test_config_bmp_loader_pixel_bgr_to_rgb.fail_on_call != 0) {
        if(s_test_config_bmp_loader_pixel_bgr_to_rgb.call_count == s_test_config_bmp_loader_pixel_bgr_to_rgb.fail_on_call) {
            return (resource_result_t)s_test_config_bmp_loader_pixel_bgr_to_rgb.forced_result;
        }
    }
#endif
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
    height = (0 < info_header_->bi_height) ? info_header_->bi_height : (size_t)(-1 * (int64_t)(info_header_->bi_height));
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

// 左下原点の画像を左上原点に直す(GLCEは左上原点の画像を基準にする)
// この関数は必ずpadding除去後に行うこと！！
static resource_result_t bmp_loader_pixel_flip(const info_header_t* info_header_, uint8_t* pixels_) {
#ifdef TEST_BUILD
    s_test_config_bmp_loader_pixel_flip.call_count++;
    if(s_test_config_bmp_loader_pixel_flip.fail_on_call != 0) {
        if(s_test_config_bmp_loader_pixel_flip.call_count == s_test_config_bmp_loader_pixel_flip.fail_on_call) {
            return (resource_result_t)s_test_config_bmp_loader_pixel_flip.forced_result;
        }
    }
#endif
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

// padding_ > 0 の 24bit BMP行データ専用
// 32bit BMPでは通常paddingが発生しないため、この関数の呼び出し対象外
static resource_result_t bmp_loader_padding_remove(const info_header_t* info_header_, size_t stride_, size_t padding_, const uint8_t* src_pixels_, uint8_t** dst_pixels_, size_t* out_new_size_) {
#ifdef TEST_BUILD
    s_test_config_bmp_loader_padding_remove.call_count++;
    if(s_test_config_bmp_loader_padding_remove.fail_on_call != 0) {
        if(s_test_config_bmp_loader_padding_remove.call_count == s_test_config_bmp_loader_padding_remove.fail_on_call) {
            return (resource_result_t)s_test_config_bmp_loader_padding_remove.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    uint8_t* new_pixel = NULL;
    size_t new_size = 0;

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

    const size_t width = (size_t)(info_header_->bi_width);
    const size_t channel_count = 3;
    const size_t height = (0 < info_header_->bi_height) ? info_header_->bi_height : (size_t)(-1 * (int64_t)(info_header_->bi_height));

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
#ifdef TEST_BUILD
    s_test_config_header_load.call_count++;
    if(s_test_config_header_load.fail_on_call != 0) {
        if(s_test_config_header_load.call_count == s_test_config_header_load.fail_on_call) {
            return (resource_result_t)s_test_config_header_load.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_pixel_load.call_count++;
    if(s_test_config_pixel_load.fail_on_call != 0) {
        if(s_test_config_pixel_load.call_count == s_test_config_pixel_load.fail_on_call) {
            return (resource_result_t)s_test_config_pixel_load.forced_result;
        }
    }
#endif
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

static resource_result_t file_header_parse(const char header_[54], file_header_t* file_header_) {
#ifdef TEST_BUILD
    s_test_config_file_header_parse.call_count++;
    if(s_test_config_file_header_parse.fail_on_call != 0) {
        if(s_test_config_file_header_parse.call_count == s_test_config_file_header_parse.fail_on_call) {
            return (resource_result_t)s_test_config_file_header_parse.forced_result;
        }
    }
#endif
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

static resource_result_t info_header_parse(const char header_[54], info_header_t* info_header_) {
#ifdef TEST_BUILD
    s_test_config_info_header_parse.call_count++;
    if(s_test_config_info_header_parse.fail_on_call != 0) {
        if(s_test_config_info_header_parse.call_count == s_test_config_info_header_parse.fail_on_call) {
            return (resource_result_t)s_test_config_info_header_parse.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_is_bmp_supported.call_count++;
    if(s_test_config_is_bmp_supported.fail_on_call != 0) {
        if(s_test_config_is_bmp_supported.call_count == s_test_config_is_bmp_supported.fail_on_call) {
            return (bmp_invalid_reason_t)s_test_config_is_bmp_supported.forced_result;
        }
    }
#endif
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

#ifdef TEST_BUILD
void NO_COVERAGE test_bmp_loader_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_bmp_loader_create.fail_on_call = config_->fail_on_call;
    s_test_config_bmp_loader_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_bmp_loader_load_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_bmp_loader_load.fail_on_call = config_->fail_on_call;
    s_test_config_bmp_loader_load.forced_result = config_->forced_result;
}

void NO_COVERAGE test_bmp_loader_pixel_move_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_bmp_loader_pixel_move.fail_on_call = config_->fail_on_call;
    s_test_config_bmp_loader_pixel_move.forced_result = config_->forced_result;
}

void NO_COVERAGE test_bmp_loader_bmp_size_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_bmp_loader_bmp_size_get.fail_on_call = config_->fail_on_call;
    s_test_config_bmp_loader_bmp_size_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_bmp_loader_config_reset(void) {
    test_call_control_reset(&s_test_config_bmp_loader_create);
    test_call_control_reset(&s_test_config_bmp_loader_load);
    test_call_control_reset(&s_test_config_bmp_loader_pixel_move);
    test_call_control_reset(&s_test_config_bmp_loader_bmp_size_get);

    test_call_control_reset(&s_test_config_bmp_loader_pixel_bgr_to_rgb);
    test_call_control_reset(&s_test_config_bmp_loader_pixel_flip);
    test_call_control_reset(&s_test_config_bmp_loader_padding_remove);
    test_call_control_reset(&s_test_config_header_load);
    test_call_control_reset(&s_test_config_pixel_load);
    test_call_control_reset(&s_test_config_file_header_parse);
    test_call_control_reset(&s_test_config_info_header_parse);
    test_call_control_reset(&s_test_config_is_bmp_supported);
}

void NO_COVERAGE test_bmp_loader(void) {
    test_bmp_loader_create();
    test_bmp_loader_destroy();
    test_bmp_loader_load();
    test_bmp_loader_pixel_move();
    test_bmp_loader_bmp_size_get();
    test_bmp_loader_pixel_bgr_to_rgb();
    test_bmp_loader_pixel_flip();
    test_bmp_loader_padding_remove();
    test_header_load();
    test_pixel_load();
    test_file_header_parse();
    test_info_header_parse();
    test_file_header_copy();
    test_info_header_copy();
    test_is_bmp_supported();
    test_invalid_reason_to_str();
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_loader_create(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // bmp_loader_create() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t* loader = NULL;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        s_test_config_bmp_loader_create.fail_on_call = 1U;
        s_test_config_bmp_loader_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == loader);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // bmp_loader_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_create(NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *bmp_loader_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t dummy_loader = { 0 };
        bmp_loader_t* loader = &dummy_loader;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_loader == loader);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t* loader = NULL;
        test_call_control_t config = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == loader);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: bmp_loader_t が確保され、全フィールドが初期化される
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t* loader = NULL;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        assert(0U == loader->file_header.bf_type);
        assert(0U == loader->file_header.bf_reserved1);
        assert(0U == loader->file_header.bf_reserved2);
        assert(0U == loader->file_header.bf_size);
        assert(0U == loader->file_header.bf_off_bits);

        assert(0U == loader->info_header.bi_size);
        assert(0 == loader->info_header.bi_width);
        assert(0 == loader->info_header.bi_height);
        assert(0U == loader->info_header.bi_planes);
        assert(0U == loader->info_header.bi_bit_count);
        assert(0U == loader->info_header.bi_compression);
        assert(0U == loader->info_header.bi_size_image);
        assert(0 == loader->info_header.bi_x_pels_per_meter);
        assert(0 == loader->info_header.bi_y_pels_per_meter);
        assert(0U == loader->info_header.bi_clr_used);
        assert(0U == loader->info_header.bi_clr_important);

        assert(0U == loader->padding);
        assert(0U == loader->stride);
        assert(false == loader->padding_removed);
        assert(NULL == loader->pixels);

        bmp_loader_destroy(&loader);
        assert(NULL == loader);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_loader_destroy(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // bmp_loader_ == NULL -> no-op
        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        bmp_loader_destroy(NULL);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *bmp_loader_ == NULL -> no-op
        bmp_loader_t* loader = NULL;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        bmp_loader_destroy(&loader);
        assert(NULL == loader);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // pixels == NULL のloaderを破棄する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t* loader = NULL;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);
        assert(NULL == loader->pixels);

        bmp_loader_destroy(&loader);
        assert(NULL == loader);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // pixels != NULL のloaderを破棄する
        // pixels と loader 本体の両方が解放され、loader は NULL になる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        bmp_loader_t* loader = NULL;

        const size_t pixel_size = 12U;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        ret_mem = memory_system_allocate(pixel_size, MEMORY_TAG_TEXTURE, (void**)&loader->pixels);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != loader->pixels);

        loader->info_header.bi_size_image = (uint32_t)pixel_size;

        bmp_loader_destroy(&loader);
        assert(NULL == loader);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // destroy後にもう一度destroyしてもno-op
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t* loader = NULL;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        bmp_loader_destroy(&loader);
        assert(NULL == loader);

        bmp_loader_destroy(&loader);
        assert(NULL == loader);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_loader_load(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // bmp_loader_load() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        s_test_config_bmp_loader_load.fail_on_call = 1U;
        s_test_config_bmp_loader_load.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // fullpath_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_load(NULL, &loader);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // bmp_loader_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // bmp_loader_->pixels != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint8_t dummy_pixels[4] = { 1U, 2U, 3U, 4U };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        loader.pixels = dummy_pixels;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(dummy_pixels == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // header_load() 失敗 -> エラーを返し、bmp_loader_ は未変更
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        s_test_config_header_load.fail_on_call = 1U;
        s_test_config_header_load.forced_result = (int)RESOURCE_FILE_OPEN_ERROR;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_FILE_OPEN_ERROR == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // is_bmp_supported() == BMP_FILE_NOT_INITIALIZED -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        s_test_config_is_bmp_supported.fail_on_call = 1U;
        s_test_config_is_bmp_supported.forced_result = (int)BMP_FILE_NOT_INITIALIZED;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        remove("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // is_bmp_supported() == BMP_FILE_UNDEFINED -> RESOURCE_UNDEFINED_ERROR
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        s_test_config_is_bmp_supported.fail_on_call = 1U;
        s_test_config_is_bmp_supported.forced_result = (int)BMP_FILE_UNDEFINED;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_UNDEFINED_ERROR == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        remove("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // is_bmp_supported() が未対応理由を返す -> RESOURCE_UNSUPPORTED_FILE
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        s_test_config_is_bmp_supported.fail_on_call = 1U;
        s_test_config_is_bmp_supported.forced_result = (int)BMP_FILE_INVALID_CHANNEL_COUNT;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_UNSUPPORTED_FILE == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        remove("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // pixel_load() 失敗 -> エラーを返し、bmp_loader_ は未変更
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        s_test_config_pixel_load.fail_on_call = 1U;
        s_test_config_pixel_load.forced_result = (int)RESOURCE_FILE_READ_ERROR;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_FILE_READ_ERROR == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        remove("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // paddingあり24bit BMPで padding_remove() 失敗 -> エラーを返し、bmp_loader_ は未変更
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        s_test_config_bmp_loader_padding_remove.fail_on_call = 1U;
        s_test_config_bmp_loader_padding_remove.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        remove("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // bgr_to_rgb() 失敗 -> エラーを返し、bmp_loader_ は未変更
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        s_test_config_bmp_loader_pixel_bgr_to_rgb.fail_on_call = 1U;
        s_test_config_bmp_loader_pixel_bgr_to_rgb.forced_result = (int)RESOURCE_BAD_OPERATION;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        remove("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // pixel_flip() 失敗 -> エラーを返し、bmp_loader_ は未変更
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        s_test_config_bmp_loader_pixel_flip.fail_on_call = 1U;
        s_test_config_bmp_loader_pixel_flip.forced_result = (int)RESOURCE_BAD_OPERATION;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", &loader);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == loader.pixels);
        assert(0U == loader.file_header.bf_type);
        assert(0U == loader.info_header.bi_size);
        assert(0U == loader.padding);
        assert(0U == loader.stride);
        assert(false == loader.padding_removed);

        remove("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / bottom-up / paddingあり BMPをロードする
        // 最終的にRGB・左上原点・padding除去済みのピクセル列になる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t* loader = NULL;

        const uint8_t expected_pixels[12] = {
            // top row
            0xFF, 0x00, 0x00,   // red
            0x00, 0xFF, 0x00,   // green

            // bottom row
            0x00, 0x00, 0xFF,   // blue
            0xFF, 0xFF, 0xFF    // white
        };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_bottom_up.bmp", loader);
        assert(RESOURCE_SUCCESS == ret);

        assert((uint16_t)0x4D42U == loader->file_header.bf_type);
        assert(70U == loader->file_header.bf_size);
        assert(54U == loader->file_header.bf_off_bits);

        assert(40U == loader->info_header.bi_size);
        assert(2 == loader->info_header.bi_width);
        assert(2 == loader->info_header.bi_height);
        assert(1U == loader->info_header.bi_planes);
        assert(24U == loader->info_header.bi_bit_count);
        assert(0U == loader->info_header.bi_compression);
        assert(12U == loader->info_header.bi_size_image);

        assert(0U == loader->padding);
        assert(6U == loader->stride);
        assert(true == loader->padding_removed);
        assert(NULL != loader->pixels);
        assert(0 == memcmp(loader->pixels, expected_pixels, sizeof(expected_pixels)));

        bmp_loader_destroy(&loader);
        assert(NULL == loader);

        remove("test_bmp_loader_load_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / top-down / paddingあり BMPをロードする
        // top-downなのでflipはno-opになり、RGB・左上原点のままになる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t* loader = NULL;

        const uint8_t expected_pixels[12] = {
            // top row
            0xFF, 0x00, 0x00,   // red
            0x00, 0xFF, 0x00,   // green

            // bottom row
            0x00, 0x00, 0xFF,   // blue
            0xFF, 0xFF, 0xFF    // white
        };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        test_bmp_file_2x2_24bit_top_down_write("test_bmp_loader_load_2x2_24bit_top_down.bmp");

        ret = bmp_loader_load("test_bmp_loader_load_2x2_24bit_top_down.bmp", loader);
        assert(RESOURCE_SUCCESS == ret);

        assert((uint16_t)0x4D42U == loader->file_header.bf_type);
        assert(70U == loader->file_header.bf_size);
        assert(54U == loader->file_header.bf_off_bits);

        assert(40U == loader->info_header.bi_size);
        assert(2 == loader->info_header.bi_width);
        assert(-2 == loader->info_header.bi_height);
        assert(1U == loader->info_header.bi_planes);
        assert(24U == loader->info_header.bi_bit_count);
        assert(0U == loader->info_header.bi_compression);
        assert(12U == loader->info_header.bi_size_image);

        assert(0U == loader->padding);
        assert(6U == loader->stride);
        assert(true == loader->padding_removed);
        assert(NULL != loader->pixels);
        assert(0 == memcmp(loader->pixels, expected_pixels, sizeof(expected_pixels)));

        bmp_loader_destroy(&loader);
        assert(NULL == loader);

        remove("test_bmp_loader_load_2x2_24bit_top_down.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2x2 / 32bit / bottom-up / paddingなし BMPをロードする
        // padding_remove() は呼ばれず、BGRA->RGBA変換とflipのみ行われる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t* loader = NULL;

        const uint8_t expected_pixels[16] = {
            // top row
            0xFF, 0x00, 0x00, 0xFF,   // red
            0x00, 0xFF, 0x00, 0xFF,   // green

            // bottom row
            0x00, 0x00, 0xFF, 0xFF,   // blue
            0xFF, 0xFF, 0xFF, 0xFF    // white
        };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        test_bmp_file_2x2_32bit_bottom_up_write("test_bmp_loader_load_2x2_32bit_bottom_up.bmp");

        s_test_config_bmp_loader_padding_remove.fail_on_call = 1U;
        s_test_config_bmp_loader_padding_remove.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = bmp_loader_load("test_bmp_loader_load_2x2_32bit_bottom_up.bmp", loader);
        assert(RESOURCE_SUCCESS == ret);

        assert(0U == s_test_config_bmp_loader_padding_remove.call_count);

        assert((uint16_t)0x4D42U == loader->file_header.bf_type);
        assert(70U == loader->file_header.bf_size);
        assert(54U == loader->file_header.bf_off_bits);

        assert(40U == loader->info_header.bi_size);
        assert(2 == loader->info_header.bi_width);
        assert(2 == loader->info_header.bi_height);
        assert(1U == loader->info_header.bi_planes);
        assert(32U == loader->info_header.bi_bit_count);
        assert(0U == loader->info_header.bi_compression);
        assert(16U == loader->info_header.bi_size_image);

        assert(0U == loader->padding);
        assert(8U == loader->stride);
        assert(true == loader->padding_removed);
        assert(NULL != loader->pixels);
        assert(0 == memcmp(loader->pixels, expected_pixels, sizeof(expected_pixels)));

        bmp_loader_destroy(&loader);
        assert(NULL == loader);

        remove("test_bmp_loader_load_2x2_32bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_loader_pixel_move(void) {
    {
        // bmp_loader_pixel_move() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint8_t pixels[4] = { 1U, 2U, 3U, 4U };
        uint8_t* out_pixels = NULL;

        test_bmp_loader_config_reset();

        loader.pixels = pixels;

        s_test_config_bmp_loader_pixel_move.fail_on_call = 1U;
        s_test_config_bmp_loader_pixel_move.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = bmp_loader_pixel_move(&loader, &out_pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(pixels == loader.pixels);
        assert(NULL == out_pixels);

        test_bmp_loader_config_reset();
    }
    {
        // bmp_loader_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        uint8_t* out_pixels = NULL;

        test_bmp_loader_config_reset();

        ret = bmp_loader_pixel_move(NULL, &out_pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == out_pixels);

        test_bmp_loader_config_reset();
    }
    {
        // out_pixels_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint8_t pixels[4] = { 1U, 2U, 3U, 4U };

        test_bmp_loader_config_reset();

        loader.pixels = pixels;

        ret = bmp_loader_pixel_move(&loader, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(pixels == loader.pixels);

        test_bmp_loader_config_reset();
    }
    {
        // *out_pixels_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint8_t pixels[4] = { 1U, 2U, 3U, 4U };
        uint8_t dummy = 0U;
        uint8_t* out_pixels = &dummy;

        test_bmp_loader_config_reset();

        loader.pixels = pixels;

        ret = bmp_loader_pixel_move(&loader, &out_pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(pixels == loader.pixels);
        assert(&dummy == out_pixels);

        test_bmp_loader_config_reset();
    }
    {
        // bmp_loader_->pixels == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint8_t* out_pixels = NULL;

        test_bmp_loader_config_reset();

        loader.pixels = NULL;

        ret = bmp_loader_pixel_move(&loader, &out_pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == loader.pixels);
        assert(NULL == out_pixels);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: bmp_loader_->pixels の管理権限を out_pixels_ へ移譲する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t loader = { 0 };
        uint8_t pixels[4] = { 1U, 2U, 3U, 4U };
        uint8_t* out_pixels = NULL;

        test_bmp_loader_config_reset();

        loader.pixels = pixels;

        ret = bmp_loader_pixel_move(&loader, &out_pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL == loader.pixels);
        assert(pixels == out_pixels);
        assert(1U == out_pixels[0]);
        assert(2U == out_pixels[1]);
        assert(3U == out_pixels[2]);
        assert(4U == out_pixels[3]);

        test_bmp_loader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_loader_bmp_size_get(void) {
    {
        // bmp_loader_bmp_size_get() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);

        s_test_config_bmp_loader_bmp_size_get.fail_on_call = 1U;
        s_test_config_bmp_loader_bmp_size_get.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = bmp_loader_bmp_size_get(&loader, &width, &height, &channel_count);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // bmp_loader_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_bmp_loader_config_reset();

        ret = bmp_loader_bmp_size_get(NULL, &width, &height, &channel_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // width_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);

        ret = bmp_loader_bmp_size_get(&loader, NULL, &height, &channel_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(222U == height);
        assert(33U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // height_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint16_t width = 111U;
        uint8_t channel_count = 33U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);

        ret = bmp_loader_bmp_size_get(&loader, &width, NULL, &channel_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(33U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // channel_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint16_t width = 111U;
        uint16_t height = 222U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);

        ret = bmp_loader_bmp_size_get(&loader, &width, &height, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);

        test_bmp_loader_config_reset();
    }
    {
        // is_bmp_supported() == BMP_FILE_NOT_INITIALIZED -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);
        loader.info_header.bi_width = 0;

        ret = bmp_loader_bmp_size_get(&loader, &width, &height, &channel_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // is_bmp_supported() == BMP_FILE_UNDEFINED -> RESOURCE_UNDEFINED_ERROR
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);

        s_test_config_is_bmp_supported.fail_on_call = 1U;
        s_test_config_is_bmp_supported.forced_result = (int)BMP_FILE_UNDEFINED;

        ret = bmp_loader_bmp_size_get(&loader, &width, &height, &channel_count);
        assert(RESOURCE_UNDEFINED_ERROR == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // サポート外BMP -> RESOURCE_UNSUPPORTED_FILE
        resource_result_t ret = RESOURCE_SUCCESS;
        bmp_loader_t loader = { 0 };
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);
        loader.info_header.bi_bit_count = 16U;

        ret = bmp_loader_bmp_size_get(&loader, &width, &height, &channel_count);
        assert(RESOURCE_UNSUPPORTED_FILE == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / bottom-up BMP -> width=2, height=2, channel=3
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t loader = { 0 };
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t channel_count = 0U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);

        ret = bmp_loader_bmp_size_get(&loader, &width, &height, &channel_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(2U == width);
        assert(2U == height);
        assert(3U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / top-down BMP -> heightは絶対値で返る
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t loader = { 0 };
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t channel_count = 0U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);
        loader.info_header.bi_height = -2;

        ret = bmp_loader_bmp_size_get(&loader, &width, &height, &channel_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(2U == width);
        assert(2U == height);
        assert(3U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 2x2 / 32bit BMP -> channel=4
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t loader = { 0 };
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t channel_count = 0U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);
        loader.info_header.bi_bit_count = 32U;

        ret = bmp_loader_bmp_size_get(&loader, &width, &height, &channel_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(2U == width);
        assert(2U == height);
        assert(4U == channel_count);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: int16_t範囲上限付近のサイズ
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        bmp_loader_t loader = { 0 };
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t channel_count = 0U;

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&loader.file_header, &loader.info_header);
        loader.info_header.bi_width = INT16_MAX;
        loader.info_header.bi_height = INT16_MIN;

        ret = bmp_loader_bmp_size_get(&loader, &width, &height, &channel_count);
        assert(RESOURCE_SUCCESS == ret);
        assert((uint16_t)INT16_MAX == width);
        assert((uint16_t)32768U == height);
        assert(3U == channel_count);

        test_bmp_loader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_loader_pixel_bgr_to_rgb(void) {
    {
        // bmp_loader_pixel_bgr_to_rgb() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            // BGR
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        const uint8_t expected_pixels[12] = {
            // unchanged
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        s_test_config_bmp_loader_pixel_bgr_to_rgb.fail_on_call = 1U;
        s_test_config_bmp_loader_pixel_bgr_to_rgb.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = bmp_loader_pixel_bgr_to_rgb(&info_header, pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // info_header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        uint8_t pixels[12] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        const uint8_t expected_pixels[12] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        test_bmp_loader_config_reset();

        ret = bmp_loader_pixel_bgr_to_rgb(NULL, pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // pixels_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        ret = bmp_loader_pixel_bgr_to_rgb(&info_header, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_bmp_loader_config_reset();
    }
    {
        // info_header_->bi_width == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        const uint8_t expected_pixels[12] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_width = 0;

        ret = bmp_loader_pixel_bgr_to_rgb(&info_header, pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // info_header_->bi_height == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        const uint8_t expected_pixels[12] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = 0;

        ret = bmp_loader_pixel_bgr_to_rgb(&info_header, pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // サポート外bit count -> RESOURCE_UNSUPPORTED_FILE
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        const uint8_t expected_pixels[12] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_bit_count = 16U;

        ret = bmp_loader_pixel_bgr_to_rgb(&info_header, pixels);
        assert(RESOURCE_UNSUPPORTED_FILE == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / bottom-up相当のBGR配列をRGBへ変換する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            // BGR
            0xFF, 0x00, 0x00,   // blue
            0xFF, 0xFF, 0xFF,   // white

            0x00, 0x00, 0xFF,   // red
            0x00, 0xFF, 0x00    // green
        };

        const uint8_t expected_pixels[12] = {
            // RGB
            0x00, 0x00, 0xFF,   // blue
            0xFF, 0xFF, 0xFF,   // white

            0xFF, 0x00, 0x00,   // red
            0x00, 0xFF, 0x00    // green
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        ret = bmp_loader_pixel_bgr_to_rgb(&info_header, pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / top-down相当でも、heightの絶対値で全ピクセルをRGBへ変換する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            // BGR
            0x00, 0x00, 0xFF,   // red
            0x00, 0xFF, 0x00,   // green

            0xFF, 0x00, 0x00,   // blue
            0xFF, 0xFF, 0xFF    // white
        };

        const uint8_t expected_pixels[12] = {
            // RGB
            0xFF, 0x00, 0x00,   // red
            0x00, 0xFF, 0x00,   // green

            0x00, 0x00, 0xFF,   // blue
            0xFF, 0xFF, 0xFF    // white
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = -2;

        ret = bmp_loader_pixel_bgr_to_rgb(&info_header, pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 2x2 / 32bit / BGRA配列をRGBAへ変換する
        // alpha値は変更されない
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[16] = {
            // BGRA
            0xFF, 0x00, 0x00, 0x11,   // blue
            0xFF, 0xFF, 0xFF, 0x22,   // white

            0x00, 0x00, 0xFF, 0x33,   // red
            0x00, 0xFF, 0x00, 0x44    // green
        };

        const uint8_t expected_pixels[16] = {
            // RGBA
            0x00, 0x00, 0xFF, 0x11,   // blue
            0xFF, 0xFF, 0xFF, 0x22,   // white

            0xFF, 0x00, 0x00, 0x33,   // red
            0x00, 0xFF, 0x00, 0x44    // green
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_bit_count = 32U;
        info_header.bi_size_image = 16U;

        ret = bmp_loader_pixel_bgr_to_rgb(&info_header, pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_loader_pixel_flip(void) {
    {
        // bmp_loader_pixel_flip() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            // bottom row
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            // top row
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        const uint8_t expected_pixels[12] = {
            // unchanged
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        s_test_config_bmp_loader_pixel_flip.fail_on_call = 1U;
        s_test_config_bmp_loader_pixel_flip.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = bmp_loader_pixel_flip(&info_header, pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // info_header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        uint8_t pixels[12] = {
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        const uint8_t expected_pixels[12] = {
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        test_bmp_loader_config_reset();

        ret = bmp_loader_pixel_flip(NULL, pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // pixels_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        ret = bmp_loader_pixel_flip(&info_header, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_bmp_loader_config_reset();
    }
    {
        // info_header_->bi_width == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        const uint8_t expected_pixels[12] = {
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_width = 0;

        ret = bmp_loader_pixel_flip(&info_header, pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // info_header_->bi_height == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        const uint8_t expected_pixels[12] = {
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = 0;

        ret = bmp_loader_pixel_flip(&info_header, pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // サポート外bit count -> RESOURCE_UNSUPPORTED_FILE
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        const uint8_t expected_pixels[12] = {
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_bit_count = 16U;

        ret = bmp_loader_pixel_flip(&info_header, pixels);
        assert(RESOURCE_UNSUPPORTED_FILE == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // top-down BMP(height < 0) -> flip不要なので何もしない
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            // top row
            0xFF, 0x00, 0x00,
            0x00, 0xFF, 0x00,

            // bottom row
            0x00, 0x00, 0xFF,
            0xFF, 0xFF, 0xFF
        };

        const uint8_t expected_pixels[12] = {
            // unchanged
            0xFF, 0x00, 0x00,
            0x00, 0xFF, 0x00,

            0x00, 0x00, 0xFF,
            0xFF, 0xFF, 0xFF
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = -2;

        ret = bmp_loader_pixel_flip(&info_header, pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // height == 1 -> flip不要なので何もしない
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[6] = {
            0xFF, 0x00, 0x00,
            0x00, 0xFF, 0x00
        };

        const uint8_t expected_pixels[6] = {
            0xFF, 0x00, 0x00,
            0x00, 0xFF, 0x00
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = 1;

        ret = bmp_loader_pixel_flip(&info_header, pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / bottom-up BMPを上下反転する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[12] = {
            // bottom row
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            // top row
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        const uint8_t expected_pixels[12] = {
            // top row
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            // bottom row
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        ret = bmp_loader_pixel_flip(&info_header, pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 3x3 / 24bit / bottom-up BMPを上下反転する
        // 中央行はそのまま、先頭行と末尾行だけが入れ替わる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[27] = {
            // row 0
            0x00, 0x00, 0x01,
            0x00, 0x00, 0x02,
            0x00, 0x00, 0x03,

            // row 1
            0x00, 0x00, 0x04,
            0x00, 0x00, 0x05,
            0x00, 0x00, 0x06,

            // row 2
            0x00, 0x00, 0x07,
            0x00, 0x00, 0x08,
            0x00, 0x00, 0x09
        };

        const uint8_t expected_pixels[27] = {
            // row 2
            0x00, 0x00, 0x07,
            0x00, 0x00, 0x08,
            0x00, 0x00, 0x09,

            // row 1
            0x00, 0x00, 0x04,
            0x00, 0x00, 0x05,
            0x00, 0x00, 0x06,

            // row 0
            0x00, 0x00, 0x01,
            0x00, 0x00, 0x02,
            0x00, 0x00, 0x03
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_width = 3;
        info_header.bi_height = 3;
        info_header.bi_bit_count = 24U;

        ret = bmp_loader_pixel_flip(&info_header, pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 2x2 / 32bit / bottom-up BMPを上下反転する
        // alpha値も含めて行単位で入れ替わる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        uint8_t pixels[16] = {
            // bottom row
            0x00, 0x00, 0xFF, 0x11,
            0x00, 0xFF, 0x00, 0x22,

            // top row
            0xFF, 0x00, 0x00, 0x33,
            0xFF, 0xFF, 0xFF, 0x44
        };

        const uint8_t expected_pixels[16] = {
            // top row
            0xFF, 0x00, 0x00, 0x33,
            0xFF, 0xFF, 0xFF, 0x44,

            // bottom row
            0x00, 0x00, 0xFF, 0x11,
            0x00, 0xFF, 0x00, 0x22
        };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_bit_count = 32U;
        info_header.bi_size_image = 16U;

        ret = bmp_loader_pixel_flip(&info_header, pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        test_bmp_loader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_loader_padding_remove(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // bmp_loader_padding_remove() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;

        const uint8_t src_pixels[16] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,
            0x00, 0x00,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,
            0x00, 0x00
        };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        s_test_config_bmp_loader_padding_remove.fail_on_call = 1U;
        s_test_config_bmp_loader_padding_remove.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // info_header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;
        const uint8_t src_pixels[16] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        ret = bmp_loader_padding_remove(NULL, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // src_pixels_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, NULL, &dst_pixels, &out_new_size);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // dst_pixels_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        size_t out_new_size = 123U;
        const uint8_t src_pixels[16] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, NULL, &out_new_size);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *dst_pixels_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t dummy = 0U;
        uint8_t* dst_pixels = &dummy;
        size_t out_new_size = 123U;
        const uint8_t src_pixels[16] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_new_size_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        const uint8_t src_pixels[16] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == dst_pixels);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stride_ == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;
        const uint8_t src_pixels[16] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        ret = bmp_loader_padding_remove(&info_header, 0U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // padding_ == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;
        const uint8_t src_pixels[16] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        ret = bmp_loader_padding_remove(&info_header, 8U, 0U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // info_header_->bi_width == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;
        const uint8_t src_pixels[16] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);
        info_header.bi_width = 0;

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // info_header_->bi_height == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;
        const uint8_t src_pixels[16] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);
        info_header.bi_height = 0;

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // info_header_->bi_bit_count != 24 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;
        const uint8_t src_pixels[16] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);
        info_header.bi_bit_count = 32U;

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // width * height が size_t を超過 -> RESOURCE_OVERFLOW
        // 防御的分岐の確認。通常は is_bmp_supported() 済みのinfo_headerでは到達しない。
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;
        const uint8_t src_pixels[1] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);
        info_header.bi_width = -1;
        info_header.bi_height = 2;
        info_header.bi_bit_count = 24U;

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_OVERFLOW == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // new_size が UINT32_MAX を超過 -> RESOURCE_OVERFLOW
        // 防御的分岐の確認。通常は is_bmp_supported() 済みのinfo_headerでは到達しない。
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;
        const uint8_t src_pixels[1] = { 0 };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);
        info_header.bi_width = INT32_MAX;
        info_header.bi_height = 1;
        info_header.bi_bit_count = 24U;

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_OVERFLOW == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // new_pixel用 memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 123U;
        test_call_control_t config = { 0 };

        const uint8_t src_pixels[16] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,
            0x00, 0x00,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,
            0x00, 0x00
        };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == dst_pixels);
        assert(123U == out_new_size);

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / bottom-up / paddingあり
        // padding 2byte/row を除去し、生のBGRピクセル順序は維持する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 0U;

        const uint8_t src_pixels[16] = {
            // bottom row
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,
            0x00, 0x00,

            // top row
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,
            0x00, 0x00
        };

        const uint8_t expected_pixels[12] = {
            // bottom row
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,

            // top row
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00
        };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != dst_pixels);
        assert(12U == out_new_size);
        assert(0 == memcmp(dst_pixels, expected_pixels, sizeof(expected_pixels)));

        memory_system_free(dst_pixels, out_new_size, MEMORY_TAG_TEXTURE);
        dst_pixels = NULL;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / top-down / paddingあり
        // padding除去のみを行い、行順は変更しない
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        info_header_t info_header = { 0 };
        uint8_t* dst_pixels = NULL;
        size_t out_new_size = 0U;

        const uint8_t src_pixels[16] = {
            // top row
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,
            0x00, 0x00,

            // bottom row
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,
            0x00, 0x00
        };

        const uint8_t expected_pixels[12] = {
            // top row
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,

            // bottom row
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF
        };

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);
        info_header.bi_height = -2;

        ret = bmp_loader_padding_remove(&info_header, 8U, 2U, src_pixels, &dst_pixels, &out_new_size);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != dst_pixels);
        assert(12U == out_new_size);
        assert(0 == memcmp(dst_pixels, expected_pixels, sizeof(expected_pixels)));

        memory_system_free(dst_pixels, out_new_size, MEMORY_TAG_TEXTURE);
        dst_pixels = NULL;

        test_bmp_loader_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_header_load(void) {
    memory_system_create();

    {
        // header_load() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        file_header.bf_type = 1;
        info_header.bi_size = 2;

        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_NO_MEMORY;
        s_test_config_header_load.fail_on_call = config.fail_on_call;
        s_test_config_header_load.forced_result = config.forced_result;

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", &file_header, &info_header);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(1 == file_header.bf_type);
        assert(2 == info_header.bi_size);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // fullpath_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // file_header_ / info_header_ は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        file_header.bf_type = 1;
        info_header.bi_size = 2;

        ret = header_load(NULL, &file_header, &info_header);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(1 == file_header.bf_type);
        assert(2 == info_header.bi_size);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // file_header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", NULL, &info_header);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // info_header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", &file_header, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // filesystem_create() 失敗 -> 変換されたResourceエラーを返す
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_NO_MEMORY;
        test_filesystem_create_config_set(&config);

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", &file_header, &info_header);
        assert(RESOURCE_NO_MEMORY == ret);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // filesystem_open() 失敗 -> RESOURCE_FILE_OPEN_ERROR
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_FILE_OPEN_ERROR;
        test_filesystem_open_config_set(&config);

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", &file_header, &info_header);
        assert(RESOURCE_FILE_OPEN_ERROR == ret);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // filesystem_byte_read() 失敗 -> RESOURCE_FILE_READ_ERROR
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_EOF;
        test_filesystem_byte_read_config_set(&config);

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", &file_header, &info_header);
        assert(RESOURCE_FILE_READ_ERROR == ret);

        remove("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // 読み込みサイズが54byte未満 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        const uint8_t short_bmp_header[10] = {
            0x42, 0x4D,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00,
            0x00, 0x00
        };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        test_bmp_file_write("test_bmp_loader_short_header.bmp", short_bmp_header, sizeof(short_bmp_header));

        ret = header_load("test_bmp_loader_short_header.bmp", &file_header, &info_header);
        assert(RESOURCE_DATA_CORRUPTED == ret);

        remove("test_bmp_loader_short_header.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // file_header_parse() 失敗 -> 注入結果が返る
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        s_test_config_file_header_parse.fail_on_call = 1U;
        s_test_config_file_header_parse.forced_result = (int)RESOURCE_DATA_CORRUPTED;

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", &file_header, &info_header);
        assert(RESOURCE_DATA_CORRUPTED == ret);

        remove("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // info_header_parse() 失敗 -> 注入結果が返る
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        s_test_config_info_header_parse.fail_on_call = 1U;
        s_test_config_info_header_parse.forced_result = (int)RESOURCE_DATA_CORRUPTED;

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", &file_header, &info_header);
        assert(RESOURCE_DATA_CORRUPTED == ret);

        remove("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // filesystem_close() 失敗 -> RESOURCE_FILE_CLOSE_ERROR
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_FILE_CLOSE_ERROR;
        test_filesystem_close_config_set(&config);

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", &file_header, &info_header);
        assert(RESOURCE_FILE_CLOSE_ERROR == ret);

        remove("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / bottom-up BMPのヘッダを読み込む
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        ret = header_load("test_bmp_loader_header_2x2_24bit_bottom_up.bmp", &file_header, &info_header);
        assert(RESOURCE_SUCCESS == ret);

        assert((uint16_t)0x4D42U == file_header.bf_type);
        assert(0U == file_header.bf_reserved1);
        assert(0U == file_header.bf_reserved2);
        assert(70U == file_header.bf_size);
        assert(54U == file_header.bf_off_bits);

        assert(40U == info_header.bi_size);
        assert(2 == info_header.bi_width);
        assert(2 == info_header.bi_height);
        assert(1U == info_header.bi_planes);
        assert(24U == info_header.bi_bit_count);
        assert(0U == info_header.bi_compression);
        assert(16U == info_header.bi_size_image);
        assert(0 == info_header.bi_x_pels_per_meter);
        assert(0 == info_header.bi_y_pels_per_meter);
        assert(0U == info_header.bi_clr_used);
        assert(0U == info_header.bi_clr_important);

        remove("test_bmp_loader_header_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_pixel_load(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // pixel_load() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        s_test_config_pixel_load.fail_on_call = 1U;
        s_test_config_pixel_load.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == pixels);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // fullpath_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        ret = pixel_load(NULL, &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == pixels);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // file_header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", NULL, &info_header, 8U, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == pixels);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // info_header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        uint8_t* pixels = NULL;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, NULL, 8U, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == pixels);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_pixels_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *out_pixels_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t dummy = 0;
        uint8_t* pixels = &dummy;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy == pixels);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stride_ == 0 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 0U, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == pixels);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // tmp_buffer用 memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;
        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == pixels);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // filesystem_create() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;
        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_NO_MEMORY;
        test_filesystem_create_config_set(&config);

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == pixels);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // filesystem_open() 失敗 -> RESOURCE_FILE_OPEN_ERROR
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;
        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_FILE_OPEN_ERROR;
        test_filesystem_open_config_set(&config);

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_FILE_OPEN_ERROR == ret);
        assert(NULL == pixels);

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // filesystem_byte_read() 失敗 -> RESOURCE_FILE_READ_ERROR
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;
        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        config.fail_on_call = 1U;
        config.forced_result = (int)FILESYSTEM_EOF;
        test_filesystem_byte_read_config_set(&config);

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_FILE_READ_ERROR == ret);
        assert(NULL == pixels);

        remove("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 読み込みサイズがbf_size未満 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;
        const uint8_t short_file[54] = { 0 };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        test_bmp_file_write("test_bmp_loader_pixel_short_file.bmp", short_file, sizeof(short_file));

        ret = pixel_load("test_bmp_loader_pixel_short_file.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == pixels);

        remove("test_bmp_loader_pixel_short_file.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stride_ * height が size_t を超過 -> RESOURCE_OVERFLOW
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, SIZE_MAX, &pixels);
        assert(RESOURCE_OVERFLOW == ret);
        assert(NULL == pixels);

        remove("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // pixel_buffer_size が uint32_t 範囲を超過 -> RESOURCE_OVERFLOW
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = INT32_MAX;

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 3U, &pixels);
        assert(RESOURCE_OVERFLOW == ret);
        assert(NULL == pixels);

        remove("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // ピクセルデータ範囲がbf_sizeを超える -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        file_header.bf_off_bits = 60U;

        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == pixels);

        remove("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // tmp_pixels用 memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        // memory_system_allocate() 呼び出し順:
        // 1回目: tmp_buffer
        // 2回目: filesystem_create() 内部
        // 3回目: tmp_pixels
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;
        test_call_control_t config = {0};

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        config.fail_on_call = 3U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == pixels);

        remove("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / bottom-up BMPのピクセルデータを読み込む
        // pixel_load() はpadding除去やBGR->RGB変換を行わず、ファイル上の生ピクセルデータを読み込む
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;

        const uint8_t expected_pixels[16] = {
            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,
            0x00, 0x00,

            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,
            0x00, 0x00
        };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        test_bmp_file_2x2_24bit_bottom_up_write("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != pixels);
        assert(16U == info_header.bi_size_image);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        memory_system_free(pixels, info_header.bi_size_image, MEMORY_TAG_TEXTURE);
        pixels = NULL;

        remove("test_bmp_loader_pixel_2x2_24bit_bottom_up.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2x2 / 24bit / top-down BMPでも、負のheightを絶対値として扱って読み込む
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };
        uint8_t* pixels = NULL;

        const uint8_t expected_pixels[16] = {
            0x00, 0x00, 0xFF,
            0x00, 0xFF, 0x00,
            0x00, 0x00,

            0xFF, 0x00, 0x00,
            0xFF, 0xFF, 0xFF,
            0x00, 0x00
        };

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = -2;

        test_bmp_file_2x2_24bit_top_down_write("test_bmp_loader_pixel_2x2_24bit_top_down.bmp");

        ret = pixel_load("test_bmp_loader_pixel_2x2_24bit_top_down.bmp", &file_header, &info_header, 8U, &pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != pixels);
        assert(16U == info_header.bi_size_image);
        assert(0 == memcmp(pixels, expected_pixels, sizeof(expected_pixels)));

        memory_system_free(pixels, info_header.bi_size_image, MEMORY_TAG_TEXTURE);
        pixels = NULL;

        remove("test_bmp_loader_pixel_2x2_24bit_top_down.bmp");

        test_bmp_loader_config_reset();
        test_filesystem_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_file_header_parse(void) {
    {
        // file_header_parse() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        char header[54] = { 0 };
        file_header_t file_header = { 0 };

        test_bmp_loader_config_reset();

        file_header.bf_type = 1;
        file_header.bf_size = 2;
        file_header.bf_reserved1 = 3;
        file_header.bf_reserved2 = 4;
        file_header.bf_off_bits = 5;

        s_test_config_file_header_parse.fail_on_call = 1U;
        s_test_config_file_header_parse.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = file_header_parse(header, &file_header);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(1 == file_header.bf_type);
        assert(2 == file_header.bf_size);
        assert(3 == file_header.bf_reserved1);
        assert(4 == file_header.bf_reserved2);
        assert(5 == file_header.bf_off_bits);

        test_bmp_loader_config_reset();
    }
    {
        // header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // file_header_ は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        file_header_t file_header = { 0 };

        test_bmp_loader_config_reset();

        file_header.bf_type = 1;
        file_header.bf_size = 2;
        file_header.bf_reserved1 = 3;
        file_header.bf_reserved2 = 4;
        file_header.bf_off_bits = 5;

        ret = file_header_parse(NULL, &file_header);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(1 == file_header.bf_type);
        assert(2 == file_header.bf_size);
        assert(3 == file_header.bf_reserved1);
        assert(4 == file_header.bf_reserved2);
        assert(5 == file_header.bf_off_bits);

        test_bmp_loader_config_reset();
    }
    {
        // file_header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        char header[54] = { 0 };

        test_bmp_loader_config_reset();

        ret = file_header_parse(header, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 各offsetからfile headerフィールドが正しくparseされる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        char header[54] = { 0 };
        file_header_t file_header = { 0 };

        test_bmp_loader_config_reset();

        // bf_type = 0x4D42("BM"), offset = 0
        header[0] = 0x42;
        header[1] = 0x4D;

        // bf_size = 0x11223344, offset = 2
        header[2] = 0x44;
        header[3] = 0x33;
        header[4] = 0x22;
        header[5] = 0x11;

        // bf_reserved1 = 0x5566, offset = 6
        header[6] = 0x66;
        header[7] = 0x55;

        // bf_reserved2 = 0x7788, offset = 8
        header[8] = (char)0x88;
        header[9] = 0x77;

        // bf_off_bits = 0x99AABBCC, offset = 10
        header[10] = (char)0xCC;
        header[11] = (char)0xBB;
        header[12] = (char)0xAA;
        header[13] = (char)0x99;

        ret = file_header_parse(header, &file_header);
        assert(RESOURCE_SUCCESS == ret);

        assert((uint16_t)0x4D42U == file_header.bf_type);
        assert((uint32_t)0x11223344U == file_header.bf_size);
        assert((uint16_t)0x5566U == file_header.bf_reserved1);
        assert((uint16_t)0x7788U == file_header.bf_reserved2);
        assert((uint32_t)0x99AABBCCU == file_header.bf_off_bits);

        test_bmp_loader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_info_header_parse(void) {
    {
        // info_header_parse() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        char header[54] = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        info_header.bi_size = 1;
        info_header.bi_width = 2;
        info_header.bi_height = 3;
        info_header.bi_planes = 4;
        info_header.bi_bit_count = 5;
        info_header.bi_compression = 6;
        info_header.bi_size_image = 7;
        info_header.bi_x_pels_per_meter = 8;
        info_header.bi_y_pels_per_meter = 9;
        info_header.bi_clr_used = 10;
        info_header.bi_clr_important = 11;

        s_test_config_info_header_parse.fail_on_call = 1U;
        s_test_config_info_header_parse.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = info_header_parse(header, &info_header);
        assert(RESOURCE_NO_MEMORY == ret);

        assert(1 == info_header.bi_size);
        assert(2 == info_header.bi_width);
        assert(3 == info_header.bi_height);
        assert(4 == info_header.bi_planes);
        assert(5 == info_header.bi_bit_count);
        assert(6 == info_header.bi_compression);
        assert(7 == info_header.bi_size_image);
        assert(8 == info_header.bi_x_pels_per_meter);
        assert(9 == info_header.bi_y_pels_per_meter);
        assert(10 == info_header.bi_clr_used);
        assert(11 == info_header.bi_clr_important);

        test_bmp_loader_config_reset();
    }
    {
        // header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // info_header_ は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        info_header.bi_size = 1;
        info_header.bi_width = 2;
        info_header.bi_height = 3;
        info_header.bi_planes = 4;
        info_header.bi_bit_count = 5;
        info_header.bi_compression = 6;
        info_header.bi_size_image = 7;
        info_header.bi_x_pels_per_meter = 8;
        info_header.bi_y_pels_per_meter = 9;
        info_header.bi_clr_used = 10;
        info_header.bi_clr_important = 11;

        ret = info_header_parse(NULL, &info_header);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(1 == info_header.bi_size);
        assert(2 == info_header.bi_width);
        assert(3 == info_header.bi_height);
        assert(4 == info_header.bi_planes);
        assert(5 == info_header.bi_bit_count);
        assert(6 == info_header.bi_compression);
        assert(7 == info_header.bi_size_image);
        assert(8 == info_header.bi_x_pels_per_meter);
        assert(9 == info_header.bi_y_pels_per_meter);
        assert(10 == info_header.bi_clr_used);
        assert(11 == info_header.bi_clr_important);

        test_bmp_loader_config_reset();
    }
    {
        // info_header_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        char header[54] = { 0 };

        test_bmp_loader_config_reset();

        ret = info_header_parse(header, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 各offsetからinfo headerフィールドが正しくparseされる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        char header[54] = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        // bi_size = 40, offset = 14
        header[14] = 0x28;
        header[15] = 0x00;
        header[16] = 0x00;
        header[17] = 0x00;

        // bi_width = 0x12345678, offset = 18
        header[18] = 0x78;
        header[19] = 0x56;
        header[20] = 0x34;
        header[21] = 0x12;

        // bi_height = 0x01020304, offset = 22
        header[22] = 0x04;
        header[23] = 0x03;
        header[24] = 0x02;
        header[25] = 0x01;

        // bi_planes = 1, offset = 26
        header[26] = 0x01;
        header[27] = 0x00;

        // bi_bit_count = 24, offset = 28
        header[28] = 0x18;
        header[29] = 0x00;

        // bi_compression = 0, offset = 30
        header[30] = 0x00;
        header[31] = 0x00;
        header[32] = 0x00;
        header[33] = 0x00;

        // bi_size_image = 0x11223344, offset = 34
        header[34] = 0x44;
        header[35] = 0x33;
        header[36] = 0x22;
        header[37] = 0x11;

        // bi_x_pels_per_meter = 3780, offset = 38
        header[38] = (char)0xC4;
        header[39] = 0x0E;
        header[40] = 0x00;
        header[41] = 0x00;

        // bi_y_pels_per_meter = 3790, offset = 42
        header[42] = (char)0xCE;
        header[43] = 0x0E;
        header[44] = 0x00;
        header[45] = 0x00;

        // bi_clr_used = 0x55667788, offset = 46
        header[46] = (char)0x88;
        header[47] = 0x77;
        header[48] = 0x66;
        header[49] = 0x55;

        // bi_clr_important = 0x99AABBCC, offset = 50
        header[50] = (char)0xCC;
        header[51] = (char)0xBB;
        header[52] = (char)0xAA;
        header[53] = (char)0x99;

        ret = info_header_parse(header, &info_header);
        assert(RESOURCE_SUCCESS == ret);

        assert(40U == info_header.bi_size);
        assert((int32_t)0x12345678 == info_header.bi_width);
        assert((int32_t)0x01020304 == info_header.bi_height);
        assert(1U == info_header.bi_planes);
        assert(24U == info_header.bi_bit_count);
        assert(0U == info_header.bi_compression);
        assert((uint32_t)0x11223344U == info_header.bi_size_image);
        assert(3780 == info_header.bi_x_pels_per_meter);
        assert(3790 == info_header.bi_y_pels_per_meter);
        assert((uint32_t)0x55667788U == info_header.bi_clr_used);
        assert((uint32_t)0x99AABBCCU == info_header.bi_clr_important);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: bi_heightが負値でも正しくparseされる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        char header[54] = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        // bi_size = 40
        header[14] = 0x28;
        header[15] = 0x00;
        header[16] = 0x00;
        header[17] = 0x00;

        // bi_width = 2
        header[18] = 0x02;
        header[19] = 0x00;
        header[20] = 0x00;
        header[21] = 0x00;

        // bi_height = -2 = 0xFFFFFFFE
        header[22] = (char)0xFE;
        header[23] = (char)0xFF;
        header[24] = (char)0xFF;
        header[25] = (char)0xFF;

        // bi_planes = 1
        header[26] = 0x01;
        header[27] = 0x00;

        // bi_bit_count = 32
        header[28] = 0x20;
        header[29] = 0x00;

        ret = info_header_parse(header, &info_header);
        assert(RESOURCE_SUCCESS == ret);

        assert(40U == info_header.bi_size);
        assert(2 == info_header.bi_width);
        assert(-2 == info_header.bi_height);
        assert(1U == info_header.bi_planes);
        assert(32U == info_header.bi_bit_count);

        test_bmp_loader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_file_header_copy(void) {
    {
        // src_ == NULL -> dst_ は変更されない
        file_header_t dst = { 0 };

        dst.bf_type = 1;
        dst.bf_reserved1 = 2;
        dst.bf_reserved2 = 3;
        dst.bf_size = 4;
        dst.bf_off_bits = 5;

        file_header_copy(NULL, &dst);

        assert(1 == dst.bf_type);
        assert(2 == dst.bf_reserved1);
        assert(3 == dst.bf_reserved2);
        assert(4 == dst.bf_size);
        assert(5 == dst.bf_off_bits);
    }
    {
        // dst_ == NULL -> no-op
        file_header_t src = { 0 };

        src.bf_type = 0x4D42;
        src.bf_reserved1 = 0;
        src.bf_reserved2 = 0;
        src.bf_size = 70;
        src.bf_off_bits = 54;

        file_header_copy(&src, NULL);

        assert(0x4D42 == src.bf_type);
        assert(0 == src.bf_reserved1);
        assert(0 == src.bf_reserved2);
        assert(70 == src.bf_size);
        assert(54 == src.bf_off_bits);
    }
    {
        // src_ == NULL && dst_ == NULL -> no-op
        file_header_copy(NULL, NULL);
    }
    {
        // 正常系: 全メンバがコピーされる
        file_header_t src = { 0 };
        file_header_t dst = { 0 };

        src.bf_type = 0x4D42;
        src.bf_reserved1 = 11;
        src.bf_reserved2 = 22;
        src.bf_size = 123456;
        src.bf_off_bits = 54;

        dst.bf_type = 1;
        dst.bf_reserved1 = 2;
        dst.bf_reserved2 = 3;
        dst.bf_size = 4;
        dst.bf_off_bits = 5;

        file_header_copy(&src, &dst);

        assert(src.bf_type == dst.bf_type);
        assert(src.bf_reserved1 == dst.bf_reserved1);
        assert(src.bf_reserved2 == dst.bf_reserved2);
        assert(src.bf_size == dst.bf_size);
        assert(src.bf_off_bits == dst.bf_off_bits);
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_info_header_copy(void) {
    {
        // src_ == NULL -> dst_ は変更されない
        info_header_t dst = { 0 };

        dst.bi_bit_count = 1;
        dst.bi_clr_important = 2;
        dst.bi_clr_used = 3;
        dst.bi_compression = 4;
        dst.bi_height = 5;
        dst.bi_planes = 6;
        dst.bi_size = 7;
        dst.bi_size_image = 8;
        dst.bi_width = 9;
        dst.bi_x_pels_per_meter = 10;
        dst.bi_y_pels_per_meter = 11;

        info_header_copy(NULL, &dst);

        assert(1 == dst.bi_bit_count);
        assert(2 == dst.bi_clr_important);
        assert(3 == dst.bi_clr_used);
        assert(4 == dst.bi_compression);
        assert(5 == dst.bi_height);
        assert(6 == dst.bi_planes);
        assert(7 == dst.bi_size);
        assert(8 == dst.bi_size_image);
        assert(9 == dst.bi_width);
        assert(10 == dst.bi_x_pels_per_meter);
        assert(11 == dst.bi_y_pels_per_meter);
    }
    {
        // dst_ == NULL -> no-op
        info_header_t src = { 0 };

        src.bi_bit_count = 24;
        src.bi_clr_important = 0;
        src.bi_clr_used = 0;
        src.bi_compression = 0;
        src.bi_height = -2;
        src.bi_planes = 1;
        src.bi_size = 40;
        src.bi_size_image = 16;
        src.bi_width = 2;
        src.bi_x_pels_per_meter = 3780;
        src.bi_y_pels_per_meter = 3780;

        info_header_copy(&src, NULL);

        assert(24 == src.bi_bit_count);
        assert(0 == src.bi_clr_important);
        assert(0 == src.bi_clr_used);
        assert(0 == src.bi_compression);
        assert(-2 == src.bi_height);
        assert(1 == src.bi_planes);
        assert(40 == src.bi_size);
        assert(16 == src.bi_size_image);
        assert(2 == src.bi_width);
        assert(3780 == src.bi_x_pels_per_meter);
        assert(3780 == src.bi_y_pels_per_meter);
    }
    {
        // src_ == NULL && dst_ == NULL -> no-op
        info_header_copy(NULL, NULL);
    }
    {
        // 正常系: 全メンバがコピーされる
        info_header_t src = { 0 };
        info_header_t dst = { 0 };

        src.bi_bit_count = 32;
        src.bi_clr_important = 12;
        src.bi_clr_used = 34;
        src.bi_compression = 0;
        src.bi_height = -123;
        src.bi_planes = 1;
        src.bi_size = 40;
        src.bi_size_image = 4096;
        src.bi_width = 456;
        src.bi_x_pels_per_meter = 3780;
        src.bi_y_pels_per_meter = 3790;

        dst.bi_bit_count = 1;
        dst.bi_clr_important = 2;
        dst.bi_clr_used = 3;
        dst.bi_compression = 4;
        dst.bi_height = 5;
        dst.bi_planes = 6;
        dst.bi_size = 7;
        dst.bi_size_image = 8;
        dst.bi_width = 9;
        dst.bi_x_pels_per_meter = 10;
        dst.bi_y_pels_per_meter = 11;

        info_header_copy(&src, &dst);

        assert(src.bi_bit_count == dst.bi_bit_count);
        assert(src.bi_clr_important == dst.bi_clr_important);
        assert(src.bi_clr_used == dst.bi_clr_used);
        assert(src.bi_compression == dst.bi_compression);
        assert(src.bi_height == dst.bi_height);
        assert(src.bi_planes == dst.bi_planes);
        assert(src.bi_size == dst.bi_size);
        assert(src.bi_size_image == dst.bi_size_image);
        assert(src.bi_width == dst.bi_width);
        assert(src.bi_x_pels_per_meter == dst.bi_x_pels_per_meter);
        assert(src.bi_y_pels_per_meter == dst.bi_y_pels_per_meter);
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_is_bmp_supported(void) {
    {
        // is_bmp_supported() 冒頭で強制的に BMP_FILE_INVALID_WIDTH を返させる
        bmp_invalid_reason_t ret = BMP_FILE_UNDEFINED;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        s_test_config_is_bmp_supported.fail_on_call = 1U;
        s_test_config_is_bmp_supported.forced_result = (int)BMP_FILE_INVALID_WIDTH;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_WIDTH == ret);

        test_bmp_loader_config_reset();
    }
    {
        // file_header_ == NULL -> BMP_FILE_UNDEFINED
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&(file_header_t){0}, &info_header);

        ret = is_bmp_supported(NULL, &info_header);
        assert(BMP_FILE_UNDEFINED == ret);

        test_bmp_loader_config_reset();
    }
    {
        // info_header_ == NULL -> BMP_FILE_UNDEFINED
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &(info_header_t){0});

        ret = is_bmp_supported(&file_header, NULL);
        assert(BMP_FILE_UNDEFINED == ret);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 24bit BMP -> BMP_FILE_VALID
        bmp_invalid_reason_t ret = BMP_FILE_UNDEFINED;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_VALID == ret);

        test_bmp_loader_config_reset();
    }
    {
        // 正常系: 32bit BMP -> BMP_FILE_VALID
        bmp_invalid_reason_t ret = BMP_FILE_UNDEFINED;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_bit_count = 32;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_VALID == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bf_type異常 -> BMP_FILE_INVALID_BF_TYPE
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        file_header.bf_type = 0;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_BF_TYPE == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bf_reserved1異常 -> BMP_FILE_INVALID_BF_RESERVED
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        file_header.bf_reserved1 = 1;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_BF_RESERVED == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bf_reserved2異常 -> BMP_FILE_INVALID_BF_RESERVED
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        file_header.bf_reserved2 = 1;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_BF_RESERVED == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bf_size <= 54 -> BMP_FILE_INVALID_BF_SIZE
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        file_header.bf_size = 54;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_BF_SIZE == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bf_off_bits < 54 -> BMP_FILE_INVALID_BF_OFF_BITS
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        file_header.bf_off_bits = 53;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_BF_OFF_BITS == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bf_size <= bf_off_bits -> BMP_FILE_INVALID_BF_OFF_BITS
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        file_header.bf_size = 70;
        file_header.bf_off_bits = 70;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_BF_OFF_BITS == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_size異常 -> BMP_FILE_INVALID_BI_SIZE
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_size = 108;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_BI_SIZE == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_planes異常 -> BMP_FILE_INVALID_BI_PLANES
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_planes = 2;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_BI_PLANES == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_compression異常 -> BMP_FILE_INVALID_COMPRESSION
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_compression = 1;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_COMPRESSION == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_bit_count == 0 -> BMP_FILE_NOT_INITIALIZED
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_bit_count = 0;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_NOT_INITIALIZED == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_height == 0 -> BMP_FILE_NOT_INITIALIZED
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = 0;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_NOT_INITIALIZED == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_width == 0 -> BMP_FILE_NOT_INITIALIZED
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_width = 0;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_NOT_INITIALIZED == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_height < INT16_MIN -> BMP_FILE_INVALID_HEIGHT
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = (int32_t)INT16_MIN - 1;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_HEIGHT == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_height > INT16_MAX -> BMP_FILE_INVALID_HEIGHT
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_height = (int32_t)INT16_MAX + 1;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_HEIGHT == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_width < 0 -> BMP_FILE_INVALID_WIDTH
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_width = -1;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_WIDTH == ret);

        test_bmp_loader_config_reset();
    }
    {
        // bi_width > INT16_MAX -> BMP_FILE_INVALID_WIDTH
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_width = (int32_t)INT16_MAX + 1;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_WIDTH == ret);

        test_bmp_loader_config_reset();
    }
    {
        // unsupported bi_bit_count -> BMP_FILE_INVALID_CHANNEL_COUNT
        bmp_invalid_reason_t ret = BMP_FILE_VALID;
        file_header_t file_header = { 0 };
        info_header_t info_header = { 0 };

        test_bmp_loader_config_reset();

        test_bmp_loader_valid_header_make(&file_header, &info_header);
        info_header.bi_bit_count = 16;

        ret = is_bmp_supported(&file_header, &info_header);
        assert(BMP_FILE_INVALID_CHANNEL_COUNT == ret);

        test_bmp_loader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_invalid_reason_to_str(void) {
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_VALID), "valid BMP file"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_BF_TYPE), "invalid bfType"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_BF_RESERVED), "invalid bfReserved field"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_BF_SIZE), "invalid bfSize"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_BF_OFF_BITS), "invalid bfOffBits"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_BI_SIZE), "invalid biSize"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_BI_PLANES), "invalid biPlanes"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_COMPRESSION), "invalid biCompression"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_HEIGHT), "invalid biHeight"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_WIDTH), "invalid biWidth"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_INVALID_CHANNEL_COUNT), "unsupported biBitCount"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_NOT_INITIALIZED), "not initialized"));
    assert(0 == strcmp(invalid_reason_to_str(BMP_FILE_UNDEFINED), "undefined"));

    assert(0 == strcmp(invalid_reason_to_str((bmp_invalid_reason_t)-1), "undefined"));
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_loader_valid_header_make(file_header_t* file_header_, info_header_t* info_header_) {
    assert(NULL != file_header_);
    assert(NULL != info_header_);

    file_header_->bf_type = 0x4D42;
    file_header_->bf_reserved1 = 0;
    file_header_->bf_reserved2 = 0;
    file_header_->bf_size = 70;
    file_header_->bf_off_bits = 54;

    info_header_->bi_size = 40;
    info_header_->bi_width = 2;
    info_header_->bi_height = 2;
    info_header_->bi_planes = 1;
    info_header_->bi_bit_count = 24;
    info_header_->bi_compression = 0;
    info_header_->bi_size_image = 16;
    info_header_->bi_x_pels_per_meter = 0;
    info_header_->bi_y_pels_per_meter = 0;
    info_header_->bi_clr_used = 0;
    info_header_->bi_clr_important = 0;
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_file_write(const char* filepath_, const uint8_t* data_, size_t size_) {
    FILE* fp = NULL;
    size_t written_size = 0U;
    int close_result = 0;

    assert(NULL != filepath_);
    assert(NULL != data_);
    assert(0U != size_);

    fp = fopen(filepath_, "wb");
    assert(NULL != fp);

    written_size = fwrite(data_, 1U, size_, fp);
    assert(size_ == written_size);

    close_result = fclose(fp);
    assert(0 == close_result);
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_file_2x2_24bit_bottom_up_write(const char* filepath_) {
    /*
     * 2x2 / 24bit / uncompressed / bottom-up BMPを生成する。
     *
     * 画像仕様:
     * - width  = 2
     * - height = 2
     * - bit count = 24
     * - row stride = 8 bytes
     * - 1行あたりの実ピクセルデータ = 2px * 3ch = 6 bytes
     * - padding = 2 bytes / row
     *
     * BMPファイル上のピクセル順序:
     * - bottom-up BMPなので、ファイルには下段行 -> 上段行の順で格納する
     * - 各ピクセルはBGR順で格納する
     *
     * 論理的な画像配置:
     * - top-left     = red
     * - top-right    = green
     * - bottom-left  = blue
     * - bottom-right = white
     */
    const uint8_t bmp_data[] = {
        // BITMAPFILEHEADER: 14 bytes
        0x42, 0x4D,                         // bfType = "BM"
        0x46, 0x00, 0x00, 0x00,             // bfSize = 70
        0x00, 0x00,                         // bfReserved1 = 0
        0x00, 0x00,                         // bfReserved2 = 0
        0x36, 0x00, 0x00, 0x00,             // bfOffBits = 54

        // BITMAPINFOHEADER: 40 bytes
        0x28, 0x00, 0x00, 0x00,             // biSize = 40
        0x02, 0x00, 0x00, 0x00,             // biWidth = 2
        0x02, 0x00, 0x00, 0x00,             // biHeight = 2
        0x01, 0x00,                         // biPlanes = 1
        0x18, 0x00,                         // biBitCount = 24
        0x00, 0x00, 0x00, 0x00,             // biCompression = BI_RGB
        0x10, 0x00, 0x00, 0x00,             // biSizeImage = 16
        0x00, 0x00, 0x00, 0x00,             // biXPelsPerMeter = 0
        0x00, 0x00, 0x00, 0x00,             // biYPelsPerMeter = 0
        0x00, 0x00, 0x00, 0x00,             // biClrUsed = 0
        0x00, 0x00, 0x00, 0x00,             // biClrImportant = 0

        // Pixel data: bottom row
        0xFF, 0x00, 0x00,                   // blue  : BGR
        0xFF, 0xFF, 0xFF,                   // white : BGR
        0x00, 0x00,                         // padding

        // Pixel data: top row
        0x00, 0x00, 0xFF,                   // red   : BGR
        0x00, 0xFF, 0x00,                   // green : BGR
        0x00, 0x00                          // padding
    };

    test_bmp_file_write(filepath_, bmp_data, sizeof(bmp_data));
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_file_2x2_24bit_top_down_write(const char* filepath_) {
    /*
     * 2x2 / 24bit / uncompressed / top-down BMPを生成する。
     *
     * 画像仕様:
     * - width  = 2
     * - height = -2
     * - bit count = 24
     * - row stride = 8 bytes
     * - padding = 2 bytes / row
     *
     * BMPファイル上のピクセル順序:
     * - top-down BMPなので、ファイルには上段行 -> 下段行の順で格納する
     * - 各ピクセルはBGR順で格納する
     *
     * 論理的な画像配置:
     * - top-left     = red
     * - top-right    = green
     * - bottom-left  = blue
     * - bottom-right = white
     */
    const uint8_t bmp_data[] = {
        // BITMAPFILEHEADER: 14 bytes
        0x42, 0x4D,                         // bfType = "BM"
        0x46, 0x00, 0x00, 0x00,             // bfSize = 70
        0x00, 0x00,                         // bfReserved1 = 0
        0x00, 0x00,                         // bfReserved2 = 0
        0x36, 0x00, 0x00, 0x00,             // bfOffBits = 54

        // BITMAPINFOHEADER: 40 bytes
        0x28, 0x00, 0x00, 0x00,             // biSize = 40
        0x02, 0x00, 0x00, 0x00,             // biWidth = 2
        0xFE, 0xFF, 0xFF, 0xFF,             // biHeight = -2
        0x01, 0x00,                         // biPlanes = 1
        0x18, 0x00,                         // biBitCount = 24
        0x00, 0x00, 0x00, 0x00,             // biCompression = BI_RGB
        0x10, 0x00, 0x00, 0x00,             // biSizeImage = 16
        0x00, 0x00, 0x00, 0x00,             // biXPelsPerMeter = 0
        0x00, 0x00, 0x00, 0x00,             // biYPelsPerMeter = 0
        0x00, 0x00, 0x00, 0x00,             // biClrUsed = 0
        0x00, 0x00, 0x00, 0x00,             // biClrImportant = 0

        // Pixel data: top row
        0x00, 0x00, 0xFF,                   // red   : BGR
        0x00, 0xFF, 0x00,                   // green : BGR
        0x00, 0x00,                         // padding

        // Pixel data: bottom row
        0xFF, 0x00, 0x00,                   // blue  : BGR
        0xFF, 0xFF, 0xFF,                   // white : BGR
        0x00, 0x00                          // padding
    };

    test_bmp_file_write(filepath_, bmp_data, sizeof(bmp_data));
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_file_2x2_32bit_bottom_up_write(const char* filepath_) {
    /*
     * 2x2 / 32bit / uncompressed / bottom-up BMPを生成する。
     *
     * 画像仕様:
     * - width  = 2
     * - height = 2
     * - bit count = 32
     * - row stride = 8 bytes
     * - 1行あたりの実ピクセルデータ = 2px * 4ch = 8 bytes
     * - padding = 0 bytes / row
     *
     * BMPファイル上のピクセル順序:
     * - bottom-up BMPなので、ファイルには下段行 -> 上段行の順で格納する
     * - 各ピクセルはBGRA順で格納する
     *
     * 論理的な画像配置:
     * - top-left     = red,   alpha = 255
     * - top-right    = green, alpha = 255
     * - bottom-left  = blue,  alpha = 255
     * - bottom-right = white, alpha = 255
     */
    const uint8_t bmp_data[] = {
        // BITMAPFILEHEADER: 14 bytes
        0x42, 0x4D,                         // bfType = "BM"
        0x46, 0x00, 0x00, 0x00,             // bfSize = 70
        0x00, 0x00,                         // bfReserved1 = 0
        0x00, 0x00,                         // bfReserved2 = 0
        0x36, 0x00, 0x00, 0x00,             // bfOffBits = 54

        // BITMAPINFOHEADER: 40 bytes
        0x28, 0x00, 0x00, 0x00,             // biSize = 40
        0x02, 0x00, 0x00, 0x00,             // biWidth = 2
        0x02, 0x00, 0x00, 0x00,             // biHeight = 2
        0x01, 0x00,                         // biPlanes = 1
        0x20, 0x00,                         // biBitCount = 32
        0x00, 0x00, 0x00, 0x00,             // biCompression = BI_RGB
        0x10, 0x00, 0x00, 0x00,             // biSizeImage = 16
        0x00, 0x00, 0x00, 0x00,             // biXPelsPerMeter = 0
        0x00, 0x00, 0x00, 0x00,             // biYPelsPerMeter = 0
        0x00, 0x00, 0x00, 0x00,             // biClrUsed = 0
        0x00, 0x00, 0x00, 0x00,             // biClrImportant = 0

        // Pixel data: bottom row
        0xFF, 0x00, 0x00, 0xFF,             // blue  : BGRA
        0xFF, 0xFF, 0xFF, 0xFF,             // white : BGRA

        // Pixel data: top row
        0x00, 0x00, 0xFF, 0xFF,             // red   : BGRA
        0x00, 0xFF, 0x00, 0xFF              // green : BGRA
    };

    test_bmp_file_write(filepath_, bmp_data, sizeof(bmp_data));
}
#endif
