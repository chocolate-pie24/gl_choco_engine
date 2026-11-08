// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/loaders/untextured_mesh_material_config_loader.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>  // for sscanf
#include <string.h>
#include <math.h>   // for isfinite

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_stream.h"
#include "engine/io_utils/glce_config_utility.h"

#include "engine/systems/renderer/resources/loaders/core/loader_types.h"
#include "engine/systems/renderer/resources/loaders/core/loader_err_utils.h"

// ============================================================
// Private Type Definitions
// ============================================================
/**
 * @brief Untextured Mesh Material configurationの一時収集state
 *
 * @details
 * 各fieldはexternal fileから取得したvalue tokenを
 * NUL terminated stringとして保持する。
 *
 * 本typeはtrust promotion前のtemporary representationであり、
 * required fieldの充足、numeric representation、
 * Material configurationとしてのsemantic validityを保証しない。
 *
 * Representation:
 * - 初期状態ではすべてのfieldがempty stringである。
 * - empty stringは、そのconfiguration keyが未収集であることを表す。
 * - 収集済みfieldには、buffer capacity内に収まる
 *   NUL terminated value tokenを保持する。
 * - 一度収集したfieldは再設定しない。
 */
typedef struct untextured_mesh_material_config_state {
    char ambient[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
    char diffuse[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
    char specular[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
    char shininess[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
} untextured_mesh_material_config_state_t;

// ============================================================
// Private Constants
// ============================================================
static const char* const s_key_str_ambient = "ambient";
static const char* const s_key_str_diffuse = "diffuse";
static const char* const s_key_str_specular = "specular";
static const char* const s_key_str_shininess = "shininess";

static const char* const s_vec3f_scan_format = " [ %f , %f , %f ] %n";
static const char* const s_float_scan_format = "%f%n";

// ============================================================
// Private Function Declarations
// ============================================================
// Collection helpers
static loader_result_t config_state_collect(fs_stream_t* fs_stream_,  choco_string_t* line_string_, untextured_mesh_material_config_state_t* out_state_);
static loader_result_t line_collect(untextured_mesh_material_config_state_t* state_, const choco_string_t* line_);
static loader_result_t key_value_collect(untextured_mesh_material_config_state_t* state_, const char* line_, size_t line_length_);
static loader_result_t value_store(char* dst_, size_t dst_buffer_size_, const char* value_);

// Parsing helpers
static loader_result_t config_state_parse(const untextured_mesh_material_config_state_t* state_, untextured_mesh_material_config_t* out_config_);

// Validators
static bool diffuse_is_valid(vec3f_t value_);
static bool ambient_is_valid(vec3f_t value_);
static bool specular_is_valid(vec3f_t value_);
static bool shininess_is_valid(float value_);

// ============================================================
// Public API
// ============================================================
/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - config_fullpath_およびout_config_がNULLではないことを
 *   checked preconditionとして検査する。
 * - config_fullpath_[0]がNULではないことを確認し、
 *   empty pathをLOADER_INVALID_ARGUMENTとしてrejectする。
 * - config_fullpath_が読み取り可能なNUL terminated stringを参照すること、
 *   out_config_が書き込み可能なstorageを参照することは、
 *   C pointerから検査できないためcaller側のtrusted / hard preconditionとする。
 *
 * External representation validation:
 * - 本APIはExternal Trust Boundary ownerであるため、
 *   external file contentのrequired validationを全Buildで実行する。
 * - config_state_collect()によってline syntax、supported key、
 *   duplicate key等を検査しながらtemporary stateを構築する。
 * - config_state_parse()によってdiffuseの必須指定と、
 *   指定されたfieldのnumeric syntaxを検査する。
 * - 省略されたOptional Fieldには規定のデフォルト値を適用し、
 *   temporary material configurationを構築する。
 * - 現行のsscanf()によるfloat変換では、表現範囲外入力の事前検証を行わない。
 *   この制約はheaderのTemporary Numeric Input Contractに従う。
 * - external file由来のunsupported representationは
 *   established internal stateのcorruptionとして扱わず、
 *   LOADER_UNSUPPORTED_FILEとしてrejectする。
 *
 * Result validation:
 * - temporary configurationをcaller-visible outputへ公開する前に、
 *   untextured_mesh_material_config_is_valid()でcanonical validationする。
 * - このvalidationは、numeric syntaxが成立したcandidateについて
 *   Material固有のsemantic validityを確認し、
 *   trusted representationとして受理するために行う。
 * - External Trust Boundaryでのtrust promotionに必要なため、
 *   BUILD_MODEによって省略しない。
 * - validation failureはunsupported external representationとして扱い、
 *   LOADER_UNSUPPORTED_FILEを返す。
 *
 * Postcondition validation:
 * - out_config_へのcopy後にautomatic Postcondition validationは行わない。
 * - validation済みtemporary configurationをvalue copyするだけであり、
 *   copyによってsemantic validityは変化しないため、
 *   同一validationを再実行しない。
 * - failure pathではOutputへ到達しないため、
 *   out_config_の既存内容は変更しない。
 *
 * DATA_CORRUPTED handling:
 * - subordinate moduleからLOADER_DATA_CORRUPTEDが伝播した場合、
 *   corrupted stateへ追加accessすることを避けるため、
 *   temporary resourceの通常destroy処理を実行しない。
 */
loader_result_t untextured_mesh_material_config_loader_load(const char* config_fullpath_, untextured_mesh_material_config_t* out_config_) {
    loader_result_t ret = LOADER_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;
    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;
    choco_string_t* line_string = NULL;

    untextured_mesh_material_config_state_t tmp_state = { 0 };
    untextured_mesh_material_config_t tmp_config = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_fullpath_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "untextured_mesh_material_config_loader_load", "config_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "untextured_mesh_material_config_loader_load", "out_config_")
    if('\0' == config_fullpath_[0]) {
        ret = LOADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_material_config_loader_load(%s) - Provided config_fullpath_ is not valid.", loader_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    // リソース確保
    ret_fs_stream = fs_stream_create(&fs_stream, config_fullpath_, FS_OPEN_MODE_READ);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = loader_result_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("untextured_mesh_material_config_loader_load(%s) - Failed to load untextured mesh material config. reason=fs_stream_create, config_path='%s'", loader_result_to_str(ret), config_fullpath_);
        goto cleanup;
    }
    ret_choco_string = choco_string_default_create(&line_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = loader_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("untextured_mesh_material_config_loader_load(%s) - Failed to load untextured mesh material config. reason=choco_string_default_create_failed(line_string).", loader_result_to_str(ret));
        goto cleanup;
    }

    // 設定値文字列の収集
    ret = config_state_collect(fs_stream, line_string, &tmp_state);
    if(LOADER_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_mesh_material_config_loader_load(%s) - config_state_collect failed.", loader_result_to_str(ret));
        goto cleanup;
    }
    // 設定値文字列のパース
    ret = config_state_parse(&tmp_state, &tmp_config);
    if(LOADER_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_mesh_material_config_loader_load(%s) - config_state_parse failed.", loader_result_to_str(ret));
        goto cleanup;
    }

    // Result validation.
    if(!untextured_mesh_material_config_is_valid(&tmp_config)) {
        ret = LOADER_UNSUPPORTED_FILE;
        ERROR_MESSAGE("untextured_mesh_material_config_loader_load(%s) - Invalid config.", loader_result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_config_ = tmp_config;

    ret = LOADER_SUCCESS;

cleanup:
    if(LOADER_DATA_CORRUPTED != ret) {
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
 * - 本API自身がuntextured_mesh_material_config_tのexplicit canonical validatorであるため、
 *   別のcanonical validationは実行しない。
 * - config_ == NULLはinvalid representationとしてfalseを返す。
 * - 各Material propertyのprivate validatorを使用し、
 *   finite性およびMaterial固有のnumeric rangeを検査する。
 * - explicit validatorであるため、
 *   BUILD_MODEによってvalidation scopeを変更しない。
 * - 本validatorはtypedなuntextured_mesh_material_config_tを対象とし、
 *   external fileのsyntax、required key、duplicate key等は検査しない。
 * - 対象stateのmutationやresult candidateの構築を行わないため、
 *   Result validationおよびPostcondition validationは必要としない。
 */
bool untextured_mesh_material_config_is_valid(const untextured_mesh_material_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(!diffuse_is_valid(config_->diffuse) || !ambient_is_valid(config_->ambient) || !specular_is_valid(config_->specular) || !shininess_is_valid(config_->shininess)) {
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
 * - out_state_は初期化済みの書き込み可能なtemporary stateである。
 *   未収集fieldはempty stringとして表現される。
 *
 * Responsibility:
 * - fs_stream_からlogical lineをEOFまで順次読み取る。
 * - 各lineをline_collect()へ渡し、supported configuration valueを
 *   out_state_へ収集する。
 *
 * Postconditions:
 * - LOADER_SUCCESSの場合、fileはEOFまで走査され、
 *   supported keyのvalueがout_state_へ収集されている。
 * - この段階ではrequired fieldの充足やnumeric / semantic validityを保証しない。
 * - failure時、out_state_は途中まで更新されている場合がある。
 *
 * Validation:
 * - line_countがsize_t overflowしないことを確認する。
 * - file読み取りのfailureをLoader resultへ変換して返す。
 * - line contentのformat validationはline_collect()以下へ委譲する。
 */
static loader_result_t config_state_collect(fs_stream_t* fs_stream_,  choco_string_t* line_string_, untextured_mesh_material_config_state_t* out_state_) {
    loader_result_t ret = LOADER_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;

    bool complete = false;
    size_t line_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(fs_stream_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "config_state_collect", "fs_stream_")
    IF_ARG_NULL_GOTO_CLEANUP(line_string_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "config_state_collect", "line_string_")
    IF_ARG_NULL_GOTO_CLEANUP(out_state_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "config_state_collect", "out_state_")

    while(!complete) {
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, line_string_);
        if(FS_STREAM_EOF == ret_fs_stream) {
            complete = true;
            break;
        }
        if(FS_STREAM_SUCCESS != ret_fs_stream) {
            if(FS_STREAM_RUNTIME_ERROR == ret_fs_stream || FS_STREAM_UNDEFINED_ERROR == ret_fs_stream) {
                ret = LOADER_FILE_READ_ERROR; // line_readのRUNTIME_ERROR, UNDEFINED_ERRORはREAD_ERRORに変換する
            } else {
                ret = loader_result_convert_fs_stream(ret_fs_stream);
            }
            ERROR_MESSAGE("config_state_collect(%s) - Failed to load untextured mesh material config. reason=fs_stream_text_file_line_read_failed, next_line=%zu", loader_result_to_str(ret), line_count + 1);
            goto cleanup;
        }

        if((SIZE_MAX - 1) < line_count) {
            ret = LOADER_OVERFLOW;
            ERROR_MESSAGE("config_state_collect(%s) - Failed to load untextured mesh material config. reason=line_count_overflow, line_count=%zu", loader_result_to_str(ret), line_count);
            goto cleanup;
        }
        line_count++;

        ret = line_collect(out_state_, line_string_);
        if(LOADER_SUCCESS != ret) {
            ERROR_MESSAGE("config_state_collect(%s) - line_collect failed.", loader_result_to_str(ret));
            goto cleanup;
        }
    }

    ret = LOADER_SUCCESS;

cleanup:
    return ret;
}

/*
 * line_collect() Contract
 *
 * Preconditions:
 * - state_およびline_はNULLではない。
 * - line_は読み取り可能なvalid choco_string_tである。
 * - state_は書き込み可能なtemporary configuration stateである。
 *
 * Responsibility:
 * - logical lineをGLCE Config Utilityによって分類する。
 * - blank lineおよびcomment lineは無視する。
 * - key/value lineはkey_value_collect()へ渡す。
 *
 * Postconditions:
 * - blank / comment lineを受理した場合、state_を変更しない。
 * - supported key/value lineのcollectionに成功した場合、
 *   対応するfieldがstate_へ反映される。
 * - invalid lineはLOADER_UNSUPPORTED_FILEとしてrejectする。
 *
 * Validation:
 * - logical lineのclassificationにより、
 *   supportedなline categoryであることを確認する。
 * - key/value syntaxおよびMaterial固有のkey validationは
 *   key_value_collect()へ委譲する。
 */
static loader_result_t line_collect(untextured_mesh_material_config_state_t* state_, const choco_string_t* line_) {
    loader_result_t ret = LOADER_INVALID_ARGUMENT;

    glce_config_utility_line_type_t line_type = GLCE_CONFIG_UTILITY_LINE_TYPE_INVALID;

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "line_collect", "state_")
    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "line_collect", "line_")

    line_type = glce_config_utility_line_type_get(choco_string_c_str(line_), choco_string_length(line_));
    switch(line_type) {
    case GLCE_CONFIG_UTILITY_LINE_TYPE_BLANK:
        ret = LOADER_SUCCESS;
        break;
    case GLCE_CONFIG_UTILITY_LINE_TYPE_COMMENT:
        ret = LOADER_SUCCESS;
        break;
    case GLCE_CONFIG_UTILITY_LINE_TYPE_KEY_VALUE:
        ret = key_value_collect(state_, choco_string_c_str(line_), choco_string_length(line_));
        break;
    case GLCE_CONFIG_UTILITY_LINE_TYPE_INVALID:
        ret = LOADER_UNSUPPORTED_FILE;
        break;
    default:
        ret = LOADER_UNDEFINED_ERROR;
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
 * - line_とline_length_は、GLCE Config Utilityが処理できる
 *   logical line representationを構成している。
 * - state_は初期化済みの書き込み可能なtemporary stateである。
 *
 * Responsibility:
 * - key/value lineをGLCE Config Utilityによってparseする。
 * - parsed keyをUntextured Mesh Material Configuration Formatの
 *   supported keyと照合する。
 * - 対応するfieldへvalue tokenを収集する。
 *
 * Postconditions:
 * - LOADER_SUCCESSの場合、supported keyに対応するfieldへ
 *   NUL terminated value tokenが格納されている。
 * - unsupported key、duplicate key、unsupported representationを
 *   validなconfiguration entryとして受理しない。
 *
 * Validation:
 * - common key/value lexical / syntactic representationの検査は
 *   glce_config_utility_key_value_parse()へ委譲する。
 * - parsed keyがambient、diffuse、specular、shininessの
 *   いずれかであることを確認する。
 * - duplicate keyおよびdestination capacityの検査は
 *   value_store()へ委譲する。
 * - numeric parsingはconfig_state_parse()へ、
 *   Material固有のsemantic validationはPublic Validatorへ委譲する。
 */
static loader_result_t key_value_collect(untextured_mesh_material_config_state_t* state_, const char* line_, size_t line_length_) {
    loader_result_t ret = LOADER_INVALID_ARGUMENT;

    glce_config_utility_result_t ret_glce_config_utility = GLCE_CONFIG_UTILITY_INVALID_ARGUMENT;

    glce_config_utility_key_value_t key_value = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "key_value_collect", "state_")
    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "key_value_collect", "line_")
    if(0 == line_length_) {
        ret = LOADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("key_value_collect(%s) - Provided line_length_ is not valid.", loader_result_to_str(ret));
        goto cleanup;
    }

    ret_glce_config_utility = glce_config_utility_key_value_parse(line_, line_length_, &key_value);
    if(GLCE_CONFIG_UTILITY_SUCCESS != ret_glce_config_utility) {
        ret = loader_result_convert_glce_config_utility(ret_glce_config_utility);
        ERROR_MESSAGE("key_value_collect(%s) - glce_config_utility_key_value_parse failed.", loader_result_to_str(ret));
        goto cleanup;
    }

    if(choco_string_is_equal(s_key_str_ambient, key_value.key)) {
        ret = value_store(state_->ambient, GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE, key_value.value);
        if(LOADER_SUCCESS != ret) {
            ERROR_MESSAGE("key_value_collect(%s) - value_store failed. key = %s, value = %s.", loader_result_to_str(ret), key_value.key, key_value.value);
            goto cleanup;
        }
    } else if(choco_string_is_equal(s_key_str_diffuse, key_value.key)) {
        ret = value_store(state_->diffuse, GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE, key_value.value);
        if(LOADER_SUCCESS != ret) {
            ERROR_MESSAGE("key_value_collect(%s) - value_store failed. key = %s, value = %s.", loader_result_to_str(ret), key_value.key, key_value.value);
            goto cleanup;
        }
    } else if(choco_string_is_equal(s_key_str_shininess, key_value.key)) {
        ret = value_store(state_->shininess, GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE, key_value.value);
        if(LOADER_SUCCESS != ret) {
            ERROR_MESSAGE("key_value_collect(%s) - value_store failed. key = %s, value = %s.", loader_result_to_str(ret), key_value.key, key_value.value);
            goto cleanup;
        }
    } else if(choco_string_is_equal(s_key_str_specular, key_value.key)) {
        ret = value_store(state_->specular, GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE, key_value.value);
        if(LOADER_SUCCESS != ret) {
            ERROR_MESSAGE("key_value_collect(%s) - value_store failed. key = %s, value = %s.", loader_result_to_str(ret), key_value.key, key_value.value);
            goto cleanup;
        }
    } else {
        ret = LOADER_UNSUPPORTED_FILE;
        ERROR_MESSAGE("key_value_collect(%s) - Undefined key. key = %s", loader_result_to_str(ret), key_value.key);
        goto cleanup;
    }

    ret = LOADER_SUCCESS;

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
 * - value_はGLCE Config Utilityによって取得された
 *   non-empty value tokenである。
 * - dst_[0] == '\0'は未収集fieldを表し、
 *   dst_[0] != '\0'は収集済みfieldを表す。
 *
 * Responsibility:
 * - configuration value tokenをdestination bufferへ格納する。
 * - destinationのexisting stateを利用してduplicate keyを検出する。
 *
 * Postconditions:
 * - LOADER_SUCCESSの場合、dst_はvalue_と同一内容の
 *   NUL terminated stringを保持する。
 * - failure時、dst_の内容は変更しない。
 *
 * Validation:
 * - dst_[0]がNULであることを確認し、
 *   既に収集済みのfieldへの再設定をLOADER_UNSUPPORTED_FILEとしてrejectする。
 * - value lengthと終端NULがdestination capacity内に収まることを確認し、
 *   fixed-size bufferへのcopyを安全に実行できることを保証する。
 */
static loader_result_t value_store(char* dst_, size_t dst_buffer_size_, const char* value_) {
    loader_result_t ret = LOADER_INVALID_ARGUMENT;

    size_t value_length = 0;

    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "value_store", "dst_")
    IF_ARG_NULL_GOTO_CLEANUP(value_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "value_store", "value_")
    if(0 == dst_buffer_size_) {
        ret = LOADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("value_store(%s) - Provided dst_buffer_size_ is not valid.", loader_result_to_str(ret));
        goto cleanup;
    }
    if('\0' != dst_[0]) {
        ret = LOADER_UNSUPPORTED_FILE;
        ERROR_MESSAGE("value_store(%s) - Duplicate key.", loader_result_to_str(ret));
        goto cleanup;
    }
    value_length = strlen(value_);
    if((value_length + 1) > dst_buffer_size_) {
        ret = LOADER_UNSUPPORTED_FILE;
        ERROR_MESSAGE("value_store(%s) - Buffer size error.", loader_result_to_str(ret));
        goto cleanup;
    }

    memcpy(dst_, value_, value_length + 1);

    ret = LOADER_SUCCESS;

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
 * - state_はconfig_state_collect()によって構築された
 *   temporary configuration stateである。
 * - 各fieldはbuffer capacity内のNUL terminated stringであり、
 *   未収集fieldはempty stringとして表現される。
 * - out_config_は書き込み可能なuntextured_mesh_material_config_t storageである。
 *
 * Responsibility:
 * - temporary stateに収集されたvalue tokenを解析し、
 *   untextured_mesh_material_config_tのcandidateを構築する。
 * - required fieldであるdiffuseをparseする。
 * - Optional Fieldは指定されていればparseし、
 *   未指定の場合は規定のデフォルト値を適用する。
 * - required fieldの欠落やunsupported numeric syntaxをrejectする。
 * - Material固有のsemantic validityの判定は行わない。
 *
 * Postconditions:
 * - LOADER_SUCCESSの場合、指定値のparseまたは
 *   デフォルト値の補完によって4つのfieldがすべて確定し、
 *   out_config_へ格納されている。
 * - この段階ではfinite性やMaterial固有のnumeric rangeを保証しない。
 * - failure時、out_config_の既存内容は変更しない。
 *
 * Validation:
 * - sscanf()の変換個数によってrequired numeric componentの存在と
 *   numeric syntaxの成立を確認する。
 * - %nで取得したconsumed lengthをvalue tokenのlengthと比較し、
 *   余分な文字が残っていないことを確認する。
 * - malformed numeric representationはLOADER_UNSUPPORTED_FILEとしてrejectする。
 * - 現行のsscanf("%f")はfloatの表現範囲外入力を安全にrejectしない。
 *   その制約はheaderのTemporary Numeric Input Contractに従う。
 * - 構築されたcandidateのsemantic validationは、
 *   上位のuntextured_mesh_material_config_loader_load()が所有する。
 */
static loader_result_t config_state_parse(const untextured_mesh_material_config_state_t* state_, untextured_mesh_material_config_t* out_config_) {
    loader_result_t ret = LOADER_INVALID_ARGUMENT;

    int consumed = -1;  // sscanf()の%nより前に読み取った文字数
    int parse_result = 0;
    vec3f_t tmp_diffuse = { 0 };
    vec3f_t tmp_ambient = { 0 };
    vec3f_t tmp_specular = { 0 };
    float tmp_shininess = 0.0f;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "config_state_parse", "state_")
    IF_ARG_NULL_GOTO_CLEANUP(out_config_, ret, LOADER_INVALID_ARGUMENT, loader_result_to_str(LOADER_INVALID_ARGUMENT), "config_state_parse", "out_config_")

    // Prepare.
    // Diffuse
    consumed = -1;
    parse_result = sscanf(state_->diffuse, s_vec3f_scan_format, &tmp_diffuse.elem[0], &tmp_diffuse.elem[1], &tmp_diffuse.elem[2], &consumed);
    if(3 != parse_result || 0 > consumed || (size_t)consumed != strlen(state_->diffuse)) { // (size_t)consumed != strlen(state_->diffuse)によって']'以降に無効な文字がないことを確認する
        ret = LOADER_UNSUPPORTED_FILE;
        ERROR_MESSAGE("config_state_parse(%s) - parse failed.", loader_result_to_str(ret));
        goto cleanup;
    }

    // Ambient 設定値がない場合はdiffuse x 0.1を設定する
    if('\0' != state_->ambient[0]) {    // 設定値あり
        consumed = -1;
        parse_result = sscanf(state_->ambient, s_vec3f_scan_format, &tmp_ambient.elem[0], &tmp_ambient.elem[1], &tmp_ambient.elem[2], &consumed);
        if(3 != parse_result || 0 > consumed || (size_t)consumed != strlen(state_->ambient)) {
            ret = LOADER_UNSUPPORTED_FILE;
            ERROR_MESSAGE("config_state_parse(%s) - parse failed.", loader_result_to_str(ret));
            goto cleanup;
        }
    } else {
        tmp_ambient = vec3f_initialize(tmp_diffuse.elem[0] * 0.1f, tmp_diffuse.elem[1] * 0.1f, tmp_diffuse.elem[2] * 0.1f);
    }

    // Specular 設定値がない場合は0を設定する
    if('\0' != state_->specular[0]) {    // 設定値あり
        consumed = -1;
        parse_result = sscanf(state_->specular, s_vec3f_scan_format, &tmp_specular.elem[0], &tmp_specular.elem[1], &tmp_specular.elem[2], &consumed);
        if(3 != parse_result || 0 > consumed || (size_t)consumed != strlen(state_->specular)) {
            ret = LOADER_UNSUPPORTED_FILE;
            ERROR_MESSAGE("config_state_parse(%s) - parse failed.", loader_result_to_str(ret));
            goto cleanup;
        }
    } else {
        tmp_specular = vec3f_initialize(0.0f, 0.0f, 0.0f);
    }

    // Shininess 設定値がない場合は32を設定する
    if('\0' != state_->shininess[0]) {    // 設定値あり
        consumed = -1;
        parse_result = sscanf(state_->shininess, s_float_scan_format, &tmp_shininess, &consumed);
        if(1 != parse_result || 0 > consumed || (size_t)consumed != strlen(state_->shininess)) {
            ret = LOADER_UNSUPPORTED_FILE;
            ERROR_MESSAGE("config_state_parse(%s) - parse failed.", loader_result_to_str(ret));
            goto cleanup;
        }
    } else {
        tmp_shininess = 32.0f;
    }

    // Output.
    out_config_->ambient = tmp_ambient;
    out_config_->diffuse = tmp_diffuse;
    out_config_->shininess = tmp_shininess;
    out_config_->specular = tmp_specular;

    ret = LOADER_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Validators
// ============================================================
/*
 * diffuse_is_valid() Validation
 *
 * Purpose:
 * - diffuse反射係数が、Untextured Mesh Materialの
 *   canonical semantic validityを満たすことを確認する。
 *
 * Validation Scope:
 * - vec3f_tの3成分すべてがfiniteである。
 * - 各成分が0.0f以上1.0f以下である。
 * - 他のMaterial propertyとのrelationは検査しない。
 */
static bool diffuse_is_valid(vec3f_t value_) {
    if(!vec3f_is_finite(value_)) {
        return false;
    }
    if(0.0f > value_.elem[0] || 0.0f > value_.elem[1] || 0.0f > value_.elem[2]) {
        return false;
    }
    if(1.0f < value_.elem[0] || 1.0f < value_.elem[1] || 1.0f < value_.elem[2]) {
        return false;
    }
    return true;
}

/*
 * ambient_is_valid() Validation
 *
 * Purpose:
 * - ambient反射係数が、Untextured Mesh Materialの
 *   canonical semantic validityを満たすことを確認する。
 *
 * Validation Scope:
 * - vec3f_tの3成分すべてがfiniteである。
 * - 各成分が0.0f以上1.0f以下である。
 * - 他のMaterial propertyとのrelationは検査しない。
 */
static bool ambient_is_valid(vec3f_t value_) {
    if(!vec3f_is_finite(value_)) {
        return false;
    }
    if(0.0f > value_.elem[0] || 0.0f > value_.elem[1] || 0.0f > value_.elem[2]) {
        return false;
    }
    if(1.0f < value_.elem[0] || 1.0f < value_.elem[1] || 1.0f < value_.elem[2]) {
        return false;
    }
    return true;
}

/*
 * specular_is_valid() Validation
 *
 * Purpose:
 * - specular反射係数が、Untextured Mesh Materialの
 *   canonical semantic validityを満たすことを確認する。
 *
 * Validation Scope:
 * - vec3f_tの3成分すべてがfiniteである。
 * - 各成分が0.0f以上1.0f以下である。
 * - 他のMaterial propertyとのrelationは検査しない。
 */
static bool specular_is_valid(vec3f_t value_) {
    if(!vec3f_is_finite(value_)) {
        return false;
    }
    if(0.0f > value_.elem[0] || 0.0f > value_.elem[1] || 0.0f > value_.elem[2]) {
        return false;
    }
    if(1.0f < value_.elem[0] || 1.0f < value_.elem[1] || 1.0f < value_.elem[2]) {
        return false;
    }
    return true;
}

/*
 * shininess_is_valid() Validation
 *
 * Purpose:
 * - shininessが、Untextured Mesh Materialの
 *   canonical semantic validityを満たすことを確認する。
 *
 * Validation Scope:
 * - value_がfiniteである。
 * - value_が0.0f以上である。
 * - floatの表現範囲とは別の固定上限は設けない。
 * - 他のMaterial propertyとのrelationは検査しない。
 */
static bool shininess_is_valid(float value_) {
    if(!isfinite(value_)) {
        return false;
    }
    if(0.0f > value_) {
        return false;
    }
    return true;
}
