// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/*
 * Module Internal Contract
 *
 * Canonical state:
 * - len + 1はsize_tで表現可能である。
 * - capacityはbufferとして使用可能なstorageのbyte数を表し、終端NULを格納する領域を含む。
 * - capacity == 0の場合、buffer == NULLである。
 * - capacity != 0の場合、buffer != NULLである。
 * - len != 0の場合、len + 1 <= capacityが成立する。
 * - buffer != NULLの場合、buffer[0]からbuffer[len - 1]までにNULを含まず、buffer[len]が終端NULである。
 * - len == 0かつcapacity == 0、buffer == NULLの状態は、空文字列を表すcanonical stateとする。
 * - len == 0かつcapacity > 0の場合も、buffer[0] == '\0'であれば空文字列を表すcanonical stateとする。
 *
 * Representation / Ownership:
 * - bufferはchoco_string_tが所有する文字列storageである。
 * - buffer != NULLの場合、そのstorageはGeneral Allocatorから取得したlive allocationである。
 * - choco_string_tはbufferのownershipを単独で保持し、そのlifetimeを管理する。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

/*
 * Module Validation Policy
 *
 * - ValidationはModule Internal Contractで定義したcanonical stateを基準として行う。
 *
 * - private shallow validatorは、choco_string_t自身のroot fieldのみを検証する。
 * - shallow validationではbufferが指すstorageをdereferenceせず、
 *   General Allocatorへのallocation queryも行わない。
 * - shallow validatorは、len + 1のrepresentability、lenとcapacityのrelation、
 *   capacityとbufferのNULL / non-NULL relationを検証する。
 *
 * - canonical validatorはshallow validationの成功後、owned bufferに対するvalidationを行う。
 * - buffer != NULLの場合は、bufferをdereferenceする前に
 *   general_allocator_ptr_is_allocated()でcurrent allocationであることを確認する。
 * - allocation validityを確認した後、buffer[len]が終端NULであること、および
 *   buffer[0]からbuffer[len - 1]までにNULが存在しないことを検証する。
 *
 * - canonical validatorは、引数string_自身のallocation validityを検証しない。
 *   string_をowned pointerとして保持するownerが、そのallocation validityを
 *   ownership closureの一部として検証する責務を持つ。
 * - canonical validatorにおいて、General Allocatorから取得できるallocation sizeと、len、capacityとの整合性の検証については今後検討する。
 *
 * - validatorは対象stateを変更せず、validation failure時はfalseを返す。
 * - explicit validatorのvalidation semanticsはBUILD_MODEによって変更しない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#include "engine/containers/choco_string.h"

#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h> // for SIZE_MAX

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

// ============================================================
// Private Type Definitions
// ============================================================
/**
 * @brief 文字列コンテナ内部状態管理構造体
 *
 */
struct choco_string {
    size_t len;         /**< 文字列長さ(終端文字は含まない) */
    size_t capacity;    /**< バッファサイズ */
    char* buffer;       /**< 文字列格納バッファ */
};

// ============================================================
// Private Constants
// ============================================================
static const char* const s_result_str_success = "SUCCESS";                    /**< 実行結果コード(成功)文字列 */
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コード(内部データ整合異常)文字列 */
static const char* const s_result_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コード(API誤用)文字列 */
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コード(無効な引数)文字列 */
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";        /**< 実行結果コード(実行時エラー)文字列 */
static const char* const s_result_str_no_memory = "NO_MEMORY";                /**< 実行結果コード(メモリ不足)文字列 */
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コード(未定義エラー)文字列 */
static const char* const s_result_str_overflow = "OVERFLOW";                  /**< 実行結果コード(計算過程でオーバーフロー発生)文字列 */
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コード(システム使用範囲上限超過) */

// ============================================================
// Private Function Declarations
// ============================================================
// Mock functions
static size_t mock_strlen(const char* str_);
static int mock_strcmp(const char *s1_, const char *s2_);

// Buffer operations
static choco_string_result_t buffer_reserve(size_t size_, choco_string_t* string_);
static choco_string_result_t buffer_resize(size_t size_, choco_string_t* string_);

// Utilities
static const char* result_to_str(choco_string_result_t result_);
static choco_string_result_t result_convert_general_allocator(general_allocator_result_t result_);

// Validators
static bool is_valid_shallow(const choco_string_t* string_);

// ============================================================
// Public API
// ============================================================
// choco_string_default_create Validation Policy
//
// - out_string_のpointer contract、および*out_string_ == NULLであることは、
//   output slotへ新規objectをcommitするために必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
// - *out_string_ != NULLは既存pointerを上書きするAPI misuseであるため、CHOCO_STRING_BAD_OPERATIONとして扱う。
//
// - General Allocatorはallocation成功時に取得領域を0で初期化するcontractを持つ。
//   choco_string_tの全fieldが0であるstateはcanonicalな空文字列を表すため、
//   allocation成功後のstateはoperation implementationと下位allocator contractから直接保証される。
// - このAPIの処理はリソースの確保のみであるため、canonical Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
choco_string_result_t choco_string_default_create(choco_string_t** out_string_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    choco_string_t* tmp_string = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_default_create", "out_string_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_string_, ret, CHOCO_STRING_BAD_OPERATION, result_to_str(CHOCO_STRING_BAD_OPERATION), "choco_string_default_create", "*out_string_")

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(*tmp_string), GENERAL_ALLOCATOR_MEMORY_TAG_STRING, (void**)&tmp_string);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("choco_string_default_create(%s) - Failed to allocate memory for 'tmp_string'.", result_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_string_ = tmp_string;
    tmp_string = NULL;

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_DATA_CORRUPTED != ret) {
        if(NULL != tmp_string) {
            general_allocator_free((void**)&tmp_string, GENERAL_ALLOCATOR_MEMORY_TAG_STRING);
        }
    }
    return ret;
}

