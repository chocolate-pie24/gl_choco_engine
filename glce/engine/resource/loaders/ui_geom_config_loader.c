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
#include <inttypes.h>

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
/**
 * @brief `.ui_geom` fileから収集したconfiguration value textを保持するprivate intermediate state
 *
 * @details
 * 各fieldはexternal fileから取得したvalue tokenをNUL terminated stringとして保持する。
 *
 * 本typeはtrust promotion前のtemporary representationであり、
 * fieldが設定済みであることやnumeric / semantic validityを保証しない。
 *
 * empty stringは、そのconfiguration keyがまだ収集されていないstateを表す。
 */
typedef struct ui_geom_config_state {
    char icon_width[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
    char icon_height[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
} ui_geom_config_state_t;

// ============================================================
// Private Constants
// ============================================================
static const char* const s_key_str_icon_width = "icon_width";       /**< 設定値(矩形領域形状指定(幅): key文字列) */
static const char* const s_key_str_icon_height = "icon_height";     /**< 設定値(矩形領域形状指定(高さ): key文字列) */
static const char* const s_int64_scan_format = "%" SCNd64 " %c";

// ============================================================
// Private Function Declarations
// ============================================================
// Collection helpers
static resource_result_t config_state_collect(fs_stream_t* fs_stream_,  choco_string_t* line_string_, ui_geom_config_state_t* out_state_);
static resource_result_t line_collect(ui_geom_config_state_t* state_, const choco_string_t* line_);
static resource_result_t key_value_collect(ui_geom_config_state_t* state_, const char* line_, size_t line_length_);
static resource_result_t value_store(char* dst_, size_t dst_buffer_size_, const char* value_);

// Parsing helpers
static resource_result_t config_state_parse(const ui_geom_config_state_t* state_, ui_geom_config_t* out_config_);

// ============================================================
// Public API
// ============================================================
/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - config_fullpath_およびout_config_がNULLではないことをchecked preconditionとして検査する。
 * - config_fullpath_[0]がNULではないことを確認し、
 *   empty pathをRESOURCE_INVALID_ARGUMENTとしてrejectする。
 * - config_fullpath_が読み取り可能なNUL terminated stringを参照すること、および
 *   out_config_が書き込み可能なstorageを参照することは
 *   C pointerから検査できないため、caller側のtrusted / hard preconditionとして扱う。
 *
 * External representation validation:
 * - 本APIは`.ui_geom` fileに対するExternal Trust Boundaryを所有するため、
 *   external file contentのvalidationはRELEASE_BUILDを含む通常実行経路で行う。
 * - config_state_collect()によってline syntax、supported key、
 *   duplicate key等のfile-level representationを検査しながら
 *   temporary configuration stateを構築する。
 * - config_state_parse()によってrequired field、numeric representation、
 *   int32_tへの変換可能性を検査しながらtemporary ui_geom_config_tを構築する。
 * - external file由来のunsupported representationは
 *   established internal stateのcorruptionとは扱わず、
 *   RESOURCE_UNSUPPORTED_FILEとしてrejectする。
 *
 * Result validation:
 * - constructed temporary configはcaller-visible outputへcopyする前に
 *   ui_geom_config_is_valid()によってcanonical validationする。
 * - このvalidationは、external representationから構築されたcandidateを
 *   GLCE内部のtrusted ui_geom_config_tとしてadmissionする
 *   trust promotionの最終判定として行う。
 * - External Trust Boundaryで必要なsemantic validationであるため、
 *   BUILD_MODEによって省略しない。
 * - validation failureはuntrusted external inputのunsupported semanticとして扱い、
 *   RESOURCE_UNSUPPORTED_FILEを返す。
 *
 * Postcondition validation:
 * - out_config_へのcopy後にautomatic Postcondition validationは行わない。
 * - canonical validation済みのtemporary configをvalue copyするだけであり、
 *   copyによってsemantic validityは変化しないため、
 *   同一validationをcopy後に再実行しない。
 * - failure pathではOutputへ到達しないため、
 *   out_config_の既存内容は変更しない。
 *
 * DATA_CORRUPTED handling:
 * - subordinate moduleからRESOURCE_DATA_CORRUPTEDが伝播した場合は、
 *   normal cleanupによってcorrupted stateへ追加accessすることを避けるため、
 *   cleanup resourceのdestroy処理を実行しない。
 */
resource_result_t ui_geom_config_loader_load(const char* config_fullpath_, ui_geom_config_t* out_config_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;
    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;
    choco_string_t* line_string = NULL;

    ui_geom_config_state_t tmp_state = { 0 };
    ui_geom_config_t tmp_config = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "ui_geom_config_loader_load", "config_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "ui_geom_config_loader_load", "out_config_")
    if('\0' == config_fullpath_[0]) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Provided config_fullpath_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    // リソース確保
    ret_fs_stream = fs_stream_create(&fs_stream, config_fullpath_, FS_OPEN_MODE_READ);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_result_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=fs_stream_create, config_path='%s'", resource_result_to_str(ret), config_fullpath_);
        goto cleanup;
    }
    ret_choco_string = choco_string_default_create(&line_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Failed to load ui geometry config. reason=choco_string_default_create_failed(line_string).", resource_result_to_str(ret));
        goto cleanup;
    }

    // 設定値文字列の収集
    ret = config_state_collect(fs_stream, line_string, &tmp_state);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - config_state_collect failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    // 設定値文字列のパース
    ret = config_state_parse(&tmp_state, &tmp_config);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - config_state_parse failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Result validation.
    if(!ui_geom_config_is_valid(&tmp_config)) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("ui_geom_config_loader_load(%s) - Invalid config.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_config_ = tmp_config;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != line_string) {
            choco_string_destroy(&line_string);
        }
        if(NULL != fs_stream) {
            fs_stream_destroy(&fs_stream, NULL);
        }
    }
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * - 本API自身がui_geom_config_tのexplicit canonical validatorであるため、
 *   別のcanonical validationは実行しない。
 * - config_ == NULLはinvalid representationとしてfalseを返す。
 * - icon_width > 0およびicon_height > 0を
 *   ui_geom_config_tのcanonical semantic validityとして検査する。
 * - explicit validatorであるため、BUILD_MODEによってvalidation scopeを変更しない。
 * - 本validatorはtypedなui_geom_config_tだけを対象とし、
 *   source `.ui_geom` fileのsyntax、required key、duplicate key等は検査しない。
 * - validation中に対象stateを変更しないため、
 *   Result validationおよびPostcondition validationは必要としない。
 */
bool ui_geom_config_is_valid(const ui_geom_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 >= config_->icon_height || 0 >= config_->icon_width) {
        return false;
    }
    return true;
}

