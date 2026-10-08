// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/**
 * @file glce_config_utility.h
 * @author chocolate-pie24
 * @brief GLCE configuration textに共通する低水準の解析機能を提供する
 *
 * @details
 * GLCE Config Utility moduleは、GLCE configuration textに共通する
 * lexical / syntactic representationを扱うEngine内部utilityである。
 *
 * logical lineの種別判定およびkey/value representationの解析を提供し、
 * configuration format固有のsupported key、required field、value range、
 * value interpretation等のsemanticは扱わない。
 * これらのformat-specific semanticは、各configuration formatを所有する
 * 上位moduleが担当する。
 *
 * @section glce_config_utility_boundary_contract Module Boundary Contract
 *
 * Trust Boundary:
 * - 本moduleはExternal Trust Boundaryを所有しない。
 * - external configurationをGLCE内部のtrusted representationとして
 *   admissionする責務は持たない。
 * - external configurationに対するtrust promotionは、
 *   configuration formatを所有するLoader等の上位moduleが行う。
 * - 本moduleによるline classificationまたはkey/value parsingの成功は、
 *   configuration format全体のsemantic validityを保証しない。
 *
 * Line Representation:
 * - input lineはcaller-ownedのread-only borrowed storageとして扱う。
 * - 本moduleはinput line storageを所有、変更、解放しない。
 * - logical line bodyはchar_count byteで構成され、
 *   body内にNUL、CR、LFを含まない。
 * - line[char_count]は終端NULである。
 * - callerはline[0]からline[char_count]までを
 *   読み取り可能なstorageとして提供しなければならない。
 * - char_count == 0のlineはempty line representationとして許可する。
 *
 * Key / Value Representation:
 * - parsed key/valueはglce_config_utility_key_value_tとして保持する。
 * - key/value textはsource lineからcopyされ、
 *   source line storageを参照するborrowed viewではない。
 * - key/valueはそれぞれNUL terminated stringとして保持する。
 * - 各tokenの最大文字数はGLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTHであり、
 *   storage sizeはGLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZEである。
 * - parsed key/valueのstorageはcaller-provided object内に存在し、
 *   本moduleはそのstorageに対するownershipを取得しない。
 * - parse成功後のkey/valueはsource lineのlifetimeから独立して使用できる。
 *
 * State / Resource Ownership:
 * - 本moduleはpersistent module stateを持たない。
 * - dynamic memory allocationを行わない。
 * - callerから渡されたtext storage、key/value storage、
 *   その他のresourceに対するownershipを取得しない。
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

/**
 * @brief configuration tokenとして保持できる最大文字数
 *
 * 終端NULはこの文字数に含まない。
 */
#define GLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH 31

/**
 * @brief configuration tokenをNUL terminated stringとして保持するためのbuffer size
 *
 * GLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH文字と終端NULを格納できる。
 */
#define GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE (GLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH + 1)

/**
 * @brief GLCE Config Utility APIの実行結果コード
 */
typedef enum {
    GLCE_CONFIG_UTILITY_SUCCESS = 0,            /**< 処理成功 */
    GLCE_CONFIG_UTILITY_INVALID_ARGUMENT,       /**< 引数がAPI contractを満たしていない */
    GLCE_CONFIG_UTILITY_BAD_OPERATION,          /**< APIの使用順序または対象状態がoperation contractを満たしていない */
    GLCE_CONFIG_UTILITY_UNDEFINED_ERROR,        /**< 想定されていない、または分類不能な内部エラー */
    GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMAT,     /**< configuration textが対応するlexical / syntactic formatを満たしていない */
} glce_config_utility_result_t;

/**
 * @brief configuration logical lineの分類
 */
