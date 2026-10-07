
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/io_utils/glce_config_utility.h"

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/*
 * Module Internal Contract
 *
 * Line Processing Model:
 * - lineはchar_countを基準とするbounded character sequenceとして処理する。
 * - line bodyの走査では、character positionは0以上char_count未満の範囲として扱う。
 * - line全体に対する処理と、key / value tokenに対する処理を分離する。
 *
 * Key / Value Range Representation:
 * - token_ranges_get()が生成するleft / right rangeは、
 *   それぞれtoken candidateの先頭位置と末尾位置をinclusive indexで表す。
 * - 成功時には次のrelationが成立する。
 *
 *     left_start <= left_end < equal_index
 *     equal_index < right_start <= right_end < char_count
 *
 * - key / value rangeには、delimiterである'='および
 *   token前後のspace / tabを含めない。
 * - range constructionではkey / value textをcopyせず、
 *   source line上の位置情報だけを導出する。
 *
 * Key / Value View Construction:
 * - key / value viewはrange constructionで確定したinclusive rangeから構築する。
 * - key_ptr / value_ptrは、それぞれ対応するrangeの先頭characterを指す。
 * - key_length / value_lengthは、それぞれ
 *   end - start + 1によって算出する。
 * - view construction中はtemporary viewを使用し、
 *   range情報とview fieldのrelationを崩したpartial representationを
 *   caller-visibleなoutputへ直接構築しない。
 *
 * Token Representation:
 * - keyおよびvalueは1文字以上の連続したtokenとして扱う。
 * - token内部にはspace、tab、CR、LF、'='、NULを含めない。
 * - token前後のspace / tabはtoken representationには含めない。
 *
 * Helper Responsibility:
 * - range constructionはdelimiterの位置とkey / value candidate rangeの導出を担当する。
 * - token character判定は、1 characterがtoken representationへ含められるかだけを判定する。
 * - helper間で同一のrepresentation ruleを別々の形で重複定義しない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

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
 * - このvalidationは、後続のclassification処理がchar_count_によるboundedな走査と、
 *   line_[char_count_]のNUL terminationを前提としてline_を参照するために行う。
 * - logical line representationのinvalidityはrecoverableなinput failureとして扱い、
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_INVALIDを返す。
 * - line_[0]からline_[char_count_]までが実際に読み取り可能であることは
 *   C pointerから検査できないため、Public API Contractで定義された
 *   caller側のtrusted / hard preconditionとして扱う。
 *
 * Postconditions:
 * - automatic Postcondition validationは行わない。
 * - 本APIはinput lineをread-onlyでclassificationするoperationであり、
 *   module-owned state、ownership、lifecycleまたはその他のpersistent stateを変更しない。
 * - outputは明示的に定義されたglce_config_utility_line_type_tの値を
 *   returnするだけであり、stable stateに対する追加validationを必要としない。
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
 * - line_およびout_view_がNULLでないことを検査する。
 * - line_is_valid()によって、line_とchar_count_が
 *   validなlogical line representationを構成していることを検査する。
 * - これらは、後続のrange constructionがline_をboundedに参照し、
 *   success時にout_view_へ結果を書き込むために必要なconditionである。
 * - checked precondition violationはrecoverableなinput failureとして扱い、
 *   falseを返す。
 * - line_[0]からline_[char_count_]までが実際に読み取り可能であることは
 *   C pointerから検査できないため、Public API Contractで定義された
 *   caller側のtrusted / hard preconditionとして扱う。
 *
 * Output Candidate Validation:
 * - key_value_range_get()の成功後、導出されたrangeからtemporaryな
 *   glce_config_utility_key_value_view_tを構築する。
 * - temporary viewはcaller-visibleなoutputへcopyする前に、
 *   glce_config_utility_key_value_view_is_valid()によって検査する。
 * - このvalidationは、module自身が構築したoutput candidateが
 *   public key/value view contractを満たしていることを確認するために行う。
 * - candidate validationに失敗した場合はout_view_を変更せずfalseを返す。
 *
 * Postconditions:
 * - out_view_へのcopy後にautomatic Postcondition validationは行わない。
 * - output前にvalidation済みのtemporary viewをvalue copyするだけであり、
 *   copyによって新しいsemantic state、ownership relationまたはlifecycle transitionは
 *   発生しないため、同一validationをcopy後に再実行しない。
 * - falseを返すfailure pathではout_view_へのOutputへ到達しないため、
 *   out_view_の既存内容は維持される。
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
 * - lineとchar_countの組み合わせが、
 *   本moduleで扱うvalidなlogical line representationとして
 *   成立していることを確認する。
 * - 後続処理がchar_countによるboundedな走査とNUL terminated representationを、
 *   一貫した前提として扱えることを保証する。
 *
 * Validation Scope:
 * - lineがNULLではないこと。
 * - line[0]からline[char_count - 1]までにNUL ('\0')を含まないこと。
 * - line[0]からline[char_count - 1]までに
 *   carriage return ('\r')またはline feed ('\n')を含まないこと。
 * - line[char_count]が終端NUL ('\0')であること。
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
 * - 1 characterが、本moduleで定義するkey / value tokenの
 *   constituent characterとして使用可能であることを確認する。
 * - key / value tokenのcharacter-level validityを一箇所で定義する。
 *
 * Validation Scope:
 * - characterがspace (' ')ではないこと。
 * - characterがhorizontal tab ('\t')ではないこと。
 * - characterがcarriage return ('\r')ではないこと。
 * - characterがline feed ('\n')ではないこと。
 * - characterがkey / value delimiter ('=')ではないこと。
 * - characterがNUL ('\0')ではないこと。
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
