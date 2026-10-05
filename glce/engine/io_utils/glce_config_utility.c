
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/io_utils/glce_config_utility.h"

#include <stddef.h>
#include <stdbool.h>

/*
 * Module Internal Contract
 *
 * Line Processing Model:
 * - lineはchar_countを基準とするbounded character sequenceとして処理する。
 * - line bodyの走査では、character positionは0以上char_count未満の範囲として扱う。
 * - line全体に対する処理と、key / value tokenに対する処理を分離する。
 *
 * Key / Value Range Representation:
 * - key_value_range_get()が生成するleft / right rangeは、
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
// Private Function Declarations
// ============================================================
// Parsing helpers
static bool key_value_range_get(const char *line_, size_t char_count_, size_t* left_start_, size_t* left_end_, size_t* right_start_, size_t* right_end_);

// Validators
static bool line_is_valid(const char* line_, size_t char_count_);
static bool token_char_is_valid(char c);

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
    glce_config_utility_key_value_view_t dummy_view = { 0 };

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
    if(glce_config_utility_key_value_parse(line_, char_count_, &dummy_view)) {
        return GLCE_CONFIG_UTILITY_LINE_TYPE_KEY_VALUE;
    }
    return GLCE_CONFIG_UTILITY_LINE_TYPE_INVALID;
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
bool glce_config_utility_key_value_parse(const char* line_, size_t char_count_, glce_config_utility_key_value_view_t* out_view_) {
    size_t left_start = 0;
    size_t left_end = 0;

    size_t right_start = 0;
    size_t right_end = 0;

    glce_config_utility_key_value_view_t tmp_view = { 0 };

    // Preconditions.
    if(NULL == line_ || NULL == out_view_) {
        return false;
    }
    if(!line_is_valid(line_, char_count_)) {
        return false;
    }

    // Prepare.
    if(!key_value_range_get(line_, char_count_, &left_start, &left_end, &right_start, &right_end)) {
        return false;
    }
    tmp_view.key_length = left_end - left_start + 1;
    tmp_view.value_length = right_end - right_start + 1;
    tmp_view.key_ptr = &line_[left_start];
    tmp_view.value_ptr = &line_[right_start];

    // Result validation.
    if(!glce_config_utility_key_value_view_is_valid(&tmp_view)) {
        return false;
    }

    // Output.
    *out_view_ = tmp_view;

    return true;
}

bool glce_config_utility_key_value_view_is_valid(const glce_config_utility_key_value_view_t* view_) {
    if(NULL == view_) {
        return false;
    }
    if(NULL == view_->key_ptr || NULL == view_->value_ptr) {
        return false;
    }
    if(0 == view_->key_length || 0 == view_->value_length) {
        return false;
    }

    for(size_t i = 0; i != view_->key_length; ++i) {
        if(!token_char_is_valid(view_->key_ptr[i])) {
            return false;
        }
    }

    for(size_t i = 0; i != view_->value_length; ++i) {
        if(!token_char_is_valid(view_->value_ptr[i])) {
            return false;
        }
    }

    return true;
}

// ============================================================
// Parsing helpers
// ============================================================
static bool key_value_range_get(const char *line_, size_t char_count_, size_t* left_start_, size_t* left_end_, size_t* right_start_, size_t* right_end_) {
    size_t equal_index = 0;
    size_t equal_count = 0;

    size_t left_start = 0;
    size_t left_end = 0;

    size_t right_start = 0;
    size_t right_end = 0;

    bool found_right_start = false;
    bool found_left_start = false;

    if(NULL == line_) {
        return false;
    }
    if(NULL == left_start_ || NULL == left_end_) {
        return false;
    }
    if(NULL == right_start_ || NULL == right_end_) {
        return false;
    }

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
            left_start = i;
            found_left_start = true;
            break;
        }
    }
    if(!found_left_start) {
        return false; // 左辺が空
    }
    left_end = equal_index;
    while(left_end > 0) {
        left_end--;
        if(token_char_is_valid(line_[left_end])) {
            break;
        }
    }

    // 右辺のチェック
    for(size_t i = equal_index + 1; i != char_count_; ++i) {
        if(token_char_is_valid(line_[i])) {
            right_start = i;
            found_right_start = true;
            break;
        }
    }
    if(!found_right_start) {
        return false; // 右辺が空
    }
    right_end = char_count_;
    while(right_end > right_start) {
        right_end--;
        if(token_char_is_valid(line_[right_end])) {
            break;
        }
    }

    *left_start_ = left_start;
    *left_end_ = left_end;

    *right_start_ = right_start;
    *right_end_ = right_end;

    return true;
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
