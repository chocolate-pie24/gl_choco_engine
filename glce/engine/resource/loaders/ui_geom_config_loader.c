// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file ui_geom_config_loader.c
 * @author chocolate-pie24
 * @brief UI描画用ジオメトリコンフィグレーションファイルのローダーAPI実装
 *
 * @date 2026-06-30
 *
 */
#include "engine/resource/loaders/ui_geom_config_loader.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>  // for sscanf
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_stream.h"
#include "engine/io_utils/glce_config_utility.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

// ============================================================
// Private Type Definitions
// ============================================================

typedef struct ui_geom_config_state {
    char icon_width[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
    char icon_height[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
} ui_geom_config_state_t;

// ============================================================
// Private Constants
// ============================================================
static const char* const s_key_str_icon_width = "icon_width";       /**< 設定値(矩形領域形状指定(幅): key文字列) */
static const char* const s_key_str_icon_height = "icon_height";     /**< 設定値(矩形領域形状指定(高さ): key文字列) */

// ============================================================
// Private Function Declarations
// ============================================================
// Collection helpers
static resource_result_t config_state_collect(fs_stream_t* fs_stream_, ui_geom_config_state_t* out_state_);
static resource_result_t line_collect(ui_geom_config_state_t* state_, const choco_string_t* line_);
static resource_result_t key_value_collect(ui_geom_config_state_t* state_, const char* line_, size_t line_length_);
static resource_result_t value_store(char* dst_, size_t dst_buffer_size_, const char* value_);

// Parsing helpers
static resource_result_t config_state_parse(const ui_geom_config_state_t* state_, ui_geom_config_t* out_config_);

// Validators
static bool key_is_valid(const choco_string_t* key_);

// ============================================================
// Public API
// ============================================================
resource_result_t ui_geom_config_loader_load(const char* config_fullpath_, ui_geom_config_t* out_config_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;

    ui_geom_config_state_t tmp_state = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(config_fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "ui_geom_config_loader_load", "config_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "ui_geom_config_loader_load", "out_config_")
    if('\0' == config_fullpath_[0]) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Provided config_fullpath_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_stream = fs_stream_create(&fs_stream, config_fullpath_, FS_OPEN_MODE_READ);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_result_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=fs_stream_create, config_path='%s'", resource_result_to_str(ret), config_fullpath_);
        goto cleanup;
    }
    ret = config_state_collect(fs_stream, &tmp_state);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - config_state_collect failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    fs_stream_destroy(&fs_stream, NULL);

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != fs_stream) {
            fs_stream_destroy(&fs_stream, NULL);
        }
    }
    return ret;
}

// ============================================================
// Collection helpers
// ============================================================
static resource_result_t config_state_collect(fs_stream_t* fs_stream_, ui_geom_config_state_t* out_state_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;
    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* line_string = NULL;

    bool complete = false;
    size_t line_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(fs_stream_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "config_state_collect", "fs_stream_")
    IF_ARG_NULL_GOTO_CLEANUP(out_state_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "config_state_collect", "out_state_")

    ret_choco_string = choco_string_default_create(&line_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=choco_string_default_create_failed(line_string).", resource_result_to_str(ret));
        goto cleanup;
    }

    while(!complete) {
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, line_string);
        if(FS_STREAM_EOF == ret_fs_stream) {
            complete = true;
            break;
        }
        if(FS_STREAM_SUCCESS != ret_fs_stream) {
            if(FS_STREAM_RUNTIME_ERROR == ret_fs_stream || FS_STREAM_UNDEFINED_ERROR == ret_fs_stream) {
                ret = RESOURCE_FILE_READ_ERROR; // line_readのRUNTIME_ERROR, UNDEFINED_ERRORはREAD_ERRORに変換する
            } else {
                ret = resource_result_convert_fs_stream(ret_fs_stream);
            }
            ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=fs_stream_text_file_line_read_failed, next_line=%zu", resource_result_to_str(ret), line_count + 1);
            goto cleanup;
        }

        if((SIZE_MAX - 1) < line_count) {
            ret = RESOURCE_OVERFLOW;
            ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=line_count_overflow, line_count=%zu", resource_result_to_str(ret), line_count);
            goto cleanup;
        }
        line_count++;

        ret = line_collect(out_state_, line_string);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("ui_geom_config_loader_load(%s) - line_collect failed.", resource_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        choco_string_destroy(&line_string);
    }
    return ret;
}

static resource_result_t line_collect(ui_geom_config_state_t* state_, const choco_string_t* line_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    glce_config_utility_line_type_t line_type = GLCE_CONFIG_UTILITY_LINE_TYPE_INVALID;

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "line_collect", "state_")
    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "line_collect", "line_")

    line_type = glce_config_utility_line_type_get(choco_string_c_str(line_), choco_string_length(line_));
    switch(line_type) {
    case GLCE_CONFIG_UTILITY_LINE_TYPE_BLANK:
        ret = RESOURCE_SUCCESS;
        break;
    case GLCE_CONFIG_UTILITY_LINE_TYPE_COMMENT:
        ret = RESOURCE_SUCCESS;
        break;
    case GLCE_CONFIG_UTILITY_LINE_TYPE_KEY_VALUE:
        ret = key_value_collect(state_, choco_string_c_str(line_), choco_string_length(line_));
        break;
    case GLCE_CONFIG_UTILITY_LINE_TYPE_INVALID:
        ret = RESOURCE_UNSUPPORTED_FILE;
        break;
    default:
        ret = RESOURCE_UNDEFINED_ERROR;
        break;
    }

cleanup:
    return ret;
}

static resource_result_t key_value_collect(ui_geom_config_state_t* state_, const char* line_, size_t line_length_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    glce_config_utility_key_value_t key_value = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "key_value_collect", "state_")
    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "key_value_collect", "line_")
    if(0 == line_length_) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("key_value_collect(%s) - Provided line_length_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }

    if(!glce_config_utility_key_value_parse(line_, line_length_, &key_value)) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("key_value_collect(%s) - glce_config_utility_key_value_parse failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    if(choco_string_is_equal(s_key_str_icon_height, key_value.key)) {
        ret = value_store(state_->icon_height, GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE, key_value.value);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("key_value_collect(%s) - value_store failed. key = %s, value = %s.", resource_result_to_str(ret), key_value.key, key_value.value);
            goto cleanup;
        }
    } else if(choco_string_is_equal(s_key_str_icon_width, key_value.key)) {
        ret = value_store(state_->icon_width, GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE, key_value.value);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("key_value_collect(%s) - value_store failed. key = %s, value = %s.", resource_result_to_str(ret), key_value.key, key_value.value);
            goto cleanup;
        }
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("key_value_collect(%s) - Undefined key. key = %s", resource_result_to_str(ret), key_value.key);
        goto cleanup;
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

static resource_result_t value_store(char* dst_, size_t dst_buffer_size_, const char* value_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    size_t value_length = 0;

    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "value_store", "dst_")
    IF_ARG_NULL_GOTO_CLEANUP(value_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "value_store", "value_")
    if(0 == dst_buffer_size_) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("value_store(%s) - Provided dst_buffer_size_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }
    if('\0' == dst_[0]) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("key_value_collect(%s) - Duplicate key.", resource_result_to_str(ret));
        goto cleanup;
    }
    value_length = strlen(value_);
    if((value_length + 1) > dst_buffer_size_) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("key_value_collect(%s) - Buffer size error.", resource_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != value_length; ++i) {
        dst_[i] = value_[i];
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Validators
// ============================================================
static bool key_is_valid(const choco_string_t* key_) {
    return true;
}
