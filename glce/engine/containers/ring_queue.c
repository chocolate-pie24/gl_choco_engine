// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/containers/ring_queue.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h> // for memcpy

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/memory_utility.h"

#include "engine/memory/general_allocator/general_allocator.h"

/*
 * Module Internal Contract
 *
 * Canonical state:
 * - max_element_countは0より大きい。
 * - element_sizeは0より大きい。
 * - element_alignは0より大きい2の冪乗であり、alignof(max_align_t)以下である。
 * - strideはelement_sizeをelement_align境界へ切り上げた値である。
 * - strideは0より大きい。
 * - capacityはstride * max_element_countで表され、この計算はsize_tの表現可能範囲内である。
 *
 * - memory_poolはNULLではない。
 * - memory_poolはelement_align境界にalignmentされている。
 * - memory_pool上の各slotはstride byte間隔で配置され、
 *   各slotの先頭element_size byteを要素dataの格納領域として使用する。
 *
 * - headおよびtailはmax_element_count未満である。
 * - lenはmax_element_count以下である。
 * - len == 0の場合、head == 0かつtail == 0である。
 * - tailはheadからlen個の要素をring上で進めた位置と一致する。
 * - len == max_element_countの場合、tail == headとなる。
 *
 * Representation / Ownership:
 * - ring_queue_tはmemory_poolを単独で所有し、そのlifetimeを管理する。
 * - memory_poolはGeneral Allocatorから取得したlive allocationである。
 * - capacityはqueueが要素storageとして必要とするlogical byte sizeを表す。
 * - queueへ格納された要素はmemory_pool内へbyte sequenceとしてcopyされる。
 * - 要素data内部に含まれるpointer等が参照するresourceはring_queue_tのownershipには含まれない。
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
 * - private shallow validatorは、ring_queue_t自身のroot fieldと、
 *   root field間の局所的なstructural relationを検証する。
 * - shallow validationではmemory_poolが指すstorageをdereferenceせず、
 *   General Allocatorへのallocation queryも行わない。
 * - shallow validatorは、element configuration、stride / capacity relation、
 *   head / tail / lenのrange、empty state、memory_poolの存在およびalignmentを検証する。
 *
 * - canonical validatorは、owned memory_poolがGeneral Allocator上の
 *   current allocationであることを確認する。
 * - canonical validatorは、head、tail、lenおよびmax_element_countから定まる
 *   ring上のlogical relationを検証する。
 *
 * - ring_queue_tは格納要素をgenericなbyte sequenceとして扱うため、
 *   queue内部に格納された各要素のsemantic validityは
 *   Ring Queue moduleのcanonical validation対象としない。
 * - memory_pool内部のpadding領域についても、特定のbyte patternを要求しない。
 *
 * - canonical validatorは、引数queue_自身のallocation validityを検証しない。
 *   queue_をowned pointerとして保持するownerが、そのallocation validityを
 *   ownership closureの一部として検証する責務を持つ。
 *
 * - General Allocatorから取得可能なactual allocation sizeとcapacityの
 *   整合性検証については、allocation metadataの利用方法と合わせて将来検討する。
 *
 * - validatorは対象stateを変更せず、validation failure時はfalseを返す。
 * - explicit validatorのvalidation semanticsはBUILD_MODEによって変更しない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
/**
 * @brief ring_queue_t内部データ構造
 *
 */
struct ring_queue {
    size_t head;                /**< リングキュー配列の先頭インデックス */
    size_t tail;                /**< リングキューに次に追加する要素インデックス */
    size_t len;                 /**< リングキューに格納済みの要素数 */
    size_t element_align;       /**< リングキューに格納する要素のアライメント要件 */
    size_t max_element_count;   /**< リングキューに格納可能な最大要素数 */
    size_t element_size;        /**< 格納要素のサイズ(パディングサイズを含まない実際の構造体のサイズ) */
    size_t stride;              /**< 1要素に必要なメモリ領域(element_size + padding) */
    size_t capacity;            /**< memory_poolのサイズ */
    void* memory_pool;          /**< 要素を格納するバッファ */
};

