/** @ingroup resource
 *
 * @file texture.c
 * @author chocolate-pie24
 * @brief テクスチャCPU側リソース管理モジュールAPI実装
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
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/resource/texture/texture.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/filesystem/filesystem.h"
#include "engine/core/buffer_utils/buffer_utils.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

#include "engine/resource/loaders/bmp_loader.h"

/**
 * @brief テスト用テクスチャ名称リスト
 *
 */
typedef enum {
    TEST_TEXTURE_RED,       /**< テスト用テクスチャピクセルデータ: 赤色 */
    TEST_TEXTURE_GREEN,     /**< テスト用テクスチャピクセルデータ: 緑色 */
    TEST_TEXTURE_BLUE,      /**< テスト用テクスチャピクセルデータ: 青色 */
} test_texture_t;

/**
 * @brief テクスチャCPU側リソース構造体
 *
 */
struct texture {
    choco_string_t* name;   /**< テクスチャCPU側リソース名称 */

    uint16_t width;         /**< テクスチャ幅 */
    uint16_t height;        /**< テクスチャ高さ(左上原点の画像を基準にする) */
    uint8_t channel_count;  /**< チャンネルカウント(RGB or RGBAのみサポート) */

    uint8_t* pixels;        /**< テクスチャピクセルデータ */
};

static resource_result_t bmp_load(const char* fullpath_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, uint8_t** out_pixels_);
static resource_result_t test_texture_generate(test_texture_t test_texture_color_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, uint8_t** out_pixels_);

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "test_controller.h"

#include "engine/resource/texture/test_texture.h"

#include "engine/core/memory/test_choco_memory.h"
#include "engine/containers/test_choco_string.h"
#include "engine/io_utils/fs_utils/test_fs_utils.h"
#include "engine/resource/loaders/test_bmp_loader.h"

// texture用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_texture_create;            /**< texture_create()テスト設定 */
static test_call_control_t s_test_config_texture_pixel_load;        /**< texture_pixel_load()テスト設定 */
static test_call_control_t s_test_config_texture_pixel_unload;      /**< texture_pixel_unload()テスト設定 */
static test_call_control_t s_test_config_texture_pixel_get;         /**< texture_pixel_get()テスト設定 */
static test_call_control_t s_test_config_texture_pixel_size_get;    /**< texture_pixel_size_get()テスト設定 */