// ============================================================
// Collection helpers
// ============================================================
/*
 * config_state_collect() Contract
 *
 * Preconditions:
 * - fs_stream_、line_string_、out_state_はNULLではない。
 * - fs_stream_はtext fileの読み取りに使用可能なstateである。
 * - line_string_はfs_stream_text_file_line_read()のoutputとして
 *   使用可能なchoco_string_tである。
 * - out_state_は書き込み可能なui_geom_config_state_tを参照する。
 *
 * Responsibility:
 * - fs_stream_からlogical lineをEOFまで順次読み取る。
 * - 各lineをline_collect()へ渡し、supported configuration valueを
 *   out_state_へ収集する。
 *
 * Postconditions:
 * - RESOURCE_SUCCESSの場合、fileはEOFまで走査され、
 *   file内で受理されたsupported keyのvalueがout_state_へ収集されている。
 * - RESOURCE_SUCCESSはrequired keyがすべて存在することや、
 *   collected valueのnumeric / semantic validityを保証しない。
 * - failure時、out_state_は途中まで更新されている場合がある。
 *
 * Validation:
 * - line_countのincrementがsize_t overflowしないことを確認する。
 *   diagnostic用line countをboundedに更新するために必要である。
 * - fs_stream_text_file_line_read()のfailureはResource layer resultへ変換して返す。
 * - line contentのformat validationはline_collect()以下へ委譲する。
 */