typedef enum glce_config_utility_line_type {
    GLCE_CONFIG_UTILITY_LINE_TYPE_BLANK = 0,    /**< emptyまたはblank characterのみで構成されるline */
    GLCE_CONFIG_UTILITY_LINE_TYPE_COMMENT,      /**< commentとして扱うline */
    GLCE_CONFIG_UTILITY_LINE_TYPE_KEY_VALUE,    /**< key/value representationとして解析を試みるline */
    GLCE_CONFIG_UTILITY_LINE_TYPE_INVALID,      /**< 本moduleで有効なline typeへ分類できないline */
} glce_config_utility_line_type_t;

/**
 * @brief parsed key/value tokenを保持するvalue type
 *
 * keyとvalueを、それぞれ独立したNUL terminated stringとして
 * fixed-size buffer内に保持する。
 *
 * source lineへのborrowed referenceは保持しないため、
 * parse成功後の内容はsource lineのlifetimeから独立している。
 */
typedef struct glce_config_utility_key_value {
    char key[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];    /**< parsed key */
    char value[GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE];  /**< parsed value */
} glce_config_utility_key_value_t;

/**
 * @brief configuration logical lineの種別を判定する
 *
 * @details
 * 指定されたlogical lineを、blank、comment、key/value candidate、invalidのいずれかへ分類する。
 *
 * @par Public API Contract
 *
 * Input Representation:
 * - line bodyはchar_count_ byteで構成される。
 * - validなlogical line representationでは、
 *   line body内にNUL、CR、LFを含まず、
 *   line_[char_count_]が終端NULである。
 * - char_count_ == 0はvalidなempty line representationとして扱う。
 * - validなlogical line representationを満たさないinputは
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_INVALIDとして扱う。
 *
 * Classification:
 * - empty line、またはspace (' ') / horizontal tab ('\t')のみで
 *   構成されるlineはGLCE_CONFIG_UTILITY_LINE_TYPE_BLANKとする。
 * - leading space / horizontal tabを除いた最初のcharacterが'#'であるlineは
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_COMMENTとする。
 * - 上記に該当せず、line body内に'='を1文字以上含むlineは
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_KEY_VALUEとする。
 * - 上記のいずれにも該当しないlineは
 *   GLCE_CONFIG_UTILITY_LINE_TYPE_INVALIDとする。
 * - comment判定はkey/value判定より優先されるため、
 *   comment bodyに'='が含まれていてもCOMMENTとして扱う。
 *
 * GLCE_CONFIG_UTILITY_LINE_TYPE_KEY_VALUEは、
 * validなkey/value representationであることを保証しない。
 * key/value syntaxのvalidityはglce_config_utility_key_value_parse()が判定する。
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

/**
 * @brief key/value objectをreset stateへ戻す
 *
 * @details
 * glce_config_utility_key_value_tのkeyおよびvalueをempty stringとして扱える状態へ戻す。
 *
 * @par Public API Contract
 *
 * Reset State:
 * - reset後はkey[0]およびvalue[0]が終端NULとなる。
 * - reset stateのobjectはglce_config_utility_key_value_parse()の
 *   output objectとして使用できる。
 * - key[0]およびvalue[0]より後方のbuffer内容はreset contractの対象外であり、
 *   値は保証されない。
 *
 * Null Handling:
 * - key_value_ == NULLの場合は何も行わずreturnする。
 *
 * @param[in,out] key_value_ reset対象のkey/value object
 */
void glce_config_utility_key_value_reset(glce_config_utility_key_value_t* key_value_);