// ============================================================
// Private Constants
// ============================================================
static const char* const s_result_str_success = "SUCCESS";                    /**< リングキューAPI実行結果コード(処理成功)に対応する文字列 */
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";  /**< リングキューAPI実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_result_str_no_memory = "NO_MEMORY";                /**< リングキューAPI実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";        /**< リングキューAPI実行結果コード(実行時エラー)に対応する文字列 */
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";    /**< リングキューAPI実行結果コード(未定義エラー)に対応する文字列 */
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< リングキューAPI実行結果コード(システム使用可能範囲上限超過)に対応する文字列 */
static const char* const s_result_str_bad_operation = "BAD_OPERATION";        /**< リングキューAPI実行結果コード(API誤用)に対応する文字列 */
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";      /**< リングキューAPI実行結果コード(内部データ破損)に対応する文字列 */
static const char* const s_result_str_overflow = "OVERFLOW";                  /**< リングキューAPI実行結果コード(計算過程でオーバーフロー発生)に対応する文字列 */
static const char* const s_result_str_empty = "EMPTY";                        /**< リングキューAPI実行結果コード(キューが空)に対応する文字列 */

// ============================================================
// Private Function Declarations
// ============================================================
// Utilities
static const char* result_to_str(ring_queue_result_t result_);
static ring_queue_result_t result_convert_general_allocator(general_allocator_result_t result_);

// Validators
static bool is_valid_shallow(const ring_queue_t* queue_);

// ============================================================
// Public API
// ============================================================

// ring_queue_create Validation Policy
//
// - out_queue_のpointer contract、および*out_queue_ == NULLであることは、
//   新規objectを安全にcommitするために必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
// - *out_queue_ != NULLは既存pointerを上書きするAPI misuseであるため、
//   RING_QUEUE_BAD_OPERATIONとして扱う。
//
// - max_element_count_およびelement_size_が0より大きいこと、
//   element_align_が0より大きい2の冪乗かつalignof(max_align_t)以下であることは、
//   queue storage layoutを構築するために必要なchecked preconditionとして
//   RELEASE_BUILDを含む全BUILDで検証する。
//
// - element_size_をelement_align_へ切り上げたstride、および
//   stride * max_element_count_で求めるcapacityがsize_tの表現可能範囲内であることを、
//   operation-specific checked conditionとして全BUILDで検証する。
//
// - 本operationは既存のring_queue_t stateをconsumeしないため、
//   PreconditionsでRing Queue validatorを使用しない。
//
// - 本operationでは、object生成、owned memory_pool確保、storage layout設定、
//   queue state初期化という複数のstate構築を行うため、
//   DEBUG_BUILD / TEST_BUILDではpublic commit前のstable boundaryで
//   完成したtmp_queueにcanonical Postcondition validationを行う。
// - canonical Postcondition validationに成功した後だけ、tmp_queueのownershipを
//   *out_queue_へcommitする。
//
// - DATA_CORRUPTED確定後はtemporary resourceを含む通常cleanupを行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
ring_queue_result_t ring_queue_create(size_t max_element_count_, size_t element_size_, size_t element_align_, ring_queue_t** out_queue_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    ring_queue_t* tmp_queue = NULL;
    void* tmp_memory_pool = NULL;
    size_t capacity = 0;
    size_t stride = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_queue_, ret, RING_QUEUE_INVALID_ARGUMENT, result_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_create", "out_queue_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_queue_, ret, RING_QUEUE_BAD_OPERATION, result_to_str(RING_QUEUE_BAD_OPERATION), "ring_queue_create", "*out_queue_")
    if(0 == max_element_count_ || 0 == element_size_) {
        ret = RING_QUEUE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ring_queue_create(%s) - Provided max_element_count_ or element_size_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(0 == element_align_ || !IS_POWER_OF_TWO(element_align_) || element_align_ > alignof(max_align_t)) {
        ret = RING_QUEUE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ring_queue_create(%s) - Provided element_align_ is not valid.", result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    if(!memory_utility_align_up(element_size_, element_align_, &stride)) {
        ret = RING_QUEUE_OVERFLOW;
        ERROR_MESSAGE("ring_queue_create(%s) - memory_utility_align_up failed.", result_to_str(ret));
        goto cleanup;
    }
    if(SIZE_MAX / max_element_count_ < stride) {
        ret = RING_QUEUE_OVERFLOW;
        ERROR_MESSAGE("ring_queue_create(%s) - ring_queue overflow.", result_to_str(ret));
        goto cleanup;
    }
    capacity = stride * max_element_count_;

    ret_general_allocator = general_allocator_allocate(sizeof(ring_queue_t), GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE, (void**)&tmp_queue);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("ring_queue_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(capacity, GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE, (void**)&tmp_memory_pool);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("ring_queue_create(%s) - general_allocator_allocate failed.", result_to_str(ret));
        goto cleanup;
    }

    tmp_queue->capacity = capacity;
    tmp_queue->memory_pool = tmp_memory_pool;
    tmp_queue->element_align = element_align_;
    tmp_queue->element_size = element_size_;
    tmp_queue->head = 0;
    tmp_queue->len = 0;
    tmp_queue->max_element_count = max_element_count_;
    tmp_queue->stride = stride;
    tmp_queue->tail = 0;

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(tmp_queue)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_create(%s) - Postcondition validation failed for 'tmp_queue'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_queue_ = tmp_queue;
    tmp_queue = NULL;
    tmp_memory_pool = NULL;

    ret = RING_QUEUE_SUCCESS;

cleanup:
    if(RING_QUEUE_DATA_CORRUPTED != ret) {
        if(NULL != tmp_memory_pool) {
            general_allocator_free((void**)&tmp_memory_pool, GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE);
        }
        if(NULL != tmp_queue) {
            general_allocator_free((void**)&tmp_queue, GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE);
        }
    }
    return ret;
}