static resource_result_t config_state_collect(fs_stream_t* fs_stream_,  choco_string_t* line_string_, ui_geom_config_state_t* out_state_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;

    bool complete = false;
    size_t line_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(fs_stream_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "config_state_collect", "fs_stream_")
    IF_ARG_NULL_GOTO_CLEANUP(line_string_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "config_state_collect", "line_string_")
    IF_ARG_NULL_GOTO_CLEANUP(out_state_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "config_state_collect", "out_state_")

    while(!complete) {
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, line_string_);
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

        ret = line_collect(out_state_, line_string_);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("ui_geom_config_loader_load(%s) - line_collect failed.", resource_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

/*
 * line_collect() Contract
 *
 * Preconditions:
 * - state_およびline_はNULLではない。
 * - line_は読み取り可能なvalid choco_string_tである。
 * - state_は書き込み可能なui_geom_config_state_tを参照する。
 *
 * Responsibility:
 * - logical lineをGLCE Config Utilityによって分類する。
 * - blank lineおよびcomment lineは無視する。
 * - key/value candidateはkey_value_collect()へ渡してcollection処理を行う。
 *
 * Postconditions:
 * - blank / comment lineを受理した場合、state_を変更せずRESOURCE_SUCCESSを返す。
 * - supported key/value lineのcollectionに成功した場合、
 *   対応するfieldがstate_へ反映される。
 * - invalid lineはRESOURCE_UNSUPPORTED_FILEとしてrejectする。
 *
 * Validation:
 * - glce_config_utility_line_type_get()によってlogical line representationを分類する。
 * - UI Geometry Configuration Formatで許可されないline typeを
 *   external file format violationとしてrejectするために行う。
 * - key/value syntaxおよびformat-specific key validationは
 *   key_value_collect()へ委譲する。
 */
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

/*
 * key_value_collect() Contract
 *
 * Preconditions:
 * - state_およびline_はNULLではない。
 * - line_length_は0ではない。
 * - line_[0]からline_[line_length_]までが読み取り可能であり、
 *   glce_config_utility_key_value_parse()へ渡せるlogical line representationである。
 * - state_は書き込み可能なui_geom_config_state_tを参照する。
 *
 * Responsibility:
 * - key/value lineをGLCE Config Utilityでparseする。
 * - parsed keyをUI Geometry Configuration Formatのsupported keyと照合する。
 * - supported keyに対応するvalueをstate_へ収集する。
 *
 * Postconditions:
 * - RESOURCE_SUCCESSの場合、supported keyに対応するstate_ fieldへ
 *   parsed valueが格納されている。
 * - unsupported key、duplicate key、またはunsupported key/value representationは
 *   trusted configuration stateとして受理しない。
 *
 * Validation:
 * - glce_config_utility_key_value_parse()によって
 *   common key/value lexical / syntactic representationを検査する。
 * - parsed keyが`icon_width`または`icon_height`のいずれかであることを確認する。
 *   `.ui_geom` formatで定義されていないkeyをrejectするために必要である。
 * - duplicate keyおよびdestination capacityのvalidationはvalue_store()へ委譲する。
 * - numeric valueのparseおよびsemantic range validationは
 *   config_state_parse()へ委譲する。
 */
static resource_result_t key_value_collect(ui_geom_config_state_t* state_, const char* line_, size_t line_length_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    glce_config_utility_result_t ret_glce_config_utility = GLCE_CONFIG_UTILITY_INVALID_ARGUMENT;

    glce_config_utility_key_value_t key_value = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "key_value_collect", "state_")
    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "key_value_collect", "line_")
    if(0 == line_length_) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("key_value_collect(%s) - Provided line_length_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_glce_config_utility = glce_config_utility_key_value_parse(line_, line_length_, &key_value);
    if(GLCE_CONFIG_UTILITY_SUCCESS != ret_glce_config_utility) {
        ret = resource_result_convert_glce_config_utility(ret_glce_config_utility);
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

/*
 * value_store() Contract
 *
 * Preconditions:
 * - dst_およびvalue_はNULLではない。
 * - dst_buffer_size_は0ではない。
 * - dst_はdst_buffer_size_ byte以上の書き込み可能なstorageを参照する。
 * - value_は読み取り可能なNUL terminated stringを参照する。
 *
 * Responsibility:
 * - configuration value textをdestination bufferへcopyする。
 * - destinationのexisting stateを利用してduplicate keyを検出する。
 *
 * Postconditions:
 * - RESOURCE_SUCCESSの場合、dst_はvalue_と同一内容の
 *   NUL terminated stringを保持する。
 * - failure時、dst_の内容は変更しない。
 *
 * Validation:
 * - dst_[0]がNULであることを確認する。
 *   non-empty destinationは同一keyが既に収集済みであることを表すため、
 *   duplicate keyとしてRESOURCE_UNSUPPORTED_FILEを返す。
 * - value lengthと終端NULがdst_buffer_size_内に収まることを確認する。
 *   fixed-size destinationへのbounded copyを保証するために必要である。
 */
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
    if('\0' != dst_[0]) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("value_store(%s) - Duplicate key.", resource_result_to_str(ret));
        goto cleanup;
    }
    value_length = strlen(value_);
    if((value_length + 1) > dst_buffer_size_) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("value_store(%s) - Buffer size error.", resource_result_to_str(ret));
        goto cleanup;
    }

    memcpy(dst_, value_, value_length + 1);

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Parsing helpers
// ============================================================
/*
 * config_state_parse() Contract
 *
 * Preconditions:
 * - state_およびout_config_はNULLではない。
 * - state_は読み取り可能なui_geom_config_state_tを参照する。
 * - out_config_は書き込み可能なui_geom_config_tを参照する。
 *
 * Responsibility:
 * - state_に収集されたicon_width / icon_heightのtext representationを
 *   integer valueへparseする。
 * - parse済みvalueからui_geom_config_t candidateを構築する。
 *
 * Postconditions:
 * - RESOURCE_SUCCESSの場合、
 *   out_config_->icon_widthおよびout_config_->icon_heightへ
 *   parse済みのint32_t valueを出力する。
 * - RESOURCE_SUCCESSはui_geom_config_tのcanonical semantic validityを保証しない。
 *   icon_width > 0およびicon_height > 0の最終validationは
 *   ui_geom_config_is_valid()へ委譲する。
 * - failure時、out_config_の既存内容は変更しない。
 *
 * Validation:
 * - icon_width / icon_heightの先頭characterがASCII digit ('0'..'9')であることを確認する。
 *   required keyが未収集のempty state、および'+' / '-' signを含む
 *   unsupported value representationをrejectするために行う。
 * - 各value textが10文字以下であることを確認する。
 *   `.ui_geom` formatで許可するint32_t decimal representationの
 *   最大桁数を超えるvalueをparse前にrejectするために行う。
 * - sscanf()によってvalue全体がdecimal integerとしてparse可能であり、
 *   integer以外の追加characterを含まないことを確認する。
 * - parse resultがINT32_MAX以下であることを確認する。
 *   ui_geom_config_tのint32_t fieldへ安全に変換できる範囲へ制限するために行う。
 *
 * - numeric valueが0より大きいことは本helperでは検査しない。
 *   これはui_geom_config_tのsemantic validityであり、
 *   public canonical validatorであるui_geom_config_is_valid()が所有する。
 */
static resource_result_t config_state_parse(const ui_geom_config_state_t* state_, ui_geom_config_t* out_config_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    char extra = '\0';
    int parse_result = 0;
    int64_t tmp_height = 0;
    int64_t tmp_width = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "config_state_parse", "state_")
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "config_state_parse", "out_config_")
    if(state_->icon_height[0] < '0' || state_->icon_height[0] > '9') {  // 文字列先頭の数値以外を拒否
        ret = RESOURCE_UNSUPPORTED_FILE;
        goto cleanup;
    }
    if(state_->icon_width[0] < '0' || state_->icon_width[0] > '9') {  // 文字列先頭の数値以外を拒否
        ret = RESOURCE_UNSUPPORTED_FILE;
        goto cleanup;
    }
    if(10 < strlen(state_->icon_height) || 10 < strlen(state_->icon_width)) {  // int32_tの最大桁数チェック
        ret = RESOURCE_UNSUPPORTED_FILE;
        goto cleanup;
    }

    // Prepare.
    parse_result = sscanf(state_->icon_height, s_int64_scan_format, &tmp_height, &extra);
    if(1 != parse_result) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("config_state_parse(%s) - parse failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    parse_result = sscanf(state_->icon_width, s_int64_scan_format, &tmp_width, &extra);
    if(1 != parse_result) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("config_state_parse(%s) - parse failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Result validation.
    if(INT32_MAX < tmp_height || INT32_MAX < tmp_width) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("config_state_parse(%s) - parse failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Output.
    out_config_->icon_height = (int32_t)tmp_height;
    out_config_->icon_width = (int32_t)tmp_width;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}
