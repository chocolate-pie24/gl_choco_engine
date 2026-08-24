// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file range_allocator.c
 * @brief 固定alignmentのGPU buffer内rangeを管理するRange Allocatorの実装
 *
 * @details
 * 本実装は、論理メモリプール全体をFREE／ALLOCATED nodeへ分割し、
 * address orderの双方向range listとして管理する。
 *
 * 実メモリ、base pointer、GPU buffer object、およびbuffer内のデータは保持せず、
 * offset 0を起点とするbyte単位のrangeだけを管理する。
 *
 * @par Node poolとrange list
 * create時に、次の式で求めた数のnodeを連続配列として事前確保する。
 *
 * @code{.c}
 * max_node_count = max_allocation_count * 2 + 1;
 * @endcode
 *
 * ALLOCATED rangeとFREE rangeが交互に並び、両端にFREE rangeが存在する配置が、
 * 一定のallocation数に対して最も多くnodeを使用する。
 * 上記の式は、この最悪配置を表現できるnode数である。
 *
 * create成功後のallocate／freeでは動的メモリ確保を行わない。
 *
 * range listはnode pool内のFREE／ALLOCATED nodeをaddress orderで接続する。
 * NOT_USED nodeとTRANSITIONING nodeはrange listへ接続されない。
 *
 * @par Node state
 * node pool上の利用状態、range listへの接続状態、およびnodeが表すrangeの用途を、単一のnode stateで管理する。
 *
 * public APIの入口、正常終了時、および通常の失敗終了時に許可される安定状態は次の三つである。
 *
 * - NOT_USED: node pool内で未使用であり、range listへ接続されていない
 * - FREE: range listへ接続され、allocation可能なrangeを表す
 * - ALLOCATED: range listへ接続され、live allocationが所有するrangeを表す
 *
 * TRANSITIONINGは、nodeのacquire、insert、remove、releaseを行う
 * private操作の途中だけで使用する。
 * 通常はpublic APIから戻る時点でTRANSITIONINGが残ってはならない。
 *
 * @par Range listの不変条件
 * 内部状態が正常な場合、range listは次の条件を満たす。
 *
 * - range listはmemory pool全体を隙間および重複なく表現する
 * - 先頭nodeのoffsetは0である
 * - 各nodeの終端は次nodeのoffsetと一致する
 * - 末尾nodeの終端はmemory pool sizeと一致する
 * - range listはaddress orderの双方向listである
 * - range listへ接続されたnodeのstateはFREEまたはALLOCATEDである
 * - 接続されたnodeのblock sizeは0ではない
 * - 隣接する二つのnodeがともにFREEになることはない
 * - 同じnodeがrange listへ重複して接続されることはない
 *
 * @par Alignmentの不変条件
 * base alignmentは0以外の2の冪乗であり、Range Allocatorの生成後は変更しない。
 *
 * - range listへ接続されたすべてのnodeのoffsetはbase alignment境界にある
 * - ALLOCATED nodeのblock sizeはbase alignmentの倍数である
 * - required sizeはallocate時にbase alignment単位へ切り上げる
 * - allocationは選択したFREE rangeの先頭から行う
 *
 * memory pool size自体にはbase alignmentの倍数であることを要求しない。
 * このため、memory pool末尾を含むFREE nodeのblock sizeは
 * base alignmentの倍数ではない場合がある。
 *
 * @par Allocation方式
 * allocation対象のFREE rangeはfirst-fit方式で検索する。
 * range listを先頭から走査し、alignment調整後の実確保サイズを収容できる
 * 最初のFREE nodeを選択する。
 *
 * allocationは次の二方式で行う。
 *
 * - Exact fit:
 *   選択したFREE nodeを、そのままALLOCATEDへ遷移させる
 * - Partial allocation:
 *   選択したFREE nodeの先頭をALLOCATED rangeとし、
 *   残りを新しい後方FREE nodeとして表現する
 *
 * partial allocationに必要なNOT_USED nodeは、既存FREE nodeのrange情報を変更する前に取得する。
 * 後方FREE nodeの挿入に失敗した場合は、取得したnodeをreleaseし、既存FREE nodeのblock sizeを復元する。
 *
 * allocation count、total allocated size、および出力descriptorは、
 * rangeの確保処理が完了した後に更新する。
 *
 * @par Range Allocator方式とfree保証
 * FREE rangeだけでなく、ALLOCATED rangeもnodeとしてrange list上で管理する。
 *
 * allocate成功時に、後続のfreeに必要なALLOCATED nodeとrange情報を確保済みとする。
 * freeでは新しいnodeの取得や動的メモリ確保を行わず、
 * 対応するALLOCATED nodeをFREEへ遷移させるか、隣接するFREE nodeへmergeする。
 *
 * これにより、内部データが正常であり、allocateが返した有効なdescriptorを
 * 使用する限り、freeはnode不足やメモリ不足によって失敗しない。
 *
 * このfree保証によって、Range Allocatorのallocate成功後に上位処理が
 * 失敗した場合、確保済みrangeを解放してallocate前の状態へrollbackできる。
 * これは、FREE rangeだけを保持するRange Free Listではなく、
 * ALLOCATED rangeも保持するRange Allocatorを採用した主要な理由である。
 *
 * @par Freeとmerge
 * free開始前に、descriptorのowner、node index、offset、allocated size、
 * および対応nodeのstateを検証する。
 *
 * 対象ALLOCATED nodeと隣接FREE nodeの関係に応じて、次の四方式でfreeする。
 *
 * - 前後ともmergeしない
 * - 前方FREE nodeとのみmergeする
 * - 後方FREE nodeとのみmergeする
 * - 前後両方のFREE nodeとmergeする
 *
 * mergeによって不要になったnodeはrange listから切断し、
 * 正規化されたNOT_USED状態へ戻す。
 *
 * @par Allocation identity
 * allocation identityは、Range Allocatorのowner pointerと
 * ALLOCATED nodeのnode pool indexによって表す。
 *
 * free時にはidentityに加えて、descriptorのoffsetおよびallocated sizeが
 * 対応nodeのrange情報と一致することを検証する。
 * privateなnode pointerは公開しない。
 *
 * node generationは保持しないため、解放済みnodeが再利用され、
 * owner、node index、offset、allocated sizeがすべて一致した場合は、
 * stale descriptorを検出できない。
 *
 * @par 管理値の不変条件
 * 内部状態が正常な場合、次の条件を満たす。
 *
 * - allocation countはALLOCATED node数と一致する
 * - allocation countはmax allocation count以下である
 * - total allocated sizeは全ALLOCATED nodeのblock size合計と一致する
 * - unused node countはNOT_USED node数と一致する
 * - range list接続node数とunused node countの合計はmax node countと一致する
 *
 * @par Validation
 * public allocate／freeの入口では、Range Allocator全体のdeep validationを実行する。
 *
 * deep validationはrange list上の各nodeについてnode poolへの所属確認を行うため、
 * 現在の時間計算量はO(n^2)である。
 *
 * private関数ではRange Allocator全体のvalidationを繰り返さない。
 * 呼び出し元が必要なvalidationを完了していることを事前条件とし、
 * 各private関数は担当範囲の引数、node state、および局所的な接続関係を検証する。
 *
 * @par 破損状態の扱い
 * validなdescriptorと正常な内部状態を前提とするfree処理では、
 * 各private操作は失敗しない設計とする。
 *
 * この前提でnode操作やmergeが失敗した場合は、通常のリソース不足ではなく
 * 内部データ破損として扱う。
 * 内部データ破損が検出された後の処理では、呼び出し前の状態へ
 * rollbackできない場合がある。
 *
 * status取得は破損状態でも管理値を観測できるよう、NULL checkだけを行い、
 * range listの走査やdeep validationを実行しない。
 *
 * @date 2026-07-31
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 * 実装コードはプロジェクト作成者が作成し、その内容に責任を負う。
 */
#include "engine/systems/renderer/resources/allocators/range_allocator.h"

#include <stdio.h>  // for fprintf
#include <string.h> // for memset
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

/**
 * @brief nodeの利用状態、range用途、およびlist接続状態を表す
 *
 * @details
 * node pool内での利用状態、range listへの接続状態、およびnodeが表す
 * rangeの用途を、単一のstateとして管理する。
 *
 * @par 安定状態
 * public APIの入口、正常終了時、および通常の失敗終了時に許可される
 * 安定状態は次の三つである。
 *
 * - NODE_STATE_NOT_USED:
 *   node pool内で未使用であり、range listへ接続されていない
 * - NODE_STATE_FREE:
 *   range listへ接続され、allocation可能なrangeを表している
 * - NODE_STATE_ALLOCATED:
 *   range listへ接続され、live allocationが所有するrangeを表している
 *
 * @par 内部遷移状態
 * NODE_STATE_TRANSITIONINGは、nodeのacquire、insert、remove、releaseを行う
 * private操作の途中だけで使用する。
 *
 * TRANSITIONING nodeはrange listへ接続されず、prevとnextはNULLとなる。
 * offsetとblock sizeには、挿入予定または切断直後のrange情報を保持する。
 *
 * node_is_valid()はTRANSITIONINGを局所的に有効なstateとして扱うが、
 * Range Allocator全体の安定状態ではTRANSITIONING nodeを許可しない。
 *
 * @par 状態遷移
 * 通常のnode操作では、次の状態遷移を行う。
 *
 * - acquire:
 *   NOT_USEDからTRANSITIONINGへ遷移する
 * - insert:
 *   TRANSITIONINGからFREEまたはALLOCATEDへ遷移する
 * - remove:
 *   FREEまたはALLOCATEDからTRANSITIONINGへ遷移する
 * - release:
 *   TRANSITIONINGからNOT_USEDへ遷移する
 * - exact fit allocation:
 *   range listへ接続したままFREEからALLOCATEDへ遷移する
 * - mergeを伴わないfree:
 *   range listへ接続したままALLOCATEDからFREEへ遷移する
 *
 * state変更を開始する前に、失敗し得る検証を完了する。
 * private操作が失敗した場合は、通常、public APIから戻る前に
 * 安定状態へrollbackする。
 *
 * @warning
 * rollback中のnode操作に失敗した場合、TRANSITIONING nodeが残る可能性がある。
 * この状態はRange Allocatorの内部データ破損として扱う。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
typedef enum {
    NODE_STATE_NOT_USED,        /**< node pool内で未使用であり、range listへ接続されていない */
    NODE_STATE_TRANSITIONING,   /**< private操作によるrange情報、state、またはlist接続の更新中 */
    NODE_STATE_FREE,            /**< range list上のallocation可能なFREE range */
    NODE_STATE_ALLOCATED,       /**< range list上でlive allocationが所有するALLOCATED range */
} node_state_t;

/**
 * @brief 論理メモリプール内の一つの連続rangeを表すnode
 *
 * @details
 * offsetとblock_sizeによって、論理メモリプール内の半開区間
 * `[offset, offset + block_size)`を表す。
 *
 * FREEまたはALLOCATED状態のnodeは、address orderの双方向range listへ接続される。
 * NOT_USEDおよびTRANSITIONING状態のnodeはrange listへ接続されない。
 *
 * nodeはcreate時に確保される連続したnode poolの要素であり、
 * Range Allocatorの生存期間中に再配置されない。
 * このため、prevおよびnextはnode pool内の要素を直接指す。
 *
 * @par Stateごとのfield条件
 * NODE_STATE_NOT_USEDの場合は次の状態となる。
 *
 * - offsetは0
 * - block_sizeは0
 * - prevはNULL
 * - nextはNULL
 *
 * NODE_STATE_TRANSITIONINGの場合は次の状態となる。
 *
 * - offsetとblock_sizeは挿入予定または切断直後のrangeを表す
 * - block_sizeは0ではない
 * - prevはNULL
 * - nextはNULL
 *
 * NODE_STATE_FREEまたはNODE_STATE_ALLOCATEDの場合は次の状態となる。
 *
 * - offsetとblock_sizeはmemory pool内の有効なrangeを表す
 * - block_sizeは0ではない
 * - prevとnextはaddress order上の隣接nodeを指す
 * - range listの先頭ではprevがNULLとなる
 * - range listの末尾ではnextがNULLとなる
 *
 * range listにnodeが一つだけ存在する場合、接続済みnodeであっても
 * prevとnextはともにNULLとなる。
 * listへの接続状態はpointerだけでなくnode_stateと合わせて判断する。
 *
 * @par Allocation identity
 * ALLOCATED nodeのnode pool indexは、range_allocation_tの
 * allocation identityの一部として使用される。
 *
 * node自身はownerやnode indexを保持しない。
 * ownerはnode poolを保持するrange_allocator_tによって決まり、
 * node indexはnode pool上の位置から取得する。
 *
 * @note
 * nodeはrange情報だけを管理し、対応する実メモリやGPU buffer内データを所有しない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
typedef struct node {
    struct node* next;          /**< address orderで次に接続されたrange node。
                                 *   末尾node、NOT_USED、またはTRANSITIONINGではNULL */
    struct node* prev;          /**< address orderで前に接続されたrange node。
                                 *   先頭node、NOT_USED、またはTRANSITIONINGではNULL */

    size_t block_size;          /**< このnodeが表すrangeのサイズ(byte)。
                                 *   NOT_USEDでは0、それ以外のstateでは0より大きい */
    size_t offset;              /**< 論理メモリプール先頭からのrange開始offset(byte)。
                                 *   NOT_USEDでは0 */

    node_state_t node_state;    /**< node pool上の利用状態、range listへの接続状態、
                                 *   およびrangeの用途を表すstate */
} node_t;

/**
 * @brief Range Allocatorの内部状態を保持する
 *
 * @details
 * 一つの論理メモリプールに対する容量、固定base alignment、
 * allocation数、node pool、およびFREE／ALLOCATED range listを管理する。
 *
 * 本構造体とnode poolはRange Allocator自身が所有する。
 * 管理対象となる実メモリ、GPU buffer object、およびbuffer内データは所有しない。
 *
 * 本構造体のaddressは、range_allocation_tのowner identityとして使用される。
 *
 * @par Node pool
 * node_poolはcreate時に確保されるnode_tの連続配列であり、
 * 要素数はmax_node_countと一致する。
 *
 * max_node_countは次の式で決定され、Range Allocatorの生存期間中に変更されない。
 *
 * @code{.c}
 * max_node_count = max_allocation_count * 2 + 1;
 * @endcode
 *
 * create成功後にnode poolの拡張や再配置は行わない。
 *
 * @par Range list
 * range_list_headは、論理メモリプール全体を表すaddress orderの
 * FREE／ALLOCATED range listの先頭nodeを指す。
 *
 * range listへ接続されるnodeはすべてnode_poolの要素である。
 * NOT_USEDおよびTRANSITIONING nodeはrange listへ接続されない。
 *
 * 内部状態が安定している場合、range_list_headはNULLではなく、
 * 先頭nodeのoffsetは0となる。
 *
 * @par Cached管理値
 * allocation_count、unused_node_count、およびtotal_allocated_sizeは、
 * range listを走査せず状態を取得できるよう、allocate／freeによって
 * 更新される管理値である。
 *
 * 内部状態が正常な場合、次が成立する。
 *
 * - allocation_countはALLOCATED node数と一致する
 * - allocation_countはmax_allocation_count以下である
 * - unused_node_countはNOT_USED node数と一致する
 * - total_allocated_sizeは全ALLOCATED nodeのblock size合計と一致する
 * - total_allocated_sizeはmemory_pool_size以下である
 * - range list接続node数とunused_node_countの合計はmax_node_countと一致する
 *
 * @todo Free Blockの最大値としてmax_free_block_sizeを保持する
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
struct range_allocator {
    size_t memory_pool_size;        /**< 管理対象となる論理メモリプールの総容量(byte) */
    size_t max_node_count;          /**< node poolの総node数。max_allocation_count * 2 + 1で固定される */
    size_t max_allocation_count;    /**< 同時に生存できるallocation数の上限 */
    size_t unused_node_count;       /**< node pool内でNOT_USED状態にあるnode数 */
    size_t allocation_count;        /**< 現在生存しているallocation数。内部状態が正常であればALLOCATED node数と一致する */
    size_t base_align;              /**< allocationに使用する固定base alignment(byte) */
    size_t total_allocated_size;    /**< alignment paddingを含む全ALLOCATED rangeの合計サイズ(byte) */

    node_t* node_pool;              /**< 本Range Allocatorが所有するnode_tの連続配列。要素数はmax_node_count */
    node_t* range_list_head;        /**< FREE／ALLOCATED range listの先頭node。node_pool内の要素を指す */
};

/**
 * @name Range Allocator実行結果コード文字列
 *
 * @details
 * range_allocator_result_tをログ出力用文字列へ変換するための静的文字列定数。
 *
 * すべて静的記憶域期間を持ち、呼び出し側へ所有権を移動しない。
 * 文字列および文字列pointerは変更できない。
 *
 * rslt_to_str()は対応する結果コードの文字列を返す。
 * 定義されていない結果コードにはs_rslt_str_undefined_errorを使用する。
 *
 * @{
 */
static const char* const s_rslt_str_success = "SUCCESS";                    /** @brief RANGE_ALLOCATOR_SUCCESSに対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /** @brief RANGE_ALLOCATOR_INVALID_ARGUMENTに対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /** @brief RANGE_ALLOCATOR_LIMIT_EXCEEDEDに対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /** @brief RANGE_ALLOCATOR_NO_MEMORYに対応する文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /** @brief RANGE_ALLOCATOR_DATA_CORRUPTEDに対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /** @brief RANGE_ALLOCATOR_BAD_OPERATIONに対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /** @brief RANGE_ALLOCATOR_OVERFLOWに対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**
                                                                             * @brief RANGE_ALLOCATOR_UNDEFINED_ERRORおよび
                                                                             *        定義されていない結果コードに対応する文字列
                                                                             */
/** @} */

// Allocate
static range_allocator_result_t align_up(size_t base_align_, size_t required_size_, size_t* out_allocation_size_);
static range_allocator_result_t find_first_fit_node(const range_allocator_t* range_allocator_, size_t allocation_size_, node_t** out_node_);
static range_allocator_result_t allocate_from_node(range_allocator_t* range_allocator_, node_t* node_, size_t allocation_size_);

