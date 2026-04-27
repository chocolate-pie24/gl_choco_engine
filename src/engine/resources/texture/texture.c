#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/resources/texture/texture.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

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
static const char* s_rslt_str_undefined_error = "UNDEFINED_ERROR";      /**< 実行結果コード文字列: 未定義エラー */

static const char* rslt_to_str(texture_result_t rslt_);
static texture_result_t choco_string_result_convert(choco_string_result_t result_);
static texture_result_t memory_system_result_convert(memory_system_result_t result_);

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
        ret = TEXTURE_BAD_OPERATION;
        ERROR_MESSAGE("texture_pixel_load(%s) - Not implemented yet.", rslt_to_str(ret));
        goto cleanup;
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
    }

    texture_->pixels = tmp_pixels;
    texture_->height = tmp_height;
    texture_->width = tmp_width;
    texture_->channel_count = tmp_channel_count;
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