// プライベート関数テスト設定
static test_call_control_t s_test_config_bmp_load;                  /**< bmp_load()テスト設定 */
static test_call_control_t s_test_config_test_texture_generate;     /**< test_texture_generate()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_texture_create(void);
static void test_texture_destroy(void);
static void test_texture_pixel_load(void);
static void test_texture_pixel_unload(void);
static void test_texture_pixel_get(void);
static void test_texture_pixel_size_get(void);
static void test_texture_name_get(void);
static void test_bmp_load(void);
static void test_test_texture_generate(void);

// テスト用ヘルパー関数
static void test_texture_bmp_file_write(const char* filepath_, const uint8_t* data_, size_t size_);
static void test_texture_bmp_file_2x2_24bit_bottom_up_write(const char* filepath_);

#endif

resource_result_t texture_create(const char* name_, texture_t** texture_) {
#ifdef TEST_BUILD
    s_test_config_texture_create.call_count++;
    if(s_test_config_texture_create.fail_on_call != 0) {
        if(s_test_config_texture_create.call_count == s_test_config_texture_create.fail_on_call) {
            return (resource_result_t)s_test_config_texture_create.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    texture_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_create", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_create", "texture_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_create", "*texture_")

    ret_mem = memory_system_allocate(sizeof(texture_t), MEMORY_TAG_TEXTURE, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("texture_create(%s) - Failed to allocate memory for texture_t.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    tmp->name = NULL;
    tmp->channel_count = 0;
    tmp->height = 0;
    tmp->width = 0;
    tmp->pixels = NULL;

    ret_string = choco_string_create_from_c_string(name_, &tmp->name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("texture_create(%s) - Failed to create texture name.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *texture_ = tmp;
    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp) {
            if(NULL != tmp->name) {
                choco_string_destroy(&tmp->name);
            }
            memory_system_free(tmp, sizeof(texture_t), MEMORY_TAG_TEXTURE);
            tmp = NULL;
        }
    }
    return ret;
}

void texture_destroy(texture_t** texture_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    if(NULL == texture_) {
        return;
    }
    if(NULL == *texture_) {
        return;
    }
    if(NULL != (*texture_)->pixels) {
        ret = texture_pixel_unload(*texture_);
        if(RESOURCE_SUCCESS != ret) {
            const char* texture_name = NULL;
            if(NULL == (*texture_)->name) {
                texture_name = "corrupted";
            } else {
                texture_name = choco_string_c_str((*texture_)->name);
            }
            WARN_MESSAGE("texture_destroy(%s) - Failed to unload texture pixels during destroy. texture_name = '%s'. Continue destroying texture object.", resource_rslt_to_str(ret), texture_name);
        }
    }
    if(NULL != (*texture_)->name) {
        choco_string_destroy(&(*texture_)->name);
    }

    memory_system_free(*texture_, sizeof(texture_t), MEMORY_TAG_TEXTURE);
    *texture_ = NULL;
}

resource_result_t texture_pixel_load(texture_t* texture_, const char* filepath_, const char* extension_) {
#ifdef TEST_BUILD
    s_test_config_texture_pixel_load.call_count++;
    if(s_test_config_texture_pixel_load.fail_on_call != 0) {
        if(s_test_config_texture_pixel_load.call_count == s_test_config_texture_pixel_load.fail_on_call) {
            return (resource_result_t)s_test_config_texture_pixel_load.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;
    uint8_t* tmp_pixels = NULL;

    fs_utils_t* fs_utils = NULL;
    choco_string_t* fullpath = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_pixel_load", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->name, ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "texture_pixel_load", "texture_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(texture_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_load", "texture_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->channel_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_load", "0 != texture_->channel_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_load", "0 != texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_load", "0 != texture_->height")

    if(choco_string_equal("test_texture_red", choco_string_c_str(texture_->name))) {
        ret = test_texture_generate(TEST_TEXTURE_RED, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixels);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to create red test texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal("test_texture_green", choco_string_c_str(texture_->name))) {
        ret = test_texture_generate(TEST_TEXTURE_GREEN, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixels);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to create green test texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal("test_texture_blue", choco_string_c_str(texture_->name))) {
        ret = test_texture_generate(TEST_TEXTURE_BLUE, &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixels);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to create blue test texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal(".bmp", extension_)) { // NOTE: choco_string_equalは引数 == NULLでfalse
        ret_fs_utils = fs_utils_create(filepath_, choco_string_c_str(texture_->name), extension_, FILESYSTEM_MODE_READ, &fs_utils);
        if(FS_UTILS_SUCCESS != ret_fs_utils) {
            ret = resource_rslt_convert_fs_utils(ret_fs_utils);
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to create fs_utils.", resource_rslt_to_str(ret));
            goto cleanup;
        }

        ret_string = choco_string_default_create(&fullpath);
        if(CHOCO_STRING_SUCCESS != ret_string) {
            ret = resource_rslt_convert_choco_string(ret_string);
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to create fullpath.", resource_rslt_to_str(ret));
            goto cleanup;
        }

        ret_fs_utils = fs_utils_fullpath_get(fs_utils, fullpath);
        if(FS_UTILS_SUCCESS != ret_fs_utils) {
            ret = resource_rslt_convert_fs_utils(ret_fs_utils);
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to create fullpath.", resource_rslt_to_str(ret));
            goto cleanup;
        }

        ret = bmp_load(choco_string_c_str(fullpath), &tmp_width, &tmp_height, &tmp_channel_count, &tmp_pixels);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to load BMP texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }

        choco_string_destroy(&fullpath);
        fs_utils_destroy(&fs_utils);
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("texture_pixel_load(%s) - Unsupported file type.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    texture_->channel_count = tmp_channel_count;
    texture_->height = tmp_height;
    texture_->width = tmp_width;
    texture_->pixels = tmp_pixels;
    tmp_pixels = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        choco_string_destroy(&fullpath);
        fs_utils_destroy(&fs_utils);
    }
    return ret;
}

resource_result_t texture_pixel_unload(texture_t* texture_) {
#ifdef TEST_BUILD
    s_test_config_texture_pixel_unload.call_count++;
    if(s_test_config_texture_pixel_unload.fail_on_call != 0) {
        if(s_test_config_texture_pixel_unload.call_count == s_test_config_texture_pixel_unload.fail_on_call) {
            return (resource_result_t)s_test_config_texture_pixel_unload.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_pixel_unload", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->name, ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "texture_pixel_unload", "texture_->name")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->channel_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_unload", "texture_->channel_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_unload", "texture_->height")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_unload", "texture_->width")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_unload", "texture_->pixels")

    memory_system_free(texture_->pixels, (size_t)texture_->width * (size_t)texture_->height * (size_t)texture_->channel_count, MEMORY_TAG_TEXTURE);
    texture_->pixels = NULL;
    texture_->channel_count = 0;
    texture_->width = 0;
    texture_->height = 0;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t texture_pixel_get(const texture_t* texture_, uint8_t** out_pixels_) {
#ifdef TEST_BUILD
    s_test_config_texture_pixel_get.call_count++;
    if(s_test_config_texture_pixel_get.fail_on_call != 0) {
        if(s_test_config_texture_pixel_get.call_count == s_test_config_texture_pixel_get.fail_on_call) {
            return (resource_result_t)s_test_config_texture_pixel_get.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_pixel_get", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_pixel_get", "out_pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->channel_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_get", "texture_->channel_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_get", "texture_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_get", "texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_get", "texture_->height")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->name, ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "texture_pixel_get", "texture_->name")

    *out_pixels_ = texture_->pixels;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t texture_pixel_size_get(const texture_t* texture_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_) {
#ifdef TEST_BUILD
    s_test_config_texture_pixel_size_get.call_count++;
    if(s_test_config_texture_pixel_size_get.fail_on_call != 0) {
        if(s_test_config_texture_pixel_size_get.call_count == s_test_config_texture_pixel_size_get.fail_on_call) {
            return (resource_result_t)s_test_config_texture_pixel_size_get.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_pixel_size_get", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(width_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_pixel_size_get", "width_")
    IF_ARG_NULL_GOTO_CLEANUP(height_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_pixel_size_get", "height_")
    IF_ARG_NULL_GOTO_CLEANUP(channel_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_pixel_size_get", "channel_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->channel_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_size_get", "texture_->channel_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_size_get", "texture_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_size_get", "texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_size_get", "texture_->height")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->name, ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "texture_pixel_size_get", "texture_->name")

    *width_ = texture_->width;
    *height_ = texture_->height;
    *channel_count_ = texture_->channel_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

const char* texture_name_get(const texture_t* texture_) {
    if(NULL == texture_) {
        return NULL;
    }
    if(NULL == texture_->name) {
        WARN_MESSAGE("texture_name_get - Provided texture is corrupted.");
        return NULL;
    }
    return choco_string_c_str(texture_->name);
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
#ifdef TEST_BUILD
    s_test_config_bmp_load.call_count++;
    if(s_test_config_bmp_load.fail_on_call != 0) {
        if(s_test_config_bmp_load.call_count == s_test_config_bmp_load.fail_on_call) {
            return (resource_result_t)s_test_config_bmp_load.forced_result;
        }
    }
#endif
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
static resource_result_t test_texture_generate(test_texture_t test_texture_color_, uint16_t* out_width_, uint16_t* out_height_, uint8_t* out_channel_count_, uint8_t** out_pixels_) {
#ifdef TEST_BUILD
    s_test_config_test_texture_generate.call_count++;
    if(s_test_config_test_texture_generate.fail_on_call != 0) {
        if(s_test_config_test_texture_generate.call_count == s_test_config_test_texture_generate.fail_on_call) {
            return (resource_result_t)s_test_config_test_texture_generate.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    const uint16_t tmp_width = 32;
    const uint16_t tmp_height = 32;
    const uint8_t tmp_channel_count = 3;
    const size_t pixel_size = tmp_width * tmp_height * tmp_channel_count;
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

#ifdef TEST_BUILD

void NO_COVERAGE test_texture_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_create.fail_on_call = config_->fail_on_call;
    s_test_config_texture_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_pixel_load_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_pixel_load.fail_on_call = config_->fail_on_call;
    s_test_config_texture_pixel_load.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_pixel_unload_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_pixel_unload.fail_on_call = config_->fail_on_call;
    s_test_config_texture_pixel_unload.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_pixel_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_pixel_get.fail_on_call = config_->fail_on_call;
    s_test_config_texture_pixel_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_pixel_size_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_texture_pixel_size_get.fail_on_call = config_->fail_on_call;
    s_test_config_texture_pixel_size_get.forced_result = config_->forced_result;
}

void NO_COVERAGE test_texture_config_reset(void) {
    test_call_control_reset(&s_test_config_texture_create);
    test_call_control_reset(&s_test_config_texture_pixel_load);
    test_call_control_reset(&s_test_config_texture_pixel_unload);
    test_call_control_reset(&s_test_config_texture_pixel_get);
    test_call_control_reset(&s_test_config_texture_pixel_size_get);

    test_call_control_reset(&s_test_config_bmp_load);
    test_call_control_reset(&s_test_config_test_texture_generate);
}

void NO_COVERAGE test_texture(void) {
    test_texture_create();
    test_texture_destroy();
    test_texture_pixel_load();
    test_texture_pixel_unload();
    test_texture_pixel_get();
    test_texture_pixel_size_get();
    test_texture_name_get();
    test_bmp_load();
    test_test_texture_generate();
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_create(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // texture_create() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        s_test_config_texture_create.fail_on_call = 1U;
        s_test_config_texture_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create(NULL, &texture);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // *texture_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t dummy_texture = { 0 };
        texture_t* texture = &dummy_texture;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_texture == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        test_call_control_t config = { 0 };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // choco_string_create_from_c_string() 失敗 -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        test_call_control_t config = { 0 };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_INVALID_ARGUMENT;
        test_choco_string_create_from_c_string_config_set(&config);

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // 正常系: texture_t が確保され、全フィールドが初期化される
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        assert(NULL != texture->name);
        assert(choco_string_equal("test_texture", choco_string_c_str(texture->name)));

        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);
        assert(NULL == texture->pixels);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_destroy(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // texture_ == NULL -> no-op
        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        texture_destroy(NULL);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // *texture_ == NULL -> no-op
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // pixels == NULL の texture を破棄する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        assert(NULL != texture->name);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);
        assert(NULL == texture->pixels);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // pixels != NULL の texture を破棄する
        // texture_pixel_unload() により pixels が解放され、texture本体も破棄される
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);
        assert(32U == texture->width);
        assert(32U == texture->height);
        assert(3U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_pixel_unload() が失敗しても、texture_destroy() は texture本体の破棄を継続する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->width = 1U;
        texture->height = 1U;
        texture->channel_count = 3U;
        texture->pixels = dummy_pixels;

        s_test_config_texture_pixel_unload.fail_on_call = 1U;
        s_test_config_texture_pixel_unload.forced_result = (int)RESOURCE_BAD_OPERATION;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // destroy後にもう一度destroyしてもno-op
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture_destroy(&texture);
        assert(NULL == texture);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->name == NULL かつ pixels != NULL の破損状態でも、
        // texture_destroy() はwarning出力時にNULL文字列を渡さず、texture本体の破棄を継続する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);
        assert(NULL != texture->name);

        choco_string_destroy(&texture->name);
        assert(NULL == texture->name);

        texture->width = 1U;
        texture->height = 1U;
        texture->channel_count = 3U;
        texture->pixels = dummy_pixels;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_pixel_load(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // texture_pixel_load() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        s_test_config_texture_pixel_load.fail_on_call = 1U;
        s_test_config_texture_pixel_load.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // texture_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_pixel_load(NULL, NULL, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // texture_->name == NULL -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);
        assert(NULL != texture->name);

        choco_string_destroy(&texture->name);
        assert(NULL == texture->name);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // texture_->pixels != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->pixels = dummy_pixels;

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(dummy_pixels == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture->pixels = NULL;
        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // texture_->channel_count != 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;
        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // texture_->width != 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->width = 1U;

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == texture->pixels);
        assert(1U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture->width = 0U;
        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // texture_->height != 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->height = 1U;

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(1U == texture->height);
        assert(0U == texture->channel_count);

        texture->height = 0U;
        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // 未対応拡張子 -> RESOURCE_UNSUPPORTED_FILE
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("ordinary_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, ".png");
        assert(RESOURCE_UNSUPPORTED_FILE == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // 特殊テクスチャ名以外で extension_ == NULL -> RESOURCE_UNSUPPORTED_FILE
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("ordinary_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_UNSUPPORTED_FILE == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // test_texture_generate() 失敗 -> エラーを返し、texture状態は未ロードのまま
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        s_test_config_test_texture_generate.fail_on_call = 1U;
        s_test_config_test_texture_generate.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // 正常系: test_texture_red をロードする
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        const size_t pixel_size = 32U * 32U * 3U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);
        assert(32U == texture->width);
        assert(32U == texture->height);
        assert(3U == texture->channel_count);

        for(size_t i = 0U; i != pixel_size; i += 3U) {
            assert(255U == texture->pixels[i]);
            assert(0U == texture->pixels[i + 1U]);
            assert(0U == texture->pixels[i + 2U]);
        }

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // 正常系: test_texture_green をロードする
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        const size_t pixel_size = 32U * 32U * 3U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_green", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);
        assert(32U == texture->width);
        assert(32U == texture->height);
        assert(3U == texture->channel_count);

        for(size_t i = 0U; i != pixel_size; i += 3U) {
            assert(0U == texture->pixels[i]);
            assert(255U == texture->pixels[i + 1U]);
            assert(0U == texture->pixels[i + 2U]);
        }

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // 正常系: test_texture_blue をロードする
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        const size_t pixel_size = 32U * 32U * 3U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_blue", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);
        assert(32U == texture->width);
        assert(32U == texture->height);
        assert(3U == texture->channel_count);

        for(size_t i = 0U; i != pixel_size; i += 3U) {
            assert(0U == texture->pixels[i]);
            assert(0U == texture->pixels[i + 1U]);
            assert(255U == texture->pixels[i + 2U]);
        }

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // ロード済みtextureに再ロードしようとすると RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        uint8_t* loaded_pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);

        loaded_pixels = texture->pixels;

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(loaded_pixels == texture->pixels);
        assert(32U == texture->width);
        assert(32U == texture->height);
        assert(3U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // .bmp 経路: choco_string_default_create() 失敗 -> texture状態は未ロードのまま
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        test_call_control_t config = { 0 };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_choco_string_fail", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        config.fail_on_call = 1U;
        config.forced_result = (int)CHOCO_STRING_INVALID_ARGUMENT;
        test_choco_string_default_create_config_set(&config);

        ret = texture_pixel_load(texture, "", ".bmp");
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // .bmp 経路: bmp_load() 失敗 -> texture状態は未ロードのまま
        // bmp_load() まで到達させるため、有効なBMPファイル自体は作成しておく
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        test_texture_bmp_file_2x2_24bit_bottom_up_write("test_texture_bmp_load_fail.bmp");

        ret = texture_create("test_texture_bmp_load_fail", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        s_test_config_bmp_load.fail_on_call = 1U;
        s_test_config_bmp_load.forced_result = (int)RESOURCE_FILE_READ_ERROR;

        ret = texture_pixel_load(texture, "", ".bmp");
        assert(RESOURCE_FILE_READ_ERROR == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        remove("test_texture_bmp_load_fail.bmp");

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // 正常系: .bmpファイルをロードし、textureへサイズとピクセルが反映される
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        const uint8_t expected_pixels[12] = {
            0xFFU, 0x00U, 0x00U,
            0x00U, 0xFFU, 0x00U,
            0x00U, 0x00U, 0xFFU,
            0xFFU, 0xFFU, 0xFFU
        };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        test_texture_bmp_file_2x2_24bit_bottom_up_write("test_texture_2x2_24bit_bottom_up.bmp");

        ret = texture_create("test_texture_2x2_24bit_bottom_up", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, "", ".bmp");
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);
        assert(2U == texture->width);
        assert(2U == texture->height);
        assert(3U == texture->channel_count);
        assert(0 == memcmp(expected_pixels, texture->pixels, sizeof(expected_pixels)));

        texture_destroy(&texture);
        assert(NULL == texture);

        remove("test_texture_2x2_24bit_bottom_up.bmp");

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // test_texture_generate() 失敗: green -> エラーを返し、texture状態は未ロードのまま
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_green", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        s_test_config_test_texture_generate.fail_on_call = 1U;
        s_test_config_test_texture_generate.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // test_texture_generate() 失敗: blue -> エラーを返し、texture状態は未ロードのまま
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();

        ret = texture_create("test_texture_blue", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        s_test_config_test_texture_generate.fail_on_call = 1U;
        s_test_config_test_texture_generate.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
        test_fs_utils_config_reset();
        test_bmp_loader_config_reset();
    }
    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_pixel_unload(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // texture_pixel_unload() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        // 強制失敗時は texture のロード済み状態が維持される
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t* loaded_pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);

        loaded_pixels = texture->pixels;

        s_test_config_texture_pixel_unload.fail_on_call = 1U;
        s_test_config_texture_pixel_unload.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = texture_pixel_unload(texture);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(loaded_pixels == texture->pixels);
        assert(32U == texture->width);
        assert(32U == texture->height);
        assert(3U == texture->channel_count);

        test_texture_config_reset();

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_pixel_unload(NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->name == NULL -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);
        assert(NULL != texture->name);

        choco_string_destroy(&texture->name);
        assert(NULL == texture->name);

        texture->width = 1U;
        texture->height = 1U;
        texture->channel_count = 3U;
        texture->pixels = dummy_pixels;

        ret = texture_pixel_unload(texture);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(1U == texture->width);
        assert(1U == texture->height);
        assert(3U == texture->channel_count);
        assert(dummy_pixels == texture->pixels);

        texture->width = 0U;
        texture->height = 0U;
        texture->channel_count = 0U;
        texture->pixels = NULL;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->channel_count == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_unload(texture);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->height == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;

        ret = texture_pixel_unload(texture);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->width == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;
        texture->height = 1U;

        ret = texture_pixel_unload(texture);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(1U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;
        texture->height = 0U;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->pixels == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;
        texture->height = 1U;
        texture->width = 1U;

        ret = texture_pixel_unload(texture);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == texture->pixels);
        assert(1U == texture->width);
        assert(1U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;
        texture->height = 0U;
        texture->width = 0U;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // 正常系: ロード済みtextureのpixelsを解放し、サイズ情報を0に戻す
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);
        assert(32U == texture->width);
        assert(32U == texture->height);
        assert(3U == texture->channel_count);

        ret = texture_pixel_unload(texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);
        assert(NULL != texture->name);
        assert(choco_string_equal("test_texture_red", choco_string_c_str(texture->name)));

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // unload後にもう一度unloadすると RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);

        ret = texture_pixel_unload(texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        ret = texture_pixel_unload(texture);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(0U == texture->channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_pixel_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // texture_pixel_get() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        // 強制失敗時は out_pixels が変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_out = 0U;
        uint8_t* out_pixels = &dummy_out;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);

        s_test_config_texture_pixel_get.fail_on_call = 1U;
        s_test_config_texture_pixel_get.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = texture_pixel_get(texture, &out_pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(&dummy_out == out_pixels);

        test_texture_config_reset();

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // out_pixels は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint8_t dummy_out = 0U;
        uint8_t* out_pixels = &dummy_out;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_pixel_get(NULL, &out_pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_out == out_pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // out_pixels_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_get(texture, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->channel_count == 0 -> RESOURCE_BAD_OPERATION
        // out_pixels は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_out = 0U;
        uint8_t* out_pixels = &dummy_out;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_get(texture, &out_pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(&dummy_out == out_pixels);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->pixels == NULL -> RESOURCE_BAD_OPERATION
        // out_pixels は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_out = 0U;
        uint8_t* out_pixels = &dummy_out;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;

        ret = texture_pixel_get(texture, &out_pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(&dummy_out == out_pixels);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->width == 0 -> RESOURCE_BAD_OPERATION
        // out_pixels は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };
        uint8_t dummy_out = 0U;
        uint8_t* out_pixels = &dummy_out;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;
        texture->pixels = dummy_pixels;

        ret = texture_pixel_get(texture, &out_pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(&dummy_out == out_pixels);
        assert(dummy_pixels == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;
        texture->pixels = NULL;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->height == 0 -> RESOURCE_BAD_OPERATION
        // out_pixels は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };
        uint8_t dummy_out = 0U;
        uint8_t* out_pixels = &dummy_out;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;
        texture->pixels = dummy_pixels;
        texture->width = 1U;

        ret = texture_pixel_get(texture, &out_pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(&dummy_out == out_pixels);
        assert(dummy_pixels == texture->pixels);
        assert(1U == texture->width);
        assert(0U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;
        texture->pixels = NULL;
        texture->width = 0U;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->name == NULL -> RESOURCE_DATA_CORRUPTED
        // out_pixels は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };
        uint8_t dummy_out = 0U;
        uint8_t* out_pixels = &dummy_out;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);
        assert(NULL != texture->name);

        choco_string_destroy(&texture->name);
        assert(NULL == texture->name);

        texture->channel_count = 3U;
        texture->pixels = dummy_pixels;
        texture->width = 1U;
        texture->height = 1U;

        ret = texture_pixel_get(texture, &out_pixels);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(&dummy_out == out_pixels);
        assert(dummy_pixels == texture->pixels);
        assert(1U == texture->width);
        assert(1U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;
        texture->pixels = NULL;
        texture->width = 0U;
        texture->height = 0U;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // 正常系: ロード済みtextureの内部pixels参照を取得する
        // 所有権は移譲されず、out_pixels は texture->pixels と同じアドレスになる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        uint8_t* out_pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);
        assert(32U == texture->width);
        assert(32U == texture->height);
        assert(3U == texture->channel_count);

        ret = texture_pixel_get(texture, &out_pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != out_pixels);
        assert(texture->pixels == out_pixels);

        assert(255U == out_pixels[0]);
        assert(0U == out_pixels[1]);
        assert(0U == out_pixels[2]);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_pixel_size_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // texture_pixel_size_get() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        // 強制失敗時は width / height / channel_count が変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);

        s_test_config_texture_pixel_size_get.fail_on_call = 1U;
        s_test_config_texture_pixel_size_get.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = texture_pixel_size_get(texture, &width, &height, &channel_count);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        test_texture_config_reset();

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // width / height / channel_count は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_pixel_size_get(NULL, &width, &height, &channel_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // width_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // height / channel_count は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);

        ret = texture_pixel_size_get(texture, NULL, &height, &channel_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(222U == height);
        assert(33U == channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // height_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // width / channel_count は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint16_t width = 111U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);

        ret = texture_pixel_size_get(texture, &width, NULL, &channel_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(33U == channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // channel_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // width / height は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint16_t width = 111U;
        uint16_t height = 222U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);

        ret = texture_pixel_size_get(texture, &width, &height, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->channel_count == 0 -> RESOURCE_BAD_OPERATION
        // width / height / channel_count は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_size_get(texture, &width, &height, &channel_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->pixels == NULL -> RESOURCE_BAD_OPERATION
        // width / height / channel_count は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;

        ret = texture_pixel_size_get(texture, &width, &height, &channel_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->width == 0 -> RESOURCE_BAD_OPERATION
        // width / height / channel_count は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;
        texture->pixels = dummy_pixels;

        ret = texture_pixel_size_get(texture, &width, &height, &channel_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(dummy_pixels == texture->pixels);
        assert(0U == texture->width);
        assert(0U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;
        texture->pixels = NULL;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->height == 0 -> RESOURCE_BAD_OPERATION
        // width / height / channel_count は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        texture->channel_count = 3U;
        texture->pixels = dummy_pixels;
        texture->width = 1U;

        ret = texture_pixel_size_get(texture, &width, &height, &channel_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(dummy_pixels == texture->pixels);
        assert(1U == texture->width);
        assert(0U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;
        texture->pixels = NULL;
        texture->width = 0U;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->name == NULL -> RESOURCE_DATA_CORRUPTED
        // width / height / channel_count は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        uint8_t dummy_pixels[3] = { 0U, 0U, 0U };
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);
        assert(NULL != texture->name);

        choco_string_destroy(&texture->name);
        assert(NULL == texture->name);

        texture->channel_count = 3U;
        texture->pixels = dummy_pixels;
        texture->width = 1U;
        texture->height = 1U;

        ret = texture_pixel_size_get(texture, &width, &height, &channel_count);
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(dummy_pixels == texture->pixels);
        assert(1U == texture->width);
        assert(1U == texture->height);
        assert(3U == texture->channel_count);

        texture->channel_count = 0U;
        texture->pixels = NULL;
        texture->width = 0U;
        texture->height = 0U;

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // 正常系: ロード済みtextureの幅・高さ・チャンネル数を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t channel_count = 0U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture->pixels);
        assert(32U == texture->width);
        assert(32U == texture->height);
        assert(3U == texture->channel_count);

        ret = texture_pixel_size_get(texture, &width, &height, &channel_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(32U == width);
        assert(32U == height);
        assert(3U == channel_count);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_name_get(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // texture_ == NULL -> NULL
        const char* name = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        name = texture_name_get(NULL);
        assert(NULL == name);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // texture_->name == NULL -> NULL
        resource_result_t ret = RESOURCE_SUCCESS;
        texture_t* texture = NULL;
        const char* name = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);
        assert(NULL != texture->name);

        choco_string_destroy(&texture->name);
        assert(NULL == texture->name);

        name = texture_name_get(texture);
        assert(NULL == name);

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // 正常系: texture名を取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        const char* name = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        name = texture_name_get(texture);
        assert(NULL != name);
        assert(choco_string_equal("test_texture", name));

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }
    {
        // 正常系: pixel load後もtexture名は維持される
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        texture_t* texture = NULL;
        const char* name = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();

        ret = texture_create("test_texture_red", &texture);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != texture);

        ret = texture_pixel_load(texture, NULL, NULL);
        assert(RESOURCE_SUCCESS == ret);

        name = texture_name_get(texture);
        assert(NULL != name);
        assert(choco_string_equal("test_texture_red", name));

        texture_destroy(&texture);
        assert(NULL == texture);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_choco_string_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_bmp_load(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // bmp_load() 冒頭で強制的に RESOURCE_FILE_READ_ERROR を返させる
        // 強制失敗時は out 引数が変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t dummy_pixel = 0U;
        uint8_t* pixels = &dummy_pixel;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        s_test_config_bmp_load.fail_on_call = 1U;
        s_test_config_bmp_load.forced_result = (int)RESOURCE_FILE_READ_ERROR;

        ret = bmp_load("test_texture_bmp_load.bmp", &width, &height, &channel_count, &pixels);
        assert(RESOURCE_FILE_READ_ERROR == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(&dummy_pixel == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // fullpath_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        ret = bmp_load(NULL, &width, &height, &channel_count, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // out_width_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // 他の out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        ret = bmp_load("test_texture_bmp_load.bmp", NULL, &height, &channel_count, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // out_height_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // 他の out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        ret = bmp_load("test_texture_bmp_load.bmp", &width, NULL, &channel_count, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(33U == channel_count);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // out_channel_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // 他の out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t* pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        ret = bmp_load("test_texture_bmp_load.bmp", &width, &height, NULL, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // out_pixels_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // 他の out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        ret = bmp_load("test_texture_bmp_load.bmp", &width, &height, &channel_count, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // *out_pixels_ != NULL -> RESOURCE_INVALID_ARGUMENT
        // out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t dummy_pixel = 0U;
        uint8_t* pixels = &dummy_pixel;

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        ret = bmp_load("test_texture_bmp_load.bmp", &width, &height, &channel_count, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(&dummy_pixel == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // bmp_loader_create() 失敗 -> エラーを返し、out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;
        test_call_control_t config = { 0 };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_NO_MEMORY;
        test_bmp_loader_create_config_set(&config);

        ret = bmp_load("test_texture_bmp_load.bmp", &width, &height, &channel_count, &pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // bmp_loader_load() 失敗 -> エラーを返し、out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;
        test_call_control_t config = { 0 };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_FILE_READ_ERROR;
        test_bmp_loader_load_config_set(&config);

        ret = bmp_load("test_texture_bmp_load.bmp", &width, &height, &channel_count, &pixels);
        assert(RESOURCE_FILE_READ_ERROR == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // bmp_loader_bmp_size_get() 失敗 -> エラーを返し、out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;
        test_call_control_t config = { 0 };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        test_texture_bmp_file_2x2_24bit_bottom_up_write("test_texture_bmp_size_get_fail.bmp");

        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_BAD_OPERATION;
        test_bmp_loader_bmp_size_get_config_set(&config);

        ret = bmp_load("test_texture_bmp_size_get_fail.bmp", &width, &height, &channel_count, &pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == pixels);

        remove("test_texture_bmp_size_get_fail.bmp");

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // bmp_loader_pixel_move() 失敗 -> エラーを返し、out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;
        test_call_control_t config = { 0 };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        test_texture_bmp_file_2x2_24bit_bottom_up_write("test_texture_bmp_pixel_move_fail.bmp");

        config.fail_on_call = 1U;
        config.forced_result = (int)RESOURCE_BAD_OPERATION;
        test_bmp_loader_pixel_move_config_set(&config);

        ret = bmp_load("test_texture_bmp_pixel_move_fail.bmp", &width, &height, &channel_count, &pixels);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == pixels);

        remove("test_texture_bmp_pixel_move_fail.bmp");

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }
    {
        // 正常系: BMPファイルをロードし、サイズとピクセル所有権をoutへ渡す
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t channel_count = 0U;
        uint8_t* pixels = NULL;
        const uint8_t expected_pixels[12] = {
            0xFFU, 0x00U, 0x00U,
            0x00U, 0xFFU, 0x00U,
            0x00U, 0x00U, 0xFFU,
            0xFFU, 0xFFU, 0xFFU
        };

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();

        test_texture_bmp_file_2x2_24bit_bottom_up_write("test_texture_bmp_load_success.bmp");

        ret = bmp_load("test_texture_bmp_load_success.bmp", &width, &height, &channel_count, &pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(2U == width);
        assert(2U == height);
        assert(3U == channel_count);
        assert(NULL != pixels);
        assert(0 == memcmp(expected_pixels, pixels, sizeof(expected_pixels)));

        memory_system_free(pixels, (size_t)width * (size_t)height * (size_t)channel_count, MEMORY_TAG_TEXTURE);
        pixels = NULL;

        remove("test_texture_bmp_load_success.bmp");

        test_texture_config_reset();
        test_choco_memory_config_reset();
        test_bmp_loader_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_test_texture_generate(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // test_texture_generate() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        // 強制失敗時は out 引数が変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t dummy_pixel = 0U;
        uint8_t* pixels = &dummy_pixel;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        s_test_config_test_texture_generate.fail_on_call = 1U;
        s_test_config_test_texture_generate.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = test_texture_generate(TEST_TEXTURE_RED, &width, &height, &channel_count, &pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(&dummy_pixel == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_width_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // 他の out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        ret = test_texture_generate(TEST_TEXTURE_RED, NULL, &height, &channel_count, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_height_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // 他の out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        ret = test_texture_generate(TEST_TEXTURE_RED, &width, NULL, &channel_count, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(33U == channel_count);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_channel_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // 他の out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t* pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        ret = test_texture_generate(TEST_TEXTURE_RED, &width, &height, NULL, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_pixels_ == NULL -> RESOURCE_INVALID_ARGUMENT
        // 他の out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        ret = test_texture_generate(TEST_TEXTURE_RED, &width, &height, &channel_count, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *out_pixels_ != NULL -> RESOURCE_INVALID_ARGUMENT
        // out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t dummy_pixel = 0U;
        uint8_t* pixels = &dummy_pixel;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        ret = test_texture_generate(TEST_TEXTURE_RED, &width, &height, &channel_count, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(&dummy_pixel == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        // out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;
        test_call_control_t config = { 0 };

        test_texture_config_reset();
        test_choco_memory_config_reset();

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = test_texture_generate(TEST_TEXTURE_RED, &width, &height, &channel_count, &pixels);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 不正な test_texture_color_ -> RESOURCE_INVALID_ARGUMENT
        // 確保済み tmp_pixels は内部で解放され、out 引数は変更されない
        resource_result_t ret = RESOURCE_SUCCESS;
        uint16_t width = 111U;
        uint16_t height = 222U;
        uint8_t channel_count = 33U;
        uint8_t* pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        ret = test_texture_generate((test_texture_t)999, &width, &height, &channel_count, &pixels);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(111U == width);
        assert(222U == height);
        assert(33U == channel_count);
        assert(NULL == pixels);

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: TEST_TEXTURE_RED を生成する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t channel_count = 0U;
        uint8_t* pixels = NULL;
        const size_t pixel_size = 32U * 32U * 3U;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        ret = test_texture_generate(TEST_TEXTURE_RED, &width, &height, &channel_count, &pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(32U == width);
        assert(32U == height);
        assert(3U == channel_count);
        assert(NULL != pixels);

        for(size_t i = 0U; i != pixel_size; i += 3U) {
            assert(255U == pixels[i]);
            assert(0U == pixels[i + 1U]);
            assert(0U == pixels[i + 2U]);
        }

        memory_system_free(pixels, pixel_size, MEMORY_TAG_TEXTURE);
        pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: TEST_TEXTURE_GREEN を生成する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t channel_count = 0U;
        uint8_t* pixels = NULL;
        const size_t pixel_size = 32U * 32U * 3U;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        ret = test_texture_generate(TEST_TEXTURE_GREEN, &width, &height, &channel_count, &pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(32U == width);
        assert(32U == height);
        assert(3U == channel_count);
        assert(NULL != pixels);

        for(size_t i = 0U; i != pixel_size; i += 3U) {
            assert(0U == pixels[i]);
            assert(255U == pixels[i + 1U]);
            assert(0U == pixels[i + 2U]);
        }

        memory_system_free(pixels, pixel_size, MEMORY_TAG_TEXTURE);
        pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: TEST_TEXTURE_BLUE を生成する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        uint16_t width = 0U;
        uint16_t height = 0U;
        uint8_t channel_count = 0U;
        uint8_t* pixels = NULL;
        const size_t pixel_size = 32U * 32U * 3U;

        test_texture_config_reset();
        test_choco_memory_config_reset();

        ret = test_texture_generate(TEST_TEXTURE_BLUE, &width, &height, &channel_count, &pixels);
        assert(RESOURCE_SUCCESS == ret);
        assert(32U == width);
        assert(32U == height);
        assert(3U == channel_count);
        assert(NULL != pixels);

        for(size_t i = 0U; i != pixel_size; i += 3U) {
            assert(0U == pixels[i]);
            assert(0U == pixels[i + 1U]);
            assert(255U == pixels[i + 2U]);
        }

        memory_system_free(pixels, pixel_size, MEMORY_TAG_TEXTURE);
        pixels = NULL;

        test_texture_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_texture_bmp_file_write(const char* filepath_, const uint8_t* data_, size_t size_) {
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
static void NO_COVERAGE test_texture_bmp_file_2x2_24bit_bottom_up_write(const char* filepath_) {
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
     *
     * bmp_loader_load() 通過後の期待ピクセル列:
     * - RGB順
     * - 左上原点
     * - padding除去済み
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

    test_texture_bmp_file_write(filepath_, bmp_data, sizeof(bmp_data));
}
#endif
