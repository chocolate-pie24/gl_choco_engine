#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/resources/texture/texture.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/filesystem/filesystem.h"
#include "engine/core/buffer_utils/buffer_utils.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

#include "engine/containers/choco_string.h"

struct texture {
    choco_string_t* name;   /**< テクスチャCPU側リソース名称 */

    uint16_t width;
    uint16_t height;
    uint8_t channel_count;

    uint8_t* pixels;
};

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
static const char* s_rslt_str_undefined_error = "UNDEFINED_ERROR";      /**< 実行結果コード文字列: 未定義エラー */

static texture_result_t bmp_load(texture_t* texture_, const char* filepath_, const char* extension_);
static const char* rslt_to_str(texture_result_t rslt_);
static texture_result_t choco_string_result_convert(choco_string_result_t result_);
static texture_result_t memory_system_result_convert(memory_system_result_t result_);
static texture_result_t filesystem_result_convert(filesystem_result_t result_);
static texture_result_t fs_utils_result_convert(fs_utils_result_t result_);

texture_result_t texture_create(const char* name_, texture_t** texture_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    texture_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_create", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_create", "texture_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_create", "*texture_")

    ret_mem = memory_system_allocate(sizeof(texture_t), MEMORY_TAG_TEXTURE, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = memory_system_result_convert(ret_mem);
        ERROR_MESSAGE("texture_create(%s) - Failed to allocate memory for texture_t.", rslt_to_str(ret));
        goto cleanup;
    }
    tmp->name = NULL;
    tmp->channel_count = 0;
    tmp->height = 0;
    tmp->width = 0;
    tmp->pixels = NULL;

    ret_string = choco_string_create_from_c_string(name_, &tmp->name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = choco_string_result_convert(ret_string);
        ERROR_MESSAGE("texture_create(%s) - Failed to create texture name.", rslt_to_str(ret));
        goto cleanup;
    }

    *texture_ = tmp;
    ret = TEXTURE_SUCCESS;

cleanup:
    if(TEXTURE_SUCCESS != ret) {
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
    if(NULL == texture_) {
        return;
    }
    if(NULL == *texture_) {
        return;
    }
    if(NULL != (*texture_)->name) {
        choco_string_destroy(&(*texture_)->name);
    }
    if(NULL != (*texture_)->pixels) {
        texture_pixel_unload(*texture_);
    }
    memory_system_free(*texture_, sizeof(texture_t), MEMORY_TAG_TEXTURE);
    *texture_ = NULL;
}

texture_result_t texture_pixel_load(texture_t* texture_, const char* filepath_, const char* extension_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    uint8_t test_color = 0; // 0: red, 1: green, 2: blue
    bool use_test_texture = false;

    uint8_t* tmp_pixels = NULL;
    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_pixel_load", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->name, ret, TEXTURE_DATA_CORRUPTED, rslt_to_str(TEXTURE_DATA_CORRUPTED), "texture_pixel_load", "texture_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(texture_->pixels, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_load", "texture_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->channel_count, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_load", "0 != texture_->channel_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->width, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_load", "0 != texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->height, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_load", "0 != texture_->height")

    if(choco_string_equal("test_texture_red", choco_string_c_str(texture_->name))) {
        test_color = 0;
        use_test_texture = true;
    } else if(choco_string_equal("test_texture_green", choco_string_c_str(texture_->name))) {
        test_color = 1;
        use_test_texture = true;
    } else if(choco_string_equal("test_texture_blue", choco_string_c_str(texture_->name))) {
        test_color = 2;
        use_test_texture = true;
    } else {
        if(choco_string_equal(".bmp", extension_)) {
            ret = bmp_load(texture_, filepath_, extension_);
            if(TEXTURE_SUCCESS != ret) {
                ERROR_MESSAGE("texture_pixel_load(%s) - Failed to load bmp texture.", rslt_to_str(ret));
                goto cleanup;
            }
        } else {
            ret = TEXTURE_BAD_OPERATION;
            ERROR_MESSAGE("texture_pixel_load(%s) - Not implemented yet.", rslt_to_str(ret));
            goto cleanup;
        }
    }

    if(use_test_texture) {
        tmp_width = 512;
        tmp_height = 512;
        tmp_channel_count = 3;
        ret_mem = memory_system_allocate((size_t)tmp_width * (size_t)tmp_height * (size_t)tmp_channel_count, MEMORY_TAG_TEXTURE, (void**)&tmp_pixels);
        if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
            ret = memory_system_result_convert(ret_mem);
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to allocate memory for pixels.", rslt_to_str(ret));
            goto cleanup;
        }
        for(size_t i = 0, ii = 0; i != (512 * 512); ++i, ii += 3) {
            tmp_pixels[ii] = 0;
            tmp_pixels[ii + 1] = 0;
            tmp_pixels[ii + 2] = 0;
            tmp_pixels[ii + (size_t)test_color] = 255;
        }
        texture_->pixels = tmp_pixels;
        texture_->height = tmp_height;
        texture_->width = tmp_width;
        texture_->channel_count = tmp_channel_count;
    }

    ret = TEXTURE_SUCCESS;

cleanup:
    if(TEXTURE_SUCCESS != ret) {
        if(NULL != tmp_pixels) {
            memory_system_free(tmp_pixels, (size_t)tmp_width * (size_t)tmp_height * (size_t)tmp_channel_count, MEMORY_TAG_TEXTURE);
            tmp_pixels = NULL;
        }
    }
    return ret;
}

