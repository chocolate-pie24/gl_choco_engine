/** @ingroup resource
 *
 * @file texture_cpu_resource.c
 * @author chocolate-pie24
 * @brief テクスチャCPU側リソースを操作するモジュールAPIの実装
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
#include "engine/resource/texture/texture_cpu_resource.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

#include "engine/resource/loaders/bmp_loader.h"

/**
 * @brief テスト用テクスチャ名称リスト
 *
 */
typedef enum {
    TEST_TEXTURE_RED,       /**< テスト用テクスチャピクセルデータ: 赤色 */
    TEST_TEXTURE_GREEN,     /**< テスト用テクスチャピクセルデータ: 緑色 */
    TEST_TEXTURE_BLUE,      /**< テスト用テクスチャピクセルデータ: 青色 */
} test_texture_color_t;

/**
 * @brief テクスチャCPU側リソース構造体
 *
 */
struct texture_cpu_resource {
    uint16_t width;         /**< テクスチャ幅 */
    uint16_t height;        /**< テクスチャ高さ(左上原点の画像を基準にする) */
    uint8_t channel_count;  /**< チャンネルカウント(RGB or RGBAのみサポート) */
    uint8_t* pixels;        /**< テクスチャピクセルデータ */
};

static resource_result_t bmp_load(const char* fullpath_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, uint8_t** out_pixels_);
static resource_result_t test_texture_generate(test_texture_color_t test_texture_color_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, uint8_t** out_pixels_);
static bool texture_cpu_resource_is_unloaded(const texture_cpu_resource_t* texture_);

resource_result_t texture_cpu_resource_create(texture_cpu_resource_t** texture_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    texture_cpu_resource_t* tmp_cpu_resource = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "texture_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_create", "*texture_")

    ret_mem = memory_system_allocate(sizeof(texture_cpu_resource_t), MEMORY_TAG_TEXTURE, (void**)&tmp_cpu_resource);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("texture_cpu_resource_create(%s) - Failed to allocate memory for texture_cpu_resource_t.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_cpu_resource->channel_count = 0;
    tmp_cpu_resource->height = 0;
    tmp_cpu_resource->width = 0;
    tmp_cpu_resource->pixels = NULL;

    *texture_ = tmp_cpu_resource;
    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_cpu_resource) {
            memory_system_free(tmp_cpu_resource, sizeof(texture_cpu_resource_t), MEMORY_TAG_TEXTURE);
            tmp_cpu_resource = NULL;
        }
    }
    return ret;
}

void texture_cpu_resource_destroy(texture_cpu_resource_t** texture_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    if(NULL == texture_) {
        return;
    }
    if(NULL == *texture_) {
        return;
    }
    if(NULL != (*texture_)->pixels) {
        ret = texture_cpu_resource_pixel_unload(*texture_);
        if(RESOURCE_SUCCESS != ret) {
            WARN_MESSAGE("texture_cpu_resource_destroy(%s) - Failed to unload texture pixels during destroy. Continue destroying texture object.", resource_rslt_to_str(ret));
        }
    }

    memory_system_free(*texture_, sizeof(texture_cpu_resource_t), MEMORY_TAG_TEXTURE);
    *texture_ = NULL;
}