// ring_queue_destroy Validation Policy
//
// - queue_ == NULLまたは*queue_ == NULLは、destroy対象が存在しない状態としてno-opで正常に終了する。
//
// - 本operationはring_queue_tが所有するmemory_poolを解放した後、ring_queue_t自身のstorageを解放する。
// - corrupted stateに基づいてowned resourceを解放することを避けるため、
//   DEBUG_BUILD / TEST_BUILDではresource解放前にcanonical validatorを実行する。
// - RELEASE_BUILDではinternal invariantのdiagnostic目的だけの
//   automatic canonical validationは行わない。
//
// - canonical validation failureは成立済みinternal objectのcorruptionとして扱い、
//   DATA_CORRUPTED相当のdiagnosticを出力した後、
//   memory_poolおよびobject自身の解放を行わず終了する。
// - logical lifetime終了後のPostcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
void ring_queue_destroy(ring_queue_t** queue_) {
    if(NULL == queue_) {
        return;
    }
    if(NULL == *queue_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(*queue_)) {
        ERROR_MESSAGE("ring_queue_destroy(%s) - Precondition validation failed for '*queue_'.", result_to_str(RING_QUEUE_DATA_CORRUPTED));
        return;
    }
#endif

    if(NULL != (*queue_)->memory_pool) {
        general_allocator_free((void**)&(*queue_)->memory_pool, GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE);
    }
    general_allocator_free((void**)queue_, GENERAL_ALLOCATOR_MEMORY_TAG_RING_QUEUE);
}

// ring_queue_push Validation Policy
//
// - queue_およびdata_のpointer contractは、operationを開始するために必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - element_size_およびelement_align_は、生成時に固定されたqueueの要素形式と一致している必要がある。
// - 両者の不一致は、本operationへ異なる要素形式を渡したcaller-controlled conditionとして
//   全BUILDで検証し、RING_QUEUE_INVALID_ARGUMENTとして扱う。
//
// - data_は少なくともqueueのelement_size byteを読み取り可能なstorageを指すことをcaller contractとして扱う。
// - data_が指すstorageのallocation identityやlivenessをRing Queue module自身では再認証しない。
//
// - 本operationはtail slotのowned storageへwriteし、head / tail / lenをconsumeして
//   queue stateを更新するため、DEBUG_BUILD / TEST_BUILDでは
//   mutation前にqueue_へcanonical validationを行う。
// - RELEASE_BUILDでは正規APIを通して成立しているqueue_のcanonical stateを
//   trusted internal contractとして扱い、automatic canonical validationは行わない。
//
// - queueが満杯であることはvalidなruntime stateであり、failureとして扱わない。
//   この場合は最古の要素を破棄して新しい要素を格納する。
//
// - mutation後のstable stateについて、DEBUG_BUILD / TEST_BUILDでは
//   internal invariant破損の局所化を目的としてcanonical Postcondition validationを行う。
// - Postcondition validation failureはRING_QUEUE_DATA_CORRUPTEDとして扱う。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
ring_queue_result_t ring_queue_push(const void* data_, size_t element_size_, size_t element_align_, ring_queue_t* queue_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;

    char* mem_ptr = NULL;
    char* target_ptr = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(queue_, ret, RING_QUEUE_INVALID_ARGUMENT, result_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "queue_")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RING_QUEUE_INVALID_ARGUMENT, result_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_push", "data_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_push(%s) - Precondition validation failed for 'queue_'.", result_to_str(ret));
        goto cleanup;
    }