// choco_string_create_from_c_string Validation Policy
//
// - src_およびout_string_のpointer contract、および*out_string_ == NULLであることは、
//   operationを開始し、新規objectを安全にcommitするために必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
// - *out_string_ != NULLは既存pointerを上書きするAPI misuseであるため、
//   CHOCO_STRING_BAD_OPERATIONとして扱う。
//
// - src_はModule Boundary Contractで定義された、有効な終端NUL付きC stringという
//   trusted representationとして扱う。
// - src_の文字列長は本operationが実際にconsumeするため取得するが、
//   C string representation自体を別のvalidatorで再認証しない。
// - src_len + 1をbuffer sizeとして使用するため、加算がsize_tの表現可能範囲を
//   超えないことをoperation-specific checked conditionとして全BUILDで検証する。
//
// - 本operationは既存のchoco_string_t stateをconsumeしないため、
//   PreconditionsでChoco String validatorを使用しない。
//
// - 本operationでは、object生成、owned buffer確保、C string dataのcopy、len更新という
//   複数のstate構築を行うため、DEBUG_BUILD / TEST_BUILDではpublic commit前のstable boundaryで
//   完成したtmp_stringにcanonical Postcondition validationを行う。
// - canonical Postcondition validationに成功した後だけ、tmp_stringのownershipを
//   *out_string_へcommitする。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
choco_string_result_t choco_string_create_from_c_string(const char* src_, choco_string_t** out_string_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* tmp_string = NULL;
    size_t src_len = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_create_from_c_string", "src_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_create_from_c_string", "out_string_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_string_, ret, CHOCO_STRING_BAD_OPERATION, result_to_str(CHOCO_STRING_BAD_OPERATION), "choco_string_create_from_c_string", "*out_string_")

    // Prepare.
    ret = choco_string_default_create(&tmp_string);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_create_from_c_string(%s) - Failed to create temporary string.", result_to_str(ret));
        goto cleanup;
    }

    src_len = mock_strlen(src_);
    if(0 != src_len) {
        if((SIZE_MAX - 1) < src_len) {
            ret = CHOCO_STRING_OVERFLOW;
            ERROR_MESSAGE("choco_string_create_from_c_string(%s) - Provided string is too large.", result_to_str(ret));
            goto cleanup;
        }
        ret = buffer_reserve(src_len + 1, tmp_string);
        if(CHOCO_STRING_SUCCESS != ret) {
            ERROR_MESSAGE("choco_string_create_from_c_string(%s) - Failed to reserve buffer space.", result_to_str(ret));
            goto cleanup;
        }
        memcpy(tmp_string->buffer, src_, src_len + 1);
        tmp_string->len = src_len;
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(tmp_string)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_create_from_c_string(%s) - Postcondition validation failed for 'tmp_string'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_string_ = tmp_string;
    tmp_string = NULL;

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_DATA_CORRUPTED != ret) {
        if(NULL != tmp_string) {
            choco_string_destroy(&tmp_string);
        }
    }
    return ret;
}

// choco_string_destroy Validation Policy
//
// - string_ == NULLまたは*string_ == NULLは、destroy対象が存在しない状態として
//   no-opで正常に終了する。
//
// - 本operationはchoco_string_tが所有するbufferを参照し、必要に応じて解放した後、
//   choco_string_t自身のstorageを解放する。
// - corrupted stateのowned bufferをtraverse / freeすることを避けるため、
//   DEBUG_BUILD / TEST_BUILDではresource解放前にcanonical validatorを実行する。
// - RELEASE_BUILDではinternal invariantのdiagnostic目的だけのautomatic canonical validationは行わない。
//
// - canonical validation failureは成立済みinternal objectのcorruptionとして扱い、
//   DATA_CORRUPTED相当のdiagnosticを出力した後、bufferおよびobjectの解放を行わず終了する。
// - logical lifetime終了後のPostcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
void choco_string_destroy(choco_string_t** string_) {
    if(NULL == string_) {
        goto cleanup;
    }
    if(NULL == *string_) {
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(*string_)) {
        ERROR_MESSAGE("choco_string_destroy(%s) - Precondition validation failed for '*string_'.", result_to_str(CHOCO_STRING_DATA_CORRUPTED));
        return;
    }
#endif

    if(NULL != (*string_)->buffer) {
        general_allocator_free((void**)&(*string_)->buffer, GENERAL_ALLOCATOR_MEMORY_TAG_STRING);
    }
    general_allocator_free((void**)string_, GENERAL_ALLOCATOR_MEMORY_TAG_STRING);
cleanup:
    return;
}

