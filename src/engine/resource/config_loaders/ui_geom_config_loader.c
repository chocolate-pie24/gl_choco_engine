/** @ingroup resource
 *
 * @file ui_geom_config_loader.c
 * @author chocolate-pie24
 * @brief UI描画用ジオメトリコンフィグレーションファイルのローダーAPI実装
 *
 * @version 0.1
 * @date 2026-06-30
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/resource/config_loaders/ui_geom_config_loader.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>  // for sscanf

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

/**
 * @brief コンフィグレーションロード用一時データ格納構造体
 *
 */
typedef struct ui_geom_config_state {
    ui_geom_config_t config;    /**< ロードした設定値を格納する構造体インスタンス */
    bool icon_width_is_valid;   /**< ロードした矩形領域形状指定値(幅)が有効 */
    bool icon_height_is_valid;  /**< ロードした矩形領域形状指定値(高さ)が有効 */
} ui_geom_config_state_t;

static bool key_is_valid(const choco_string_t* key_);
static void config_loader_initialize(ui_geom_config_state_t* out_loader_);
static resource_result_t line_parse(ui_geom_config_state_t* out_config_, const choco_string_t* line_, choco_string_t* tmp_key_, choco_string_t* tmp_value_);

static const char* const s_key_str_icon_width = "icon_width";       /**< 設定値(矩形領域形状指定(幅): key文字列) */
static const char* const s_key_str_icon_height = "icon_height";     /**< 設定値(矩形領域形状指定(高さ): key文字列) */

resource_result_t ui_geom_config_loader_load(const char* name_, ui_geom_config_t* out_config_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_utils_t* fs_utils = NULL;
    choco_string_t* line_string = NULL;
    choco_string_t* key_string = NULL;
    choco_string_t* value_string = NULL;

    ui_geom_config_state_t tmp_state = { 0 };

    size_t line_count = 0;

    bool complete = false;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_geom_config_loader_load", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ui_geom_config_loader_load", "out_config_")

    config_loader_initialize(&tmp_state);

    ret_fs_utils = fs_utils_create("assets/geometries/", name_, ".ui_geom", FILESYSTEM_MODE_READ, &fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = resource_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=fs_utils_create_failed, config_name='%s'", resource_rslt_to_str(ret), name_);
        goto cleanup;
    }

    ret_string = choco_string_default_create(&line_string);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=choco_string_default_create_failed(line_string), config_name='%s'", resource_rslt_to_str(ret), name_);
        goto cleanup;
    }

    ret_string = choco_string_default_create(&key_string);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=choco_string_default_create_failed(key_string), config_name='%s'", resource_rslt_to_str(ret), name_);
        goto cleanup;
    }

    ret_string = choco_string_default_create(&value_string);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=choco_string_default_create_failed(value_string), config_name='%s'", resource_rslt_to_str(ret), name_);
        goto cleanup;
    }

    while(!complete) {
        ret_fs_utils = fs_utils_text_file_line_read(fs_utils, line_string);
        if(FS_UTILS_EOF == ret_fs_utils) {
            complete = true;
        } else if(FS_UTILS_SUCCESS == ret_fs_utils) {
            if((SIZE_MAX - 1) < line_count) {
                ret = RESOURCE_OVERFLOW;
                ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=line_count_overflow, config_name='%s', line_count=%zu", resource_rslt_to_str(ret), name_, line_count);
                goto cleanup;
            }
            line_count++;

            ret = line_parse(&tmp_state, line_string, key_string, value_string);
            if(RESOURCE_SUCCESS != ret) {
                ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=line_parse_failed, config_name='%s', line=%zu, content='%s'", resource_rslt_to_str(ret), name_, line_count, choco_string_c_str(line_string));
                goto cleanup;
            }
        } else {
            if(FS_UTILS_RUNTIME_ERROR == ret_fs_utils || FS_UTILS_UNDEFINED_ERROR == ret_fs_utils) {
                ret = RESOURCE_FILE_READ_ERROR; // line_readのRUNTIME_ERROR, UNDEFINED_ERRORはREAD_ERRORに変換する
            } else {
                ret = resource_rslt_convert_fs_utils(ret_fs_utils);
            }
            ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=fs_utils_text_file_line_read_failed, config_name='%s', next_line=%zu", resource_rslt_to_str(ret), name_, line_count + 1);
            goto cleanup;
        }
    }

    if(!tmp_state.icon_height_is_valid || !tmp_state.icon_width_is_valid) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=required_field_missing, config_name='%s', icon_width_is_valid=%s, icon_height_is_valid=%s, icon_width=%u, icon_height=%u",
                    resource_rslt_to_str(ret),
                    name_,
                    tmp_state.icon_width_is_valid ? "true" : "false",
                    tmp_state.icon_height_is_valid ? "true" : "false",
                    (unsigned)tmp_state.config.icon_width,
                    (unsigned)tmp_state.config.icon_height);
        goto cleanup;
    }

    *out_config_ = tmp_state.config;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != fs_utils) {
        fs_utils_destroy(&fs_utils);
    }
    if(NULL != line_string) {
        choco_string_destroy(&line_string);
    }
    if(NULL != key_string) {
        choco_string_destroy(&key_string);
    }
    if(NULL != value_string) {
        choco_string_destroy(&value_string);
    }
    return ret;
}

