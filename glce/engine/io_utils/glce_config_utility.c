
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/io_utils/glce_config_utility.h"

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

// ============================================================
// Private Type Definitions
// ============================================================
typedef struct token_range {
    size_t start;
    size_t end;
} token_range_t;

// ============================================================
// Private Constants
// ============================================================
static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";
static const char* const s_result_str_unsupported_format = "UNSUPPORTED_FORMAT";

// ============================================================
// Private Function Declarations
// ============================================================
// Parsing helpers
static bool token_ranges_get(const char *line_, size_t char_count_, token_range_t* left_range_, token_range_t* right_range_);
static void token_copy(const char* line_, const token_range_t* range_, char out_token_[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE]);

// Utilities
static const char* result_to_str(glce_config_utility_result_t result_);

// Validators
static bool line_is_valid(const char* line_, size_t char_count_);
static bool token_char_is_valid(char c);
static bool token_range_is_valid(const token_range_t* range_);
static bool token_is_valid(const char token_[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE]);

// ============================================================
// Public API
// ============================================================

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - line_is_valid()によって、line_とchar_count_が
 *   validなlogical line representationを構成していることを検査する。
 * - このvalidationは、後続のclassification処理がchar_count_によるbounded scanと
 *   line_[char_count_]のNUL terminationを前提としてline_を参照するために行う。
 * - logical line representationがinvalidな場合はrecoverableなinput failureとして扱い、
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_INVALIDを返す。
 * - line_[0]からline_[char_count_]までが実際に読み取り可能であることは
 *   C pointerから検査できないため、Public API Contractで定義された
 *   caller側のtrusted / hard preconditionとして扱う。
 *
 * Result / Postcondition validation:
 * - automaticなResult validationおよびPostcondition validationは行わない。
 * - classification resultはdocumentedなglce_config_utility_line_type_tの
 *   enumeratorから直接選択して返すため、追加のcandidate validationを必要としない。
 * - 本APIはinputをread-onlyで参照するだけであり、
 *   persistent state、ownership、lifecycleを変更しないため、
 *   stable stateに対するPostcondition validationも必要としない。
 */
