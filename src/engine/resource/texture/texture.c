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

typedef enum {
    TEST_TEXTURE_RED,
    TEST_TEXTURE_GREEN,
    TEST_TEXTURE_BLUE,
} test_texture_t;

struct texture {
    choco_string_t* name;   /**< テクスチャCPU側リソース名称 */

    uint16_t width;
    uint16_t height;
    uint8_t channel_count;

    uint8_t* pixels;
};

static resource_result_t bmp_load(texture_t* texture_, const char* filepath_, const char* extension_);

static resource_result_t test_texture_generate(test_texture_t test_texture_color_, texture_t* texture_);

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>

#include "test_controller.h"

#include "engine/resource/texture/test_texture.h"

// texture用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_texture_create;            /**< texture_create()テスト設定 */
static test_call_control_t s_test_config_texture_pixel_load;        /**< texture_pixel_load()テスト設定 */
static test_call_control_t s_test_config_texture_pixel_unload;      /**< texture_pixel_unload()テスト設定 */
static test_call_control_t s_test_config_texture_pixel_get;         /**< texture_pixel_get()テスト設定 */
static test_call_control_t s_test_config_texture_pixel_size_get;    /**< texture_pixel_size_get()テスト設定 */

// プライベート関数テスト設定
static test_call_control_t s_test_config_bmp_load;                  /**< bmp_load()テスト設定 */
static test_call_control_t s_test_config_test_texture_generage;     /**< test_texture_generate()テスト設定 */

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

    uint8_t* tmp_pixels = NULL;
    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "texture_pixel_load", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->name, ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "texture_pixel_load", "texture_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(texture_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_load", "texture_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->channel_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_load", "0 != texture_->channel_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_load", "0 != texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_load", "0 != texture_->height")

    if(choco_string_equal("test_texture_red", choco_string_c_str(texture_->name))) {
        ret = test_texture_generate(TEST_TEXTURE_RED, texture_);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to create red test texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal("test_texture_green", choco_string_c_str(texture_->name))) {
        ret = test_texture_generate(TEST_TEXTURE_GREEN, texture_);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to create green test texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal("test_texture_blue", choco_string_c_str(texture_->name))) {
        ret = test_texture_generate(TEST_TEXTURE_BLUE, texture_);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to create blue test texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else if(choco_string_equal(".bmp", extension_)) {
        ret = bmp_load(texture_, filepath_, extension_);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("texture_pixel_load(%s) - Failed to load BMP texture.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("texture_pixel_load(%s) - Unsupported file type.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_pixels) {
            memory_system_free(tmp_pixels, (size_t)tmp_width * (size_t)tmp_height * (size_t)tmp_channel_count, MEMORY_TAG_TEXTURE);
            tmp_pixels = NULL;
        }
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
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "texture_pixel_unload", "texture_->pixels")

    memory_system_free(texture_->pixels, (size_t)texture_->width * (size_t)texture_->height * (size_t)texture_->channel_count, MEMORY_TAG_TEXTURE);
    texture_->pixels = NULL;
    texture_->channel_count = 0;
    texture_->width = 0;
    texture_->height = 0;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t texture_pixel_get(texture_t* texture_, uint8_t** out_pixels_) {
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

resource_result_t texture_pixel_size_get(texture_t* texture_, uint16_t* width_, uint16_t* height_, uint8_t* channel_count_) {
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
    return choco_string_c_str(texture_->name);
}

static resource_result_t bmp_load(texture_t* texture_, const char* filepath_, const char* extension_) {
#ifdef TEST_BUILD
    s_test_config_bmp_load.call_count++;
    if(s_test_config_bmp_load.fail_on_call != 0) {
        if(s_test_config_bmp_load.call_count == s_test_config_bmp_load.fail_on_call) {
            return (resource_result_t)s_test_config_bmp_load.forced_result;
        }
    }
#endif
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_utils_t* fs_utils = NULL;
    choco_string_t* fullpath = NULL;
    bmp_loader_t* bmp_loader = NULL;

    uint16_t tmp_width = 0;
    uint16_t tmp_height = 0;
    uint8_t tmp_channel_count = 0;
    uint8_t* tmp_pixels = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_load", "texture_")
    IF_ARG_NULL_GOTO_CLEANUP(filepath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_load", "filepath_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "bmp_load", "extension_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->channel_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_load", "texture_->channel_count")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(texture_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_load", "texture_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_load", "texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "bmp_load", "texture_->height")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->name, ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "bmp_load", "texture_->name")

    ret_fs_utils = fs_utils_create(filepath_, choco_string_c_str(texture_->name), extension_, FILESYSTEM_MODE_READ, &fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = resource_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("bmp_load(%s) - Failed to create fs_utils.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_string = choco_string_default_create(&fullpath);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("bmp_load(%s) - Failed to create fullpath.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_utils = fs_utils_fullpath_get(fs_utils, fullpath);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = resource_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("bmp_load(%s) - Failed to create fullpath.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = bmp_loader_create(&bmp_loader);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_load(%s) - Failed to create bmp_loader.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = bmp_loader_load(choco_string_c_str(fullpath), bmp_loader);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_load(%s) - Failed to load BMP file(%s).", resource_rslt_to_str(ret), choco_string_c_str(fullpath));
        goto cleanup;
    }

    ret = bmp_loader_bmp_size_get(bmp_loader, &tmp_width, &tmp_height, &tmp_channel_count);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_load(%s) - Failed to get BMP size(%s).", resource_rslt_to_str(ret), choco_string_c_str(fullpath));
        goto cleanup;
    }

    ret = bmp_loader_pixel_move(bmp_loader, &tmp_pixels);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("bmp_load(%s) - Failed to move BMP pixels.", resource_rslt_to_str(ret), choco_string_c_str(fullpath));
        goto cleanup;
    }

    bmp_loader_destroy(&bmp_loader);
    choco_string_destroy(&fullpath);
    fs_utils_destroy(&fs_utils);

    texture_->pixels = tmp_pixels;
    texture_->width = tmp_width;
    texture_->height = tmp_height;
    texture_->channel_count = tmp_channel_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        bmp_loader_destroy(&bmp_loader);
        choco_string_destroy(&fullpath);
        fs_utils_destroy(&fs_utils);
    }
    return ret;
}