texture_result_t texture_pixel_unload(texture_t* texture_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_pixel_unload", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->name, ret, TEXTURE_DATA_CORRUPTED, rslt_to_str(TEXTURE_DATA_CORRUPTED), "texture_pixel_unload", "texture_->name")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->channel_count, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_unload", "texture_->channel_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->height, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_unload", "texture_->height")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->width, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_unload", "texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->pixels, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_unload", "texture_->pixels")

    memory_system_free(texture_->pixels, (size_t)texture_->width * (size_t)texture_->height * (size_t)texture_->channel_count, MEMORY_TAG_TEXTURE);
    texture_->pixels = NULL;
    texture_->channel_count = 0;
    texture_->width = 0;
    texture_->height = 0;

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

texture_result_t texture_pixel_get(texture_t* texture_, uint8_t** out_pixels_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_pixel_get", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(out_pixels_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_pixel_get", "out_pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->channel_count, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_get", "texture_->channel_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->pixels, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_get", "texture_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->width, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_get", "texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->height, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_get", "texture_->height")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->name, ret, TEXTURE_DATA_CORRUPTED, rslt_to_str(TEXTURE_DATA_CORRUPTED), "texture_pixel_get", "texture_->name")

    *out_pixels_ = texture_->pixels;

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

texture_result_t texture_pixel_size_get(texture_t* texture_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_pixel_size_get", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(width_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_pixel_size_get", "width_")
    IF_ARG_NULL_GOTO_CLEANUP(height_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_pixel_size_get", "height_")
    IF_ARG_NULL_GOTO_CLEANUP(channel_count_, ret, TEXTURE_INVALID_ARGUMENT, rslt_to_str(TEXTURE_INVALID_ARGUMENT), "texture_pixel_size_get", "channel_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->channel_count, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_size_get", "texture_->channel_count")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->pixels, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_size_get", "texture_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->width, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_size_get", "texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->height, ret, TEXTURE_BAD_OPERATION, rslt_to_str(TEXTURE_BAD_OPERATION), "texture_pixel_size_get", "texture_->height")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->name, ret, TEXTURE_DATA_CORRUPTED, rslt_to_str(TEXTURE_DATA_CORRUPTED), "texture_pixel_size_get", "texture_->name")

    *width_ = texture_->width;
    *height_ = texture_->height;
    *channel_count_ = texture_->channel_count;

    ret = TEXTURE_SUCCESS;

cleanup:
    return ret;
}

const char* texture_name_get(const texture_t* texture_) {
    if(NULL == texture_) {
        return NULL;
    }
    return choco_string_c_str(texture_->name);
}