#endif
    if(queue_->element_size != element_size_ || queue_->element_align != element_align_) {
        ret = RING_QUEUE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ring_queue_push(%s) - Provided element_size_ or element_align_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(queue_->max_element_count == queue_->len) {
        DEBUG_MESSAGE("Ring queue is full; overwriting the oldest element.");
    }

    // Commit.
    mem_ptr = (char*)queue_->memory_pool;
    target_ptr = mem_ptr + (queue_->stride * queue_->tail);
    memcpy(target_ptr, data_, queue_->element_size);
    queue_->tail = (queue_->tail + 1) % queue_->max_element_count;
    if(queue_->len != queue_->max_element_count) {
        queue_->len++;
    } else {
        queue_->head = (queue_->head + 1) % queue_->max_element_count;
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_push(%s) - Postcondition validation failed for 'queue_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RING_QUEUE_SUCCESS;

cleanup:
    return ret;
}

// ring_queue_pop Validation Policy
//
// - queue_およびout_data_のpointer contractは、operationを開始するために必要な
//   checked preconditionとしてRELEASE_BUILDを含む全BUILDで検証する。
//
// - element_size_およびelement_align_は、生成時に固定されたqueueの要素形式と一致している必要がある。
// - 両者の不一致は、本operationへ異なる要素形式を指定したcaller-controlled conditionとして
//   全BUILDで検証し、RING_QUEUE_INVALID_ARGUMENTとして扱う。
//
// - out_data_は少なくともqueueのelement_size byteを書き込み可能なstorageを指すことをcaller contractとして扱う。
// - out_data_が指すstorageのallocation identityやcapacityを
//   Ring Queue module自身では再認証しない。
//
// - 本operationはhead slotのowned storageをreadし、head / tail / lenをconsumeして
//   queue stateを更新するため、DEBUG_BUILD / TEST_BUILDでは
//   operation開始前にqueue_へcanonical validationを行う。
// - RELEASE_BUILDでは正規APIを通して成立しているqueue_のcanonical stateを
//   trusted internal contractとして扱い、automatic canonical validationは行わない。
//
// - queueが空であることはinternal corruptionではなく通常発生し得るruntime stateであるため、
//   RING_QUEUE_EMPTYとして扱い、queue stateおよびout_data_を変更せず終了する。
//
// - mutation後のstable stateについて、DEBUG_BUILD / TEST_BUILDでは
//   internal invariant破損の局所化を目的としてcanonical Postcondition validationを行う。
// - Postcondition validation failureはRING_QUEUE_DATA_CORRUPTEDとして扱う。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
ring_queue_result_t ring_queue_pop(size_t element_size_, size_t element_align_, ring_queue_t* queue_, void* out_data_) {
    ring_queue_result_t ret = RING_QUEUE_INVALID_ARGUMENT;

    char* mem_ptr = NULL;
    char* head_ptr = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(queue_, ret, RING_QUEUE_INVALID_ARGUMENT, result_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "queue_")
    IF_ARG_NULL_GOTO_CLEANUP(out_data_, ret, RING_QUEUE_INVALID_ARGUMENT, result_to_str(RING_QUEUE_INVALID_ARGUMENT), "ring_queue_pop", "out_data_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_pop(%s) - Precondition validation failed for 'queue_'.", result_to_str(ret));
        goto cleanup;
    }
#endif
    if(queue_->element_size != element_size_ || queue_->element_align != element_align_) {
        ret = RING_QUEUE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ring_queue_pop(%s) - Provided element_size_ or element_align_ is not valid.", result_to_str(ret));
        goto cleanup;
    }
    if(ring_queue_is_empty(queue_)) {
        DEBUG_MESSAGE("Ring queue is empty.");
        ret = RING_QUEUE_EMPTY;
        goto cleanup;
    }

    // Commit.
    mem_ptr = (char*)queue_->memory_pool;
    head_ptr = mem_ptr + (queue_->head * queue_->stride);
    memcpy(out_data_, head_ptr, queue_->element_size);
    queue_->len--;
    queue_->head = (queue_->head + 1) % queue_->max_element_count;
    if(0 == queue_->len) {
        queue_->head = 0;
        queue_->tail = 0;
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!ring_queue_is_valid(queue_)) {
        ret = RING_QUEUE_DATA_CORRUPTED;
        ERROR_MESSAGE("ring_queue_pop(%s) - Postcondition validation failed for 'queue_'.", result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RING_QUEUE_SUCCESS;

cleanup:
    return ret;
}

// ring_queue_is_empty Validation Policy
//
// - queue_ == NULLの場合は、APIで定義されたfallback valueとしてtrueを返す。
//
// - queue_ != NULLの場合、本operationが実際にconsumeするring_queue_t stateは
//   len fieldのみであり、memory_pool、storage layout、head / tail relationを参照しない。
// - lenの判定に不要なroot field relationやowned memory_poolのvalidityを
//   再認証するためだけのshallow / canonical validationは行わない。
// - non-NULLのqueue_は、Module Boundary Contractを満たすlifetime中のobjectへの
//   pointerであることをtrusted contractとして扱う。
//
// - 本operationはobject stateを変更しないため、Postcondition validationは行わない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool ring_queue_is_empty(const ring_queue_t* queue_) {
    if(NULL == queue_) {
        return true;
    }
    if(0 != queue_->len) {
        return false;
    } else {
        return true;
    }
}

// ring_queue_is_valid Validation Policy
//
// - 本APIはring_queue_tのpublic canonical validatorである。
// - queue_ == NULLの場合はfalseを返す。
// - Module Internal Contractで定義されたcanonical state全体を検証する。
//
// - memory_pool == NULLの場合はfalseを返す。
// - owned memory_poolについて、General Allocator上のcurrent allocationであることを
//   general_allocator_ptr_is_allocated()で確認する。
//
// - allocation validity確認後、private shallow validatorを実行し、
//   ring_queue_t自身のroot fieldおよびroot field間のstructural relationを検証する。
// - shallow validationに成功した後、head、tail、lenおよびmax_element_countから定まる
//   ring上のlogical relationを検証する。
//
// - queue_自身のallocation validityは本validatorでは検証しない。
//   queue_をowned pointerとして保持するowner側が、そのallocation validityを
//   ownership closureの一部として検証する。
//
// - queue内部に格納された各要素のsemantic validityは検証しない。
// - memory_poolのactual allocation sizeとcapacityの整合性検証については、
//   allocation metadataの利用方法と合わせて将来検討する。
//
// - explicit validatorであるため、BUILD_MODEによってvalidation semanticsを変更しない。
// - validation中に対象stateを変更しない。
// - validation failure時はfalseを返すのみとし、error messageは出力しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
bool ring_queue_is_valid(const ring_queue_t* queue_) {
    size_t distance_to_end = 0;
    size_t expected_tail = 0;
    if(NULL == queue_) {
        return false;
    }
    if(NULL == queue_->memory_pool) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated((const void*)queue_->memory_pool)) {
        return false;
    }
    if(!is_valid_shallow(queue_)) {
        return false;
    }

    distance_to_end = queue_->max_element_count - queue_->head;
    if(queue_->len < distance_to_end) {
        expected_tail = queue_->head + queue_->len;
    } else {
        expected_tail = queue_->len - distance_to_end;
    }
    if(queue_->tail != expected_tail) {
        return false;
    }

    return true;
}