glce_config_utility_line_type_t glce_config_utility_line_type_get(const char *line_, size_t char_count_) {
    size_t first_char_idx = 0;

    // Preconditions.
    if(!line_is_valid(line_, char_count_)) {
        return GLCE_CONFIG_UTILITY_LINE_TYPE_INVALID;
    }

    // Prepare.
    for(size_t i = 0; i != char_count_; ++i) {
        if(' ' == line_[first_char_idx] || '\t' == line_[first_char_idx]) {
            first_char_idx++;
        } else {
            break;
        }
    }

    // Output.
    if('\0' == line_[first_char_idx]) {
        return GLCE_CONFIG_UTILITY_LINE_TYPE_BLANK;
    }
    if(line_[first_char_idx] == '#') {
        return GLCE_CONFIG_UTILITY_LINE_TYPE_COMMENT;
    }
    if(NULL != memchr(line_, '=', char_count_)) {
        return GLCE_CONFIG_UTILITY_LINE_TYPE_KEY_VALUE;
    }
    return GLCE_CONFIG_UTILITY_LINE_TYPE_INVALID;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - key_value_ == NULLはno-opとして許可するため、
 *   recoverable failureとして扱う追加validationは行わない。
 * - key_value_ != NULLの場合、そのstorageが書き込み可能であることは
 *   C pointerから検査できないため、caller側のtrusted / hard preconditionとして扱う。
 * - 本APIはexisting key/value representationの内容をconsumeせず、
 *   reset stateへ上書きするoperationであるため、
 *   operation開始前のcanonical validationは行わない。
 *
 * Postcondition validation:
 * - automatic Postcondition validationは行わない。
 * - reset stateはkey[0]およびvalue[0]へ終端NULを書き込むことで直接成立し、
 *   その成立がoperationそのものから導出できるため、同一conditionを再検査しない。
 */
void glce_config_utility_key_value_reset(glce_config_utility_key_value_t* key_value_) {
    if(NULL == key_value_) {
        return;
    }

    key_value_->key[0] = '\0';
    key_value_->value[0] = '\0';
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - line_およびout_key_value_がNULLではないことをchecked preconditionとして検査する。
 * - line_is_valid()によって、line_とchar_count_が
 *   validなlogical line representationを構成していることを検査する。
 * - これらのvalidationは、後続のparsing処理がline_をboundedに参照し、
 *   success時にout_key_value_へ結果を書き込むために必要である。
 * - NULLまたはinvalidなlogical line representationは
 *   GLCE_CONFIG_UTILITY_INVALID_ARGUMENTとして扱う。
 * - out_key_value_についてkey[0] == '\0'かつvalue[0] == '\0'であることを検査する。
 * - parse outputはreset stateのobjectへだけ書き込むcontractであるため、
 *   reset stateを満たさない場合はGLCE_CONFIG_UTILITY_BAD_OPERATIONとして扱う。
 * - line_[0]からline_[char_count_]までが実際に読み取り可能であること、および
 *   out_key_value_のstorageが書き込み可能であることはC pointerから検査できないため、
 *   caller側のtrusted / hard preconditionとして扱う。
 *
 * Prepare validation:
 * - token_ranges_get()によってkey/value candidate rangeを導出する。
 * - delimiterが一意に定まらない、またはkey/value candidateを構成できない場合は、
 *   input textのunsupported syntaxとしてGLCE_CONFIG_UTILITY_UNSUPPORTED_FORMATを返す。
 * - token_range_is_valid()によって、導出されたleft / right rangeが
 *   token copyに使用可能なrange representationを満たすことを確認する。
 * - このvalidationは、range lengthを前提としてfixed-size output bufferへcopyする前に、
 *   unsupportedなrangeをrejectするために行う。
 * - range validation failureはcaller-provided textから導出された
 *   unsupported representationとしてGLCE_CONFIG_UTILITY_UNSUPPORTED_FORMATを返す。
 *
 * Result validation:
 * - rangeから構築したtemporary key/value candidateを、
 *   glce_config_utility_key_value_is_valid()でcanonical validationする。
 * - このvalidationは、caller-visible outputへ公開する前に、
 *   candidateがpublic key/value representation contractを満たしていることを
 *   確認するために行う。
 * - candidateはcaller-provided textから構築されたtrust promotion前のrepresentationであるため、
 *   validation failureはinternal corruptionとは扱わず、
 *   GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMATとしてrejectする。
 *
 * Postcondition validation:
 * - out_key_value_へのcopy後にautomatic Postcondition validationは行わない。
 * - output前にcanonical validation済みのtemporary candidateをvalue copyするだけであり、
 *   copyによってrepresentation validity、ownership、lifecycle relationは変化しないため、
 *   同一validationをcopy後に再実行しない。
 * - failure pathではOutputへ到達しないため、out_key_value_の既存内容は変更しない。
 */
glce_config_utility_result_t glce_config_utility_key_value_parse(const char* line_, size_t char_count_, glce_config_utility_key_value_t* out_key_value_) {
    glce_config_utility_result_t ret = GLCE_CONFIG_UTILITY_INVALID_ARGUMENT;

    token_range_t right_range = { 0 };
    token_range_t left_range = { 0 };
    glce_config_utility_key_value_t key_value = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, GLCE_CONFIG_UTILITY_INVALID_ARGUMENT, result_to_str(GLCE_CONFIG_UTILITY_INVALID_ARGUMENT), "glce_config_utility_key_value_parse", "line_")
    IF_ARG_NULL_GOTO_CLEANUP(out_key_value_, ret, GLCE_CONFIG_UTILITY_INVALID_ARGUMENT, result_to_str(GLCE_CONFIG_UTILITY_INVALID_ARGUMENT), "glce_config_utility_key_value_parse", "out_key_value_")
    if(!line_is_valid(line_, char_count_)) {
        ret = GLCE_CONFIG_UTILITY_INVALID_ARGUMENT;
        ERROR_MESSAGE("glce_config_utility_key_value_parse(%s) - Provided line_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if('\0' != out_key_value_->key[0] || '\0' != out_key_value_->value[0]) {
        ret = GLCE_CONFIG_UTILITY_BAD_OPERATION;
        ERROR_MESSAGE("glce_config_utility_key_value_parse(%s) - Provided out_key_value_ is already initialized.", result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    if(!token_ranges_get(line_, char_count_, &left_range, &right_range)) {
        ret = GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMAT;
        ERROR_MESSAGE("glce_config_utility_key_value_parse(%s) - token_ranges_get failed.", result_to_str(ret));
        goto cleanup;
    }
    if(!token_range_is_valid(&left_range) || !token_range_is_valid(&right_range)) {
        ret = GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMAT;
        ERROR_MESSAGE("glce_config_utility_key_value_parse(%s) - Unsupported key, value.", result_to_str(ret));
        goto cleanup;
    }
    token_copy(line_, &left_range, key_value.key);
    token_copy(line_, &right_range, key_value.value);

    // Result validation.
    if(!glce_config_utility_key_value_is_valid(&key_value)) {
        ret = GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMAT;
        ERROR_MESSAGE("glce_config_utility_key_value_parse(%s) - Result validation failed.", result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_key_value_ = key_value;

    ret = GLCE_CONFIG_UTILITY_SUCCESS;

cleanup:
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * - 本API自身がglce_config_utility_key_value_tのexplicit canonical validatorであるため、
 *   別のcanonical validationは実行しない。
 * - key_value_ == NULLはinvalid representationとしてfalseを返す。
 * - keyおよびvalueそれぞれについてtoken_is_valid()を実行し、
 *   public key/value representation contractを構成するtoken validityを検査する。
 * - explicit validatorであるため、BUILD_MODEによってvalidation scopeを変更しない。
 * - validation中に対象stateを変更せず、validation failure時はfalseを返すのみとする。
 * - state mutationやresult candidate constructionを行わないため、
 *   Result validationおよびPostcondition validationは必要としない。
 */
bool glce_config_utility_key_value_is_valid(const glce_config_utility_key_value_t* key_value_) {
    if(NULL == key_value_) {
        return false;
    }
    if(!token_is_valid(key_value_->key)) {
        return false;
    }
    if(!token_is_valid(key_value_->value)) {
        return false;
    }
    return true;
}

// ============================================================
// Parsing helpers
// ============================================================
/*
 * token_ranges_get() Contract
 *
 * Preconditions:
 * - line_はNULLではない。
 * - left_range_およびright_range_はNULLではない。
 * - line_[0]からline_[char_count_ - 1]までを読み取り可能である。
 *
 * Responsibility:
 * - line_からkey / value delimiterとなる'='を特定し、
 *   delimiterの左右にあるtoken candidateのrangeを導出する。
 * - token前後のtoken characterではない領域をrangeから除外する。
 *
 * Postconditions:
 * - trueを返した場合、left_range_およびright_range_へ
 *   inclusive indexによるrangeを出力する。
 * - left rangeはdelimiterより左側にあり、
 *   right rangeはdelimiterより右側にある。
 * - 両rangeはsource lineのbody内に収まる。
 *
 * Validation:
 * - line内の'='がちょうど1つであることを確認する。
 *   key/value separationを一意に決定するために必要である。
 * - delimiterの左右にtoken candidateが存在することを確認する。
 *   empty key / valueのrangeを生成しないために必要である。
 */
static bool token_ranges_get(const char *line_, size_t char_count_, token_range_t* left_range_, token_range_t* right_range_) {
    size_t equal_index = 0;
    size_t equal_count = 0;

    token_range_t left_range = { 0 };
    token_range_t right_range = { 0 };

    bool found_right_start = false;
    bool found_left_start = false;

    // Preconditions.
    if(NULL == line_) {
        return false;
    }
    if(NULL == left_range_ || NULL == right_range_) {
        return false;
    }

    // Prepare.
    for(size_t i = 0; i != char_count_; ++i) {
        if('=' == line_[i]) {
            equal_index = i;
            equal_count++;
        }
    }
    if(1 != equal_count) {
        return false;   // '='が見つからない or 複数存在する
    }

    // 左辺のチェック
    for(size_t i = 0; i != equal_index; ++i) {
        if(token_char_is_valid(line_[i])) {
            left_range.start = i;
            found_left_start = true;
            break;
        }
    }
    if(!found_left_start) {
        return false; // 左辺が空
    }
    left_range.end = equal_index;
    while(left_range.end > 0) {
        left_range.end--;
        if(token_char_is_valid(line_[left_range.end])) {
            break;
        }
    }

    // 右辺のチェック
    for(size_t i = equal_index + 1; i != char_count_; ++i) {
        if(token_char_is_valid(line_[i])) {
            right_range.start = i;
            found_right_start = true;
            break;
        }
    }
    if(!found_right_start) {
        return false; // 右辺が空
    }
    right_range.end = char_count_;
    while(right_range.end > right_range.start) {
        right_range.end--;
        if(token_char_is_valid(line_[right_range.end])) {
            break;
        }
    }

    *left_range_ = left_range;
    *right_range_ = right_range;

    return true;
}

/*
 * token_copy() Contract
 *
 * Preconditions:
 * - line_、range_、out_token_はNULLではない。
 * - range_はtoken copyに使用可能なvalid rangeである。
 *
 * Responsibility:
 * - range_で指定されたsource line上のcharacter sequenceを
 *   out_token_へcopyし、終端NULを付加する。
 *
 * Postconditions:
 * - out_token_はrange lengthと同じ文字列内容を持つ
 *   NUL terminated stringとなる。
 */
static void token_copy(const char* line_, const token_range_t* range_, char out_token_[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE]) {
    if(NULL == line_ || NULL == range_ || NULL == out_token_) {
        return;
    }

    const size_t length = range_->end - range_->start + 1;
    memcpy(out_token_, &line_[range_->start], length);
    out_token_[length] = '\0';
}

// ============================================================
// Utilities
// ============================================================
static const char* result_to_str(glce_config_utility_result_t result_) {
    switch(result_) {
    case GLCE_CONFIG_UTILITY_SUCCESS:
        return s_result_str_success;
    case GLCE_CONFIG_UTILITY_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case GLCE_CONFIG_UTILITY_BAD_OPERATION:
        return s_result_str_bad_operation;
    case GLCE_CONFIG_UTILITY_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    case GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMAT:
        return s_result_str_unsupported_format;
    default:
        return s_result_str_undefined_error;
    }
}

// ============================================================
// Validators
// ============================================================
/*
 * line_is_valid() Validation
 *
 * Purpose:
 * - line_とchar_count_が、本moduleで扱えるlogical line representationを構成していることを確認する。
 *
 * Validation Scope:
 * - line_がNULLではないこと。
 * - line_[0]からline_[char_count_ - 1]までにNUL、CR、LFを含まないこと。
 * - line_[char_count_]が終端NULであること。
 * - char_count_ == 0のempty lineはvalidとして扱う。
 */
static bool line_is_valid(const char* line_, size_t char_count_) {
    if(NULL == line_) {
        return false;
    }

    for(size_t i = 0; i != char_count_; ++i) {
        if('\0' == line_[i]) {
            return false;
        }
        if('\r' == line_[i] || '\n' == line_[i]) {
            return false;
        }
    }

    if('\0' != line_[char_count_]) {
        return false;
    }

    return true;
}

/*
 * token_char_is_valid() Validation
 *
 * Purpose:
 * - 1 characterがkey / value tokenを構成するcharacterとして使用可能であることを確認する。
 *
 * Validation Scope:
 * - space (' ')ではないこと。
 * - horizontal tab ('\t')ではないこと。
 * - carriage return ('\r')ではないこと。
 * - line feed ('\n')ではないこと。
 * - key / value delimiter ('=')ではないこと。
 * - NUL ('\0')ではないこと。
 * - 上記以外のcharacterはvalidなtoken characterとして扱う。
 */
static bool token_char_is_valid(char c) {
    return (
        ' '  != c &&
        '\t' != c &&
        '\r' != c &&
        '\n' != c &&
        '='  != c &&
        '\0' != c
    );
}

/*
 * token_range_is_valid() Validation
 *
 * Purpose:
 * - token_range_tがtokenを保持可能なrange representationとしてvalidであることを確認する。
 *
 * Validation Scope:
 * - range_がNULLではないこと。
 * - start <= endであること。
 * - end - start + 1で表されるrange lengthが
 *   GLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH以下であること。
 *
 * - source line上のboundsやdelimiterとの位置関係は検査しない。
 */
static bool token_range_is_valid(const token_range_t* range_) {
    size_t length = 0;
    if(NULL == range_) {
        return false;
    }
    if(range_->start > range_->end) {
        return false;
    }

    length = range_->end - range_->start + 1;
    if(length > GLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH) {
        return false;
    }
    return true;
}

/*
 * token_is_valid() Validation
 *
 * Purpose:
 * - fixed-size token bufferが、本moduleで扱うcomplete token representationとして
 *   validであることを確認する。
 *
 * Validation Scope:
 * - token_がNULLではないこと。
 * - GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE内に終端NULが存在すること。
 * - tokenがempty stringではないこと。
 * - 終端NULより前のすべてのcharacterがvalidなtoken characterであること。
 *
 * - GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE内に終端NULが必要であるため、
 *   token lengthはGLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH以下となる。
 */
static bool token_is_valid(const char token_[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE]) {
    const char* terminator = NULL;
    size_t length = 0;

    if(NULL == token_) {
        return false;
    }

    terminator = (const char*)memchr(token_, '\0', GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE);
    if(NULL == terminator) {
        return false;
    }

    length = (size_t)(terminator - token_);
    if(0 == length) {
        return false;
    }

    for(size_t i = 0; i != length; ++i) {
        if(!token_char_is_valid(token_[i])) {
            return false;
        }
    }

    return true;
}