// 仮実装
// TODO: 上下反転チェック
// TODO: 読み込みサイズエラーチェック
// TODO: 各種値のキャスト前チェック
static texture_result_t bmp_load(texture_t* texture_, const char* filepath_, const char* extension_) {
    texture_result_t ret = TEXTURE_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    filesystem_result_t ret_fs = FILESYSTEM_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    filesystem_t* filesystem = NULL;
    fs_utils_t* fs_utils = NULL;
    choco_string_t* fullpath = NULL;

    size_t read_size = 0;
    char header[54] = { 0 };
    int32_t image_size = 0;
    int32_t width = 0;
    int32_t height = 0;
    int16_t channel_count = 0;
    uint8_t* tmp_pixels = NULL;

    ret_fs_utils = fs_utils_create(filepath_, choco_string_c_str(texture_->name), extension_, FILESYSTEM_MODE_READ, &fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = fs_utils_result_convert(ret_fs_utils);
        ERROR_MESSAGE("bmp_load(%s) - Failed to create fs_utils.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_string = choco_string_default_create(&fullpath);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = choco_string_result_convert(ret_string);
        ERROR_MESSAGE("bmp_load(%s) - Failed to create fullpath.", rslt_to_str(ret));
        goto cleanup;
    }
    ret_fs_utils = fs_utils_fullpath_get(fs_utils, fullpath);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = fs_utils_result_convert(ret_fs_utils);
        ERROR_MESSAGE("bmp_load(%s) - Failed to create fullpath.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs = filesystem_create(&filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("bmp_load(%s) - Failed to create filesystem.", rslt_to_str(ret));
        goto cleanup;
    }
    ret_fs = filesystem_open(choco_string_c_str(fullpath), FILESYSTEM_MODE_READ, filesystem);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("bmp_load(%s) - Failed to open file(%s).", rslt_to_str(ret), choco_string_c_str(fullpath));
        goto cleanup;
    }

    ret_fs = filesystem_byte_read(54, filesystem, &read_size, header);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("bmp_load(%s) - Failed to load bmp header.", rslt_to_str(ret));
        goto cleanup;
    }

    if(header[0]!='B' || header[1]!='M'){
        ret = TEXTURE_DATA_CORRUPTED;
        ERROR_MESSAGE("bmp_load(%s) - Invalid bmp file format.", rslt_to_str(ret));
        goto cleanup;
    }

    image_size = buffer_utils_le_int32_t_get(&header[34]);
    width = buffer_utils_le_int32_t_get(&header[18]);
    height = buffer_utils_le_int32_t_get(&header[22]);
    channel_count = (uint8_t)(buffer_utils_le_uint16_t_get(&header[28]));
    if(24 == channel_count) {
        channel_count = 3;
    } else if(32 == channel_count) {
        channel_count = 4;
    } else {
        ret = TEXTURE_RUNTIME_ERROR;
        ERROR_MESSAGE("bmp_load(%s) - Unsupported bmp file format.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_mem = memory_system_allocate((size_t)width * (size_t)height * (size_t)channel_count, MEMORY_TAG_TEXTURE, (void**)&tmp_pixels);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = memory_system_result_convert(ret_mem);
        ERROR_MESSAGE("bmp_load(%s) - Failed to allocate memory for pixels.", rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs = filesystem_byte_read(image_size, filesystem, &read_size, (char*)tmp_pixels);
    if(FILESYSTEM_SUCCESS != ret_fs) {
        ret = filesystem_result_convert(ret_fs);
        ERROR_MESSAGE("bmp_load(%s) - Failed to load bmp file.", rslt_to_str(ret));
        goto cleanup;
    }
    // BMPファイルはBGRの順で格納されているので、RGBに変換
    for(size_t i = 0, ii = 0; i != (image_size / channel_count); ++i, ii += channel_count) {
        uint8_t tmp_r = tmp_pixels[ii + 2]; // Rコピー
        tmp_pixels[ii + 2] = tmp_pixels[ii];
        tmp_pixels[ii] = tmp_r;
    }

    choco_string_destroy(&fullpath);
    filesystem_destroy(&filesystem);
    fs_utils_destroy(&fs_utils);

    texture_->pixels = tmp_pixels;
    texture_->width = width;
    texture_->height = height;
    texture_->channel_count = channel_count;
    ret = TEXTURE_SUCCESS;

cleanup:
    if(TEXTURE_SUCCESS != ret) {
        choco_string_destroy(&fullpath);
        filesystem_destroy(&filesystem);
        fs_utils_destroy(&fs_utils);
        if(NULL != tmp_pixels) {
            memory_system_free(tmp_pixels, image_size, MEMORY_TAG_TEXTURE);
            tmp_pixels = NULL;
        }
    }
    return ret;
}

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
    case TEXTURE_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

static texture_result_t choco_string_result_convert(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return TEXTURE_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return TEXTURE_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return TEXTURE_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return TEXTURE_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return TEXTURE_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return TEXTURE_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return TEXTURE_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return TEXTURE_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return TEXTURE_LIMIT_EXCEEDED;
    default:
        return TEXTURE_UNDEFINED_ERROR;
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

static texture_result_t fs_utils_result_convert(fs_utils_result_t result_) {
    switch(result_) {
    case FS_UTILS_SUCCESS:
        return TEXTURE_SUCCESS;
    case FS_UTILS_INVALID_ARGUMENT:
        return TEXTURE_INVALID_ARGUMENT;
    case FS_UTILS_BAD_OPERATION:
        return TEXTURE_BAD_OPERATION;
    case FS_UTILS_DATA_CORRUPTED:
        return TEXTURE_DATA_CORRUPTED;
    case FS_UTILS_NO_MEMORY:
        return TEXTURE_NO_MEMORY;
    case FS_UTILS_LIMIT_EXCEEDED:
        return TEXTURE_LIMIT_EXCEEDED;
    case FS_UTILS_OVERFLOW:
        return TEXTURE_OVERFLOW;
    case FS_UTILS_FILE_OPEN_ERROR:
        return TEXTURE_FILE_OPEN_ERROR;
    case FS_UTILS_RUNTIME_ERROR:
        return TEXTURE_RUNTIME_ERROR;
    case FS_UTILS_UNDEFINED_ERROR:
        return TEXTURE_UNDEFINED_ERROR;
    default:
        return TEXTURE_UNDEFINED_ERROR;
    }
}
