// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/**
 * @section glce_config_utility_boundary_contract Module Boundary Contract
 *
 * Module Scope:
 * - GLCE Config UtilityはEngine内部で使用するlow-level utility moduleであり、
 *   Applicationレイヤーから直接使用しない。
 * - 本moduleはGLCE configuration textに共通する低水準な
 *   lexical / syntactic representationを扱う。
 * - supported key、required field、value range、value interpretation等の
 *   format-specific semanticは本moduleでは所有せず、
 *   各configuration formatを扱う上位moduleが所有する。
 *
 * Trust Boundary:
 * - 本moduleはExternal Trust Boundaryを所有せず、
 *   external configurationをGLCE内部trusted representationとして
 *   admissionする責務を持たない。
 * - external configurationに対するtrust promotionは、
 *   そのconfiguration formatを所有する上位Loader等のboundary ownerが行う。
 * - 本moduleが返すclassificationまたはparsed viewは、
 *   source representation自体のtrust promotionを意味しない。
 *
 * Line Representation:
 * - lineはcaller-ownedのread-only borrowed storageとして扱い、
 *   本moduleはそのstorageを所有、変更、解放しない。
 * - line bodyはchar_count byteで構成され、
 *   body内にNUL、CR、LFを含まない。
 * - line[char_count]は終端NULである。
 * - callerはline[0]からline[char_count]までを
 *   読み取り可能なstorageとして提供しなければならない。
 * - char_count == 0のlineはempty line representationとして許可する。
 *
 * Borrowed View:
 * - key / value viewはsource line storageの一部を参照するnon-owning borrowed viewであり、
 *   key / value textのcopyまたは新しいstorageのallocationを行わない。
 * - viewが参照するstorageのownershipはsource lineのownerに残る。
 * - viewはsource line storageが生存し、かつ参照範囲の内容が変更されない間だけ有効である。
 * - viewが保持するpointerをcallerが個別に解放してはならない。
 *
 * State / Resource Ownership:
 * - 本moduleはpersistent stateを持たず、
 *   callerから渡されたtext storageまたはその他のresourceを所有しない。
 *
 * @par AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_IO_UTILS_GLCE_CONFIG_UTILITY_H
#define GLCE_ENGINE_IO_UTILS_GLCE_CONFIG_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/*
 * GLCE configuration tokenの最大文字数。暫定的に31文字で、バッファサイズが32。
 */
#define GLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH 31

#define GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE (GLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH + 1)

typedef enum {
    GLCE_CONFIG_UTILITY_SUCCESS = 0,
    GLCE_CONFIG_UTILITY_INVALID_ARGUMENT,
    GLCE_CONFIG_UTILITY_BAD_OPERATION,
    GLCE_CONFIG_UTILITY_UNDEFINED_ERROR,
    GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMAT,
} glce_config_utility_result_t;

typedef enum glce_config_utility_line_type {
    GLCE_CONFIG_UTILITY_LINE_TYPE_BLANK = 0,
    GLCE_CONFIG_UTILITY_LINE_TYPE_COMMENT,
    GLCE_CONFIG_UTILITY_LINE_TYPE_KEY_VALUE,
    GLCE_CONFIG_UTILITY_LINE_TYPE_INVALID,
} glce_config_utility_line_type_t;