// ============================================================
// Utilities
// ============================================================
/**
 * @brief リングキュー実行結果コードを文字列に変換する
 *
 * @param[in] result_ リングキュー実行結果コード
 * @return const char* 変換された文字列
 */
static const char* result_to_str(ring_queue_result_t result_) {
    switch(result_) {
    case RING_QUEUE_SUCCESS:
        return s_result_str_success;
    case RING_QUEUE_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case RING_QUEUE_NO_MEMORY:
        return s_result_str_no_memory;
    case RING_QUEUE_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case RING_QUEUE_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    case RING_QUEUE_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case RING_QUEUE_BAD_OPERATION:
        return s_result_str_bad_operation;
    case RING_QUEUE_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case RING_QUEUE_OVERFLOW:
        return s_result_str_overflow;
    case RING_QUEUE_EMPTY:
        return s_result_str_empty;
    default:
        return s_result_str_undefined_error;
    }
}

static ring_queue_result_t result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return RING_QUEUE_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return RING_QUEUE_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return RING_QUEUE_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return RING_QUEUE_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return RING_QUEUE_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return RING_QUEUE_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return RING_QUEUE_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return RING_QUEUE_UNDEFINED_ERROR;
    default:
        return RING_QUEUE_UNDEFINED_ERROR;
    }
}