/**
 * @brief コンフィグレーションファイルに記載されたkey=valueのkeyが有効な文字列かを判定する
 *
 * @param[in] key_ 判定対象key文字列
 *
 * @retval true key_が有効な文字列
 * @retval false 以下のいずれか
 * - key_ == NULL
 * - key_が規定値外の文字列
 */
static bool key_is_valid(const choco_string_t* key_) {
    if(NULL == key_) {
        return false;
    }
    if(choco_string_equal(s_key_str_icon_width, choco_string_c_str(key_))) {
        return true;
    } else if(choco_string_equal(s_key_str_icon_height, choco_string_c_str(key_))) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief ui_geom_config_state_t構造体インスタンスを初期化する
 *
 * @note out_loader_==NULLの場合は何もせずreturnする
 *
 * @param[out] out_loader_ 初期化対象ui_geom_config_state_t構造体インスタンスへのポインタ
 */
static void config_loader_initialize(ui_geom_config_state_t* out_loader_) {
    if(NULL == out_loader_) {
        return;
    }
    out_loader_->config.icon_height = 0;
    out_loader_->config.icon_width = 0;
    out_loader_->icon_height_is_valid = false;
    out_loader_->icon_width_is_valid = false;
}

/**
 * @brief 設定ファイルの各行をparseする
 *
 * @note コメント行等でconfig取得ができなかった場合も成功とする
 * @note keyはあるがvalueがない行については、現状では異常だが, 今後の機能拡張によって設定値が空を許可するkeyが出た時のために成功にする
 * @note key=value形式ではない行は無視される。ただし、未知keyを含むkey=value行は不正な設定行として扱う
 *
 * @param[in,out] out_config_ ui_geom_config_state_t構造体インスタンスへのポインタ
 * @param[in] line_ 設定ファイルから読み込んだ文字列1行分
 * @param[in,out] tmp_key_ key処理作業用choco_string_t構造体インスタンスへのポインタ
 * @param[in,out] tmp_value_ value処理作業用choco_string_t構造体インスタンスへのポインタ
 *
 * @retval 以下のいずれか
 * - line_ == NULL
 * - tmp_key_ == NULL
 * - tmp_value_ == NULL
 * - out_config_ == NULL
 * @retval RESOURCE_DATA_CORRUPTED 以下のいずれか
 * - 読み込んだkeyが有効な文字列ではない
 * - tmp_key_もしくはtmp_value_の内部状態破損
 * - valueの文字列に数字以外の無効な文字列が混じっている(例: 32abc)
 * - valueの値が0以下
 * @retval RESOURCE_OVERFLOW valueの値がUINT16_MAXを超過
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t line_parse(ui_geom_config_state_t* out_config_, const choco_string_t* line_, choco_string_t* tmp_key_, choco_string_t* tmp_value_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    int parse_result = 0;
    int width = 0;
    int height = 0;
    char extra = '\0';

    if(NULL == line_ || NULL == tmp_key_ || NULL == tmp_value_ || NULL == out_config_) {
        ret = RESOURCE_INVALID_ARGUMENT;
        goto cleanup;
    }

    ret_string = choco_string_key_value_key_get(choco_string_c_str(line_), tmp_key_);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        if(CHOCO_STRING_BAD_OPERATION == ret_string) {
            // コメント行等の場合にはここに来る, 正常として扱う
            ret = RESOURCE_SUCCESS;
            goto cleanup;
        } else {
            ret = resource_rslt_convert_choco_string(ret_string);
            goto cleanup;
        }
    }
    if(!key_is_valid(tmp_key_)) {
        ret = RESOURCE_DATA_CORRUPTED;
        goto cleanup;
    }

    ret_string = choco_string_key_value_value_get(choco_string_c_str(line_), tmp_value_);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        if(CHOCO_STRING_BAD_OPERATION == ret_string) {
            // 設定値がない場合は異常だが, 今後の機能拡張によって設定値が空を許可するkeyが出た時のために成功にする
            ret = RESOURCE_SUCCESS;
            goto cleanup;
        } else {
            ret = resource_rslt_convert_choco_string(ret_string);
            goto cleanup;
        }
    }

    if(choco_string_equal(s_key_str_icon_width, choco_string_c_str(tmp_key_))) {
        parse_result = sscanf(choco_string_c_str(tmp_value_), " %d %c", &width, &extra); // 数字以外の文字が入っていた場合を検出するためextraを追加する
        if(1 != parse_result) {
            ret = RESOURCE_DATA_CORRUPTED;
            goto cleanup;
        }
        if(0 >= width) {
            ret = RESOURCE_DATA_CORRUPTED;
            goto cleanup;
        }
        if(width > UINT16_MAX) {
            ret = RESOURCE_OVERFLOW;
            goto cleanup;
        }
        out_config_->config.icon_width = (uint16_t)width;
        out_config_->icon_width_is_valid = true;
    } else if(choco_string_equal(s_key_str_icon_height, choco_string_c_str(tmp_key_))) {
        parse_result = sscanf(choco_string_c_str(tmp_value_), " %d %c", &height, &extra);   // 数字以外の文字が入っていた場合を検出するためextraを追加する
        if(1 != parse_result) {
            ret = RESOURCE_DATA_CORRUPTED;
            goto cleanup;
        }
        if(0 >= height) {
            ret = RESOURCE_DATA_CORRUPTED;
            goto cleanup;
        }
        if(height > UINT16_MAX) {
            ret = RESOURCE_OVERFLOW;
            goto cleanup;
        }
        out_config_->config.icon_height = (uint16_t)height;
        out_config_->icon_height_is_valid = true;
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}