static resource_result_t test_texture_generate(test_texture_t test_texture_color_, texture_t* texture_) {
#ifdef TEST_BUILD
    s_test_config_test_texture_generage.call_count++;
    if(s_test_config_test_texture_generage.fail_on_call != 0) {
        if(s_test_config_test_texture_generage.call_count == s_test_config_test_texture_generage.fail_on_call) {
            return (resource_result_t)s_test_config_test_texture_generage.forced_result;
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

    IF_ARG_NULL_GOTO_CLEANUP(texture_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "test_texture_create", "texture_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->channel_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "test_texture_create", "texture_->channel_count")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(texture_->pixels, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "test_texture_create", "texture_->pixels")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->width, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "test_texture_create", "texture_->width")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == texture_->height, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "test_texture_create", "texture_->height")
    IF_ARG_NULL_GOTO_CLEANUP(texture_->name, ret, RESOURCE_DATA_CORRUPTED, resource_rslt_to_str(RESOURCE_DATA_CORRUPTED), "test_texture_create", "texture_->name")

    ret_mem = memory_system_allocate(pixel_size, MEMORY_TAG_TEXTURE, (void**)&tmp_pixels);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("texture_pixel_load(%s) - Failed to allocate memory for pixels.", resource_rslt_to_str(ret));
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
        ERROR_MESSAGE("test_texture_create(%s) - Invalid test_texture_color_.", resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT));
        goto cleanup;
    }
    for(size_t i = 0, ii = 0; i != (tmp_width * tmp_height); ++i, ii += 3) {
        tmp_pixels[ii] = 0;
        tmp_pixels[ii + 1] = 0;
        tmp_pixels[ii + 2] = 0;
        tmp_pixels[ii + index] = 255;
    }
    texture_->pixels = tmp_pixels;
    texture_->height = tmp_height;
    texture_->width = tmp_width;
    texture_->channel_count = tmp_channel_count;

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
    test_call_control_reset(&s_test_config_test_texture_generage);
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

static void NO_COVERAGE test_texture_create(void) {

}

static void NO_COVERAGE test_texture_destroy(void) {

}

static void NO_COVERAGE test_texture_pixel_load(void) {

}

static void NO_COVERAGE test_texture_pixel_unload(void) {

}

static void NO_COVERAGE test_texture_pixel_get(void) {

}

static void NO_COVERAGE test_texture_pixel_size_get(void) {

}

static void NO_COVERAGE test_texture_name_get(void) {

}

static void NO_COVERAGE test_bmp_load(void) {

}

static void NO_COVERAGE test_test_texture_generate(void) {

}
#endif