// ============================================================
// Validators
// ============================================================

// is_valid_shallow Validation Policy
//
// - 本helperはring_queue_tのprivate shallow validatorである。
// - queue_ == NULLの場合はfalseを返す。
//
// - shallow validationではring_queue_t自身のroot fieldと、
//   root field間の局所的なstructural relationのみを検証する。
// - memory_poolが指すstorageをdereferenceしない。
// - owned memory_poolのallocation validityも検証せず、
//   general_allocator_ptr_is_allocated()は呼び出さない。
//
// - element_align、max_element_count、element_size、strideおよびcapacityが
//   queue representationとして成立する値であることを検証する。
// - element_alignが2の冪乗であり、alignof(max_align_t)以下であることを検証する。
// - element_sizeをelement_align境界へ切り上げた値とstrideが一致することを検証する。
// - stride * max_element_countがsize_tの表現可能範囲内であり、
//   その値とcapacityが一致することを検証する。
//
// - headおよびtailがmax_element_count未満であり、
//   lenがmax_element_count以下であることを検証する。
// - len == 0の場合、head == 0かつtail == 0であることを検証する。
//
// - memory_poolがNULLではないこと、および
//   memory_poolがelement_align境界にalignmentされていることを検証する。
// - memory_poolのallocation validity、actual allocation size、
//   およびmemory_pool内部のdataは本helperでは検証しない。
// - head / tail / lenから定まるring上のlogical relationは
//   canonical validationの責務とし、本helperでは検証しない。
//
// - 本helperは対象stateを変更しない。
// - validation failure時はfalseを返すのみとし、error messageは出力しない。
//
// AI支援:
// - 本セクションはChatGPTを用いて草案を作成し、
//   プロジェクト作成者が実装との整合性を確認・修正した。
// - 実装コードはプロジェクト作成者が作成した。
static bool is_valid_shallow(const ring_queue_t* queue_) {
    size_t expected_stride = 0;
    if(NULL == queue_) {
        return false;
    }
    if(0 == queue_->element_align || 0 == queue_->max_element_count || 0 == queue_->element_size || 0 == queue_->stride || 0 == queue_->capacity) {
        return false;
    }
    if(!memory_utility_align_up(queue_->element_size, queue_->element_align, &expected_stride)) {
        return false;
    }
    if(queue_->stride != expected_stride) {
        return false;
    }
    if(0 == queue_->len && 0 != queue_->head) {
        return false;
    }
    if(0 == queue_->len && 0 != queue_->tail) {
        return false;
    }
    if(alignof(max_align_t) < queue_->element_align || !IS_POWER_OF_TWO(queue_->element_align)) {
        return false;
    }
    if(queue_->head >= queue_->max_element_count) {
        return false;
    }
    if(queue_->tail >= queue_->max_element_count) {
        return false;
    }
    if(queue_->len > queue_->max_element_count) {
        return false;
    }
    if((SIZE_MAX / queue_->stride) < queue_->max_element_count) {
        return false;
    }
    if(queue_->capacity != queue_->stride * queue_->max_element_count) {
        return false;
    }
    if(NULL == queue_->memory_pool) {
        return false;
    }
    if(0 != ((uintptr_t)queue_->memory_pool % queue_->element_align)) {
        return false;
    }
    return true;
}