typedef struct glce_config_utility_key_value {
    char key[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
    char value[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];
} glce_config_utility_key_value_t;

/**
 * @brief configuration lineの種別を判定する
 *
 * @details
 * 指定されたlogical lineを解析し、blank line、comment line、
 * key/value line、invalid lineのいずれかへ分類する。
 *
 * @par Public API Contract
 *
 * Input Representation:
 * - line bodyはchar_count_ byteで構成される。
 * - validなlogical line representationでは、
 *   line body内にNUL、CR、LFを含まず、
 *   line_[char_count_]が終端NULである。
 * - validなlogical line representationを満たさないinputは、
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_INVALIDとして分類する。
 * - char_count_ == 0はvalidなempty line representationとして扱う。
 *
 * Classification:
 * - empty line、またはspace (' ') / horizontal tab ('\t')のみで
 *   構成されるlineはGLCE_CONFIG_UTILITY_LINE_TYPE_BLANKとする。
 * - leading space / horizontal tabを除いた最初のcharacterが'#'であるlineは、
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_COMMENTとする。
 * - validなkey/value representationとして解析できるlineは、
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_KEY_VALUEとする。
 * - 上記のいずれにも該当しないlineは、
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_INVALIDとする。
 * - comment classificationはkey/value classificationより先に行われるため、
 *   comment bodyに'='が含まれていてもcommentとして扱う。
 *
 * Side Effects:
 * - 本APIはline_を変更せず、persistent module stateも変更しない。
 *
 * @param[in] line_ 判定対象となるlogical line
 * @param[in] char_count_ 終端NULを含まないline bodyのbyte数
 *
 * @return 判定されたglce_config_utility_line_type_t
 *
 * @pre callerはline_[0]からline_[char_count_]までを
 *      読み取り可能なstorageとして提供すること。
 */
glce_config_utility_line_type_t glce_config_utility_line_type_get(const char *line_, size_t char_count_);

void glce_config_utility_key_value_reset(glce_config_utility_key_value_t* key_value_);

/**
 * @brief configuration lineをkey/value viewへ解析する
 *
 * @details
 * 指定されたlogical lineをkey/value representationとして解析し、
 * validな場合はsource line storageの一部を参照するborrowed viewを
 * out_view_へ出力する。
 *
 * @par Public API Contract
 * Input Representation:
 * - line bodyはchar_count_ byteで構成される。
 * - line body内にNUL、CR、LFを含まない。
 * - line_[char_count_]は終端NULである。
 * - key/value representationでは'='をdelimiterとして使用し、
 *   line内にはちょうど1つの'='が存在する。
 * - keyおよびvalueはnon-emptyである。
 * - keyおよびvalueの前後にあるspace / horizontal tabは
 *   token representationには含めない。
 * - keyおよびvalueの内部にはspace、horizontal tab、CR、LF、'='、NULを含まない。
 *
 * Output Contract:
 * - success時、out_view_はsource line storageを参照する
 *   non-owning borrowed viewとなる。
 * - key_ptrおよびvalue_ptrは、それぞれ対応するtokenの先頭を指す。
 * - key_lengthおよびvalue_lengthは、それぞれ1以上となる。
 * - failure時、out_view_の内容は変更されない。
 *
 * Lifetime / Ownership:
 * - 本moduleはsource line storageを所有、変更、解放しない。
 * - returned viewはsource line storageが生存し、
 *   参照範囲が変更されない間だけ有効である。
 * - callerはkey_ptrおよびvalue_ptrを解放してはならない。
 *
 * @param[in] line_ 解析対象となるlogical line
 * @param[in] char_count_ 終端NULを含まないline bodyのbyte数
 * @param[out] out_view_ 解析結果となるkey/value borrowed viewの出力先
 *
 * @retval true validなkey/value representationとして解析できた場合
 * @retval false input representationを受理できない場合、またはout_view_がNULLの場合
 *
 * @pre callerはline_[0]からline_[char_count_]までを
 *      読み取り可能なstorageとして提供すること。
 */
glce_config_utility_result_t glce_config_utility_key_value_parse(const char* line_, size_t char_count_, glce_config_utility_key_value_t* out_key_value_);

/*
 * glce_config_utility_key_value_view_is_valid() Validation
 *
 * Purpose:
 * - glce_config_utility_key_value_view_tが、
 *   本moduleで定義するvalidなkey / value token representationを
 *   満たしていることを確認する。
 * - key / value viewのstructural validityと、
 *   viewが参照する各tokenのcharacter-level semantic validityを検査する。
 *
 * Validation Scope:
 * - view自身が存在すること。
 * - key_ptrおよびvalue_ptrが存在すること。
 * - key_lengthおよびvalue_lengthが1以上であること。
 * - keyおよびvalueの各characterについて、
 *   space (' ')、horizontal tab ('\t')、carriage return ('\r')、
 *   line feed ('\n')、key/value delimiter ('=')、NUL ('\0')
 *   のいずれにも該当しないこと。
 * - したがって、keyおよびvalueは上記characterを内部に含まない
 *   連続したnon-empty tokenとして成立すること。
 */
bool glce_config_utility_key_value_is_valid(const glce_config_utility_key_value_t* key_value_);

#ifdef __cplusplus
}
#endif
#endif