// Free
static range_allocator_result_t allocation_resolve_node(const range_allocator_t* range_allocator_, const range_allocation_t* allocation_info_, node_t** out_node_);
static range_allocator_result_t free_merge_plan_get(const node_t* node_, bool* out_should_merge_prev_, bool* out_should_merge_next_);
static range_allocator_result_t free_from_node(range_allocator_t* range_allocator_, node_t* node_, bool should_merge_prev_, bool should_merge_next_);
static range_allocator_result_t free_node_without_merge(node_t* node_);
static range_allocator_result_t free_node_merge_prev(range_allocator_t* range_allocator_, node_t* node_);
static range_allocator_result_t free_node_merge_next(range_allocator_t* range_allocator_, node_t* node_);
static range_allocator_result_t free_node_merge_prev_next(range_allocator_t* range_allocator_, node_t* node_);

// Node / List Management
static range_allocator_result_t node_acquire(range_allocator_t* range_allocator_, size_t offset_, size_t block_size_, node_t** out_node_);
static range_allocator_result_t node_insert_between(range_allocator_t* range_allocator_, node_t* insert_node_, node_state_t next_state_, node_t* prev_, node_t* next_);
static range_allocator_result_t node_remove(range_allocator_t* range_allocator_, node_t* node_);
static range_allocator_result_t node_release(range_allocator_t* range_allocator_, node_t* node_);
static range_allocator_result_t node_pool_find_index(const range_allocator_t* range_allocator_, const node_t* node_, size_t* out_index_);
static void set_node_to_not_used(node_t* target_);
static void set_node_to_transitioning(node_t* target_, size_t offset_, size_t block_size_);
static void set_node_to_free(node_t* target_, node_t* prev_, node_t* next_);
static void set_node_to_allocated(node_t* target_, node_t* prev_, node_t* next_);

// Validation
static bool range_allocator_is_valid(const range_allocator_t* range_allocator_);            // deep validation
static bool range_allocator_is_valid_shallow(const range_allocator_t* range_allocator_);    // shallow validation
static bool node_is_valid(const node_t* node_);

// Utilities
static void status_print(const range_allocator_status_t* status_);
static const char* rslt_to_str(range_allocator_result_t rslt_);
static range_allocator_result_t rslt_convert_choco_memory(memory_system_result_t rslt_);

range_allocator_result_t range_allocator_create(size_t memory_pool_size_, size_t max_allocation_count_, size_t base_align_, range_allocator_t** out_range_allocator_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;
    memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;

    range_allocator_t* tmp_range_allocator = NULL;

    size_t max_node_count = 0;
    size_t node_pool_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_create", "out_range_allocator_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_create", "*out_range_allocator_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != memory_pool_size_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_create", "memory_pool_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != max_allocation_count_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_create", "max_allocation_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != base_align_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_create", "base_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(base_align_), ret, RANGE_ALLOCATOR_BAD_OPERATION, rslt_to_str(RANGE_ALLOCATOR_BAD_OPERATION), "range_allocator_create", "base_align_")

    if(((SIZE_MAX - 1) / 2) < max_allocation_count_) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("range_allocator_create(%s) - Failed to create range allocator. reason=max_node_count_overflow, max_allocation_count=%zu, max_safe_allocation_count=%zu", rslt_to_str(ret), max_allocation_count_, (SIZE_MAX - 1) / 2);
        goto cleanup;
    }
    max_node_count = max_allocation_count_ * 2 + 1;

    if((SIZE_MAX / max_node_count) < sizeof(node_t)) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("range_allocator_create(%s) - Failed to create range allocator. reason=node_pool_size_overflow, max_node_count=%zu, node_size=%zu, max_safe_node_count=%zu", rslt_to_str(ret), max_node_count, sizeof(node_t), SIZE_MAX / sizeof(node_t));
        goto cleanup;
    }
    node_pool_size = sizeof(node_t) * max_node_count;

    ret_memory = memory_system_allocate(sizeof(range_allocator_t), MEMORY_TAG_RENDERER, (void**)&tmp_range_allocator);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory) {
        ret = rslt_convert_choco_memory(ret_memory);
        ERROR_MESSAGE("range_allocator_create(%s) - Failed to create range allocator. reason=allocator_instance_allocation_failed, allocation_size=%zu, memory_system_result=%d", rslt_to_str(ret), sizeof(range_allocator_t), (int)ret_memory);
        goto cleanup;
    }
    memset(tmp_range_allocator, 0, sizeof(range_allocator_t));

    ret_memory = memory_system_allocate(node_pool_size, MEMORY_TAG_RENDERER, (void**)&tmp_range_allocator->node_pool);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory) {
        ret = rslt_convert_choco_memory(ret_memory);
        ERROR_MESSAGE("range_allocator_create(%s) - Failed to create range allocator. reason=node_pool_allocation_failed, node_pool_size=%zu, max_node_count=%zu, node_size=%zu, memory_system_result=%d", rslt_to_str(ret), node_pool_size, max_node_count, sizeof(node_t), (int)ret_memory);
        goto cleanup;
    }
    memset(tmp_range_allocator->node_pool, 0, node_pool_size);

    for(size_t i = 0; i != max_node_count; ++i) {
        memset(&tmp_range_allocator->node_pool[i], 0, sizeof(node_t));
        set_node_to_not_used(&tmp_range_allocator->node_pool[i]);
    }

    tmp_range_allocator->max_node_count = max_node_count;
    tmp_range_allocator->max_allocation_count = max_allocation_count_;
    tmp_range_allocator->allocation_count = 0;
    tmp_range_allocator->memory_pool_size = memory_pool_size_;
    tmp_range_allocator->unused_node_count = max_node_count - 1;
    tmp_range_allocator->base_align = base_align_;
    tmp_range_allocator->total_allocated_size = 0;

    tmp_range_allocator->range_list_head = &tmp_range_allocator->node_pool[0];
    tmp_range_allocator->range_list_head->block_size = memory_pool_size_;
    tmp_range_allocator->range_list_head->offset = 0;
    set_node_to_free(tmp_range_allocator->range_list_head, NULL, NULL);