// choco_string_copy Validation Policy
//
// - src_およびdst_のpointer contractは、operationを開始するために必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - src_の文字列semanticおよびowned bufferをcopy sourceとして実際にconsumeするため、
//   DEBUG_BUILD / TEST_BUILDではsrc_にcanonical validationを行う。
// - dst_についても、既存owned bufferへのwriteまたはbufferの解放 / 再確保を行う可能性があり、
//   buffer allocation validityを含むstateを安全にconsumeするため、
//   DEBUG_BUILD / TEST_BUILDではcanonical validationを行う。
// - RELEASE_BUILDでは、正規APIを通して成立しているsrc_ / dst_のcanonical stateを
//   trusted internal contractとして扱い、automatic canonical validationは行わない。
//
// - src_ == dst_は許可されたself-copyとして扱い、stateを変更しない。
//   self-copyであることをBAD_OPERATIONとはしない。
//
// - dst_を変更した後のstable stateについて、DEBUG_BUILD / TEST_BUILDでは
//   mutationによるinternal invariant破損の局所化を目的としてcanonical Postcondition validationを行う。
// - Postcondition validation failureはCHOCO_STRING_DATA_CORRUPTEDとして扱う。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
choco_string_result_t choco_string_copy(const choco_string_t* src_, choco_string_t* dst_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy", "src_")
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy", "dst_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(src_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Precondition validation failed for 'src_'.", result_to_str(ret));
        goto cleanup;
    }
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Precondition validation failed for 'dst_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Commit.
    if(src_ == dst_) {
        // 何もしない
    } else {
        if(0 == src_->len) {
            if(NULL != dst_->buffer) {
                dst_->buffer[0] = '\0';
            }
            dst_->len = 0;
        } else {
            if(dst_->capacity >= (src_->len + 1)) {
                memcpy(dst_->buffer, src_->buffer, src_->len + 1);  // 終端文字を含めてコピー
                dst_->len = src_->len;
            } else {
                ret = buffer_resize(src_->len + 1, dst_);
                if(CHOCO_STRING_SUCCESS != ret) {
                    ERROR_MESSAGE("choco_string_copy(%s) - Failed to reserve buffer space.", result_to_str(ret));
                    goto cleanup;
                }
                memcpy(dst_->buffer, src_->buffer, src_->len + 1);  // 終端文字を含めてコピー
                dst_->len = src_->len;
            }
        }
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy(%s) - Postcondition validation failed for 'dst_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

// choco_string_copy_from_c_string Validation Policy
//
// - src_およびdst_のpointer contractは、operationを開始するために必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - src_はModule Boundary Contractで定義された、有効な終端NUL付きC stringという
//   trusted representationとして扱う。
// - src_の文字列長は本operationが実際にconsumeするため取得するが、
//   C string representation自体を別のvalidatorで再認証しない。
// - src_len + 1をbuffer sizeとして使用するため、加算がsize_tの表現可能範囲を
//   超えないことをoperation-specific checked conditionとして全BUILDで検証する。
//
// - 本operationは既存owned bufferへのwriteまたはbufferの解放 / 再確保を行う可能性があり、
//   buffer allocation validityを含むstateを安全にconsumeする必要がある。
// - Choco String moduleではbuffer allocation validityをcanonical depthで検証するため、
//   DEBUG_BUILD / TEST_BUILDではdst_にcanonical validationを行う。
// - RELEASE_BUILDではdst_のcanonical stateをtrusted internal contractとして扱い、
//   automatic canonical validationは行わない。
//
// - dst_を変更した後のstable stateについて、DEBUG_BUILD / TEST_BUILDでは
//   mutationによるinternal invariant破損の局所化を目的としてcanonical Postcondition validationを行う。
// - Postcondition validation failureはCHOCO_STRING_DATA_CORRUPTEDとして扱う。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
choco_string_result_t choco_string_copy_from_c_string(const char* src_, choco_string_t* dst_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;
    size_t src_len = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(src_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy_from_c_string", "src_")
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_copy_from_c_string", "dst_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Precondition validation failed for 'dst_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    src_len = mock_strlen(src_);
    if(0 == src_len) {
        if(NULL != dst_->buffer) {
            dst_->buffer[0] = '\0';
        }
        dst_->len = 0;
    } else {
        if((SIZE_MAX - 1) < src_len) {
            ret = CHOCO_STRING_OVERFLOW;
            ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Provided string is too large.", result_to_str(ret));
            goto cleanup;
        }
        if(dst_->capacity >= (src_len + 1)) {
            memcpy(dst_->buffer, src_, src_len + 1);
            dst_->len = src_len;
        } else {
            ret = buffer_resize(src_len + 1, dst_);
            if(CHOCO_STRING_SUCCESS != ret) {
                ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Failed to resize the buffer.", result_to_str(ret));
                goto cleanup;
            }
            memcpy(dst_->buffer, src_, src_len + 1);
            dst_->len = src_len;
        }
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_copy_from_c_string(%s) - Postcondition validation failed for 'dst_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

// choco_string_concat Validation Policy
//
// - string_およびdst_のpointer contractは、operationを開始するために必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
// - string_ == dst_の場合、本実装ではsourceとdestinationを同一objectとして扱う
//   concatを許可しないため、operation-specific checked preconditionとして
//   全BUILDで検証し、CHOCO_STRING_BAD_OPERATIONとして扱う。
//
// - string_の文字列内容およびowned bufferはconcat sourceとして実際にconsumeするため、
//   DEBUG_BUILD / TEST_BUILDではstring_にcanonical validationを行う。
// - dst_についても、既存文字列を保持したまま末尾へdataを追加し、
//   owned bufferへのread / writeまたはbufferの解放 / 再確保を行う可能性があるため、
//   DEBUG_BUILD / TEST_BUILDではcanonical validationを行う。
// - RELEASE_BUILDでは、正規APIを通して成立しているstring_ / dst_のcanonical stateを
//   trusted internal contractとして扱い、automatic canonical validationは行わない。
//
// - dst_->len + string_->len + 1を新しいstorage sizeとして使用するため、
//   この計算がsize_tの表現可能範囲を超えないことを
//   operation-specific checked conditionとして全BUILDで検証する。
//
// - dst_を変更した後のstable stateについて、DEBUG_BUILD / TEST_BUILDでは
//   mutationによるinternal invariant破損の局所化を目的として
//   canonical Postcondition validationを行う。
// - Postcondition validation failureはCHOCO_STRING_DATA_CORRUPTEDとして扱う。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
choco_string_result_t choco_string_concat(const choco_string_t* string_, choco_string_t* dst_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    size_t dst_len_new = 0;
    char* tmp_buffer = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_concat", "dst_")
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_concat", "string_")
    if(string_ == dst_) {
        ret = CHOCO_STRING_BAD_OPERATION;
        ERROR_MESSAGE("choco_string_concat(%s) - provided dst_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(string_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Precondition validation failed for 'string_'.", result_to_str(ret));
        goto cleanup;
    }
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Precondition validation failed for 'dst_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    if((SIZE_MAX - dst_->len - 1) < string_->len) {
        ret = CHOCO_STRING_OVERFLOW;
        ERROR_MESSAGE("choco_string_concat(%s) - Resulting string length is too large.", result_to_str(ret));
        goto cleanup;
    }
    if(0 != string_->len) {
        dst_len_new = string_->len + dst_->len;
        if((dst_len_new + 1) <= dst_->capacity) {
            memcpy(dst_->buffer + dst_->len, string_->buffer, string_->len + 1);
            dst_->len = dst_len_new;
        } else {
            ret_general_allocator = general_allocator_allocate(dst_len_new + 1, GENERAL_ALLOCATOR_MEMORY_TAG_STRING, (void**)&tmp_buffer);
            if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
                ret = result_convert_general_allocator(ret_general_allocator);
                ERROR_MESSAGE("choco_string_concat(%s) - Failed to allocate memory for 'tmp_buffer'.", result_to_str(ret));
                goto cleanup;
            }
            if(0 != dst_->len) {
                memcpy(tmp_buffer, dst_->buffer, dst_->len);
            }
            memcpy(tmp_buffer + dst_->len, string_->buffer, string_->len + 1);
            if(0 != dst_->capacity) {
                general_allocator_free((void**)&dst_->buffer, GENERAL_ALLOCATOR_MEMORY_TAG_STRING);
            }

            dst_->buffer = tmp_buffer;
            dst_->capacity = dst_len_new + 1;
            dst_->len = dst_len_new;
        }
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat(%s) - Postcondition validation failed for 'dst_'.", result_to_str(ret));
        goto cleanup;
    }
#endif
    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

// choco_string_concat_from_c_string Validation Policy
//
// - string_およびdst_のpointer contractは、operationを開始するために必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - string_はModule Boundary Contractで定義された、有効な終端NUL付きC stringという
//   trusted representationとして扱う。
// - string_の文字列長はconcat処理で実際にconsumeするため取得するが、
//   C string representation自体を別のvalidatorで再認証しない。
//
// - dst_は既存文字列を保持したまま末尾へdataを追加し、
//   owned bufferへのread / writeまたはbufferの解放 / 再確保を行う可能性があるため、
//   DEBUG_BUILD / TEST_BUILDではcanonical validationを行う。
// - RELEASE_BUILDではdst_のcanonical stateをtrusted internal contractとして扱い、
//   automatic canonical validationは行わない。
//
// - dst_->len + source文字列長 + 1を新しいstorage sizeとして使用するため、
//   この計算がsize_tの表現可能範囲を超えないことを
//   operation-specific checked conditionとして全BUILDで検証する。
//
// - dst_を変更した後のstable stateについて、DEBUG_BUILD / TEST_BUILDでは
//   mutationによるinternal invariant破損の局所化を目的として
//   canonical Postcondition validationを行う。
// - Postcondition validation failureはCHOCO_STRING_DATA_CORRUPTEDとして扱う。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
choco_string_result_t choco_string_concat_from_c_string(const char* string_, choco_string_t* dst_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    size_t dst_len_new = 0;
    size_t src_len = 0;
    char* tmp_buffer = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(dst_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_concat_from_c_string", "dst_")
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_concat_from_c_string", "string_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Precondition validation failed for 'dst_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Prepare.
    src_len = mock_strlen(string_);
    if((SIZE_MAX - dst_->len - 1) < src_len) {
        ret = CHOCO_STRING_OVERFLOW;
        ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Resulting string length is too large.", result_to_str(ret));
        goto cleanup;
    }
    if(0 != src_len) {
        dst_len_new = src_len + dst_->len;
        if((dst_len_new + 1) <= dst_->capacity) {
            memcpy(dst_->buffer + dst_->len, string_, src_len + 1);
            dst_->len = dst_len_new;
        } else {
            ret_general_allocator = general_allocator_allocate(dst_len_new + 1, GENERAL_ALLOCATOR_MEMORY_TAG_STRING, (void**)&tmp_buffer);
            if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
                ret = result_convert_general_allocator(ret_general_allocator);
                ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Failed to allocate memory for 'tmp_buffer'.", result_to_str(ret));
                goto cleanup;
            }
            if(0 != dst_->len) {
                memcpy(tmp_buffer, dst_->buffer, dst_->len);
            }
            memcpy(tmp_buffer + dst_->len, string_, src_len + 1);
            if(0 != dst_->capacity) {
                general_allocator_free((void**)&dst_->buffer, GENERAL_ALLOCATOR_MEMORY_TAG_STRING);
            }

            dst_->buffer = tmp_buffer;
            dst_->capacity = dst_len_new + 1;
            dst_->len = dst_len_new;
        }
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!choco_string_is_valid(dst_)) {
        ret = CHOCO_STRING_DATA_CORRUPTED;
        ERROR_MESSAGE("choco_string_concat_from_c_string(%s) - Postcondition validation failed for 'dst_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

// choco_string_length Validation Policy
//
// - string_ == NULLの場合は、APIで定義されたfallback valueとして0を返す。
//
// - string_ != NULLの場合、本operationが実際にconsumeするchoco_string_t stateは
//   len fieldのみであり、owned bufferおよび文字列内容を参照しない。
// - lenの取得に不要なcapacity / buffer relationまたは文字列semanticを
//   再認証するためだけのshallow / canonical validationは行わない。
// - non-NULLのstring_は、Module Boundary Contractを満たすlifetime中のobjectへの
//   pointerであることをtrusted contractとして扱う。
//
// - 本operationはobject stateを変更しないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
size_t choco_string_length(const choco_string_t* string_) {
    if(NULL == string_) {
        return 0;
    } else {
        return string_->len;
    }
}

// choco_string_c_str Validation Policy
//
// - string_ == NULLの場合は、APIで定義されたfallback valueとして空C stringを返す。
//
// - string_ != NULLの場合、本operationはbuffer pointerの値のみを参照し、
//   bufferが指すstorageまたは文字列内容をdereferenceしない。
// - buffer allocation validityや文字列semanticを本operation自身がconsumeしないため、
//   それらを再認証するためだけのshallow / canonical validationは行わない。
// - non-NULLのstring_は、Module Boundary Contractを満たすlifetime中のobjectへの
//   pointerであることをtrusted contractとして扱う。
//
// - buffer == NULLの場合はcanonicalな空文字列representationとして
//   APIで定義された空C stringを返す。
// - buffer != NULLの場合は、choco_string_tが所有する文字列storageへの
//   borrowed pointerを返す。
//
// - 本operationはobject stateを変更しないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
const char* choco_string_c_str(const choco_string_t* string_) {
    if(NULL == string_) {
        return "";
    } else {
        if(NULL != string_->buffer) {
            return string_->buffer;
        } else {
            return "";
        }
    }
}

// choco_string_is_equal Validation Policy
//
// - str1_ == NULLまたはstr2_ == NULLの場合はfalseを返す。
//
// - non-NULLのstr1_およびstr2_は、Module Boundary Contractで定義された
//   有効な終端NUL付きC stringというtrusted representationとして扱う。
// - 本operationはC stringの文字列内容を比較のためにconsumeするが、
//   source representationそのものを事前に別のvalidatorで再認証しない。
//
// - 本operationはchoco_string_t objectを受け取らないため、
//   Choco Stringのshallow / canonical validatorは使用しない。
// - state mutationを行わないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool choco_string_is_equal(const char* str1_, const char* str2_) {
    if(NULL == str1_ || NULL == str2_) {
        return false;
    }
    return (0 == mock_strcmp(str1_, str2_) ? true : false);
}

// choco_string_substring_exists Validation Policy
//
// - str_ == NULLまたはtarget_ == NULLの場合はfalseを返す。
//
// - non-NULLのstr_およびtarget_は、Module Boundary Contractで定義された
//   有効な終端NUL付きC stringというtrusted representationとして扱う。
// - 本operationはsubstring検索のために両C stringの文字列内容をconsumeするが、
//   source representationそのものを事前に別のvalidatorで再認証しない。
//
// - 本operationはchoco_string_t objectを受け取らないため、
//   Choco Stringのshallow / canonical validatorは使用しない。
// - state mutationを行わないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool choco_string_substring_exists(const char* str_, const char* target_) {
    if(NULL == str_ || NULL == target_) {
        return false;
    }

    const char* result = strstr(str_, target_);
    return (NULL == result) ? false : true;
}

// choco_string_key_value_key_get Validation Policy
//
// - line_およびout_key_のpointer contractは、operationを開始するために必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - line_はModule Boundary Contractで定義された、有効な終端NUL付きC stringという
//   trusted representationとして扱う。
// - 本operationはkey-value形式を解析するためにline_の文字列内容をconsumeするが、
//   C string representation自体を別のvalidatorで再認証しない。
//
// - '='が存在しない場合、または'='の前に有効なkey文字列が存在しない場合は、
//   本operationが要求するkey-value形式を満たさないため
//   CHOCO_STRING_BAD_OPERATIONとして扱う。
//
// - out_key_が指すchoco_string_tの既存stateを本operation自身では直接consumeせず、
//   生成したtemporary C stringをchoco_string_copy_from_c_string()へ転送する。
// - out_key_の既存stateに対するvalidationおよびmutation後のcanonical validityの保証は、
//   semantic ownerであるchoco_string_copy_from_c_string()へ委譲する。
// - このため、本operationではout_key_に対するshallow / canonical validationを
//   重複して実行せず、automatic Postcondition validationも行わない。
//
// - temporary bufferのallocation / releaseについてはGeneral Allocatorのcontractへ委譲する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
choco_string_result_t choco_string_key_value_key_get(const char* line_, choco_string_t* out_key_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    char* tmp_buff = NULL;
    size_t len = 0;
    size_t equal_index = 0;
    size_t start_index = 0;
    size_t buff_size = 0;
    bool equal_found = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_key_value_key_get", "line_")
    IF_ARG_NULL_GOTO_CLEANUP(out_key_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_key_value_key_get", "out_key_")

    // Prepare.
    len = mock_strlen(line_);
    for(size_t i = 0; i != len; ++i) {
        if('=' == line_[i]) {
            equal_found = true;
            equal_index = i;
            break;
        }
    }
    if(!equal_found || 0 == equal_index) {
        ret = CHOCO_STRING_BAD_OPERATION;
        // ファイルをロードし、xxx = yyyの行じゃなければ無視する処理を想定し、メッセージは出さない
        goto cleanup;
    }

    for(size_t i = 0; i != equal_index; ++i) {
        if(' ' == line_[i]) {
            start_index++;
        } else {
            break;
        }
    }
    if(equal_index == start_index) {    // =の前に有効文字がない
        ret = CHOCO_STRING_BAD_OPERATION;
        // ファイルをロードし、xxx = yyyの行じゃなければ無視する処理を想定し、メッセージは出さない
        goto cleanup;
    }

    buff_size = equal_index - start_index + 1;

    ret_general_allocator = general_allocator_allocate(buff_size, GENERAL_ALLOCATOR_MEMORY_TAG_STRING, (void**)&tmp_buff);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("choco_string_key_value_key_get(%s) - Failed to get key-value key. reason=tmp_buffer_allocate_failed, bytes=%zu", result_to_str(ret), buff_size);
        goto cleanup;
    }

    for(size_t i = start_index, j = 0; i != equal_index; ++i, ++j) {
        tmp_buff[j] = line_[i];
    }

    for(size_t i = (buff_size - 1); i != 0; --i) {
        if(' ' == tmp_buff[i - 1]) {
            tmp_buff[i - 1] = '\0';
        } else {
            break;
        }
    }

    ret = choco_string_copy_from_c_string(tmp_buff, out_key_);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_key_value_key_get(%s) - choco_string_copy_from_c_string failed.", result_to_str(ret));
        goto cleanup;
    }

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_DATA_CORRUPTED != ret) {
        if(NULL != tmp_buff) {
            general_allocator_free((void**)&tmp_buff, GENERAL_ALLOCATOR_MEMORY_TAG_STRING);
        }
    }

    return ret;
}

// choco_string_key_value_value_get Validation Policy
//
// - line_およびout_value_のpointer contractは、operationを開始するために必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - line_はModule Boundary Contractで定義された、有効な終端NUL付きC stringという
//   trusted representationとして扱う。
// - 本operationはkey-value形式を解析するためにline_の文字列内容をconsumeするが、
//   C string representation自体を別のvalidatorで再認証しない。
//
// - '='が存在しない場合、'='の前に有効なkey文字列が存在しない場合、
//   または'='の後に有効なvalue文字列が存在しない場合は、
//   本operationが要求するkey-value形式を満たさないため
//   CHOCO_STRING_BAD_OPERATIONとして扱う。
//
// - out_value_が指すchoco_string_tの既存stateを本operation自身では直接consumeせず、
//   生成したtemporary C stringをchoco_string_copy_from_c_string()へ転送する。
// - out_value_の既存stateに対するvalidationおよびmutation後のcanonical validityの保証は、
//   semantic ownerであるchoco_string_copy_from_c_string()へ委譲する。
// - このため、本operationではout_value_に対するshallow / canonical validationを
//   重複して実行せず、automatic Postcondition validationも行わない。
//
// - temporary bufferのallocation / releaseについてはGeneral Allocatorのcontractへ委譲する。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
choco_string_result_t choco_string_key_value_value_get(const char* line_, choco_string_t* out_value_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    char* tmp_buff = NULL;
    size_t len = 0;
    size_t equal_index = 0;
    size_t buff_size = 0;
    bool equal_found = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(line_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_key_value_value_get", "line_")
    IF_ARG_NULL_GOTO_CLEANUP(out_value_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "choco_string_key_value_value_get", "out_value_")

    // Prepare.
    len = mock_strlen(line_);
    for(size_t i = 0; i != len; ++i) {
        if('=' == line_[i]) {
            equal_found = true;
            equal_index = i;
            break;
        }
    }
    if(!equal_found || 0 == equal_index || (len - 1) == equal_index) {
        ret = CHOCO_STRING_BAD_OPERATION;
        // ファイルをロードし、xxx = yyyの行じゃなければ無視する処理を想定し、メッセージは出さない
        goto cleanup;
    }

    // =の後のスペースをtrim
    for(size_t i = (equal_index + 1); i != len; ++i) {
        if(' ' == line_[i]) {
            equal_index++;
        } else {
            break;
        }
    }
    if((len - 1) == equal_index) {
        ret = CHOCO_STRING_BAD_OPERATION;
        goto cleanup;
    }

    buff_size = len - equal_index;

    ret_general_allocator = general_allocator_allocate(buff_size, GENERAL_ALLOCATOR_MEMORY_TAG_STRING, (void**)&tmp_buff);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("choco_string_key_value_value_get(%s) - Failed to get key-value value. reason=tmp_buffer_allocate_failed, bytes=%zu", result_to_str(ret), buff_size);
        goto cleanup;
    }

    for(size_t i = (equal_index + 1), j = 0; i != len; ++i, ++j) {
        tmp_buff[j] = line_[i];
    }

    // 末尾のスペースをtrim
    for(size_t i = (len - equal_index - 1); i != 0; --i) {
        if(tmp_buff[i - 1] != ' ') {
            break;
        } else {
            tmp_buff[i - 1] = '\0';
        }
    }
    ret = choco_string_copy_from_c_string(tmp_buff, out_value_);
    if(CHOCO_STRING_SUCCESS != ret) {
        ERROR_MESSAGE("choco_string_key_value_value_get(%s) - choco_string_copy_from_c_string failed.", result_to_str(ret));
        goto cleanup;
    }

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    if(CHOCO_STRING_DATA_CORRUPTED != ret) {
        if(NULL != tmp_buff) {
            general_allocator_free((void**)&tmp_buff, GENERAL_ALLOCATOR_MEMORY_TAG_STRING);
        }
    }

    return ret;
}

// choco_string_is_valid Validation Policy
//
// - 本APIはchoco_string_tのpublic canonical validatorである。
// - string_ == NULLの場合はfalseを返す。
// - Module Internal Contractで定義されたcanonical state全体を検証する。
//
// - canonical validationでは最初にprivate shallow validatorを実行し、
//   root field間の局所的なstructural invariantを検証する。
// - shallow validationに成功した後、buffer != NULLの場合は
//   general_allocator_ptr_is_allocated()でowned bufferがcurrent allocationであることを
//   確認してからbufferをdereferenceする。
// - allocation validity確認後、文字列領域にembedded NULが存在しないこと、および
//   文字列長に対応する位置に終端NULが存在することを検証する。
//
// - string_自身のallocation validityは本validatorでは検証しない。
//   string_をowned pointerとして保持するowner側が、そのallocation validityを
//   ownership closureの一部として検証する。
//
// - bufferのactual allocation sizeとcapacityの整合性検証については、
//   allocation metadataの利用方法と合わせて将来検討する。
//
// - explicit validatorであるため、BUILD_MODEによってvalidation semanticsを変更しない。
// - validation中に対象stateを変更しない。
// - validation failure時はfalseを返すのみとし、error messageは出力しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool choco_string_is_valid(const choco_string_t* string_) {
    if(NULL == string_) {
        return false;
    }
    if(!is_valid_shallow(string_)) {
        return false;
    }
    if(NULL != string_->buffer) {
        if(!general_allocator_ptr_is_allocated((const void*)string_->buffer)) {
            return false;
        }
        for(size_t i = 0; i != string_->len; ++i) {
            if('\0' == string_->buffer[i]) {
                return false;
            }
        }
        if(0 != string_->len && '\0' != string_->buffer[string_->len]) {
            return false;
        } else if(0 == string_->len && 0 < string_->capacity && '\0' != string_->buffer[0]) {
            return false;
        }
    }
    return true;
}

// ============================================================
// Mock functions
// ============================================================
static size_t NO_COVERAGE mock_strlen(const char* str_) {
    return strlen(str_);
}

static int NO_COVERAGE mock_strcmp(const char *s1_, const char *s2_) {
    return strcmp(s1_, s2_);
}

// ============================================================
// Buffer operations
// ============================================================

// string_のbufferのメモリを初回に確保するためのAPI。既にbufferのメモリを確保済の場合にはbuffer_resizeを使用する
// 処理に失敗した場合(返り値がCHOCO_STRING_SUCCESS以外)には引数のstring_の状態は不変。
static choco_string_result_t buffer_reserve(size_t size_, choco_string_t* string_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    char* tmp_buffer = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_reserve", "string_")
    if(0 == size_) {
        ret = CHOCO_STRING_INVALID_ARGUMENT;
        ERROR_MESSAGE("buffer_reserve(%s) - Provided size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(0 != string_->capacity) {
        ret = CHOCO_STRING_BAD_OPERATION;
        ERROR_MESSAGE("buffer_reserve(%s) - Provided string_ is not empty.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(size_, GENERAL_ALLOCATOR_MEMORY_TAG_STRING, (void**)&tmp_buffer);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        goto cleanup;
    }
    string_->buffer = tmp_buffer;
    string_->len = 0;
    string_->capacity = size_;

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

// 既に確保済のbufferサイズを変更(拡張or縮小)する場合に使用する。
// 初回のメモリ確保に使用することも可能だが、実行速度がbuffer_reserveの方が若干速いため、そちらの使用を推奨する。
// 処理に失敗した場合(返り値がCHOCO_STRING_SUCCESS以外)はstring_の状態は不変。
// サイズを変更後、bufferのデータは全て0に初期化され、lenの値も0になる。
// データをそのまま残してもよいが、サイズを縮小した場合にはデータが削られることになる。
// この関数を読んだ後のbufferの状態を拡大、縮小共に共通にしたいため、全て0に初期化することにする
// このため、バッファの拡張を目的に本関数を使用する場合には一旦内部データを退避してから呼び出すこと。
static choco_string_result_t buffer_resize(size_t size_, choco_string_t* string_) {
    choco_string_result_t ret = CHOCO_STRING_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    char* tmp_buffer = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(string_, ret, CHOCO_STRING_INVALID_ARGUMENT, result_to_str(CHOCO_STRING_INVALID_ARGUMENT), "buffer_resize", "string_")
    if(0 == size_) {
        ret = CHOCO_STRING_INVALID_ARGUMENT;
        ERROR_MESSAGE("buffer_resize(%s) - Provided size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Simulation.
    ret_general_allocator = general_allocator_allocate(size_, GENERAL_ALLOCATOR_MEMORY_TAG_STRING, (void**)&tmp_buffer);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        goto cleanup;
    }

    // Commit.
    if(0 != string_->capacity) {
        general_allocator_free((void**)&string_->buffer, GENERAL_ALLOCATOR_MEMORY_TAG_STRING);
    }
    string_->buffer = tmp_buffer;
    string_->len = 0;
    string_->capacity = size_;

    ret = CHOCO_STRING_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Utilities
// ============================================================

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param[in] result_ 文字列に変換する実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* result_to_str(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return s_result_str_success;
    case CHOCO_STRING_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case CHOCO_STRING_BAD_OPERATION:
        return s_result_str_bad_operation;
    case CHOCO_STRING_NO_MEMORY:
        return s_result_str_no_memory;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case CHOCO_STRING_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    case CHOCO_STRING_OVERFLOW:
        return s_result_str_overflow;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    default:
        return s_result_str_undefined_error;
    }
}

static choco_string_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return CHOCO_STRING_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return CHOCO_STRING_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return CHOCO_STRING_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return CHOCO_STRING_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return CHOCO_STRING_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return CHOCO_STRING_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return CHOCO_STRING_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return CHOCO_STRING_UNDEFINED_ERROR;
    default:
        return CHOCO_STRING_UNDEFINED_ERROR;
    }
}

// ============================================================
// Validators
// ============================================================

// is_valid_shallow Validation Policy
//
// - 本helperはchoco_string_tのprivate shallow validatorである。
// - string_ == NULLの場合はfalseを返す。
//
// - shallow validationではchoco_string_t自身のroot fieldのみを検証し、
//   bufferが指すstorageをdereferenceしない。
// - owned bufferのallocation validityも検証せず、
//   general_allocator_ptr_is_allocated()は呼び出さない。
//
// - len + 1がsize_tで表現可能であることを検証する。
// - len != 0の場合、終端NULを格納する領域を含めて
//   len + 1 <= capacityが成立することを検証する。
// - capacity == 0の場合はbuffer == NULL、
//   capacity != 0の場合はbuffer != NULLであることを検証する。
//
// - bufferのallocation validity、文字列領域内のembedded NUL、
//   およびbuffer[len]の終端NULはcanonical validationの責務とし、
//   本helperでは検証しない。
//
// - 本helperは対象stateを変更しない。
// - validation failure時はfalseを返すのみとし、error messageは出力しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
static bool is_valid_shallow(const choco_string_t* string_) {
    if(NULL == string_) {
        return false;
    }
    if((SIZE_MAX - 1) < string_->len) {
        return false;
    } else if(string_->capacity < (string_->len + 1) && 0 != string_->len) {
        return false;
    } else if(0 == string_->capacity && NULL != string_->buffer) {
        return false;
    } else if(0 != string_->capacity && NULL == string_->buffer) {
        return false;
    }
    return true;
}