// NOTE: test_texture以外はtexture_source_にはBMPファイルのフルパスを渡す
resource_result_t texture_cpu_resource_pixel_load(texture_cpu_resource_t* texture_, const char* texture_source_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;
    uint8_t* tmp_pixels = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_load", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_source_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_load", "texture_source_")
    if(!texture_cpu_resource_is_valid(texture_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixel_load(%s) - Provided texture is corrupted.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(texture_cpu_resource_is_loaded(texture_)) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("texture_cpu_resource_pixel_load(%s) - provided texture is already loaded.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(choco_string_equal("test_texture_red", texture_source_)) {
        ret = test_texture_generate(TEST_TEXTURE_RED, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixels);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_cpu_resource_pixel_load(%s) - Failed to create red test texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal("test_texture_green", texture_source_)) {
        ret = test_texture_generate(TEST_TEXTURE_GREEN, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixels);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_cpu_resource_pixel_load(%s) - Failed to create green test texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal("test_texture_blue", texture_source_)) {
        ret = test_texture_generate(TEST_TEXTURE_BLUE, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixels);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_cpu_resource_pixel_load(%s) - Failed to create blue test texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else {
        ret = bmp_load(texture_source_, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixels);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_cpu_resource_pixel_load(%s) - Failed to load BMP texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    }

    texture_->channel_count = tmp_channel_count;
    texture_->height = tmp_height;
    texture_->width = tmp_width;
    texture_->pixels = tmp_pixels;
    tmp_pixels = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t texture_cpu_resource_pixel_unload(texture_cpu_resource_t* texture_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_unload", "texture_")
    if(texture_cpu_resource_is_unloaded(texture_)) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("texture_cpu_resource_pixel_unload(%s) - provided texture_ is unloaded.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(!texture_cpu_resource_is_valid(texture_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixel_unload(%s) - provided texture_ is corrupted.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    memory_system_free(texture_->pixels, (size_t)texture_->width * (size_t)texture_->height * (size_t)texture_->channel_count, MEMORY_TAG_TEXTURE);
    texture_->pixels = NULL;
    texture_->channel_count = 0;
    texture_->width = 0;
    texture_->height = 0;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t texture_cpu_resource_pixel_get(const texture_cpu_resource_t* texture_, const uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_get", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_get", "out_pixels_")

    if(texture_cpu_resource_is_unloaded(texture_)) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("texture_cpu_resource_pixel_get(%s) - provided texture_ is unloaded.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(!texture_cpu_resource_is_valid(texture_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixel_get(%s) - provided texture_ is corrupted.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    *out_pixels_ = texture_->pixels;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t texture_cpu_resource_pixel_size_get(const texture_cpu_resource_t* texture_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(width_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "width_")
    IF_ARG_NULL_GOTO_CLEANUP(height_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "height_")
    IF_ARG_NULL_GOTO_CLEANUP(channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_cpu_resource_pixel_size_get", "channel_count_")

    if(!texture_cpu_resource_is_valid(texture_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_cpu_resource_pixel_size_get(%s) - provided texture_ is corrupted.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(texture_cpu_resource_is_unloaded(texture_)) {
        ret = RESOURCE_BAD_OPERATION;
        ERROR_MESSAGE("texture_cpu_resource_pixel_size_get(%s) - provided texture_ is unloaded.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *width_ = texture_->width;
    *height_ = texture_->height;
    *channel_count_ = texture_->channel_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

bool texture_cpu_resource_is_valid(const texture_cpu_resource_t* texture_) {
    if(NULL == texture_) {
        return false;
    }
    if(texture_cpu_resource_is_unloaded(texture_) || texture_cpu_resource_is_loaded(texture_)) {
        return true;
    }
    return false;
}

bool texture_cpu_resource_is_loaded(const texture_cpu_resource_t* texture_) {
    if(NULL == texture_) {
        return false;
    }
    if(0 == texture_->height || 0 == texture_->width || NULL == texture_->pixels) {
        return false;
    }
    if(3 != texture_->channel_count && 4 != texture_->channel_count) {
        return false;
    }
    return true;
}

/**
 * @brief BMPファイルを読み込む
 *
 * @note 読み込んだBMPファイルのピクセルデータは以下の形式となる
 * - 原点は左上
 * - チャンネルカウントRGBのピクセルデータについてもpaddingは除去される
 * - BGRではなく、RGBの順でピクセルが格納される
 * - 高さの値は常に正
 * @note 処理に失敗した場合、out引数の状態は全て不変
 *
 * @param[in] fullpath_ BMPファイルのフルパス情報
 * @param[out] out_width_ 読み込んだBMPファイルの幅格納先
 * @param[out] out_height_ 読み込んだBMPファイルの高さ(元画像の原点が左下だった場合、左上に変換される)格納先
 * @param[out] out_channel_count_ BMPファイルのチャンネルカウント(RGB or RGBAのみをサポート)格納先
 * @param[out] out_pixels_ ピクセルデータ格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - fullpath_ == NULL
 * - out_width_ == NULL
 * - out_height_ == NULL
 * - out_channel_count_ == NULL
 * - out_pixels_ == NULL
 * - *out_pixels_ == NULL
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステムの使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_FILE_OPEN_ERROR ファイルオープン失敗
 * @retval RESOURCE_FILE_CLOSE_ERROR ファイルクローズ失敗
 * @retval RESOURCE_UNDEFINED_ERROR 未定義エラーが発生
 * @retval RESOURCE_FILE_READ_ERROR ヘッダまたはピクセル情報の読み込みに失敗
 * @retval RESOURCE_UNSUPPORTED_FILE サポート対象外のBMPファイル(DEBUG_BUILD or TEST_BUILDで詳細なログが出力される)
 * @retval RESOURCE_OVERFLOW 計算過程でオーバーフロー発生
 * @retval RESOURCE_DATA_CORRUPTED ピクセル読み込みサイズ異常
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t bmp_load(const char* fullpath_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    bmp_loader_t* bmp_loader = NULL;

    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;
    uint8_t* tmp_pixels = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_width_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_load", "out_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_height_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_load", "out_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_load", "out_channel_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_load", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_load", "*out_pixels_")

    ret = bmp_loader_create(&bmp_loader);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_load(%s) - Failed to create bmp_loader.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = bmp_loader_load(fullpath_, bmp_loader);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_load(%s) - Failed to load BMP file(%s).", resource_rslt_to_str(ret), fullpath_);
        goto cleanup;
    }

    ret = bmp_loader_bmp_size_get(bmp_loader, &tmp_width, &tmp_height, &tmp_channel_count);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_load(%s) - Failed to get BMP size(%s).", resource_rslt_to_str(ret), fullpath_);
        goto cleanup;
    }

    ret = bmp_loader_pixel_move(bmp_loader, &tmp_pixels);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_load(%s) - Failed to move BMP pixels.", resource_rslt_to_str(ret), fullpath_);
        goto cleanup;
    }

    bmp_loader_destroy(&bmp_loader);

    *out_width_ = tmp_width;
    *out_height_ = tmp_height;
    *out_channel_count_ = tmp_channel_count;
    *out_pixels_ = tmp_pixels;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        bmp_loader_destroy(&bmp_loader);
    }
    return ret;
}

/**
 * @brief テスト用テクスチャピクセルデータを生成する
 *
 * @note 処理に失敗した場合、out_width_ / out_height_ / out_channel_count_ / out_pixels_の値は不変
 *
 * @param[in] test_texture_color_ 生成するテスト用テクスチャ選択値
 * @param[out] out_width_ 生成したテクスチャの幅
 * @param[out] out_height_ 生成したテクスチャの高さ(原点は左上)
 * @param[out] out_channel_count_ チャンネルカウント(当面はテストテクスチャはRGBのみ)
 * @param[out] out_pixels_ 生成したピクセルデータ格納先(メモリは本関数内で確保する)
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - out_width_ == NULL
 * - out_height_ == NULL
 * - out_channel_count_ == NULL
 * - out_pixels_ == NULL
 * - *out_pixels_ != NULL
 * - test_texture_color_が規定値外
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステムの使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ割り当て失敗
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t test_texture_generate(test_texture_color_t test_texture_color_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, uint8_t** out_pixels_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    const uint16_t tmp_width = 32;
    const uint16_t tmp_height = 32;
    const uint8_t tmp_channel_count = 3;
    const size_t pixel_size = (size_t)tmp_width * (size_t)tmp_height * (size_t)tmp_channel_count;
    uint8_t* tmp_pixels = NULL;
    size_t index = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_width_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "test_texture_generate", "out_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_height_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "test_texture_generate", "out_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "test_texture_generate", "out_channel_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "test_texture_generate", "out_pixels_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "test_texture_generate", "*out_pixels_")

    ret_mem = memory_system_allocate(pixel_size, MEMORY_TAG_TEXTURE, (void**)&tmp_pixels);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("test_texture_generate(%s) - Failed to allocate memory for pixels.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if(TEST_TEXTURE_RED == test_texture_color_) {
        index = 0;
    } else if(TEST_TEXTURE_GREEN == test_texture_color_) {
        index = 1;
    } else if(TEST_TEXTURE_BLUE == test_texture_color_) {
        index = 2;
    } else {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("test_texture_generate(%s) - Invalid test_texture_color_.", resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT));
        goto cleanup;
    }
    for(size_t i = 0, ii = 0; i != (tmp_width * tmp_height); ++i, ii += 3) {
        tmp_pixels[ii] = 0;
        tmp_pixels[ii + 1] = 0;
        tmp_pixels[ii + 2] = 0;
        tmp_pixels[ii + index] = 255;
    }

    *out_width_ = tmp_width;
    *out_height_ = tmp_height;
    *out_channel_count_ = tmp_channel_count;
    *out_pixels_ = tmp_pixels;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_pixels) {
            memory_system_free(tmp_pixels, pixel_size, MEMORY_TAG_TEXTURE);
            tmp_pixels = NULL;
        }
    }
    return ret;
}

static bool texture_cpu_resource_is_unloaded(const texture_cpu_resource_t* texture_) {
    if(NULL == texture_) {
        return false;
    }
    if(0 == texture_->width && 0 == texture_->height && 0 == texture_->channel_count && NULL == texture_->pixels) {
        return true;
    } else {
        return false;
    }
}