#if defined(TEST_BUILD) || defined(DEBUG_BUILD)
    if(!range_allocator_is_valid(tmp_range_allocator)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("range_allocator_create - Postcondition validation failed for 'tmp_range_allocator'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_range_allocator_ = tmp_range_allocator;
    tmp_range_allocator = NULL;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    if(NULL != tmp_range_allocator) {
        if(NULL != tmp_range_allocator->node_pool) {
            memory_system_free((void*)tmp_range_allocator->node_pool, node_pool_size, MEMORY_TAG_RENDERER);
            tmp_range_allocator->node_pool = NULL;
        }
        memory_system_free((void*)tmp_range_allocator, sizeof(range_allocator_t), MEMORY_TAG_RENDERER);
        tmp_range_allocator = NULL;
    }
    return ret;
}

void range_allocator_destroy(range_allocator_t** range_allocator_) {
    if(NULL == range_allocator_) {
        return;
    }
    if(NULL == *range_allocator_) {
        return;
    }

    if(NULL != (*range_allocator_)->node_pool) {
        memory_system_free((void*)(*range_allocator_)->node_pool, sizeof(node_t) * (*range_allocator_)->max_node_count, MEMORY_TAG_RENDERER);
        (*range_allocator_)->node_pool = NULL;
    }

    (*range_allocator_)->max_node_count = 0;
    (*range_allocator_)->memory_pool_size = 0;

    memory_system_free((void*)*range_allocator_, sizeof(range_allocator_t), MEMORY_TAG_RENDERER);
    *range_allocator_ = NULL;
}

range_allocator_result_t range_allocator_allocate(range_allocator_t* range_allocator_, size_t required_size_, size_t required_align_, range_allocation_t* out_allocation_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    size_t allocation_size = 0;
    size_t allocated_index = 0;
    node_t* node = NULL;
    range_allocation_t tmp_descriptor = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_allocate", "range_allocator_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_size_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_allocate", "required_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_align_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_allocate", "required_align_")
    IF_ARG_NULL_GOTO_CLEANUP(out_allocation_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_allocate", "out_allocation_")

#if defined(DEBUG_BUILD)
    if(!range_allocator_is_valid_shallow(range_allocator_)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("range_allocator_allocate(%s) - Precondition validation failed for 'range_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
#if defined(TEST_BUILD)
    if(!range_allocator_is_valid(range_allocator_)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("range_allocator_allocate(%s) - Precondition validation failed for 'range_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    if(range_allocator_->base_align != required_align_) {
        ret = RANGE_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("range_allocator_allocate(%s) - Provided requred_align_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(range_allocator_->allocation_count >= range_allocator_->max_allocation_count) {
        ret = RANGE_ALLOCATOR_LIMIT_EXCEEDED;
        ERROR_MESSAGE("range_allocator_allocate(%s) - allocation count limit exceeded.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = align_up(range_allocator_->base_align, required_size_, &allocation_size);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("range_allocator_allocate(%s) - Failed to allocate range. reason=allocation_size_alignment_failed, required_size=%zu, base_align=%zu", rslt_to_str(ret), required_size_, range_allocator_->base_align);
        goto cleanup;
    }

    ret = find_first_fit_node(range_allocator_, allocation_size, &node);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("range_allocator_allocate(%s) - Failed to allocate range. reason=free_block_search_failed, required_size=%zu, allocation_size=%zu, total_free_size=%zu", rslt_to_str(ret), required_size_, allocation_size, range_allocator_->memory_pool_size - range_allocator_->total_allocated_size);
        goto cleanup;
    }

    ret = node_pool_find_index(range_allocator_, node, &allocated_index);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("range_allocator_allocate(%s) - Failed to allocate range. reason=free_block_node_index_lookup_failed, required_size=%zu, allocation_size=%zu, node_offset=%zu, node_block_size=%zu", rslt_to_str(ret), required_size_, allocation_size, node->offset, node->block_size);
        goto cleanup;
    }

    tmp_descriptor.node_index = allocated_index;
    tmp_descriptor.allocated_size = allocation_size;
    tmp_descriptor.offset = node->offset;
    tmp_descriptor.owner = range_allocator_;

#if defined(TEST_BUILD) || defined(DEBUG_BUILD)
    if(!range_allocation_is_valid(&tmp_descriptor)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("range_allocator_allocate - Postcondition validation failed for 'tmp_descriptor'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = allocate_from_node(range_allocator_, node, allocation_size);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("range_allocator_allocate(%s) - Failed to allocate range. reason=free_block_consumption_failed, required_size=%zu, allocation_size=%zu, node_index=%zu, node_offset=%zu, node_block_size=%zu", rslt_to_str(ret), required_size_, allocation_size, allocated_index, node->offset, node->block_size);
        goto cleanup;
    }

    range_allocator_->allocation_count++;
    range_allocator_->total_allocated_size += allocation_size;

#if defined(TEST_BUILD) || defined(DEBUG_BUILD)
    if(!range_allocator_is_valid(range_allocator_)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("range_allocator_allocate - Postcondition validation failed for 'range_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_allocation_ = tmp_descriptor;
    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

range_allocator_result_t range_allocator_free(range_allocator_t* range_allocator_, const range_allocation_t* allocation_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    node_t* tmp_node = NULL;
    bool should_merge_prev = false;
    bool should_merge_next = false;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_free", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(allocation_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "range_allocator_free", "allocation_")

    if(!range_allocation_is_valid(allocation_)) {
        ret = RANGE_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("range_allocator_free(%s) - Precondition validation failed for 'allocation_'.", rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD)
    if(!range_allocator_is_valid_shallow(range_allocator_)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("range_allocator_free(%s) - Precondition validation failed for 'range_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
#if defined(TEST_BUILD)
    if(!range_allocator_is_valid(range_allocator_)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("range_allocator_free(%s) - Precondition validation failed for 'range_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = allocation_resolve_node(range_allocator_, allocation_, &tmp_node);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("range_allocator_free(%s) - Failed to free range. reason=allocation_node_resolution_failed, node_index=%zu, max_node_count=%zu, allocation_offset=%zu, allocation_size=%zu, owner_matches_allocator=%d", rslt_to_str(ret), allocation_->node_index, range_allocator_->max_node_count, allocation_->offset, allocation_->allocated_size, (int)(allocation_->owner == range_allocator_));
        goto cleanup;
    }

    if(0 == range_allocator_->allocation_count || range_allocator_->total_allocated_size < allocation_->allocated_size) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("range_allocator_free(%s) - range allocator data corrupted.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = free_merge_plan_get(tmp_node, &should_merge_prev, &should_merge_next);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("range_allocator_free(%s) - Failed to free range. reason=free_merge_plan_get_failed, free_merge_plan_result=%s, node_index=%zu, node_offset=%zu, node_block_size=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), allocation_->node_index, tmp_node->offset, tmp_node->block_size);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;   // range_allocator_およびallocation_が正常であれば失敗しないはずなのでDATA_CORRUPTED
        goto cleanup;
    }

    ret = free_from_node(range_allocator_, tmp_node, should_merge_prev, should_merge_next);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("range_allocator_free(%s) - Failed to free range. reason=allocation_node_free_failed, free_from_node_result=%s, node_index=%zu, allocation_offset=%zu, allocation_size=%zu, merge_prev=%d, merge_next=%d", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), allocation_->node_index, allocation_->offset, allocation_->allocated_size, (int)should_merge_prev, (int)should_merge_next);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;   // range_allocator_およびallocation_が正常であれば失敗しないはずなのでDATA_CORRUPTED
        goto cleanup;
    }

    range_allocator_->allocation_count--;
    range_allocator_->total_allocated_size -= allocation_->allocated_size;

#if defined(TEST_BUILD) || defined(DEBUG_BUILD)
    if(!range_allocator_is_valid(range_allocator_)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("range_allocator_free - Postcondition validation failed for 'range_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// このAPIは破損状態でも中身を確認したいため、エラー処理はNULLチェックのみとする
void range_allocator_status_get(const range_allocator_t* range_allocator_, range_allocator_status_t* out_status_) {
    size_t tmp_used_node_count = 0;
    size_t tmp_free_block_count = 0;
    size_t tmp_total_free_size = 0;

    if(NULL == range_allocator_ || NULL == out_status_) {
        return;
    }
    out_status_->memory_pool_size = range_allocator_->memory_pool_size;
    out_status_->base_align = range_allocator_->base_align;
    out_status_->max_node_count = range_allocator_->max_node_count;
    out_status_->max_allocation_count = range_allocator_->max_allocation_count;
    out_status_->total_allocated_size = range_allocator_->total_allocated_size;
    out_status_->unused_node_count = range_allocator_->unused_node_count;

    if(range_allocator_->memory_pool_size > out_status_->total_allocated_size) {
        tmp_total_free_size = range_allocator_->memory_pool_size - out_status_->total_allocated_size;
    } else {
        tmp_total_free_size = 0;
    }
    out_status_->total_free_size = tmp_total_free_size;

    if(range_allocator_->max_node_count > range_allocator_->unused_node_count) {
        tmp_used_node_count = range_allocator_->max_node_count - range_allocator_->unused_node_count;
    } else {
        tmp_used_node_count = 0;
    }
    out_status_->used_node_count = tmp_used_node_count;

    if(out_status_->used_node_count > range_allocator_->allocation_count) {
        tmp_free_block_count = out_status_->used_node_count - range_allocator_->allocation_count;
    } else {
        tmp_free_block_count = 0;
    }
    out_status_->free_block_count = tmp_free_block_count;

    out_status_->allocation_count = range_allocator_->allocation_count;
}

void range_allocator_status_print(const range_allocator_status_t* status_) {
    flockfile(stdout); // 同一ストリームの同時書き込みをまとめる

    fprintf(stdout, "\033[1;35m[RANGE ALLOCATOR STATUS]\n");
    status_print(status_);
    fprintf(stdout, "\033[0m");

    funlockfile(stdout);
}

void range_allocator_debug_print(const range_allocator_t* range_allocator_) {
    size_t index = 0;
    size_t max_free_block_size = 0;
    const node_t* node = NULL;
    range_allocator_status_t status = { 0 };
    bool valid = false;
    static const char* const state_allocated = "ALLOCATED";
    static const char* const state_free = "FREE";
    static const char* const state_transitioning = "TRANSITIONING";
    static const char* const state_not_used = "NOT_USED";
    static const char* const state_undefined = "UNDEFINED";

    if(NULL == range_allocator_) {
        return;
    }

    valid = range_allocator_is_valid(range_allocator_);
    flockfile(stdout); // 同一ストリームの同時書き込みをまとめる
    fprintf(stdout, "\033[1;35m[RANGE ALLOCATOR DEBUG DUMP]\n");

    range_allocator_status_get(range_allocator_, &status);
    status_print(&status);

    fprintf(stdout, "  range_allocator_is_valid = %s\n", valid ? "true" : "false");
    fprintf(stdout, "  range nodes:\n");
    node = range_allocator_->range_list_head;
    while(NULL != node && index < range_allocator_->max_node_count) {
        const char* state_str = NULL;
        if(NODE_STATE_ALLOCATED == node->node_state) {
            state_str = state_allocated;
        } else if(NODE_STATE_FREE == node->node_state) {
            max_free_block_size = (max_free_block_size < node->block_size) ? node->block_size : max_free_block_size;
            state_str = state_free;
        } else if(NODE_STATE_NOT_USED == node->node_state) {
            state_str = state_not_used;
        } else if(NODE_STATE_TRANSITIONING == node->node_state) {
            state_str = state_transitioning;
        } else {
            state_str = state_undefined;
        }
        if((SIZE_MAX - node->block_size) < node->offset) {
            fprintf(stdout, "    list_index = %zu state = %s offset = %zu, size = %zu, end = OVERFLOW\n", index, state_str, node->offset, node->block_size);
        } else {
            fprintf(stdout, "    list_index = %zu state = %s offset = %zu, size = %zu, end = %zu\n", index, state_str, node->offset, node->block_size, node->offset + node->block_size);
        }
        node = node->next;
        index++;
    }
    if(NULL != node) {
        fprintf(stdout, "  range_node_traversal = TRUNCATED (reached max_node_count=%zu; possible cycle or node-count inconsistency)\n", range_allocator_->max_node_count);
    }
    fprintf(stdout, "  max_free_block_size = %zu\n", max_free_block_size);

    fprintf(stdout, "\033[0m");
    funlockfile(stdout);
}

bool range_allocation_is_valid(const range_allocation_t* range_allocation_) {
    if(NULL == range_allocation_) {
        return false;
    }
    if((SIZE_MAX - range_allocation_->allocated_size) < range_allocation_->offset) {
        return false;
    }
    if(0 == range_allocation_->allocated_size) {
        return false;
    }
    if(NULL == range_allocation_->owner) {
        return false;
    }
    return true;
}

/**
 * @brief required sizeをbase alignment単位に切り上げる
 *
 * @details
 * required_size_の末尾へ必要なalignment paddingを加え、
 * base_align_の倍数となる実確保サイズを算出する。
 *
 * 本関数が調整するのはsizeであり、addressやoffsetではない。
 * Range Allocatorでは各nodeのoffsetがすでにbase alignment境界にあるため、
 * required sizeを切り上げることで、allocation後の次rangeのoffsetも
 * base alignment境界に維持できる。
 *
 * required_size_がすでにbase_align_の倍数である場合、
 * paddingを追加せず、required_size_をそのまま返す。
 *
 * 本関数はRange Allocatorやnodeの状態を変更しない。
 *
 * @param[in] base_align_
 * 実確保サイズの計算に使用するbase alignment(byte)。
 * 0以外の2の冪乗でなければならない。
 *
 * @param[in] required_size_
 * 呼び出し側が必要とするサイズ(byte)。0は指定できない。
 *
 * @param[out] out_allocation_size_
 * base_align_単位に切り上げた実確保サイズの格納先。
 * 成功時のみ値が設定され、失敗時は変更されない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * 実確保サイズの算出に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * 次のいずれか。
 *
 * - out_allocation_size_がNULL
 * - base_align_が0
 * - required_size_が0
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * base_align_が2の冪乗ではない。
 *
 * 本関数は検証済みRange Allocatorのbase alignmentを受け取るprivate関数であるため、
 * この結果は呼び出し元の内部状態または事前検証に不整合があることを示す。
 *
 * @retval RANGE_ALLOCATOR_OVERFLOW
 * required_size_へalignment paddingを加えると
 * size_tの表現可能範囲を超える。
 *
 * @post
 * 成功時は次が成立する。
 *
 * - *out_allocation_size_はrequired_size_以上である
 * - *out_allocation_size_はbase_align_の倍数である
 * - *out_allocation_size_とrequired_size_の差はbase_align_未満である
 *
 * @post
 * 失敗時、*out_allocation_size_は変更されない。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t align_up(size_t base_align_, size_t required_size_, size_t* out_allocation_size_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    size_t padding = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_allocation_size_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "align_up", "out_allocation_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != base_align_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "align_up", "base_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_size_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "align_up", "required_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(base_align_), ret, RANGE_ALLOCATOR_DATA_CORRUPTED, rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), "align_up", "base_align_")

    // このrange_allocatorはcreate時に指定されたbase_align固定で範囲を管理する。
    // そのため、各空き領域ノードのoffsetは常にbase_align境界に整列している必要がある。よって、確保範囲としてpadding分も消費する
    //
    // offsetがbase_align境界に整列している場合、確保後の次の空き領域offsetを
    // base_align境界に保つために必要なpadding量は、各nodeのoffsetには依存しない
    // よって、required_size_をbase_align単位に丸めた値が、実際に消費するallocation_sizeとなる。
    padding = required_size_ % base_align_;
    if(0 != padding) {
        padding = base_align_ - padding;
    }

    if((SIZE_MAX - padding) < required_size_) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("align_up(%s) - Failed to align allocation size. reason=required_size_plus_padding_overflow, required_size=%zu, base_align=%zu, padding=%zu, max_safe_required_size=%zu", rslt_to_str(ret), required_size_, base_align_, padding, SIZE_MAX - padding);
        goto cleanup;
    }
    *out_allocation_size_ = required_size_ + padding;    // 割り当て領域の後ろにpaddingを追加し、offsetは常にbase_alignに整列されるようにする

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief first-fit方式でallocation可能なFREE nodeを検索する
 *
 * @details
 * FREE／ALLOCATED nodeがaddress orderで接続されたrange listを先頭から走査し、
 * block sizeがallocation_size_以上である最初のFREE nodeを返す。
 *
 * allocation_size_は、required sizeではなく、align_up()によって
 * base alignment単位に切り上げられた実確保サイズである。
 *
 * ALLOCATED nodeおよびallocation_size_を収容できないFREE nodeは読み飛ばす。
 * 選択したFREE nodeについては、offsetがbase alignment境界に整列していることを検証する。
 *
 * range listの末尾まで走査しても条件を満たすFREE nodeが存在しない場合は、
 * RANGE_ALLOCATOR_NO_MEMORYを返す。
 *
 * range listの走査回数がmax node countに到達しても
 * list終端であるNULLへ到達しない場合は、node数またはlist接続の
 * 内部不整合としてRANGE_ALLOCATOR_DATA_CORRUPTEDを返す。
 *
 * 本関数はRange Allocator、range list、およびnodeの状態を変更しない。
 *
 * @param[in] range_allocator_
 * 検索対象となるRange Allocator。
 *
 * @param[in] allocation_size_
 * 必要な実確保サイズ(byte)。
 * 0ではなく、range_allocator_のbase alignmentの倍数でなければならない。
 *
 * @param[in,out] out_node_
 * 検索結果となるFREE node pointerの格納先。
 * 呼び出し前に*out_node_をNULLにする必要がある。
 * 成功時のみ検索結果が格納され、失敗時は変更されない。
 *
 * @pre
 * range_allocator_に対するshallow validationまたはdeep validationが、
 * 呼び出し元によって完了していなければならない。
 *
 * @pre
 * range listへ接続されたnodeのstateはFREEまたはALLOCATEDであり、
 * range list接続数はmax node count以下でなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * allocation_size_を収容できる最初のFREE nodeが見つかった。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * 次のいずれか。
 * - range_allocator_がNULL
 * - allocation_size_が0
 * - out_node_がNULL
 * - *out_node_がNULLではない
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * allocation_size_がrange_allocator_のbase alignmentの倍数ではない。
 *
 * @retval RANGE_ALLOCATOR_NO_MEMORY
 * range listの末尾までに、allocation_size_を収容できる連続したFREE rangeが存在しない。
 *
 * total free sizeがallocation_size_以上でも、断片化によって十分な大きさの
 * 連続FREE rangeが存在しない場合は、この結果となる。
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 * - range_list_headがNULL
 * - 選択したFREE nodeのoffsetがbase alignment境界にない
 * - max node count以内にrange listの走査が終了しない
 *
 * @post 成功時は次が成立する。
 * - *out_node_はrange list上のFREE nodeを指す
 * - (*out_node_)->block_sizeはallocation_size_以上である
 * - (*out_node_)->offsetはbase alignment境界にある
 * - *out_node_より前に、allocation_size_を収容できるFREE nodeは存在しない
 *
 * @post 失敗時、*out_node_は変更されない。
 *
 * @note
 * 返されるnode pointerはnode pool内の要素を指すborrowed pointerであり、
 * 呼び出し側へ所有権は移動しない。
 *
 * @par 計算量
 * 最悪の場合はrange listをmax node countまで走査するため、
 * 時間計算量はrange list上のnode数に対してO(n)である。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t find_first_fit_node(const range_allocator_t* range_allocator_, size_t allocation_size_, node_t** out_node_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    size_t index = 0;
    bool found = false;
    node_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "find_first_fit_node", "range_allocator_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != allocation_size_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "find_first_fit_node", "allocation_size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "find_first_fit_node", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "find_first_fit_node", "*out_node_")
    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_->range_list_head, ret, RANGE_ALLOCATOR_DATA_CORRUPTED, rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), "find_first_fit_node", "range_allocator_->range_list_head")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (allocation_size_ % range_allocator_->base_align), ret, RANGE_ALLOCATOR_BAD_OPERATION, rslt_to_str(RANGE_ALLOCATOR_BAD_OPERATION), "find_first_fit_node", "allocation_size_")

    node = range_allocator_->range_list_head;
    while(NULL != node && index < range_allocator_->max_node_count) {
        if(NODE_STATE_FREE == node->node_state && node->block_size >= allocation_size_) {
            if(0 != (node->offset % range_allocator_->base_align)) {
                ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
                ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first-fit free block. reason=free_block_offset_misaligned, allocation_size=%zu, list_position=%zu, node_offset=%zu, node_block_size=%zu, base_align=%zu", rslt_to_str(ret), allocation_size_, index, node->offset, node->block_size, range_allocator_->base_align);
                goto cleanup;
            }
            found = true;
            break;
        } else {
            node = node->next;
        }
        index++;
    }

    if(!found) {
        if(NULL == node) {  // 末尾まで走査したが該当ノードなし
            ret = RANGE_ALLOCATOR_NO_MEMORY;
            ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first-fit free block. reason=contiguous_free_range_unavailable, allocation_size=%zu, total_free_size=%zu, memory_pool_size=%zu", rslt_to_str(ret), allocation_size_, range_allocator_->memory_pool_size - range_allocator_->total_allocated_size, range_allocator_->memory_pool_size);
            goto cleanup;
        } else {            // max node countまでにlist走査が終了しなかった
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first-fit free block. reason=range_list_traversal_limit_exceeded, allocation_size=%zu, traversed_node_count=%zu, max_node_count=%zu", rslt_to_str(ret), allocation_size_, index, range_allocator_->max_node_count);
            goto cleanup;
        }
    }
    *out_node_ = node;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief FREE nodeの先頭から指定サイズのrangeを確保する
 *
 * @details
 * node_が表すFREE rangeの先頭からallocation_size_を確保し、
 * node_を対応するALLOCATED nodeへ遷移させる。
 *
 * 本関数は次の二方式でallocationを行う。
 *
 * @par Exact fit
 * node_のblock sizeとallocation_size_が等しい場合、
 * node_のoffset、block size、prev、およびnextを変更せず、
 * stateをFREEからALLOCATEDへ遷移させる。
 *
 * 新しいnodeは取得せず、unused node countも変更しない。
 *
 * @par Partial allocation
 * node_のblock sizeがallocation_size_より大きい場合、
 * node_の先頭部分をALLOCATED rangeとして使用し、
 * 残りを新しい後方FREE nodeとして表現する。
 *
 * 処理後のrange構成は次のようになる。
 *
 * - node_のoffsetは変更しない
 * - node_のblock sizeをallocation_size_へ変更する
 * - 後方FREE nodeのoffsetをnode_の元offset + allocation_size_とする
 * - 後方FREE nodeのblock sizeをnode_の元block size - allocation_size_とする
 * - 後方FREE nodeをnode_の直後へ接続する
 * - node_をFREEからALLOCATEDへ遷移させる
 *
 * 後方FREE nodeはnode poolから取得し、TRANSITIONINGを経てrange listへFREEとして接続する。
 *
 * @par 失敗時のrollback
 * 後方FREE nodeの取得後にlistへの挿入が失敗した場合は、
 * node_のblock sizeを元の値へ戻し、取得したTRANSITIONING nodeをnode poolへreleaseする。
 * nodeのreleaseに成功した場合、Range Allocatorの内部状態は呼び出し前の状態へ戻る。
 *
 * @param[in,out] range_allocator_
 * node_を所有するRange Allocator。
 * partial allocationではnode pool、unused node count、および
 * range listの接続関係が更新される。
 *
 * @param[in,out] node_
 * allocation対象となるFREE node。
 * 成功時は同じnodeがALLOCATED rangeを表す。
 *
 * @param[in] allocation_size_
 * node_の先頭から確保する実確保サイズ(byte)。
 *
 * @pre
 * range_allocator_に対するshallow validationまたはdeep validationが、
 * 呼び出し元によって完了していなければならない。
 *
 * @pre
 * node_はrange_allocator_のnode poolに所属し、
 * range listへFREEとして接続されていなければならない。
 *
 * @pre
 * allocation_size_は0ではなく、range_allocator_のbase alignmentの
 * 倍数でなければならない。
 *
 * @pre
 * allocation_size_はnode_のblock size以下でなければならない。
 *
 * @pre
 * node_のoffsetはbase alignment境界にあり、
 * node_に隣接するFREE nodeが存在してはならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * exact fitまたはpartial allocationによるrange確保に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * 次のいずれか。
 * - range_allocator_がNULL
 * - node_がNULL
 * - allocation_size_が0
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * 次のいずれか。
 * - node_のstateがFREEではない
 * - allocation_size_がnode_のblock sizeを超えている
 * - 後方FREE nodeの取得または挿入に必要な操作条件を満たしていない
 *
 * @retval RANGE_ALLOCATOR_LIMIT_EXCEEDED
 * partial allocationに必要なNOT_USED nodeが存在しない。
 *
 * 正常な内部状態でallocation countがmax allocation count未満であれば、
 * max node countの設計上、この結果は発生しない。
 *
 * @retval RANGE_ALLOCATOR_OVERFLOW
 * 次のいずれか。
 * - node_のoffsetへallocation_size_を加えるとsize_tの表現可能範囲を超える
 * - 後方FREE nodeのoffsetとblock sizeからrange終端を計算するとsize_tの表現可能範囲を超える
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 * - node_が局所的不変条件を満たしていない
 * - unused node countと実際のNOT_USED node数が一致しない
 * - 取得対象node、node pool、またはrange listの接続関係に不整合がある
 * - 後方FREE nodeの挿入失敗後にnodeをreleaseできず、rollbackを完遂できなかった
 *
 * @post 成功時、node_はallocation_size_のALLOCATED rangeを表す。
 *
 * @post partial allocation成功時は次が成立する。
 * - node_の直後に残りrangeを表すFREE nodeが接続される
 * - unused node countが1減少する
 *
 * @post exact fit成功時は次が成立する。
 * - node_のrange情報とlist接続は変更されない
 * - unused node countは変更されない
 *
 * @post 失敗時、rollbackに成功した場合は、range_allocator_、node_、
 *       node pool、およびrange listが呼び出し前の状態に保たれる。
 *
 * @warning
 * rollback中のnode releaseに失敗した場合はRANGE_ALLOCATOR_DATA_CORRUPTEDを返し、
 * TRANSITIONING nodeがnode pool内に残る可能性がある。
 *
 * @par 計算量
 * exact fitの場合はO(1)である。
 *
 * partial allocationではNOT_USED nodeをnode poolから線形探索するため、
 * max node countに対してO(n)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t allocate_from_node(range_allocator_t* range_allocator_, node_t* node_, size_t allocation_size_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;
    range_allocator_result_t ret_cleanup = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    node_t* new_node = NULL;
    size_t new_node_block_size = 0;
    size_t new_node_offset = 0;
    size_t block_size_escape = 0;

    bool acquired = false;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "allocate_from_node", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "allocate_from_node", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_FREE == node_->node_state, ret, RANGE_ALLOCATOR_BAD_OPERATION, rslt_to_str(RANGE_ALLOCATOR_BAD_OPERATION), "allocate_from_node", "node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != allocation_size_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "allocate_from_node", "allocation_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_->block_size >= allocation_size_, ret, RANGE_ALLOCATOR_BAD_OPERATION, rslt_to_str(RANGE_ALLOCATOR_BAD_OPERATION), "allocate_from_node", "allocation_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_ALLOCATOR_DATA_CORRUPTED, rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), "allocate_from_node", "node_")

    if(node_->block_size == allocation_size_) {
        set_node_to_allocated(node_, node_->prev, node_->next);
    } else {
        if((SIZE_MAX - allocation_size_) < node_->offset) {
            ret = RANGE_ALLOCATOR_OVERFLOW;
            ERROR_MESSAGE("allocate_from_node(%s) - Failed to allocate range from free block. reason=remaining_free_block_offset_overflow, node_offset=%zu, node_block_size=%zu, allocation_size=%zu, max_safe_node_offset=%zu", rslt_to_str(ret), node_->offset, node_->block_size, allocation_size_, SIZE_MAX - allocation_size_);
            goto cleanup;
        }
        new_node_offset = node_->offset + allocation_size_;
        new_node_block_size = node_->block_size - allocation_size_;

        ret = node_acquire(range_allocator_, new_node_offset, new_node_block_size, &new_node);
        if(RANGE_ALLOCATOR_SUCCESS != ret) {
            ERROR_MESSAGE("allocate_from_node(%s) - Failed to allocate range from free block. reason=remaining_free_block_node_acquire_failed, allocation_size=%zu, remaining_block_offset=%zu, remaining_block_size=%zu, unused_node_count=%zu, max_node_count=%zu", rslt_to_str(ret), allocation_size_, new_node_offset, new_node_block_size, range_allocator_->unused_node_count, range_allocator_->max_node_count);
            goto cleanup;
        }
        acquired = true;
        block_size_escape = node_->block_size;
        node_->block_size = allocation_size_;

        ret = node_insert_between(range_allocator_, new_node, NODE_STATE_FREE, node_, node_->next);
        if(RANGE_ALLOCATOR_SUCCESS != ret) {
            ERROR_MESSAGE("allocate_from_node(%s) - Failed to allocate range from free block. reason=remaining_free_block_node_insert_failed, allocation_offset=%zu, allocation_size=%zu, remaining_block_offset=%zu, remaining_block_size=%zu", rslt_to_str(ret), node_->offset, allocation_size_, new_node_offset, new_node_block_size);
            goto cleanup;
        }

        set_node_to_allocated(node_, node_->prev, node_->next);
    }

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        if(acquired) {
            node_->block_size = block_size_escape;
            ret_cleanup = node_release(range_allocator_, new_node);
            if(RANGE_ALLOCATOR_SUCCESS != ret_cleanup) {
                ERROR_MESSAGE("allocate_from_node(%s) - Failed to allocate range from free block. reason=rollback_node_release_failed, original_result=%s, rollback_result=%s, allocation_offset=%zu, allocation_size=%zu, remaining_block_offset=%zu, remaining_block_size=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), rslt_to_str(ret_cleanup), node_->offset, allocation_size_, new_node_offset, new_node_block_size);
                ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
                return ret;
            }
        }
    }

    return ret;
}

/**
 * @brief allocation descriptorから対応するALLOCATED nodeを取得する
 *
 * @details
 * allocation_info_に記録されたownerとnode indexを使用して、
 * range_allocator_のnode poolから対応nodeを取得する。
 *
 * 取得したnodeについて、node単体の局所的不変条件、node state、
 * offset、およびblock sizeがdescriptorと一致することを検証する。
 *
 * descriptorの不正とRange Allocator内部データの破損を区別し、
 * descriptorがlive allocationを正しく表していない場合は
 * RANGE_ALLOCATOR_BAD_OPERATIONを返す。
 *
 * 本関数はRange Allocator、allocation descriptor、およびnodeの状態を変更しない。
 *
 * @param[in] range_allocator_
 * allocation_info_のownerであることを検証するRange Allocator。
 *
 * @param[in] allocation_info_
 * 対応nodeを取得するallocation descriptor。
 *
 * @param[in,out] out_node_
 * 取得したALLOCATED node pointerの格納先。
 * 呼び出し前に*out_node_をNULLにする必要がある。
 * 成功時のみnode pointerが格納され、失敗時は変更されない。
 *
 * @pre
 * range_allocator_に対するshallow validationまたはdeep validationが、
 * 呼び出し元によって完了していなければならない。
 *
 * @pre
 * range_allocator_のnode poolが有効であり、
 * max node count個のnodeを保持していなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * descriptorに対応するALLOCATED nodeの取得と検証に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * 次のいずれか。
 * - range_allocator_がNULL
 * - allocation_info_がNULL
 * - out_node_がNULL
 * - *out_node_がNULLではない
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * 次のいずれか。
 * - range_allocator_にlive allocationが存在しない
 * - allocation_info_->allocated_sizeが0
 * - allocation_info_->ownerがrange_allocator_と一致しない
 * - allocation_info_->node_indexがnode poolの範囲外
 * - 対応nodeのstateがALLOCATEDではない
 * - 対応nodeのblock sizeがallocation_info_->allocated_sizeと一致しない
 * - 対応nodeのoffsetがallocation_info_->offsetと一致しない
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * descriptorのnode indexに対応するnodeが、
 * node単体の局所的不変条件を満たしていない。
 *
 * @post 成功時は次が成立する。
 * - *out_node_はrange_allocator_のnode pool要素を指す
 * - (*out_node_)->node_stateはNODE_STATE_ALLOCATEDである
 * - (*out_node_)->offsetはallocation_info_->offsetと一致する
 * - (*out_node_)->block_sizeはallocation_info_->allocated_sizeと一致する
 *
 * @post 失敗時、*out_node_は変更されない。
 *
 * @note
 * 返されるnode pointerはnode pool内の要素を指すborrowed pointerであり、
 * 呼び出し側へ所有権は移動しない。
 *
 * @warning
 * allocation descriptorはnode generationを保持しない。
 * 解放済みnodeが別のallocationへ再利用され、owner、node index、offset、
 * allocated sizeがすべて一致した場合、stale descriptorを検出できない。
 *
 * @par 計算量
 * node indexからnode pool要素を直接取得するため、時間計算量はO(1)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t allocation_resolve_node(const range_allocator_t* range_allocator_, const range_allocation_t* allocation_info_, node_t** out_node_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    node_t* tmp_node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "allocation_resolve_node", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(allocation_info_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "allocation_resolve_node", "allocation_info_")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "allocation_resolve_node", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "allocation_resolve_node", "*out_node_")

    if(range_allocator_ != allocation_info_->owner) {
        ret = RANGE_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - Failed to resolve allocation node. reason=allocation_owner_mismatch, expected_owner=%p, actual_owner=%p, node_index=%zu, allocation_offset=%zu, allocation_size=%zu", rslt_to_str(ret), (void*)range_allocator_, (void*)allocation_info_->owner, allocation_info_->node_index, allocation_info_->offset, allocation_info_->allocated_size);
        goto cleanup;
    }
    if(range_allocator_->max_node_count <= allocation_info_->node_index) {
        ret = RANGE_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - Failed to resolve allocation node. reason=node_index_out_of_range, node_index=%zu, max_node_count=%zu, allocation_offset=%zu, allocation_size=%zu", rslt_to_str(ret), allocation_info_->node_index, range_allocator_->max_node_count, allocation_info_->offset, allocation_info_->allocated_size);
        goto cleanup;
    }

    tmp_node = &range_allocator_->node_pool[allocation_info_->node_index];

    if(!node_is_valid(tmp_node)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("allocation_resolve_node(%s) - Failed to resolve allocation node. reason=allocation_node_validation_failed, node_index=%zu, node_state=%d, node_offset=%zu, node_block_size=%zu, has_prev=%d, has_next=%d, prev_is_self=%d, next_is_self=%d", rslt_to_str(ret), allocation_info_->node_index, (int)tmp_node->node_state, tmp_node->offset, tmp_node->block_size, (int)(NULL != tmp_node->prev), (int)(NULL != tmp_node->next), (int)(tmp_node->prev == tmp_node), (int)(tmp_node->next == tmp_node));
        goto cleanup;
    }
    if(NODE_STATE_ALLOCATED != tmp_node->node_state) {
        ret = RANGE_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - Failed to resolve allocation node. reason=allocation_node_state_mismatch, node_index=%zu, expected_state=ALLOCATED, actual_state=%d, allocation_offset=%zu, allocation_size=%zu, node_offset=%zu, node_block_size=%zu", rslt_to_str(ret), allocation_info_->node_index, (int)tmp_node->node_state, allocation_info_->offset, allocation_info_->allocated_size, tmp_node->offset, tmp_node->block_size);
        goto cleanup;
    }
    if(tmp_node->block_size != allocation_info_->allocated_size) {
        ret = RANGE_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - Failed to resolve allocation node. reason=allocation_size_mismatch, node_index=%zu, descriptor_allocated_size=%zu, node_block_size=%zu, descriptor_offset=%zu, node_offset=%zu", rslt_to_str(ret), allocation_info_->node_index, allocation_info_->allocated_size, tmp_node->block_size, allocation_info_->offset, tmp_node->offset);
        goto cleanup;
    }
    if(tmp_node->offset != allocation_info_->offset) {
        ret = RANGE_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - Failed to resolve allocation node. reason=allocation_offset_mismatch, node_index=%zu, descriptor_offset=%zu, node_offset=%zu, allocation_size=%zu", rslt_to_str(ret), allocation_info_->node_index, allocation_info_->offset, tmp_node->offset, allocation_info_->allocated_size);
        goto cleanup;
    }

    *out_node_ = tmp_node;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief ALLOCATED nodeのfree時に必要な隣接FREE nodeとのmerge方針を取得する
 *
 * @details
 * node_の直前および直後に接続されたnodeのstateを確認し、
 * free時に前方FREE node、後方FREE nodeとmergeすべきかを返す。
 *
 * merge方針は次の四通りとなる。
 *
 * - 前後ともFREEではない: mergeなし
 * - 前方nodeだけがFREE: 前方merge
 * - 後方nodeだけがFREE: 後方merge
 * - 前後両方のnodeがFREE: 前後merge
 *
 * FREE状態の隣接nodeについては、node_is_valid()によって
 * node単体の局所的不変条件を検証する。
 *
 * 本関数はnodeの状態を変更しない。
 *
 * @param[in] node_
 * free対象となるALLOCATED node。
 *
 * @param[out] out_should_merge_prev_
 * 前方FREE nodeとmergeする必要がある場合はtrue、
 * それ以外の場合はfalseが格納される。
 * 成功時のみ値が設定され、失敗時は変更されない。
 *
 * @param[out] out_should_merge_next_
 * 後方FREE nodeとmergeする必要がある場合はtrue、
 * それ以外の場合はfalseが格納される。
 * 成功時のみ値が設定され、失敗時は変更されない。
 *
 * @pre
 * node_のstateはNODE_STATE_ALLOCATEDでなければならない。
 *
 * @pre
 * node_->prevおよびnode_->nextがNULLではない場合、
 * それぞれrange list上でnode_に直接接続されたnodeでなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * 前方および後方nodeとのmerge方針の取得に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * 次のいずれか。
 * - node_がNULL
 * - out_should_merge_prev_がNULL
 * - out_should_merge_next_がNULL
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 * - node_がnode単体の局所的不変条件を満たしていない
 * - FREE状態の前方nodeがnode単体の局所的不変条件を満たしていない
 * - FREE状態の後方nodeがnode単体の局所的不変条件を満たしていない
 *
 * @post 成功時は次が成立する。
 * - *out_should_merge_prev_は、前方nodeがFREEの場合に限りtrueとなる
 * - *out_should_merge_next_は、後方nodeがFREEの場合に限りtrueとなる
 *
 * @post 失敗時、二つの出力先は変更されない。
 *
 * @par 計算量
 * 直接接続された前後nodeだけを確認するため、時間計算量はO(1)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t free_merge_plan_get(const node_t* node_, bool* out_should_merge_prev_, bool* out_should_merge_next_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    bool tmp_should_merge_prev = false;
    bool tmp_should_merge_next = false;

    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_merge_plan_get", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_should_merge_prev_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_merge_plan_get", "out_should_merge_prev_")
    IF_ARG_NULL_GOTO_CLEANUP(out_should_merge_next_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_merge_plan_get", "out_should_merge_next_")

    // 前方ノード検証
    if(NULL != node_->prev && NODE_STATE_FREE == node_->prev->node_state) {
        if(!node_is_valid(node_->prev)) {
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("free_merge_plan_get(%s) - Failed to determine free merge plan. reason=previous_free_node_validation_failed, target_node_offset=%zu, target_node_block_size=%zu, previous_node_offset=%zu, previous_node_block_size=%zu, previous_node_prev_is_self=%d, previous_node_next_is_self=%d", rslt_to_str(ret), node_->offset, node_->block_size, node_->prev->offset, node_->prev->block_size, (int)(node_->prev->prev == node_->prev), (int)(node_->prev->next == node_->prev));
            goto cleanup;
        }
        tmp_should_merge_prev = true;
    }

    // 後方ノード検証
    if(NULL != node_->next && NODE_STATE_FREE == node_->next->node_state) {
        if(!node_is_valid(node_->next)) {
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("free_merge_plan_get(%s) - Failed to determine free merge plan. reason=next_free_node_validation_failed, target_node_offset=%zu, target_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, next_node_prev_is_self=%d, next_node_next_is_self=%d", rslt_to_str(ret), node_->offset, node_->block_size, node_->next->offset, node_->next->block_size, (int)(node_->next->prev == node_->next), (int)(node_->next->next == node_->next));
            goto cleanup;
        }
        tmp_should_merge_next = true;
    }

    *out_should_merge_prev_ = tmp_should_merge_prev;
    *out_should_merge_next_ = tmp_should_merge_next;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief merge方針に従ってALLOCATED nodeをfreeする
 *
 * @details
 * should_merge_prev_とshould_merge_next_の組み合わせに応じて、
 * 対応するfree処理へdispatchする。
 *
 * - false／false: 隣接nodeとmergeせず、node_をFREEへ遷移させる
 * - true／false: 前方FREE nodeとmergeする
 * - false／true: 後方FREE nodeとmergeする
 * - true／true: 前後両方のFREE nodeとmergeする
 *
 * 本関数および呼び出されるfree処理では、新しいnodeの取得や動的メモリ確保を行わない。
 *
 * @par 責務範囲
 * 本関数は、freeに伴うnode state、range情報、range listの接続関係、
 * node poolの利用状態、およびunused node countを更新する。
 *
 * allocation countとtotal allocated sizeは更新しない。
 * これらは、本関数の成功後に呼び出し元がcommitする。
 *
 * @param[in,out] range_allocator_
 * node_を所有するRange Allocator。
 * mergeによってnodeが不要になる場合は、node poolおよび
 * unused node countが更新される。
 *
 * @param[in,out] node_
 * free対象となるALLOCATED node。
 *
 * merge方針によって、成功後はFREE nodeとしてrange listへ残るか、
 * range listから切断されてNOT_USED状態となる。
 *
 * @param[in] should_merge_prev_
 * 前方FREE nodeとmergeする場合はtrue。
 *
 * @param[in] should_merge_next_
 * 後方FREE nodeとmergeする場合はtrue。
 *
 * @pre
 * range_allocator_に対するdeep validationが、
 * 呼び出し元によって完了していなければならない。
 *
 * @pre
 * node_はrange_allocator_のnode poolに所属し、
 * range listへALLOCATEDとして接続されていなければならない。
 *
 * @pre
 * should_merge_prev_とshould_merge_next_は、node_の前後に接続された
 * FREE nodeの有無と一致していなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * 指定されたmerge方針によるfreeに成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_またはnode_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * 次のいずれか。
 * - node_のstateがALLOCATEDではない
 * - merge対象として指定された隣接nodeがFREEではない
 *
 * @retval RANGE_ALLOCATOR_OVERFLOW
 * 隣接rangeの終端またはmerge後のblock size計算が
 * size_tの表現可能範囲を超える。
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 * - node_が局所的不変条件を満たしていない
 * - dispatch先のfree処理がnode、range list、range、または
 *   node poolの不整合を検出した
 *
 * @post 成功時は、node_と隣接FREE nodeの関係に応じて次が成立する。
 * - mergeなし: node_がFREEへ遷移する
 * - 前方merge: 前方FREE nodeが拡張され、node_がNOT_USEDとなる
 * - 後方merge: node_が後方へ拡張されたFREE nodeとなり、元の後方FREE nodeがNOT_USEDとなる
 * - 前後merge: 前方FREE nodeが前後すべてを含むrangeへ拡張され、node_と元の後方FREE nodeがNOT_USEDとなる
 *
 * @warning
 * free処理開始後に内部データ破損が検出された場合、
 * 呼び出し前の状態へrollbackできない可能性がある。
 *
 * @par 計算量
 * mergeを行わない場合はO(1)である。
 *
 * mergeを行う場合は、nodeの所属確認のためnode poolを線形探索するため、
 * max node countに対してO(n)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t free_from_node(range_allocator_t* range_allocator_, node_t* node_, bool should_merge_prev_, bool should_merge_next_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_from_node", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_from_node", "node_")

    if(!should_merge_prev_ && !should_merge_next_) {
        ret = free_node_without_merge(node_);
    } else if(!should_merge_prev_ && should_merge_next_) {
        ret = free_node_merge_next(range_allocator_, node_);
    } else if(should_merge_prev_ && !should_merge_next_) {
        ret = free_node_merge_prev(range_allocator_, node_);
    } else {
        ret = free_node_merge_prev_next(range_allocator_, node_);
    }

cleanup:
    return ret;
}

/**
 * @brief 隣接FREE nodeとmergeせずにALLOCATED nodeをfreeする
 *
 * @details
 * node_のoffset、block size、prev、およびnextを変更せず、
 * node stateだけをALLOCATEDからFREEへ遷移させる。
 *
 * node_はrange listへ接続されたままとなり、新しいnodeの取得、
 * nodeの切断、およびnode poolへのreleaseは行わない。
 *
 * @par 責務範囲
 * 本関数はnode_のstateだけを変更する。
 *
 * allocation count、total allocated size、unused node count、および
 * range listの接続関係は変更しない。
 * allocation countとtotal allocated sizeは、本関数の成功後に
 * 呼び出し元がcommitする。
 *
 * @param[in,out] node_
 * free対象となるALLOCATED node。
 * 成功時は同じrange情報とlist接続を維持したFREE nodeとなる。
 *
 * @pre
 * node_はnode poolに所属し、range listへALLOCATEDとして
 * 接続されていなければならない。
 *
 * @pre
 * node_の前後にFREE nodeが存在してはならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * node_をALLOCATEDからFREEへ遷移させることに成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * node_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * node_のstateがALLOCATEDではない。
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * node_がnode単体の局所的不変条件を満たしていない。
 *
 * @post 成功時は次が成立する。
 * - node_->node_stateはNODE_STATE_FREEである
 * - node_のoffsetとblock sizeは変更されない
 * - node_のprevとnextは変更されない
 * - node_はrange listへ接続されたままである
 *
 * @post 失敗時、node_は変更されない。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t free_node_without_merge(node_t* node_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_node_without_merge", "node_")

    node_->node_state = NODE_STATE_FREE;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief ALLOCATED nodeを前方FREE nodeへmergeしてfreeする
 *
 * @details
 * node_の直前に接続されたFREE nodeのrangeを後方へ拡張し、
 * node_が表していたALLOCATED rangeを統合する。
 *
 * 処理は次の順序で行う。
 *
 * 1. node_と前方FREE nodeのstate、接続関係、およびrange隣接性を検証する
 * 2. merge後のblock sizeを算出する
 * 3. node_をrange listから切断する
 * 4. node_をTRANSITIONINGからNOT_USEDへ遷移させる
 * 5. 前方FREE nodeのblock sizeをmerge後のサイズへ更新する
 *
 * merge後のFREE rangeのoffsetは前方FREE nodeのoffsetを維持する。
 * node_はnode poolへreleaseされ、新しいnodeの取得や動的メモリ確保は行わない。
 *
 * @par 責務範囲
 * 本関数は、前方FREE nodeのblock size、range listの接続関係、
 * node_のstate、unused node count、およびnode poolの利用状態を更新する。
 *
 * allocation countとtotal allocated sizeは更新しない。
 * これらは、本関数の成功後に呼び出し元がcommitする。
 *
 * @param[in,out] range_allocator_
 * node_と前方FREE nodeを所有するRange Allocator。
 *
 * @param[in,out] node_
 * free対象となるALLOCATED node。
 * 成功時はrange listから切断され、NOT_USED状態となる。
 *
 * @pre
 * range_allocator_に対するdeep validationが、
 * 呼び出し元によって完了していなければならない。
 *
 * @pre
 * node_はrange_allocator_のnode poolに所属し、
 * range listへALLOCATEDとして接続されていなければならない。
 *
 * @pre
 * node_->prevはNULLではなく、node_の直前へFREEとして
 * 接続されていなければならない。
 *
 * @pre
 * 前方FREE nodeの終端はnode_のoffsetと一致していなければならない。
 *
 * @pre
 * node_->nextはNULLであるか、FREE以外のstateでなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * node_を前方FREE nodeへmergeしてfreeすることに成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_またはnode_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * 次のいずれか。
 * - node_のstateがALLOCATEDではない
 * - node_->prevのstateがFREEではない
 *
 * @retval RANGE_ALLOCATOR_OVERFLOW
 * 次のいずれか。
 * - 前方FREE nodeのoffsetとblock sizeから終端を計算するとsize_tの表現可能範囲を超える
 * - 前方FREE nodeとnode_のblock sizeを加算するとsize_tの表現可能範囲を超える
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 * - node_または前方FREE nodeが局所的不変条件を満たしていない
 * - node_が前方FREE nodeのnextとして接続されていない
 * - 前方FREE nodeの終端がnode_のoffsetと一致しない
 * - node_がrange_allocator_のnode poolに所属していない
 * - node_をrange listから切断できない
 * - 切断したnode_をnode poolへreleaseできない
 *
 * @post 成功時は次が成立する。
 * - 前方FREE nodeのoffsetは変更されない
 * - 前方FREE nodeのblock sizeは、元の前方FREE rangeとnode_のrangeを合計したサイズとなる
 * - node_はrange listから切断され、NOT_USED状態となる
 * - unused node countが1増加する
 * - range listに隣接したFREE nodeは残らない
 *
 * @post
 * state変更を開始する前に失敗した場合、
 * Range Allocatorおよびnodeの状態は変更されない。
 *
 * @warning
 * node_をrange listから切断した後にnode releaseが失敗した場合、
 * 呼び出し前の状態へrollbackできない。
 * この場合はRANGE_ALLOCATOR_DATA_CORRUPTEDを返す。
 *
 * @par 計算量
 * 内部で呼び出すnode_remove()とnode_release()がnode poolを
 * 線形探索するため、時間計算量はmax node countに対してO(n)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t free_node_merge_prev(range_allocator_t* range_allocator_, node_t* node_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    size_t tmp_offset = 0;
    size_t new_block_size = 0;
    node_t* tmp_node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_node_merge_prev", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_node_merge_prev", "node_")

    if(node_ != node_->prev->next) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev(%s) - Failed to merge allocated node with previous free node. reason=previous_free_node_next_mismatch, allocated_node_offset=%zu, allocated_node_block_size=%zu, previous_node_offset=%zu, previous_node_block_size=%zu, expected_next=%p, actual_next=%p", rslt_to_str(ret), node_->offset, node_->block_size, node_->prev->offset, node_->prev->block_size, (void*)node_, (void*)node_->prev->next);
        goto cleanup;
    }

    if((SIZE_MAX - node_->prev->offset) < node_->prev->block_size) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev(%s) - Failed to merge allocated node with previous free node. reason=previous_free_block_end_overflow, previous_node_offset=%zu, previous_node_block_size=%zu, max_safe_block_size=%zu, allocated_node_offset=%zu", rslt_to_str(ret), node_->prev->offset, node_->prev->block_size, SIZE_MAX - node_->prev->offset, node_->offset);
        goto cleanup;
    }
    tmp_offset = node_->prev->offset + node_->prev->block_size;
    if(tmp_offset != node_->offset) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev(%s) - Failed to merge allocated node with previous free node. reason=previous_free_block_end_mismatch, previous_node_offset=%zu, previous_node_block_size=%zu, previous_block_end=%zu, allocated_node_offset=%zu, allocated_node_block_size=%zu", rslt_to_str(ret), node_->prev->offset, node_->prev->block_size, tmp_offset, node_->offset, node_->block_size);
        goto cleanup;
    }

    if((SIZE_MAX - node_->prev->block_size) < node_->block_size) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev(%s) - Failed to merge allocated node with previous free node. reason=merged_block_size_overflow, previous_node_offset=%zu, previous_node_block_size=%zu, allocated_node_offset=%zu, allocated_node_block_size=%zu, max_safe_allocated_block_size=%zu", rslt_to_str(ret), node_->prev->offset, node_->prev->block_size, node_->offset, node_->block_size, SIZE_MAX - node_->prev->block_size);
        goto cleanup;
    }
    new_block_size = node_->prev->block_size + node_->block_size;
    tmp_node = node_->prev;

    ret = node_remove(range_allocator_, node_);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_node_merge_prev(%s) - Failed to merge allocated node with previous free node. reason=allocated_node_remove_failed, node_remove_result=%s, allocated_node_offset=%zu, allocated_node_block_size=%zu, previous_node_offset=%zu, previous_node_block_size=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), node_->offset, node_->block_size, node_->prev->offset, node_->prev->block_size);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        goto cleanup;
    }
    ret = node_release(range_allocator_, node_);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_node_merge_prev(%s) - Failed to merge allocated node with previous free node. reason=detached_node_release_failed, node_release_result=%s, allocated_node_offset=%zu, allocated_node_block_size=%zu, previous_node_offset=%zu, previous_node_block_size=%zu, unused_node_count=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), node_->offset, node_->block_size, tmp_node->offset, tmp_node->block_size, range_allocator_->unused_node_count);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        goto cleanup;
    }

    tmp_node->block_size = new_block_size;
    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief ALLOCATED nodeを後方FREE nodeとmergeしてfreeする
 *
 * @details
 * node_が表すALLOCATED rangeを後方へ拡張し、
 * 直後に接続されたFREE nodeのrangeを統合する。
 *
 * 処理は次の順序で行う。
 *
 * 1. node_と後方FREE nodeのstate、接続関係、およびrange隣接性を検証する
 * 2. merge後のblock sizeを算出する
 * 3. 後方FREE nodeをrange listから切断する
 * 4. 後方FREE nodeをTRANSITIONINGからNOT_USEDへ遷移させる
 * 5. node_のblock sizeをmerge後のサイズへ更新する
 * 6. node_をALLOCATEDからFREEへ遷移させる
 *
 * merge後のFREE rangeのoffsetはnode_のoffsetを維持する。
 * 後方FREE nodeはnode poolへreleaseされ、新しいnodeの取得や
 * 動的メモリ確保は行わない。
 *
 * @par 責務範囲
 * 本関数は、node_のstateとblock size、range listの接続関係、
 * 後方FREE nodeのstate、unused node count、およびnode poolの
 * 利用状態を更新する。
 *
 * allocation countとtotal allocated sizeは更新しない。
 * これらは、本関数の成功後に呼び出し元がcommitする。
 *
 * @param[in,out] range_allocator_
 * node_と後方FREE nodeを所有するRange Allocator。
 *
 * @param[in,out] node_
 * free対象となるALLOCATED node。
 * 成功時は後方FREE rangeを統合したFREE nodeとなり、
 * range listへ接続されたまま残る。
 *
 * @pre
 * range_allocator_に対するdeep validationが、
 * 呼び出し元によって完了していなければならない。
 *
 * @pre
 * node_はrange_allocator_のnode poolに所属し、
 * range listへALLOCATEDとして接続されていなければならない。
 *
 * @pre
 * node_->nextはNULLではなく、node_の直後へFREEとして
 * 接続されていなければならない。
 *
 * @pre
 * node_の終端は後方FREE nodeのoffsetと一致していなければならない。
 *
 * @pre
 * node_->prevはNULLであるか、FREE以外のstateでなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * node_を後方FREE nodeとmergeしてfreeすることに成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_またはnode_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * 次のいずれか。
 * - node_のstateがALLOCATEDではない
 * - node_->nextのstateがFREEではない
 *
 * @retval RANGE_ALLOCATOR_OVERFLOW
 * 次のいずれか。
 * - node_のoffsetとblock sizeから終端を計算するとsize_tの表現可能範囲を超える
 * - node_と後方FREE nodeのblock sizeを加算するとsize_tの表現可能範囲を超える
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 * - node_または後方FREE nodeが局所的不変条件を満たしていない
 * - node_が後方FREE nodeのprevとして接続されていない
 * - node_の終端が後方FREE nodeのoffsetと一致しない
 * - 後方FREE nodeがrange_allocator_のnode poolに所属していない
 * - 後方FREE nodeをrange listから切断できない
 * - 切断した後方FREE nodeをnode poolへreleaseできない
 *
 * @post 成功時は次が成立する。
 * - node_のoffsetは変更されない
 * - node_のblock sizeは、元のnode_のrangeと後方FREE rangeを合計したサイズとなる
 * - node_はFREE状態となり、range listへ接続されたまま残る
 * - 元の後方FREE nodeはrange listから切断され、NOT_USED状態となる
 * - unused node countが1増加する
 * - range listに隣接したFREE nodeは残らない
 *
 * @post
 * state変更を開始する前に失敗した場合、
 * Range Allocatorおよびnodeの状態は変更されない。
 *
 * @warning
 * 後方FREE nodeをrange listから切断した後にnode releaseが失敗した場合、
 * 呼び出し前の状態へrollbackできない。
 * この場合はRANGE_ALLOCATOR_DATA_CORRUPTEDを返す。
 *
 * @par 計算量
 * 内部で呼び出すnode_remove()とnode_release()がnode poolを
 * 線形探索するため、時間計算量はmax node countに対してO(n)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t free_node_merge_next(range_allocator_t* range_allocator_, node_t* node_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    size_t tmp_offset = 0;
    size_t new_block_size = 0;
    node_t* tmp_next = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_node_merge_next", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_node_merge_next", "node_")

    if(node_ != node_->next->prev) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_next(%s) - Failed to merge allocated node with next free node. reason=next_free_node_prev_mismatch, allocated_node_offset=%zu, allocated_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, expected_prev=%p, actual_prev=%p", rslt_to_str(ret), node_->offset, node_->block_size, node_->next->offset, node_->next->block_size, (void*)node_, (void*)node_->next->prev);
        goto cleanup;
    }

    if((SIZE_MAX - node_->offset) < node_->block_size) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_next(%s) - Failed to merge allocated node with next free node. reason=allocated_block_end_overflow, allocated_node_offset=%zu, allocated_node_block_size=%zu, max_safe_block_size=%zu, next_node_offset=%zu", rslt_to_str(ret), node_->offset, node_->block_size, SIZE_MAX - node_->offset, node_->next->offset);
        goto cleanup;
    }
    tmp_offset = node_->offset + node_->block_size;
    if(tmp_offset != node_->next->offset) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_next(%s) - Failed to merge allocated node with next free node. reason=allocated_block_end_mismatch, allocated_node_offset=%zu, allocated_node_block_size=%zu, allocated_block_end=%zu, next_node_offset=%zu, next_node_block_size=%zu", rslt_to_str(ret), node_->offset, node_->block_size, tmp_offset, node_->next->offset, node_->next->block_size);
        goto cleanup;
    }

    if((SIZE_MAX - node_->next->block_size) < node_->block_size) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_next(%s) - Failed to merge allocated node with next free node. reason=merged_block_size_overflow, allocated_node_offset=%zu, allocated_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, max_safe_allocated_block_size=%zu", rslt_to_str(ret), node_->offset, node_->block_size, node_->next->offset, node_->next->block_size, SIZE_MAX - node_->next->block_size);
        goto cleanup;
    }
    new_block_size = node_->next->block_size + node_->block_size;

    tmp_next = node_->next;
    ret = node_remove(range_allocator_, tmp_next);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_node_merge_next(%s) - Failed to merge allocated node with next free node. reason=next_free_node_remove_failed, node_remove_result=%s, allocated_node_offset=%zu, allocated_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), node_->offset, node_->block_size, tmp_next->offset, tmp_next->block_size);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        goto cleanup;
    }
    ret = node_release(range_allocator_, tmp_next);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_node_merge_next(%s) - Failed to merge allocated node with next free node. reason=detached_next_free_node_release_failed, node_release_result=%s, allocated_node_offset=%zu, allocated_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, unused_node_count=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), node_->offset, node_->block_size, tmp_next->offset, tmp_next->block_size, range_allocator_->unused_node_count);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        goto cleanup;
    }

    node_->block_size = new_block_size;
    node_->node_state = NODE_STATE_FREE;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief ALLOCATED nodeを前後両方のFREE nodeへmergeしてfreeする
 *
 * @details
 * node_の直前および直後に接続されたFREE nodeと、node_が表すALLOCATED rangeを一つのFREE rangeへ統合する。
 *
 * merge後は前方FREE nodeをrange listへ残し、そのblock sizeを
 * 前方FREE range、node_のrange、および後方FREE rangeの合計サイズへ拡張する。
 *
 * 処理は次の順序で行う。
 *
 * 1. node_と前後FREE nodeのstate、接続関係、およびrange隣接性を検証する
 * 2. 三つのrangeを統合したblock sizeを算出する
 * 3. node_をrange listから切断し、node poolへreleaseする
 * 4. 後方FREE nodeをrange listから切断し、node poolへreleaseする
 * 5. 前方FREE nodeのblock sizeをmerge後のサイズへ更新する
 *
 * node_と後方FREE nodeはNOT_USED状態となり、新しいnodeの取得や動的メモリ確保は行わない。
 *
 * @par 責務範囲
 * 本関数は、前方FREE nodeのblock size、range listの接続関係、
 * node_と後方FREE nodeのstate、unused node count、および
 * node poolの利用状態を更新する。
 *
 * allocation countとtotal allocated sizeは更新しない。
 * これらは、本関数の成功後に呼び出し元がcommitする。
 *
 * @param[in,out] range_allocator_
 * node_と前後FREE nodeを所有するRange Allocator。
 *
 * @param[in,out] node_
 * free対象となるALLOCATED node。
 * 成功時はrange listから切断され、NOT_USED状態となる。
 *
 * @pre
 * range_allocator_に対するdeep validationが、
 * 呼び出し元によって完了していなければならない。
 *
 * @pre
 * node_はrange_allocator_のnode poolに所属し、
 * range listへALLOCATEDとして接続されていなければならない。
 *
 * @pre
 * node_->prevとnode_->nextはともにNULLではなく、
 * node_の直前および直後へFREEとして接続されていなければならない。
 *
 * @pre
 * 前方FREE nodeの終端はnode_のoffsetと一致し、
 * node_の終端は後方FREE nodeのoffsetと一致しなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * node_を前後両方のFREE nodeへmergeしてfreeすることに成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_またはnode_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * 次のいずれか。
 * - node_のstateがALLOCATEDではない
 * - node_->prevのstateがFREEではない
 * - node_->nextのstateがFREEではない
 *
 * @retval RANGE_ALLOCATOR_OVERFLOW
 * 次のいずれか。
 * - 前方FREE nodeの終端計算がsize_tの表現可能範囲を超える
 * - node_の終端計算がsize_tの表現可能範囲を超える
 * - 前方FREE nodeとnode_のblock size合計がsize_tの表現可能範囲を超える
 * - 三つのnodeのblock size合計がsize_tの表現可能範囲を超える
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 * - node_、前方FREE node、または後方FREE nodeが局所的不変条件を満たしていない
 * - node_と前後FREE nodeの双方向接続が一致していない
 * - 三つのrangeが連続していない
 * - node_または後方FREE nodeがrange_allocator_のnode poolに所属していない
 * - node_または後方FREE nodeをrange listから切断できない
 * - 切断したnode_または後方FREE nodeをnode poolへreleaseできない
 *
 * @post 成功時は次が成立する。
 * - 前方FREE nodeのoffsetは変更されない
 * - 前方FREE nodeのblock sizeは三つのrangeの合計サイズとなる
 * - 前方FREE nodeはFREE状態のままrange listへ残る
 * - node_と元の後方FREE nodeはrange listから切断され、
 *   NOT_USED状態となる
 * - unused node countが2増加する
 * - range listに隣接したFREE nodeは残らない
 *
 * @post
 * state変更を開始する前に失敗した場合、
 * Range Allocatorおよびnodeの状態は変更されない。
 *
 * @warning
 * node_または後方FREE nodeの切断後に後続処理が失敗した場合、
 * 呼び出し前の状態へrollbackできない。
 * この場合はRANGE_ALLOCATOR_DATA_CORRUPTEDを返す。
 *
 * @par 計算量
 * node_remove()とnode_release()をそれぞれ2回呼び出し、
 * 各関数がnode poolを線形探索するため、
 * 時間計算量はmax node countに対してO(n)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t free_node_merge_prev_next(range_allocator_t* range_allocator_, node_t* node_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    size_t tmp_offset = 0;
    size_t new_block_size = 0;

    node_t* tmp_node_prev = NULL;
    node_t* tmp_node_next = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_node_merge_prev_next", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "free_node_merge_prev_next", "node_")

    if(node_ != node_->next->prev) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=next_free_node_prev_mismatch, allocated_node_offset=%zu, allocated_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, expected_prev=%p, actual_prev=%p", rslt_to_str(ret), node_->offset, node_->block_size, node_->next->offset, node_->next->block_size, (void*)node_, (void*)node_->next->prev);
        goto cleanup;
    }
    if(node_ != node_->prev->next) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=previous_free_node_next_mismatch, allocated_node_offset=%zu, allocated_node_block_size=%zu, previous_node_offset=%zu, previous_node_block_size=%zu, expected_next=%p, actual_next=%p", rslt_to_str(ret), node_->offset, node_->block_size, node_->prev->offset, node_->prev->block_size, (void*)node_, (void*)node_->prev->next);
        goto cleanup;
    }

    if((SIZE_MAX - node_->prev->block_size) < node_->prev->offset) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=previous_free_block_end_overflow, previous_node_offset=%zu, previous_node_block_size=%zu, max_safe_block_size=%zu, allocated_node_offset=%zu", rslt_to_str(ret), node_->prev->offset, node_->prev->block_size, SIZE_MAX - node_->prev->offset, node_->offset);
        goto cleanup;
    }
    tmp_offset = node_->prev->offset + node_->prev->block_size;
    if(tmp_offset != node_->offset) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=previous_free_block_end_mismatch, previous_node_offset=%zu, previous_node_block_size=%zu, previous_block_end=%zu, allocated_node_offset=%zu, allocated_node_block_size=%zu", rslt_to_str(ret), node_->prev->offset, node_->prev->block_size, tmp_offset, node_->offset, node_->block_size);
        goto cleanup;
    }

    if((SIZE_MAX - node_->block_size) < node_->offset) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=allocated_block_end_overflow, allocated_node_offset=%zu, allocated_node_block_size=%zu, max_safe_block_size=%zu, next_node_offset=%zu", rslt_to_str(ret), node_->offset, node_->block_size, SIZE_MAX - node_->offset, node_->next->offset);
        goto cleanup;
    }
    tmp_offset = node_->offset + node_->block_size;
    if(tmp_offset != node_->next->offset) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=allocated_block_end_mismatch, allocated_node_offset=%zu, allocated_node_block_size=%zu, allocated_block_end=%zu, next_node_offset=%zu, next_node_block_size=%zu", rslt_to_str(ret), node_->offset, node_->block_size, tmp_offset, node_->next->offset, node_->next->block_size);
        goto cleanup;
    }

    if((SIZE_MAX - node_->prev->block_size) < node_->block_size) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=previous_and_allocated_block_size_overflow, previous_node_offset=%zu, previous_node_block_size=%zu, allocated_node_offset=%zu, allocated_node_block_size=%zu, max_safe_allocated_block_size=%zu", rslt_to_str(ret), node_->prev->offset, node_->prev->block_size, node_->offset, node_->block_size, SIZE_MAX - node_->prev->block_size);
        goto cleanup;
    }
    new_block_size = node_->prev->block_size + node_->block_size;

    if((SIZE_MAX - node_->next->block_size) < new_block_size) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=total_merged_block_size_overflow, previous_node_offset=%zu, previous_node_block_size=%zu, allocated_node_offset=%zu, allocated_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, partial_merged_block_size=%zu, max_safe_partial_merged_block_size=%zu", rslt_to_str(ret), node_->prev->offset, node_->prev->block_size, node_->offset, node_->block_size, node_->next->offset, node_->next->block_size, new_block_size, SIZE_MAX - node_->next->block_size);
        goto cleanup;
    }
    new_block_size += node_->next->block_size;

    tmp_node_prev = node_->prev;
    tmp_node_next = node_->next;

    ret = node_remove(range_allocator_, node_);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=allocated_node_remove_failed, node_remove_result=%s, allocated_node_offset=%zu, allocated_node_block_size=%zu, previous_node_offset=%zu, previous_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), node_->offset, node_->block_size, tmp_node_prev->offset, tmp_node_prev->block_size, tmp_node_next->offset, tmp_node_next->block_size);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        goto cleanup;
    }
    ret = node_release(range_allocator_, node_);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=detached_node_release_failed, node_release_result=%s, allocated_node_offset=%zu, allocated_node_block_size=%zu, previous_node_offset=%zu, previous_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, unused_node_count=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), node_->offset, node_->block_size, tmp_node_prev->offset, tmp_node_prev->block_size, tmp_node_next->offset, tmp_node_next->block_size, range_allocator_->unused_node_count);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        goto cleanup;
    }

    ret = node_remove(range_allocator_, tmp_node_next);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=next_free_node_remove_failed, node_remove_result=%s, previous_node_offset=%zu, previous_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, merged_block_size=%zu, unused_node_count=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), tmp_node_prev->offset, tmp_node_prev->block_size, tmp_node_next->offset, tmp_node_next->block_size, new_block_size, range_allocator_->unused_node_count);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        goto cleanup;
    }
    ret = node_release(range_allocator_, tmp_node_next);
    if(RANGE_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - Failed to merge allocated node with previous and next free nodes. reason=detached_next_free_node_release_failed, node_release_result=%s, previous_node_offset=%zu, previous_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, merged_block_size=%zu, unused_node_count=%zu", rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), rslt_to_str(ret), tmp_node_prev->offset, tmp_node_prev->block_size, tmp_node_next->offset, tmp_node_next->block_size, new_block_size, range_allocator_->unused_node_count);
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        goto cleanup;
    }

    tmp_node_prev->block_size = new_block_size;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief node poolから未使用nodeを取得してTRANSITIONING状態へ遷移させる
 *
 * @details
 * node poolを先頭から走査し、最初に見つかったNOT_USED nodeへ
 * offset_とblock_size_を設定してTRANSITIONING状態へ遷移させる。
 *
 * 成功時はunused node countを1減らし、取得したnodeをout_node_へ設定する。
 * 取得したnodeはrange listへ接続されておらず、prevとnextはNULLである。
 *
 * 本関数はrange list、allocation count、およびtotal allocated sizeを変更しない。
 * また、動的メモリ確保を行わない。
 *
 * @param[in,out] range_allocator_ node poolを所有するRange Allocator。
 * @param[in] offset_ 取得したnodeへ設定するrange開始offset。
 * @param[in] block_size_ 取得したnodeへ設定するrangeサイズ。
 * @param[in,out] out_node_ 取得したTRANSITIONING nodeの出力先。呼び出し時はNULLを保持していなければならない。
 *
 * @pre
 * range_allocator_に対するshallow validationまたはdeep validationが
 * 呼び出し前に完了していなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * NOT_USED nodeの取得に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_またはout_node_がNULL、もしくは
 * out_node_が呼び出し時にNULLを保持していない。
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * block_size_が0、または指定rangeが管理対象範囲を超えている。
 *
 * @retval RANGE_ALLOCATOR_OVERFLOW
 * offset_とblock_size_の加算がsize_tの表現可能範囲を超える。
 *
 * @retval RANGE_ALLOCATOR_LIMIT_EXCEEDED
 * unused node countが0であり、取得可能なnodeが存在しない。
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * unused node countが0ではないにもかかわらずNOT_USED nodeが見つからない、
 * または見つかったNOT_USED nodeの内部状態が不正である。
 *
 * @post
 * 成功時、out_node_は指定されたrangeを保持するTRANSITIONING nodeを指し、
 * unused node countは呼び出し前から1減少する。
 *
 * @post
 * 失敗時、node pool、range list、unused node count、および
 * out_node_の内容は変更されない。
 *
 * @warning
 * 成功時に返されるnodeはprivate操作途中の一時的な状態である。
 * public APIから戻る前にFREE、ALLOCATED、またはNOT_USEDの
 * 安定状態へ遷移させなければならない。
 *
 * @par 計算量
 * node pool内のnode数をnとしたとき、最悪時間計算量はO(n)である。
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t node_acquire(range_allocator_t* range_allocator_, size_t offset_, size_t block_size_, node_t** out_node_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    bool found = false;
    node_t* tmp_node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_acquire", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_acquire", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_acquire", "*out_node_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != block_size_, ret, RANGE_ALLOCATOR_BAD_OPERATION, rslt_to_str(RANGE_ALLOCATOR_BAD_OPERATION), "node_acquire", "block_size_")

    if((SIZE_MAX - block_size_) < offset_) {
        ret = RANGE_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("node_acquire(%s) - Failed to acquire node from pool. reason=requested_range_end_overflow, range_offset=%zu, range_block_size=%zu, max_safe_block_size=%zu, memory_pool_size=%zu", rslt_to_str(ret), offset_, block_size_, SIZE_MAX - offset_, range_allocator_->memory_pool_size);
        goto cleanup;
    }
    if(range_allocator_->memory_pool_size < (offset_ + block_size_)) {
        ret = RANGE_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("node_acquire(%s) - Failed to acquire node from pool. reason=requested_range_exceeds_memory_pool, range_offset=%zu, range_block_size=%zu, range_end=%zu, memory_pool_size=%zu", rslt_to_str(ret), offset_, block_size_, offset_ + block_size_, range_allocator_->memory_pool_size);
        goto cleanup;
    }

    for(size_t i = 0; i != range_allocator_->max_node_count; ++i) {
        if(NODE_STATE_NOT_USED == range_allocator_->node_pool[i].node_state) {
            tmp_node = &range_allocator_->node_pool[i];
            found = true;
            break;
        }
    }

    if(!found) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_acquire(%s) - Failed to acquire node from pool. reason=unused_node_count_mismatch, range_offset=%zu, range_block_size=%zu, unused_node_count=%zu, max_node_count=%zu, allocation_count=%zu, max_allocation_count=%zu", rslt_to_str(ret), offset_, block_size_, range_allocator_->unused_node_count, range_allocator_->max_node_count, range_allocator_->allocation_count, range_allocator_->max_allocation_count);
        goto cleanup;
    } else {
        if(!node_is_valid(tmp_node)) {
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("node_acquire(%s) - Failed to acquire node from pool. reason=not_used_node_validation_failed, node_index=%zu, node_offset=%zu, node_block_size=%zu, has_prev=%d, has_next=%d, range_offset=%zu, range_block_size=%zu", rslt_to_str(ret), (size_t)(tmp_node - range_allocator_->node_pool), tmp_node->offset, tmp_node->block_size, (int)(NULL != tmp_node->prev), (int)(NULL != tmp_node->next), offset_, block_size_);
            goto cleanup;
        }
    }

    set_node_to_transitioning(tmp_node, offset_, block_size_);

    range_allocator_->unused_node_count--;
    *out_node_ = tmp_node;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief TRANSITIONING nodeをrange listへ接続して安定stateへ遷移させる
 *
 * @details
 * insert_node_をprev_とnext_の間へ接続し、next_state_で指定された
 * FREEまたはALLOCATED状態へ遷移させる。
 *
 * 次の四つの挿入位置に対応する。
 *
 * - prev_とnext_がともにNULL: 空のrange listへ挿入する
 * - prev_がNULL: range listの先頭へ挿入する
 * - prev_とnext_がともに非NULL: 隣接する二つのnode間へ挿入する
 * - next_がNULL: range listの末尾へ挿入する
 *
 * 挿入位置の接続関係を検証した後、必要に応じてrange list headと
 * 隣接nodeを更新し、insert_node_のprev、next、およびstateを設定する。
 *
 * 本関数はinsert_node_のoffsetとblock size、unused node count、
 * allocation count、およびtotal allocated sizeを変更しない。
 *
 * insert_node_のrangeがprev_とnext_の間にaddress orderで位置すること、
 * 隣接rangeと重複しないこと、およびprev_とnext_がnode poolに
 * 所属することは検証しない。これらは呼び出し元の責務とする。
 *
 * @param[in,out] range_allocator_
 * insert_node_を接続するRange Allocator。
 *
 * @param[in,out] insert_node_
 * range listへ接続するTRANSITIONING node。
 *
 * @param[in] next_state_
 * 挿入後のnode state。NODE_STATE_FREEまたは
 * NODE_STATE_ALLOCATEDでなければならない。
 *
 * @param[in,out] prev_
 * insert_node_の直前へ接続するnode。
 * range listの先頭へ挿入する場合はNULL。
 *
 * @param[in,out] next_
 * insert_node_の直後へ接続するnode。
 * range listの末尾へ挿入する場合はNULL。
 *
 * @pre
 * 本関数を含むprivate操作シーケンスの開始前に、
 * range_allocator_に対するshallow validationまたはdeep validationが
 * 完了していなければならない。
 *
 * @pre
 * insert_node_はrange_allocator_のnode poolから取得された
 * TRANSITIONING nodeであり、有効なrange情報を保持していなければならない。
 *
 * @pre
 * prev_とnext_がNULLでない場合、それぞれinsert_node_とは異なる
 * range list上のnodeでなければならない。
 *
 * @pre
 * insert_node_のrangeはaddress order上でprev_とnext_の間に位置し、
 * 隣接rangeと重複してはならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * insert_node_の接続と指定stateへの遷移に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_またはinsert_node_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * insert_node_がTRANSITIONING状態ではない、next_state_が
 * FREE／ALLOCATEDではない、またはprev_とnext_が同じnodeを指している。
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * insert_node_の局所状態が不正、または指定された挿入位置と
 * range list headおよびprev／nextの接続関係が整合していない。
 *
 * @post
 * 成功時、insert_node_はprev_とnext_の間へ接続され、
 * stateはnext_state_と一致する。
 *
 * @post
 * 成功時、prev_とnext_がNULLでなければ、それぞれinsert_node_を
 * 隣接nodeとして指す。prev_がNULLの場合はinsert_node_が
 * range list headとなる。
 *
 * @post
 * 失敗時、range list、range list head、insert_node_、
 * および隣接nodeの状態は変更されない。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t node_insert_between(range_allocator_t* range_allocator_, node_t* insert_node_, node_state_t next_state_, node_t* prev_, node_t* next_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_insert_between", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(insert_node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_insert_between", "insert_node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_TRANSITIONING == insert_node_->node_state, ret, RANGE_ALLOCATOR_BAD_OPERATION, rslt_to_str(RANGE_ALLOCATOR_BAD_OPERATION), "node_insert_between", "insert_node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_ALLOCATED == next_state_ || NODE_STATE_FREE == next_state_, ret, RANGE_ALLOCATOR_BAD_OPERATION, rslt_to_str(RANGE_ALLOCATOR_BAD_OPERATION), "node_insert_between", "next_state_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(insert_node_), ret, RANGE_ALLOCATOR_DATA_CORRUPTED, rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), "node_insert_between", "insert_node_")

    if(NULL != prev_ && NULL != next_ && prev_ == next_) {
        ret = RANGE_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into range list. reason=neighbor_nodes_identical, insert_node_offset=%zu, insert_node_block_size=%zu, next_state=%d, neighbor_node=%p", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size, (int)next_state_, (void*)prev_);
        goto cleanup;
    }

    if(NULL == prev_ && NULL == next_) {
        if(NULL != range_allocator_->range_list_head) {
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into range list. reason=empty_list_insert_position_mismatch, insert_node_offset=%zu, insert_node_block_size=%zu, next_state=%d, head_node_offset=%zu, head_node_block_size=%zu, head_node_state=%d", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size, (int)next_state_, range_allocator_->range_list_head->offset, range_allocator_->range_list_head->block_size, (int)range_allocator_->range_list_head->node_state);
            goto cleanup;
        }
        range_allocator_->range_list_head = insert_node_;
    } else if(NULL == prev_ && NULL != next_) {
        if(next_ != range_allocator_->range_list_head || NULL != range_allocator_->range_list_head->prev) {
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into range list. reason=head_insert_position_invalid, insert_node_offset=%zu, insert_node_block_size=%zu, next_state=%d, head_is_null=%d, next_is_head=%d, head_has_prev=%d, next_node=%p, head_node=%p", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size, (int)next_state_, (int)(NULL == range_allocator_->range_list_head), (int)(next_ == range_allocator_->range_list_head), (int)(NULL != range_allocator_->range_list_head && NULL != range_allocator_->range_list_head->prev), (void*)next_, (void*)range_allocator_->range_list_head);
            goto cleanup;
        }
        next_->prev = insert_node_;
        range_allocator_->range_list_head = insert_node_;
    } else if(NULL != prev_ && NULL != next_) {
        if(prev_->next != next_ || prev_ != next_->prev) {
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into range list. reason=neighbor_link_mismatch, insert_node_offset=%zu, insert_node_block_size=%zu, next_state=%d, previous_node_offset=%zu, previous_node_block_size=%zu, next_node_offset=%zu, next_node_block_size=%zu, previous_points_to_next=%d, next_points_to_previous=%d", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size, (int)next_state_, prev_->offset, prev_->block_size, next_->offset, next_->block_size, (int)(prev_->next == next_), (int)(next_->prev == prev_));
            goto cleanup;
        }
        prev_->next = insert_node_;
        next_->prev = insert_node_;
    } else if(NULL != prev_ && NULL == next_) {
        if(NULL != prev_->next) {
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into range list. reason=tail_insert_position_occupied, insert_node_offset=%zu, insert_node_block_size=%zu, next_state=%d, previous_node_offset=%zu, previous_node_block_size=%zu, previous_node_state=%d, existing_next=%p", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size, (int)next_state_, prev_->offset, prev_->block_size, (int)prev_->node_state, (void*)prev_->next);
            goto cleanup;
        }
        prev_->next = insert_node_;
    }

    if(NODE_STATE_FREE == next_state_) {
        set_node_to_free(insert_node_, prev_, next_);
    } else if(NODE_STATE_ALLOCATED == next_state_) {
        set_node_to_allocated(insert_node_, prev_, next_);
    }

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief FREEまたはALLOCATED nodeをrange listから切断する
 *
 * @details
 * node_がrange_allocator_のnode poolに所属することを確認し、
 * node単体の局所状態、node state、および隣接nodeとの相互接続を検証する。
 *
 * 検証成功後、次の四つの切断位置に応じてrange listを更新する。
 *
 * - node_がrange list上の唯一のnode
 * - node_がrange listの先頭
 * - node_が二つのnodeの間
 * - node_がrange listの末尾
 *
 * 切断時は、必要に応じてrange list headと隣接nodeの接続情報を更新する。
 * その後、node_のoffsetとblock sizeを保持したままprevとnextをNULLにし、
 * stateをTRANSITIONINGへ遷移させる。
 *
 * 本関数はnode_をnode poolへ返却せず、unused node count、
 * allocation count、およびtotal allocated sizeを変更しない。
 *
 * @param[in,out] range_allocator_
 * node_を切断するRange Allocator。
 *
 * @param[in,out] node_
 * range listから切断するFREEまたはALLOCATED node。
 *
 * @pre
 * 本関数を含むprivate操作シーケンスの開始前に、
 * range_allocator_に対するshallow validationまたはdeep validationが
 * 完了していなければならない。
 *
 * @pre
 * node_はrange_allocator_のnode poolに所属し、
 * range listへ接続されていなければならない。
 *
 * @pre
 * node_のstateはNODE_STATE_FREEまたは
 * NODE_STATE_ALLOCATEDでなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * node_の切断とTRANSITIONING状態への遷移に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_またはnode_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 *
 * - node_がrange_allocator_のnode poolに所属していない
 * - node_の局所状態が不正である
 * - node_のstateがFREEまたはALLOCATEDではない
 * - node_と前方または後方nodeとの相互接続が整合していない
 * - node_がrange listの先頭位置を表しているにもかかわらず、range list headと一致しない
 *
 * @post
 * 成功時、node_はoffsetとblock sizeを保持した
 * TRANSITIONING nodeとなり、prevとnextはNULLとなる。
 *
 * @post
 * 成功時、node_はrange listから切断され、range list headと
 * 隣接nodeの接続情報は切断後の状態へ更新される。
 *
 * @post
 * 成功時もunused node countは変更されない。
 *
 * @post
 * 失敗時、range list、range list head、node_、
 * および隣接nodeの状態は変更されない。
 *
 * @warning
 * 成功時のnode_はprivate操作途中の一時的な状態である。
 * public APIから戻る前にrange listへ再接続するか、
 * node poolへ返却して安定状態へ遷移させなければならない。
 *
 * @par 計算量
 * node_のnode pool所属確認で線形探索を行うため、
 * node pool内のnode数をnとしたとき、時間計算量はO(n)である。
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t node_remove(range_allocator_t* range_allocator_, node_t* node_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    bool found = false;
    size_t index = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_remove", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_remove", "node_")

    for(size_t i = 0; i != range_allocator_->max_node_count; ++i) {
        if(&range_allocator_->node_pool[i] == node_) {
            found = true;
            index = i;
            break;
        }
    }

    if(!found) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_remove(%s) - Failed to remove node from range list. reason=node_not_in_pool, target_node=%p, node_pool=%p, max_node_count=%zu", rslt_to_str(ret), (void*)node_, (void*)range_allocator_->node_pool, range_allocator_->max_node_count);
        goto cleanup;
    }
    if(!node_is_valid(node_)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_remove(%s) - Failed to remove node from range list. reason=node_validation_failed, node_index=%zu, node_state=%d, node_offset=%zu, node_block_size=%zu, has_prev=%d, has_next=%d, prev_is_self=%d, next_is_self=%d", rslt_to_str(ret), index, (int)node_->node_state, node_->offset, node_->block_size, (int)(NULL != node_->prev), (int)(NULL != node_->next), (int)(node_->prev == node_), (int)(node_->next == node_));
        goto cleanup;
    }
    if(NODE_STATE_FREE != node_->node_state && NODE_STATE_ALLOCATED != node_->node_state) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_remove(%s) - Failed to remove node from range list. reason=node_state_invalid_for_removal, node_index=%zu, required_state=FREE_OR_ALLOCATED, actual_state=%d, node_offset=%zu, node_block_size=%zu, has_prev=%d, has_next=%d", rslt_to_str(ret), index, (int)node_->node_state, node_->offset, node_->block_size, (int)(NULL != node_->prev), (int)(NULL != node_->next));
        goto cleanup;
    }
    if(NULL != node_->prev && node_->prev->next != node_) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_remove(%s) - Failed to remove node from range list. reason=previous_node_next_mismatch, node_index=%zu, node_offset=%zu, node_block_size=%zu, node_state=%d, previous_node_offset=%zu, previous_node_block_size=%zu, previous_node_state=%d, expected_next=%p, actual_next=%p", rslt_to_str(ret), index, node_->offset, node_->block_size, (int)node_->node_state, node_->prev->offset, node_->prev->block_size, (int)node_->prev->node_state, (void*)node_, (void*)node_->prev->next);
        goto cleanup;
    }
    if(NULL != node_->next && node_->next->prev != node_) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_remove(%s) - Failed to remove node from range list. reason=next_node_prev_mismatch, node_index=%zu, node_offset=%zu, node_block_size=%zu, node_state=%d, next_node_offset=%zu, next_node_block_size=%zu, next_node_state=%d, expected_prev=%p, actual_prev=%p", rslt_to_str(ret), index, node_->offset, node_->block_size, (int)node_->node_state, node_->next->offset, node_->next->block_size, (int)node_->next->node_state, (void*)node_, (void*)node_->next->prev);
        goto cleanup;
    }

    if(NULL == node_->prev && NULL == node_->next) {  // node_が唯一のノード
        if(range_allocator_->range_list_head != node_) {
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("node_remove(%s) - Failed to remove node from range list. reason=isolated_node_not_list_head, node_index=%zu, node_offset=%zu, node_block_size=%zu, node_state=%d, target_node=%p, range_list_head=%p", rslt_to_str(ret), index, node_->offset, node_->block_size, (int)node_->node_state, (void*)node_, (void*)range_allocator_->range_list_head);
            goto cleanup;
        }
        range_allocator_->range_list_head = NULL;
    } else if(NULL == node_->prev && NULL != node_->next) {   // node_が先頭で、node_の次に別のノードがある
        if(range_allocator_->range_list_head != node_) {
            ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
            ERROR_MESSAGE("node_remove(%s) - Failed to remove node from range list. reason=first_node_not_list_head, node_index=%zu, node_offset=%zu, node_block_size=%zu, node_state=%d, target_node=%p, range_list_head=%p, next_node=%p", rslt_to_str(ret), index, node_->offset, node_->block_size, (int)node_->node_state, (void*)node_, (void*)range_allocator_->range_list_head, (void*)node_->next);
            goto cleanup;
        }
        range_allocator_->range_list_head = node_->next;

        node_->next->prev = NULL;
    } else if(NULL != node_->prev && NULL != node_->next) {   // node_の前後に別のノードがある
        node_->prev->next = node_->next;
        node_->next->prev = node_->prev;
    } else if(NULL != node_->prev && NULL == node_->next) {   // node_の前にノードが存在し、かつ、node_が末尾ノード
        node_->prev->next = NULL;
    }

    // block_size, offsetは保持する
    set_node_to_transitioning(node_, node_->offset, node_->block_size);

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief TRANSITIONING nodeをnode poolへ返却する
 *
 * @details
 * node_がrange_allocator_のnode poolに所属することを確認し、
 * node単体の局所状態とTRANSITIONING状態であることを検証する。
 *
 * 検証成功後、node_を次のNOT_USED状態へ正規化する。
 *
 * - offsetは0
 * - block sizeは0
 * - prevとnextはNULL
 * - stateはNODE_STATE_NOT_USED
 *
 * その後、unused node countを1増加させる。
 *
 * 本関数はrange list、range list head、allocation count、
 * およびtotal allocated sizeを変更しない。
 * また、動的に確保されたメモリを解放する処理ではない。
 *
 * @param[in,out] range_allocator_
 * node_を所有するRange Allocator。
 *
 * @param[in,out] node_
 * node poolへ返却するTRANSITIONING node。
 *
 * @pre
 * 本関数を含むprivate操作シーケンスの開始前に、
 * range_allocator_に対するshallow validationまたはdeep validationが
 * 完了していなければならない。
 *
 * @pre
 * node_はrange_allocator_のnode poolに所属する
 * TRANSITIONING nodeでなければならない。
 *
 * @pre
 * node_はrange listから切断され、prevとnextがNULLでなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * node_のnode poolへの返却に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_またはnode_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 *
 * - unused node countがmax node count以上である
 * - node_がrange_allocator_のnode poolに所属していない
 * - node_の局所状態が不正である
 * - node_のstateがTRANSITIONINGではない
 *
 * @post
 * 成功時、node_はoffsetとblock sizeが0、prevとnextがNULLの
 * NOT_USED nodeとなる。
 *
 * @post
 * 成功時、unused node countは呼び出し前から1増加する。
 *
 * @post
 * 失敗時、node_、node pool、range list、および
 * unused node countは変更されない。
 *
 * @par 計算量
 * node_のnode pool所属確認で線形探索を行うため、
 * node pool内のnode数をnとしたとき、時間計算量はO(n)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t node_release(range_allocator_t* range_allocator_, node_t* node_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    bool found = false;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_release", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_release", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(range_allocator_->unused_node_count < range_allocator_->max_node_count, ret, RANGE_ALLOCATOR_DATA_CORRUPTED, rslt_to_str(RANGE_ALLOCATOR_DATA_CORRUPTED), "node_release", "unused_node_count")

    for(size_t i = 0; i != range_allocator_->max_node_count; ++i) {
        if(&range_allocator_->node_pool[i] == node_) {
            found = true;
            break;
        }
    }

    if(!found) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_release(%s) - Failed to release node to pool. reason=node_not_in_pool, target_node=%p, node_pool=%p, max_node_count=%zu, unused_node_count=%zu", rslt_to_str(ret), (void*)node_, (void*)range_allocator_->node_pool, range_allocator_->max_node_count, range_allocator_->unused_node_count);
        goto cleanup;
    }
    if(!node_is_valid(node_)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_release(%s) - Failed to release node to pool. reason=node_validation_failed, node_index=%zu, node_state=%d, node_offset=%zu, node_block_size=%zu, has_prev=%d, has_next=%d, prev_is_self=%d, next_is_self=%d", rslt_to_str(ret), (size_t)(node_ - range_allocator_->node_pool), (int)node_->node_state, node_->offset, node_->block_size, (int)(NULL != node_->prev), (int)(NULL != node_->next), (int)(node_->prev == node_), (int)(node_->next == node_));
        goto cleanup;
    }
    if(NODE_STATE_TRANSITIONING != node_->node_state) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_release(%s) - Failed to release node to pool. reason=node_state_invalid_for_release, node_index=%zu, required_state=TRANSITIONING, actual_state=%d, node_offset=%zu, node_block_size=%zu, has_prev=%d, has_next=%d, unused_node_count=%zu", rslt_to_str(ret), (size_t)(node_ - range_allocator_->node_pool), (int)node_->node_state, node_->offset, node_->block_size, (int)(NULL != node_->prev), (int)(NULL != node_->next), range_allocator_->unused_node_count);
        goto cleanup;
    }

    set_node_to_not_used(node_);

    range_allocator_->unused_node_count++;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief node pool内におけるnodeのindexを取得する
 *
 * @details
 * range_allocator_が所有するnode poolを先頭から線形探索し、
 * node_と同じpointer identityを持つ要素のindexを取得する。
 *
 * node_のoffset、block size、またはstateによる検索ではなく、
 * node pool要素のaddressとの一致によって所属とindexを判定する。
 *
 * node poolへの所属確認後、node_is_valid()によって
 * node単体の局所状態を検証する。
 *
 * 本関数はnode_がrange listへ接続されていることや、
 * range list上の接続関係を検証しない。
 * また、Range Allocator、node pool、およびnodeの状態を変更しない。
 *
 * @param[in] range_allocator_
 * node poolを所有するRange Allocator。
 *
 * @param[in] node_
 * indexを取得するnode。
 *
 * @param[out] out_index_
 * node pool内におけるnode_のindexの格納先。
 * 成功時のみ値が設定され、失敗時は変更されない。
 *
 * @pre
 * range_allocator_に対するshallow validationまたはdeep validationが
 * 呼び出し前に完了していなければならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * node_のindex取得に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_、node_、またはout_index_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * node_がrange_allocator_のnode poolに所属していない、
 * またはnode_の局所状態が不正である。
 *
 * @post
 * 成功時、out_index_には0以上max node count未満のindexが格納される。
 *
 * @post
 * 失敗時、out_index_の内容は変更されない。
 *
 * @par 計算量
 * node poolを線形探索するため、node pool内のnode数をnとしたとき、
 * 最悪時間計算量はO(n)である。
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t node_pool_find_index(const range_allocator_t* range_allocator_, const node_t* node_, size_t* out_index_) {
    range_allocator_result_t ret = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    bool found = false;
    size_t tmp_index = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_allocator_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_pool_find_index", "range_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_pool_find_index", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_index_, ret, RANGE_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(RANGE_ALLOCATOR_INVALID_ARGUMENT), "node_pool_find_index", "out_index_")

    for(size_t i = 0; i != range_allocator_->max_node_count; ++i) {
        if(&range_allocator_->node_pool[i] == node_) {
            tmp_index = i;
            found = true;
            break;
        }
    }
    if(!found) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_pool_find_index(%s) - Failed to find node index. reason=node_not_in_pool, target_node=%p, node_pool=%p, max_node_count=%zu", rslt_to_str(ret), (void*)node_, (void*)range_allocator_->node_pool, range_allocator_->max_node_count);
        goto cleanup;
    }
    if(!node_is_valid(node_)) {
        ret = RANGE_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("node_pool_find_index(%s) - Failed to find node index. reason=node_validation_failed, node_index=%zu, node_state=%d, node_offset=%zu, node_block_size=%zu, has_prev=%d, has_next=%d, prev_is_self=%d, next_is_self=%d", rslt_to_str(ret), tmp_index, (int)node_->node_state, node_->offset, node_->block_size, (int)(NULL != node_->prev), (int)(NULL != node_->next), (int)(node_->prev == node_), (int)(node_->next == node_));
        goto cleanup;
    }
    *out_index_ = tmp_index;

    ret = RANGE_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief nodeをNOT_USED状態へ正規化する
 *
 * @details
 * target_がNULLでない場合、すべてのフィールドを
 * NODE_STATE_NOT_USEDの局所的不変条件に一致する値へ設定する。
 *
 * - offsetは0
 * - block sizeは0
 * - prevとnextはNULL
 * - stateはNODE_STATE_NOT_USED
 *
 * 遷移元のstateやrange listへの接続状態は検証しない。
 * また、range list、range list head、およびunused node countを更新しない。
 *
 * create時のnode pool初期化と、node_release()による
 * TRANSITIONING nodeの返却に使用する。
 *
 * @param[in,out] target_
 * NOT_USED状態へ設定するnode。NULLの場合は何も行わない。
 *
 * @pre
 * target_がrange listへ接続されている場合、呼び出し元によって
 * range listから切断済みでなければならない。
 *
 * @post
 * target_がNULLでない場合、target_はoffsetとblock sizeが0、
 * prevとnextがNULLのNOT_USED nodeとなる。
 *
 * @note
 * node poolのunused node countは呼び出し元が更新する。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static void set_node_to_not_used(node_t* target_) {
    if(NULL == target_) {
        return;
    }
    target_->block_size = 0;
    target_->offset = 0;
    target_->prev = NULL;
    target_->next = NULL;
    target_->node_state = NODE_STATE_NOT_USED;
}

/**
 * @brief nodeへrange情報を設定してTRANSITIONING状態へ遷移させる
 *
 * @details
 * target_がNULLでない場合、指定されたoffsetとblock sizeを設定し、
 * prevとnextをNULLにしてNODE_STATE_TRANSITIONINGへ遷移させる。
 *
 * 次の二つの処理で使用する。
 *
 * - NOT_USED nodeの取得時(range情報は新しいrange情報を設定する)
 * - FREEまたはALLOCATED nodeの切断時(既存range情報は保持)
 *
 * 遷移元のstate、指定rangeの有効性、およびtarget_がrange listから
 * 切断されていることは検証しない。
 *
 * また、range list、range list head、unused node count、
 * allocation count、およびtotal allocated sizeを更新しない。
 *
 * @param[in,out] target_
 * TRANSITIONING状態へ設定するnode。NULLの場合は何も行わない。
 *
 * @param[in] offset_
 * target_へ設定するrange開始offset。
 *
 * @param[in] block_size_
 * target_へ設定するrangeサイズ。0以外でなければならない。
 *
 * @pre
 * target_がNULLでない場合、target_はrange listから
 * 切断されていなければならない。
 *
 * @pre
 * target_がNULLでない場合、offset_とblock_size_は
 * target_を所有するRange Allocator内の有効なrangeを
 * 表していなければならない。
 *
 * @post
 * target_がNULLでない場合、target_は指定されたoffsetとblock sizeを保持する
 * TRANSITIONING nodeとなり、prevとnextはNULLとなる。
 *
 * @warning
 * TRANSITIONINGはprivate操作途中だけで使用する一時的なstateである。
 * public APIから戻る前にFREE、ALLOCATED、またはNOT_USEDの
 * 安定状態へ遷移させなければならない。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static void set_node_to_transitioning(node_t* target_, size_t offset_, size_t block_size_) {
    if(NULL == target_) {
        return;
    }
    target_->block_size = block_size_;
    target_->offset = offset_;
    target_->next = NULL;
    target_->prev = NULL;
    target_->node_state = NODE_STATE_TRANSITIONING;
}

/**
 * @brief nodeをFREE状態へ設定する
 *
 * @details
 * target_がNULLでない場合、指定されたprevとnextを設定し、
 * nodeをNODE_STATE_FREEへ遷移させる。
 *
 * target_が保持するoffsetとblock sizeは変更しない。
 *
 * create時にmemory pool全体を表す初期FREE nodeを設定する処理と、
 * TRANSITIONING nodeをrange listへ挿入してFREE状態へ
 * 遷移させる処理で使用する。
 *
 * 遷移元のstate、target_が保持するrange情報、prev_とnext_の
 * 局所状態、およびrange listの接続関係は検証しない。
 *
 * また、隣接node、range list head、unused node count、
 * allocation count、およびtotal allocated sizeを更新しない。
 *
 * @param[in,out] target_
 * FREE状態へ設定するnode。NULLの場合は何も行わない。
 *
 * @param[in] prev_
 * target_の直前へ接続するnode。
 * range listの先頭となる場合はNULL。
 *
 * @param[in] next_
 * target_の直後へ接続するnode。
 * range listの末尾となる場合はNULL。
 *
 * @pre
 * target_がNULLでない場合、target_のoffsetとblock sizeは
 * 有効なFREE rangeを表していなければならない。
 *
 * @pre
 * 呼び出し元は、prev_、next_、range list head、および隣接nodeを含む
 * range list全体が、target_の設定後に整合することを
 * 保証しなければならない。
 *
 * @post
 * target_がNULLでない場合、target_はoffsetとblock sizeを保持したまま
 * FREE状態となり、prevとnextは指定されたnodeと一致する。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static void set_node_to_free(node_t* target_, node_t* prev_, node_t* next_) {
    if(NULL == target_) {
        return;
    }
    target_->prev = prev_;
    target_->next = next_;
    target_->node_state = NODE_STATE_FREE;
}

/**
 * @brief nodeをALLOCATED状態へ設定する
 *
 * @details
 * target_がNULLでない場合、指定されたprevとnextを設定し、
 * nodeをNODE_STATE_ALLOCATEDへ遷移させる。
 *
 * target_が保持するoffsetとblock sizeは変更しない。
 *
 * 遷移元のstate、target_が保持するrange情報、prev_とnext_の
 * 局所状態、およびrange listの接続関係は検証しない。
 *
 * また、隣接node、range list head、unused node count、
 * allocation count、およびtotal allocated sizeを更新しない。
 *
 * @param[in,out] target_
 * ALLOCATED状態へ設定するnode。NULLの場合は何も行わない。
 *
 * @param[in] prev_
 * target_の直前へ接続するnode。
 * range listの先頭となる場合はNULL。
 *
 * @param[in] next_
 * target_の直後へ接続するnode。
 * range listの末尾となる場合はNULL。
 *
 * @pre
 * target_がNULLでない場合、target_のoffsetとblock sizeは
 * 有効なALLOCATED rangeを表していなければならない。
 *
 * @pre
 * 呼び出し元は、prev_、next_、range list head、および隣接nodeを含む
 * range list全体が、target_の設定後に整合することを
 * 保証しなければならない。
 *
 * @post
 * target_がNULLでない場合、target_はoffsetとblock sizeを保持したまま
 * ALLOCATED状態となり、prevとnextは指定されたnodeと一致する。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static void set_node_to_allocated(node_t* target_, node_t* prev_, node_t* next_) {
    if(NULL == target_) {
        return;
    }
    target_->prev = prev_;
    target_->next = next_;
    target_->node_state = NODE_STATE_ALLOCATED;
}

/**
 * @brief Range Allocator全体の内部整合性を検証する
 *
 * @details
 * range_allocator_is_valid_shallow()によって基本管理値を検証した後、
 * range listとnode poolを走査し、両者の内容と管理値が一致することを検証する。
 *
 * range listについて、次の条件を検証する。
 *
 * - range list headがNULLではない
 * - list走査node数がmax node countを超えない
 * - list上の各nodeがnode poolに所属する
 * - list上の各nodeが局所的不変条件を満たす
 * - list上の各nodeがFREEまたはALLOCATED状態である
 * - 各nodeのprevが直前に走査したnodeと一致する
 * - 先頭nodeのprevがNULLである
 * - 各nodeのoffsetが直前rangeの終端と一致する
 * - 各nodeのoffsetがbase alignment境界にある
 * - range終端の計算がオーバーフローしない
 * - 各rangeがmemory pool内に収まる
 * - ALLOCATED nodeのblock sizeがbase alignmentの倍数である
 * - 隣接する二つのnodeがともにFREEではない
 * - range listの末尾がmemory poolの末尾と一致する
 * - ALLOCATED nodeのblock size合計がtotal allocated sizeと一致する
 *
 * expected offsetを0から各nodeの終端へ更新することで、
 * range間のgap、overlap、およびmemory pool内の未管理rangeを検出する。
 *
 * node poolについて、次の条件を検証する。
 *
 * - 全nodeがstateに対応する局所的不変条件を満たす
 * - public API境界で許可されないTRANSITIONING nodeが存在しない
 * - ALLOCATED node数がallocation countと一致する
 * - NOT_USED node数がunused node countと一致する
 * - FREE／ALLOCATED node数がrange listの走査node数と一致する
 * - FREE、ALLOCATED、NOT_USEDの各node数の合計がmax node countと一致する
 *
 * range list上の全nodeがnode poolに所属することと、
 * node pool内のFREE／ALLOCATED node数がrange listの走査node数と
 * 一致することを照合し、range listから切断された安定状態のnodeが
 * 存在しないことを検証する。
 *
 * @param[in] range_allocator_
 * 検証するRange Allocator。NULLの場合は不正と判定する。
 *
 * @retval true
 * 基本管理値、node pool、range list、range情報、および各管理値が
 * 相互に整合している。
 *
 * @retval false
 * range_allocator_がNULL、または内部フィールド、node pool、
 * node state、range list接続、range、alignment、node数、
 * allocation数、もしくはtotal allocated sizeに不整合がある。
 *
 * @note
 * 本関数は不整合の種類を結果コードとして区別しない。
 * public APIはfalseを内部データ破損として扱う。
 *
 * @note
 * 本関数はRange Allocator、node pool、range list、および
 * 各管理値を変更しない。
 *
 * @par 計算量
 * range list上の各nodeについてnode poolへの所属を線形探索するため、
 * node pool内のnode数をnとしたとき、最悪時間計算量はO(n^2)である。
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static bool range_allocator_is_valid(const range_allocator_t* range_allocator_) {
    node_t* node = NULL;
    node_t* prev = NULL;
    size_t loop_count = 0;
    size_t expected_offset = 0;
    size_t expected_total_allocated_size = 0;
    bool found = false;

    if(NULL == range_allocator_) {
        return false;
    }
    if(!range_allocator_is_valid_shallow(range_allocator_)) {
        return false;
    }

    if(NULL == range_allocator_->range_list_head) {
        return false;
    }
    node = range_allocator_->range_list_head;
    // NOTE: 以下はrange_allocatorの動作実績が増えたらDEBUG_BUILD, TEST_BUILDのみで動かす
    while(NULL != node) {
        if(loop_count >= range_allocator_->max_node_count) {
            return false;
        }
        found = false;
        for(size_t i = 0; i != range_allocator_->max_node_count; ++i) {
            if(&range_allocator_->node_pool[i] == node) {
                found = true;
                break;
            }
        }
        if(!found) {
            return false;
        }
        if(!node_is_valid(node)) {
            return false;
        }
        if(NODE_STATE_ALLOCATED != node->node_state && NODE_STATE_FREE != node->node_state) {
            return false;
        }
        if(node->prev != prev) {
            return false;
        }
        if(node->offset != expected_offset) {
            return false;
        }
        if(0 != (node->offset % range_allocator_->base_align)) {
            return false;
        }
        if((SIZE_MAX - node->block_size) < node->offset) {
            return false;
        }
        if(range_allocator_->memory_pool_size < (node->offset + node->block_size)) {
            return false;
        }
        if(NODE_STATE_ALLOCATED == node->node_state && 0 != (node->block_size % range_allocator_->base_align)) {
            return false;
        }
        if(NULL != node->prev) {
            if(NODE_STATE_FREE == node->node_state && NODE_STATE_FREE == node->prev->node_state) {
                return false;
            }
        }
        if(NODE_STATE_ALLOCATED == node->node_state) {
            if((SIZE_MAX - node->block_size) < expected_total_allocated_size) {
                return false;
            }
            expected_total_allocated_size += node->block_size;
        }
        expected_offset = node->offset + node->block_size;  // OVERFLOWチェックは直前で完了している
        prev = node;
        node = node->next;
        loop_count++;
        found = false;
    }
    if(expected_offset != range_allocator_->memory_pool_size) { // memory_pool全体がALLOCATED / FREEノードで隙間がないため、末尾は必ずmemory_pool_sizeに等しい
        return false;
    }
    if(expected_total_allocated_size != range_allocator_->total_allocated_size) {
        return false;
    }

    size_t allocated_count = 0;
    size_t free_count = 0;
    size_t unused_count = 0;
    for(size_t i = 0; i != range_allocator_->max_node_count; ++i) {
        if(!node_is_valid(&range_allocator_->node_pool[i])) {
            return false;
        }
        if(NODE_STATE_TRANSITIONING == range_allocator_->node_pool[i].node_state) {
            return false;
        }
        if(NODE_STATE_ALLOCATED == range_allocator_->node_pool[i].node_state) {
            allocated_count++;
        }
        if(NODE_STATE_FREE == range_allocator_->node_pool[i].node_state) {
            free_count++;
        }
        if(NODE_STATE_NOT_USED == range_allocator_->node_pool[i].node_state) {
            unused_count++;
        }
    }
    if(allocated_count != range_allocator_->allocation_count) {
        return false;
    }
    if(range_allocator_->unused_node_count != unused_count) {
        return false;
    }
    if((free_count + allocated_count) != loop_count) {
        return false;
    }
    if((free_count + allocated_count + unused_count) != range_allocator_->max_node_count) {
        return false;
    }
    return true;
}

/**
 * @brief Range Allocatorの基本管理値をshallowに検証する
 *
 * @details
 * range_allocator_が保持する基本フィールドについて、
 * node poolおよびrange listを走査せず、O(1)で検証可能な次の条件を確認する。
 *
 * - base alignmentが0以外の2の冪乗である
 * - max allocation countが0ではない
 * - total allocated sizeがmemory pool size以下である
 * - max allocation countからmax node countを安全に導出できる
 * - max node countがmax allocation countの2倍+1と一致する
 * - allocation countがmax allocation count以下である
 * - unused node countがmax node count以下である
 * - memory pool sizeが0ではない
 * - node poolがNULLではない
 *
 * max allocation countについて、次の式がsize_tの表現可能範囲を
 * 超えないことを確認してからmax node countとの一致を検証する。
 *
 * @code{.c}
 * max_node_count = max_allocation_count * 2 + 1;
 * @endcode
 *
 * 本関数はrange list head、node pool内の各node、range listの接続関係、
 * range情報、各rangeのoffsetおよびALLOCATED rangeのblock sizeが
 * base alignmentに適合すること、ならびに管理値と実node数の一致を検証しない。
 * これらの検証はrange_allocator_is_valid()が行う。
 *
 * @param[in] range_allocator_
 * 検証するRange Allocator。NULLの場合は不正と判定する。
 *
 * @retval true
 * O(1)で検証可能な基本管理値が整合している。
 *
 * @retval false
 * range_allocator_がNULL、または基本フィールドのいずれかが
 * shallowな不変条件を満たしていない。
 *
 * @note
 * trueはRange Allocator全体の整合性を保証しない。
 * node poolとrange listを含む完全な検証には
 * range_allocator_is_valid()を使用する。
 *
 * @note
 * 本関数はRange Allocatorおよびnode poolの状態を変更しない。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数はnode poolおよびrange listを走査せず、
 * 動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static bool range_allocator_is_valid_shallow(const range_allocator_t* range_allocator_) {
    if(NULL == range_allocator_) {
        return false;
    }
    if(0 == range_allocator_->base_align || !IS_POWER_OF_TWO(range_allocator_->base_align)) {
        return false;
    }
    if(0 == range_allocator_->max_allocation_count) {
        return false;
    }
    if(range_allocator_->total_allocated_size > range_allocator_->memory_pool_size) {
        return false;
    }
    if(((SIZE_MAX - 1) / 2) < range_allocator_->max_allocation_count) {
        return false;
    }
    if((range_allocator_->max_allocation_count * 2 + 1) != range_allocator_->max_node_count) {
        return false;
    }
    if(range_allocator_->allocation_count > range_allocator_->max_allocation_count) {
        return false;
    }
    if(range_allocator_->unused_node_count > range_allocator_->max_node_count) {
        return false;
    }
    if(0 == range_allocator_->memory_pool_size) {
        return false;
    }
    if(NULL == range_allocator_->node_pool) {
        return false;
    }
    return true;
}

/**
 * @brief node単体のstateと局所フィールドの整合性を検証する
 *
 * @details
 * node_のnode stateに応じて、node単体で判断可能な次の局所的不変条件を検証する。
 *
 * NODE_STATE_FREEまたはNODE_STATE_ALLOCATEDでは、
 * 次の条件を確認する。
 *
 * - block sizeが0ではない
 * - prevがnode_自身を指していない
 * - nextがnode_自身を指していない
 *
 * NODE_STATE_TRANSITIONINGでは、次の条件を確認する。
 *
 * - block sizeが0ではない
 * - prevとnextがともにNULLである
 *
 * NODE_STATE_NOT_USEDでは、次の条件を確認する。
 *
 * - offsetとblock sizeがともに0である
 * - prevとnextがともにNULLである
 *
 * node stateが定義済みのいずれの状態にも該当しない場合は、
 * 不正なnodeと判定する。
 *
 * 本関数はnode単体の局所的な整合性だけを検証する。
 * 次の条件は検証しない。
 *
 * - node poolへの所属
 * - range listへの接続状態
 * - 隣接nodeとの相互接続
 * - rangeの順序、連続性、および重複
 * - memory pool内におけるrangeの有効性
 * - offsetおよびblock sizeのalignment
 * - node数および各管理値との一致
 *
 * TRANSITIONINGは本関数では有効な局所状態として扱うが、
 * public APIの入口および正常終了時に許可される安定状態ではない。
 *
 * @param[in] node_
 * 検証するnode。NULLの場合は不正と判定する。
 *
 * @retval true
 * node_のstateと局所フィールドが整合している。
 *
 * @retval false
 * node_がNULL、node stateが未定義、またはstateに対応する
 * 局所フィールドの条件を満たしていない。
 *
 * @note
 * 本関数はnode_および関連するnodeの状態を変更しない。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static bool node_is_valid(const node_t* node_) {
    if(NULL == node_) {
        return false;
    }
    if(NODE_STATE_ALLOCATED == node_->node_state || NODE_STATE_FREE == node_->node_state) {
        if(0 == node_->block_size) {
            return false;
        }
        if(NULL != node_->prev && node_->prev == node_) {   // prevが自身と同じ
            return false;
        }
        if(NULL != node_->next && node_->next == node_) {   // nextが自身と同じ
            return false;
        }
    } else if(NODE_STATE_TRANSITIONING == node_->node_state) {
        if(0 == node_->block_size) {
            return false;
        }
        if(NULL != node_->prev || NULL != node_->next) {
            return false;
        }
    } else if(NODE_STATE_NOT_USED == node_->node_state) {
        if(NULL != node_->prev || NULL != node_->next) {
            return false;
        }
        if(0 != node_->offset || 0 != node_->block_size) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

/**
 * @brief Range Allocator statusの各フィールドをstdoutへ出力する
 *
 * @details
 * status_がNULLでない場合、memory usageと
 * range_allocator_status_tが保持する各フィールドをstdoutへ出力する。
 *
 * memory pool sizeが0ではない場合、次の式でmemory usageを計算し、
 * 小数点以下2桁の百分率として出力する。
 *
 * @code{.c}
 * memory_usage_percent =
 *     total_allocated_size / memory_pool_size * 100.0;
 * @endcode
 *
 * memory pool sizeが0の場合は除算を行わず、
 * memory usageを計算できないことを示す文字列を出力する。
 *
 * status_がNULLの場合は、statusが指定されていないことを示す
 * メッセージだけを出力する。
 *
 * 本関数はstatus表示の本文だけを担当する。
 * 表示タイトル、ANSI colorの設定と解除、およびstdoutのlockは
 * 呼び出し元が担当する。
 *
 * status_が保持する各値の整合性は検証せず、
 * range listおよびnode poolを走査しない。
 * また、stdoutへの書き込み失敗は呼び出し元へ通知しない。
 *
 * @param[in] status_
 * 出力するRange Allocator status。
 * NULLの場合はNULLであることを示すメッセージを出力する。
 *
 * @pre
 * 複数行の出力を同一ストリームへ連続して書き込む必要がある場合、
 * 呼び出し元がstdoutをlockしていなければならない。
 *
 * @post
 * status_の内容は変更されない。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数はrange listおよびnode poolを走査せず、
 * 動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static void status_print(const range_allocator_status_t* status_) {
    if(NULL == status_) {
        fprintf(stdout, "  Provided range_allocator_status_t instance is null.\n");
    } else {
        if(0 != status_->memory_pool_size) {
            fprintf(stdout, "  memory_usage_percent = %.2lf%%\n", (double)status_->total_allocated_size / (double)status_->memory_pool_size * 100.0);
        } else {
            fprintf(stdout, "  memory_usage_percent = memory_pool is zero.\n");
        }
        fprintf(stdout, "  memory_pool_size = %zu\n", status_->memory_pool_size);
        fprintf(stdout, "  base_align = %zu\n", status_->base_align);
        fprintf(stdout, "  max_node_count = %zu\n", status_->max_node_count);
        fprintf(stdout, "  max_allocation_count = %zu\n", status_->max_allocation_count);
        fprintf(stdout, "  total_allocated_size = %zu\n", status_->total_allocated_size);
        fprintf(stdout, "  total_free_size = %zu\n", status_->total_free_size);
        fprintf(stdout, "  unused_node_count = %zu\n", status_->unused_node_count);
        fprintf(stdout, "  free_block_count = %zu\n", status_->free_block_count);
        fprintf(stdout, "  used_node_count = %zu\n", status_->used_node_count);
        fprintf(stdout, "  allocation_count = %zu\n", status_->allocation_count);
    }
}

/**
 * @brief Range Allocatorの結果コードを文字列へ変換する
 *
 * @details
 * rslt_に対応する静的文字列定数を返す。
 *
 * 定義済みの各range_allocator_result_tについて、
 * 結果コード名を表す文字列を返す。
 *
 * RANGE_ALLOCATOR_UNDEFINED_ERRORまたは定義されていない値が
 * 指定された場合は、UNDEFINED_ERRORを表す文字列を返す。
 *
 * 返される文字列は静的記憶域期間を持つ。
 * 呼び出し元へ所有権は移動せず、解放または変更してはならない。
 *
 * @param[in] rslt_
 * 文字列へ変換するRange Allocatorの結果コード。
 *
 * @return
 * rslt_に対応するNULLではない静的文字列。
 * 定義されていない値の場合はUNDEFINED_ERRORを表す文字列。
 *
 * @note
 * 本関数はRange Allocatorおよび結果コードを変更しない。
 *
 * @note
 * 返される文字列は読み取り専用であり、複数の呼び出し間で共有される。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static const char* rslt_to_str(range_allocator_result_t rslt_) {
    switch(rslt_) {
    case RANGE_ALLOCATOR_SUCCESS:
        return s_rslt_str_success;
    case RANGE_ALLOCATOR_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RANGE_ALLOCATOR_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RANGE_ALLOCATOR_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RANGE_ALLOCATOR_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RANGE_ALLOCATOR_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RANGE_ALLOCATOR_OVERFLOW:
        return s_rslt_str_overflow;
    case RANGE_ALLOCATOR_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

/**
 * @brief Choco Memoryの結果コードをRange Allocatorの結果コードへ変換する
 *
 * @details
 * 下位モジュールであるChoco Memoryが返したmemory_system_result_tを、
 * 同じ意味を持つrange_allocator_result_tへ変換する。
 *
 * 次の対応で変換する。
 *
 * - MEMORY_SYSTEM_SUCCESS
 *   → RANGE_ALLOCATOR_SUCCESS
 * - MEMORY_SYSTEM_INVALID_ARGUMENT
 *   → RANGE_ALLOCATOR_INVALID_ARGUMENT
 * - MEMORY_SYSTEM_LIMIT_EXCEEDED
 *   → RANGE_ALLOCATOR_LIMIT_EXCEEDED
 * - MEMORY_SYSTEM_BAD_OPERATION
 *   → RANGE_ALLOCATOR_BAD_OPERATION
 * - MEMORY_SYSTEM_NO_MEMORY
 *   → RANGE_ALLOCATOR_NO_MEMORY
 *
 * memory_system_result_tに定義されていない値は、
 * 意味を安全に変換できないためRANGE_ALLOCATOR_UNDEFINED_ERRORへ変換する。
 *
 * 本関数は結果コードの変換だけを行い、ログ出力、状態変更、
 * rollback、およびメモリ操作を行わない。
 *
 * @param[in] rslt_
 * Choco Memoryが返した結果コード。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * rslt_がMEMORY_SYSTEM_SUCCESSである。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * rslt_がMEMORY_SYSTEM_INVALID_ARGUMENTである。
 *
 * @retval RANGE_ALLOCATOR_LIMIT_EXCEEDED
 * rslt_がMEMORY_SYSTEM_LIMIT_EXCEEDEDである。
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * rslt_がMEMORY_SYSTEM_BAD_OPERATIONである。
 *
 * @retval RANGE_ALLOCATOR_NO_MEMORY
 * rslt_がMEMORY_SYSTEM_NO_MEMORYである。
 *
 * @retval RANGE_ALLOCATOR_UNDEFINED_ERROR
 * rslt_がmemory_system_result_tに定義されていない値である。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_allocator_result_t rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return RANGE_ALLOCATOR_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return RANGE_ALLOCATOR_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return RANGE_ALLOCATOR_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return RANGE_ALLOCATOR_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return RANGE_ALLOCATOR_NO_MEMORY;
    default:
        return RANGE_ALLOCATOR_UNDEFINED_ERROR;
    }
}