/**
 * @brief configuration logical lineをkey/valueへ解析する
 *
 * @details
 * 指定されたlogical lineをkey/value representationとして解析し、
 * syntaxが有効な場合はkeyおよびvalueをout_key_value_へ格納する。
 *
 * @par Public API Contract
 *
 * Input Representation:
 * - line bodyはchar_count_ byteで構成される。
 * - validなlogical line representationでは、
 *   line body内にNUL、CR、LFを含まず、
 *   line_[char_count_]が終端NULである。
 * - char_count_ == 0はvalidなlogical line representationとして扱うが、
 *   key/value syntaxとしてはGLCE_CONFIG_UTILITY_UNSUPPORTED_FORMATとなる。
 *
 * Key / Value Syntax:
 * - line bodyには'='が1文字だけ存在しなければならない。
 * - '='が存在しない場合、または複数存在する場合は
 *   GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMATとなる。
 * - '='の左側をkey、右側をvalueとして扱う。
 * - keyおよびvalueの前後にあるspace (' ') / horizontal tab ('\t')は
 *   tokenの一部として扱わない。
 * - keyおよびvalueは空であってはならない。
 * - keyおよびvalue内部にspace、horizontal tab、CR、LF、'='、NULを
 *   含んではならない。
 * - keyおよびvalueはそれぞれ
 *   GLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH文字以内でなければならない。
 *
 * Output Contract:
 * - out_key_value_はoperation開始時にreset stateでなければならない。
 * - reset stateとはkey[0] == '\0'かつvalue[0] == '\0'の状態をいう。
 * - GLCE_CONFIG_UTILITY_SUCCESSの場合、
 *   parsed keyおよびvalueをそれぞれNUL terminated stringとして格納する。
 * - outputはsource line storageへのreferenceを保持せず、
 *   source lineのlifetimeから独立する。
 *
 * Failure Contract:
 * - GLCE_CONFIG_UTILITY_SUCCESS以外を返した場合、
 *   out_key_value_の内容は変更しない。
 *
 * @param[in] line_ 解析対象となるlogical line
 * @param[in] char_count_ 終端NULを含まないline bodyのbyte数
 * @param[in,out] out_key_value_ parsed key/valueの格納先
 *
 * @retval GLCE_CONFIG_UTILITY_SUCCESS
 *         key/valueの解析に成功した
 * @retval GLCE_CONFIG_UTILITY_INVALID_ARGUMENT
 *         line_またはout_key_value_がNULL、または
 *         line_とchar_count_がvalidなlogical line representationを構成していない
 * @retval GLCE_CONFIG_UTILITY_BAD_OPERATION
 *         out_key_value_がreset stateではない
 * @retval GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMAT
 *         logical lineがsupported key/value syntaxを満たしていない。
 *         '='が存在しない場合または複数存在する場合もこれに含む
 *
 * @pre callerはline_[0]からline_[char_count_]までを
 *      読み取り可能なstorageとして提供すること。
 *
 * @post GLCE_CONFIG_UTILITY_SUCCESSの場合、
 *       out_key_value_はvalidなglce_config_utility_key_value_tとなる。
 */
glce_config_utility_result_t glce_config_utility_key_value_parse(const char* line_, size_t char_count_, glce_config_utility_key_value_t* out_key_value_);

/**
 * @brief key/value objectがvalidなconfiguration token representationか判定する
 *
 * @details
 * glce_config_utility_key_value_tが、
 * 本moduleで扱うkey/value representationとしてvalidか判定する。
 *
 * @par Public Validator Contract
 *
 * Validity Definition:
 * - key_value_はNULLではない。
 * - keyおよびvalueは、
 *   GLCE_CONFIG_UTILITY_TOKEN_BUFFER_SIZE以内に終端NULを含む
 *   NUL terminated stringである。
 * - keyおよびvalueはempty stringではない。
 * - keyおよびvalue内部にspace、horizontal tab、CR、LF、'='、NULを
 *   token characterとして含まない。
 * - keyおよびvalueの文字数は
 *   GLCE_CONFIG_UTILITY_TOKEN_MAX_LENGTH以下である。
 *
 * Validation Scope:
 * - 本validatorはkey/value tokenのstructural / lexical validityのみを判定する。
 * - supported key、required field、value range、value interpretation等の
 *   configuration format固有semanticは判定しない。
 *
 * @param[in] key_value_ 判定対象のkey/value object
 *
 * @retval true key/value objectがvalid
 * @retval false key/value objectがinvalid、またはkey_value_がNULL
 */
bool glce_config_utility_key_value_is_valid(const glce_config_utility_key_value_t* key_value_);

#ifdef __cplusplus
}
#endif
#endif
