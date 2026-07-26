/**
 * @file range_free_list.c
 * @brief 固定alignmentのメモリプール範囲を管理するRange Free Listの実装
 *
 * @details
 * FREE rangeだけでなく、メモリプール全体をFREE／ALLOCATED nodeとして
 * address orderで管理する。
 *
 * allocate成功時に、freeの完遂に必要なALLOCATED nodeを確保済みとすることで、
 * validなfreeがnode不足や動的メモリ確保失敗によって失敗しない構造を提供する。
 *
 * @par Node stateとrange listの不変条件
 * - node pool上の利用状態、range listへの接続状態、rangeの用途は、
 *   単一のnode stateで管理する。
 * - public APIの境界で許可する安定状態は、
 *   NOT_USED、FREE、ALLOCATEDの三つである。
 * - TRANSITIONINGはprivate操作の途中だけで使用し、
 *   public APIの入口、正常終了時、失敗終了時には残さない。
 * - FREE nodeとALLOCATED nodeだけがrange listへ接続される。
 * - NOT_USED nodeはrange listへ接続されず、offsetとblock sizeは0、
 *   prevとnextはNULLである。
 * - TRANSITIONING nodeのrange情報と接続状態は、
 *   遷移を担当するprivate関数の契約に従う。
 * - range listはFREE nodeとALLOCATED nodeをaddress orderで接続した
 *   双方向listである。
 * - 同じnodeがrange listへ重複して接続されてはならない。
 * - 隣接する2つのnodeが両方FREEであってはならない。
 *
 * @par Memory poolとalignmentの不変条件
 * - range listはmemory pool全体を隙間なく表現する。
 * - 先頭nodeのoffsetは0である。
 * - 各nodeの終端は次nodeのoffsetと一致する。
 * - 末尾nodeの終端はmemory pool sizeと一致する。
 * - 接続nodeのblock sizeは0ではない。
 * - 接続nodeのoffsetはbase alignment境界に整列している。
 * - ALLOCATED nodeのblock sizeはbase alignmentの倍数である。
 * - memory pool末尾を含むFREE nodeのblock sizeは、
 *   base alignmentの倍数ではない場合がある。
 *
 * @par Allocation数とnode数の不変条件
 * - ALLOCATED node数はallocation countと一致する。
 * - allocation countはmax allocation count以下である。
 * - max node countはmax allocation countの2倍である。
 * - range list接続node数とunused node countの合計は
 *   max node countと一致する。
 *
 * @par Allocate契約
 * - required alignmentはbase alignmentと一致しなければならない。
 * - required sizeはbase alignment単位に切り上げる。
 * - allocation検索にはfirst-fitを使用する。
 * - required sizeを満たさない先行FREE nodeは検索時に飛ばしてよい。
 * - allocationは選択したFREE nodeの先頭から行う。
 * - 分割はexact fitまたはALLOCATED＋後方FREEの2種類だけである。
 * - partial allocationに必要なNOT_USED nodeは状態変更前に特定する。
 * - allocate失敗時は内部状態と出力descriptorを変更しない。
 *
 * @par Allocation identityとfree契約
 * - allocation identityはownerとnode indexの組み合わせで表す。
 * - freeは状態変更前にowner、node index、node state、offset、allocated sizeを検証する。
 * - validなfreeはnode取得と動的メモリ確保を行わない。
 * - free後は隣接FREE nodeを必ずmergeする。
 * - mergeによって不要になったnodeはNOT_USEDへ戻す。
 *
 * @par エラー処理方針
 * - メモリアロケータの不具合は原因究明が困難になるため、
 *   public APIのエラー検査は厚めに行う。
 * - allocateとfreeは特に状態変更の影響が大きいため、
 *   動作が安定するまではdeep validationを含む厳格な検査を行う。
 * - 動作安定後は、リリースビルドにおける重い検査の省略を検討する。
 * - private関数ではRange Free List全体のvalidationを繰り返さず、
 *   呼び出し元が必要なvalidationを完了していることを事前条件とする。
 * - private関数自身は、担当する局所的な引数と状態の整合性を検査する。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 * 実装コードはプロジェクト作成者が作成し、その内容に責任を負う。
 */
#include "engine/systems/renderer/renderer_core/allocators/range_free_list.h"

#include <stdio.h>  // for fprintf
#include <string.h> // for memset
#include <stdbool.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

/**
 * @brief nodeの管理状態
 *
 * @details
 * node pool上の利用状態、range listへの接続状態、およびnodeが表すrangeの用途を単一のstateで管理する。
 *
 * @par 安定状態
 * public APIの入口、正常終了時、失敗終了時に許可する状態は次の三つである。
 *
 * - NOT_USED: node pool内で未使用であり、range listへ接続されていない。
 * - FREE: range listへ接続され、allocation可能なrangeを表している。
 * - ALLOCATED: range listへ接続され、live allocationが所有するrangeを表している。
 *
 * @par 内部遷移状態
 * TRANSITIONINGはprivate操作の途中だけで使用する。
 *
 * TRANSITIONING中のoffset、block size、prev、next、およびrange listへの
 * 接続状態は、遷移を担当するprivate関数の契約に従う。
 *
 * public APIの入口、正常終了時、失敗終了時にTRANSITIONINGが
 * 残ってはならない。
 *
 * @par 状態遷移
 * - acquire: NOT_USEDからTRANSITIONINGへ遷移する。
 * - insert: TRANSITIONINGからFREEまたはALLOCATEDへ遷移する。
 * - remove: FREEまたはALLOCATEDからTRANSITIONINGへ遷移する。
 * - release: TRANSITIONINGからNOT_USEDへ遷移する。
 * - exact fit allocation: FREEからALLOCATEDへ遷移する。
 * - mergeを伴わないfree: ALLOCATEDからFREEへ遷移する。
 *
 * TRANSITIONINGへの遷移前に、失敗し得る検証を完了する。
 * 遷移開始後に処理が失敗した場合は、public APIから戻る前に安定状態へ戻す。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
typedef enum {
    NODE_STATE_NOT_USED,        /**< node pool内で未使用 */
    NODE_STATE_TRANSITIONING,   /**< private操作によるstate、range情報、またはlist接続の更新中 */
    NODE_STATE_FREE,            /**< range list上のallocation可能なFREE range */
    NODE_STATE_ALLOCATED,       /**< range list上のlive allocationが所有するALLOCATED range */
} node_state_t;

/**
 * @brief メモリプール内の一つのrangeを管理するnode
 *
 * @details
 * FREEまたはALLOCATED状態のnodeはrange listへ接続され、
 * offsetとblock_sizeによって連続rangeを表す。
 *
 * NOT_USED状態ではrangeを保持せず、range listへ接続されない。
 * TRANSITIONING状態はprivate操作の途中だけで使用する。
 *
 * node_stateは、node pool上の利用状態、range listへの接続状態、
 * およびrangeの用途を統合して表す。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
typedef struct node {
    struct node* next;  /**< address orderで次に接続されたrange node */
    struct node* prev;  /**< address orderで前に接続されたrange node */

    size_t block_size;  /**< このnodeが表すrangeのサイズ(byte) */
    size_t offset;      /**< このnodeが表すrangeの開始offset(byte) */

    node_state_t node_state;          /**< nodeの利用状態、list接続状態、range用途を表す統合state */
} node_t;

/**
 * @brief Range Free List 内部状態管理構造体
 *
 */
struct range_free_list {
    size_t memory_pool_size;    /**< メモリプール容量(byte) */
    size_t max_node_count;      /**< 最大ノード数(個)(最もノードを消費する配置がALLOCATED / FREEを交互に繰り返す配置であるため、max_node_countの最大値はmax_allocation_count x 2 + 1となる) */
    size_t max_allocation_count;
    size_t unused_node_count;   /**< 未使用ノード数 */
    size_t allocation_count;
    size_t base_align;          /**< メモリプールの先頭アドレスのアライメント */
    size_t total_allocated_size;

    node_t* node_pool;             /**< range_free_listが所有する全ノードへの配列 */
    node_t* free_block_list_head;   /**< FREE／ALLOCATED range listの先頭nodeで、node_pool配列の要素 */
};

static const char* const s_rslt_str_success = "SUCCESS";                    /**< 実行結果コードRANGE_FREE_LIST_SUCCESS文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< 実行結果コードRANGE_FREE_LIST_INVALID_ARGUMENT文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< 実行結果コードRANGE_FREE_LIST_LIMIT_EXCEEDED文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< 実行結果コードRANGE_FREE_LIST_NO_MEMORY文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< 実行結果コードRANGE_FREE_LIST_DATA_CORRUPTED文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< 実行結果コードRANGE_FREE_LIST_BAD_OPERATION文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< 実行結果コードRANGE_FREE_LIST_OVERFLOW文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< 実行結果コードRANGE_FREE_LIST_UNDEFINED_ERROR文字列 */

static range_free_list_result_t align_up(size_t base_align_, size_t required_size_, size_t* out_allocation_size_);
static range_free_list_result_t find_first_fit_node(const range_free_list_t* range_free_list_, size_t allocation_size_, node_t** out_node_);
static range_free_list_result_t allocate_from_node(range_free_list_t* range_free_list_, node_t* node_, size_t allocation_size_);
static range_free_list_result_t node_release(range_free_list_t* range_free_list_, node_t* node_);
static range_free_list_result_t node_remove(range_free_list_t* range_free_list_, node_t* node_);

static range_free_list_result_t find_free_block_insert_position(const range_free_list_t* range_free_list_, size_t offset_, size_t free_size_, node_t** out_prev_node_, node_t** out_next_node_);
static range_free_list_result_t node_acquire(range_free_list_t* range_free_list_, size_t offset_, size_t block_size_, node_t** out_node_);
static range_free_list_result_t node_insert_between(range_free_list_t* range_free_list_, node_t* insert_node_, node_state_t next_state_, node_t* prev_, node_t* next_);
static range_free_list_result_t node_pool_find_index(const range_free_list_t* range_free_list_, const node_t* node_, size_t* out_index_);

static range_free_list_result_t allocation_resolve_node(range_free_list_t* range_free_list_, const range_allocation_t* allocation_info_, node_t** out_node_);
static range_free_list_result_t free_merge_plan_get(const range_free_list_t* range_free_list_, const node_t* node_, bool* out_should_merge_prev_, bool* out_should_merge_next_);
static range_free_list_result_t free_from_node(range_free_list_t* range_free_list_, node_t* node_, bool should_merge_prev_, bool should_merge_next_);
static range_free_list_result_t free_node_without_merge(node_t* node_);
static range_free_list_result_t free_node_merge_prev(range_free_list_t* range_free_list_, node_t* node_);
static range_free_list_result_t free_node_merge_next(range_free_list_t* range_free_list_, node_t* node_);
static range_free_list_result_t free_node_merge_prev_next(range_free_list_t* range_free_list_, node_t* node_);

static range_free_list_result_t node_adjacent_check_prev(const node_t* node_, bool* out_is_adjacent_);
static range_free_list_result_t node_adjacent_check_next(const node_t* node_, bool* out_is_adjacent_);
static range_free_list_result_t merge_free_block(range_free_list_t* range_free_list_, node_t* node_, bool should_merge_prev_, bool should_merge_next_);

// validation
static bool range_free_list_is_valid(const range_free_list_t* range_free_list_);            // deep validation
static bool range_free_list_is_valid_shallow(const range_free_list_t* range_free_list_);    // shallow validation
static bool node_is_valid(const node_t* node_);
static bool is_non_overlap(size_t left_node_offset_, size_t left_node_block_size_, size_t right_node_offset_);
static bool range_is_valid(const range_free_list_t* range_free_list_, size_t offset_, size_t block_size_);
static bool range_align_is_valid(size_t base_align_, size_t offset_, size_t block_size_);

// その他ヘルパー
static void set_node_to_not_used(node_t* target_);
static void set_node_to_transitioning(node_t* target_, size_t offset_, size_t block_size_);
static void set_node_to_free(node_t* target_, node_t* prev_, node_t* next_);
static void set_node_to_allocated(node_t* target_, node_t* prev_, node_t* next_);
static const char* rslt_to_str(range_free_list_result_t rslt_);
static range_free_list_result_t rslt_convert_choco_memory(memory_system_result_t rslt_);

range_free_list_result_t range_free_list_create(size_t memory_pool_size_, size_t max_allocation_count_, size_t base_align_, range_free_list_t** out_range_free_list_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;
    memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;

    range_free_list_t* tmp_range_free_list = NULL;

    size_t max_node_count = 0;
    size_t node_pool_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "out_range_free_list_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "*out_range_free_list_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != memory_pool_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "memory_pool_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != max_allocation_count_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "max_node_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != base_align_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_create", "base_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(base_align_), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "range_free_list_create", "base_align_")

    if(((SIZE_MAX - 1) / 2) < max_allocation_count_) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("range_free_list_create(%s) - range_free_list_create failed.", rslt_to_str(ret));
        goto cleanup;
    }
    max_node_count = max_allocation_count_ * 2 + 1;

    if((SIZE_MAX / max_node_count) < sizeof(node_t)) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("range_free_list_create(%s) - Failed to create range free list. reason=node pointer array size exceeds size_t range, max_node_count=%zu, node_pointer_size=%zu, size_max=%zu.", rslt_to_str(ret), max_node_count, sizeof(node_t), SIZE_MAX);
        goto cleanup;
    }
    node_pool_size = sizeof(node_t) * max_node_count;

    ret_memory = memory_system_allocate(sizeof(range_free_list_t), MEMORY_TAG_RENDERER, (void**)&tmp_range_free_list);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory) {
        ret = rslt_convert_choco_memory(ret_memory);
        ERROR_MESSAGE("range_free_list_create(%s) - range_free_list_create failed.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_range_free_list, 0, sizeof(range_free_list_t));

    ret_memory = memory_system_allocate(node_pool_size, MEMORY_TAG_RENDERER, (void**)&tmp_range_free_list->node_pool);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory) {
        ret = rslt_convert_choco_memory(ret_memory);
        ERROR_MESSAGE("range_free_list_create(%s) - range_free_list_create failed.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_range_free_list->node_pool, 0, node_pool_size);

    for(size_t i = 0; i != max_node_count; ++i) {
        memset(&tmp_range_free_list->node_pool[i], 0, sizeof(node_t));
        set_node_to_not_used(&tmp_range_free_list->node_pool[i]);
    }

    tmp_range_free_list->max_node_count = max_node_count;
    tmp_range_free_list->max_allocation_count = max_allocation_count_;
    tmp_range_free_list->allocation_count = 0;
    tmp_range_free_list->memory_pool_size = memory_pool_size_;
    tmp_range_free_list->unused_node_count = max_node_count - 1;
    tmp_range_free_list->base_align = base_align_;
    tmp_range_free_list->total_allocated_size = 0;

    tmp_range_free_list->free_block_list_head = &tmp_range_free_list->node_pool[0];
    tmp_range_free_list->free_block_list_head->block_size = memory_pool_size_;
    tmp_range_free_list->free_block_list_head->offset = 0;
    set_node_to_free(tmp_range_free_list->free_block_list_head, NULL, NULL);

    *out_range_free_list_ = tmp_range_free_list;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        if(NULL != tmp_range_free_list) {
            if(NULL != tmp_range_free_list->node_pool) {
                memory_system_free((void*)tmp_range_free_list->node_pool, node_pool_size, MEMORY_TAG_RENDERER);
                tmp_range_free_list->node_pool = NULL;
            }
            memory_system_free((void*)tmp_range_free_list, sizeof(range_free_list_t), MEMORY_TAG_RENDERER);
            tmp_range_free_list = NULL;
        }
    }
    return ret;
}

void range_free_list_destroy(range_free_list_t** range_free_list_) {
    if(NULL == range_free_list_) {
        return;
    }
    if(NULL == *range_free_list_) {
        return;
    }

    if(NULL != (*range_free_list_)->node_pool) {
        memory_system_free((void*)(*range_free_list_)->node_pool, sizeof(node_t) * (*range_free_list_)->max_node_count, MEMORY_TAG_RENDERER);
        (*range_free_list_)->node_pool = NULL;
    }

    (*range_free_list_)->max_node_count = 0;
    (*range_free_list_)->memory_pool_size = 0;

    memory_system_free((void*)*range_free_list_, sizeof(range_free_list_t), MEMORY_TAG_RENDERER);
    *range_free_list_ = NULL;
}

range_free_list_result_t range_free_list_allocate(range_free_list_t* range_free_list_, size_t required_size_, size_t required_align_, range_allocation_t* out_allocation_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    size_t allocation_size = 0;
    size_t allocated_index = 0;
    node_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_allocate", "range_free_list_")
    IF_ARG_FALSE_GOTO_CLEANUP(range_free_list_is_valid(range_free_list_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "range_free_list_allocate", "range_free_list_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_allocate", "required_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_align_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_allocate", "required_align_")
    IF_ARG_NULL_GOTO_CLEANUP(out_allocation_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_allocate", "out_allocation_")
    IF_ARG_FALSE_GOTO_CLEANUP(range_free_list_->base_align == required_align_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "range_free_list_allocate", "required_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(range_free_list_->allocation_count < range_free_list_->max_allocation_count, ret, RANGE_FREE_LIST_LIMIT_EXCEEDED, rslt_to_str(RANGE_FREE_LIST_LIMIT_EXCEEDED), "range_free_list_allocate", "allocation_count")

    ret = align_up(range_free_list_->base_align, required_size_, &allocation_size);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_allocate(%s) - Failed to allocate range. reason=failed to align required size, base_align=%zu, required_size=%zu.", rslt_to_str(ret), range_free_list_->base_align, required_size_);
        goto cleanup;
    }

    ret = find_first_fit_node(range_free_list_, allocation_size, &node);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_allocate(%s) - Failed to allocate range. reason=failed to find free block, required_size=%zu, allocation_size=%zu.", rslt_to_str(ret), required_size_, allocation_size);
        goto cleanup;
    }

    ret = node_pool_find_index(range_free_list_, node, &allocated_index);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_allocate(%s) - Failed to allocate range. reason=failed to consume free block, required_size=%zu, allocation_size=%zu.", rslt_to_str(ret), required_size_, allocation_size);
        goto cleanup;
    }

    ret = allocate_from_node(range_free_list_, node, allocation_size);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ERROR_MESSAGE("range_free_list_allocate(%s) - Failed to allocate range. reason=failed to consume free block, required_size=%zu, allocation_size=%zu.", rslt_to_str(ret), required_size_, allocation_size);
        goto cleanup;
    }

    out_allocation_->node_index = allocated_index;
    out_allocation_->allocated_size = allocation_size;
    out_allocation_->offset = node->offset;
    out_allocation_->owner = range_free_list_;
    range_free_list_->allocation_count++;
    range_free_list_->total_allocated_size += allocation_size;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

range_free_list_result_t range_free_list_free(range_free_list_t* range_free_list_, const range_allocation_t* allocation_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    node_t* tmp_node = NULL;
    bool should_merge_prev = false;
    bool should_merge_next = false;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_free", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(allocation_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_free", "allocation_")
    IF_ARG_FALSE_GOTO_CLEANUP(range_free_list_->total_allocated_size >= allocation_->allocated_size, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "range_free_list_free", "allocation_->allocated_size")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < range_free_list_->allocation_count, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "range_free_list_free", "range_free_list_->allocation_count")

    if(!range_free_list_is_valid(range_free_list_)) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = allocation_resolve_node(range_free_list_, allocation_, &tmp_node);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;// range_free_list_およびallocation_が正常であれば失敗しないはずなのでDATA_CORRUPTED
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = free_merge_plan_get(range_free_list_, tmp_node, &should_merge_prev, &should_merge_next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;// range_free_list_およびallocation_が正常であれば失敗しないはずなのでDATA_CORRUPTED
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = free_from_node(range_free_list_, tmp_node, should_merge_prev, should_merge_next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;// range_free_list_およびallocation_が正常であれば失敗しないはずなのでDATA_CORRUPTED
        ERROR_MESSAGE("range_free_list_free(%s) - range_free_list_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    range_free_list_->allocation_count--;
    range_free_list_->total_allocated_size -= allocation_->allocated_size;
    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// このAPIは破損状態でも中身を確認したいため、エラー処理はNULLチェックのみとする
void range_free_list_status_get(const range_free_list_t* range_free_list_, range_free_list_status_t* out_status_) {
    size_t tmp_used_node_count = 0;
    size_t tmp_free_block_count = 0;

    if(NULL == range_free_list_ || NULL == out_status_) {
        return;
    }
    out_status_->memory_pool_size = range_free_list_->memory_pool_size;
    out_status_->base_align = range_free_list_->base_align;
    out_status_->max_node_count = range_free_list_->max_node_count;
    out_status_->max_allocation_count = range_free_list_->max_allocation_count;
    out_status_->total_allocated_size = range_free_list_->total_allocated_size;
    out_status_->total_free_size = range_free_list_->memory_pool_size - out_status_->total_allocated_size;
    out_status_->unused_node_count = range_free_list_->unused_node_count;

    if(range_free_list_->max_node_count > range_free_list_->unused_node_count) {
        tmp_used_node_count = range_free_list_->max_node_count - range_free_list_->unused_node_count;
    } else {
        tmp_used_node_count = 0;
    }
    out_status_->used_node_count = tmp_used_node_count;

    if(out_status_->used_node_count > range_free_list_->allocation_count) {
        tmp_free_block_count = out_status_->used_node_count - range_free_list_->allocation_count;
    } else {
        tmp_free_block_count = 0;
    }
    out_status_->free_block_count = tmp_free_block_count;

    out_status_->allocation_count = range_free_list_->allocation_count;
}

void range_free_list_debug_print(const range_free_list_t* range_free_list_) {
    size_t index = 0;
    const node_t* node = NULL;
    range_free_list_status_t status = { 0 };
    bool valid = false;

    if(NULL == range_free_list_) {
        return;
    }

    valid = range_free_list_is_valid(range_free_list_);
    range_free_list_status_get(range_free_list_, &status);

    flockfile(stdout); // 同一ストリームの同時書き込みをまとめる

    fprintf(stdout, "\033[1;35m[RANGE FREE LIST DUMP MESSAGE]\n");
    fprintf(stdout, "  range_free_list_is_valid = %s\n", valid ? "true" : "false");
    fprintf(stdout, "  memory_pool_size = %zu\n", status.memory_pool_size);
    fprintf(stdout, "  base_align = %zu\n", status.base_align);
    fprintf(stdout, "  max_node_count = %zu\n", status.max_node_count);
    fprintf(stdout, "  unused_node_count = %zu\n", status.unused_node_count);
    fprintf(stdout, "  free_block_count = %zu\n", status.free_block_count);
    fprintf(stdout, "  total_free_size = %zu\n", status.total_free_size);
    fprintf(stdout, "  max_free_block_size = %zu\n", status.max_free_block_size);
    fprintf(stdout, "  free blocks:\n");

    node = range_free_list_->free_block_list_head;
    while(NULL != node && index < range_free_list_->max_node_count) {
        if((SIZE_MAX - node->block_size) < node->offset) {
            fprintf(stdout, "    [%zu] offset=%zu, size=%zu, end=OVERFLOW\n", index, node->offset, node->block_size);
        } else {
            fprintf(stdout, "    [%zu] offset=%zu, size=%zu, end=%zu\n", index, node->offset, node->block_size, node->offset + node->block_size);
        }
        node = node->next;
        index++;
    }

    fprintf(stdout, "\033[0m");
    funlockfile(stdout);
}

/**
 * @brief first-fit方式でallocation可能なFREE nodeを検索する
 *
 * @details
 * FREE／ALLOCATED nodeが混在するrange listを先頭から走査し、
 * block sizeがallocation_size_以上である最初のFREE nodeを返す。
 *
 * ALLOCATED nodeは検索対象から除外して読み飛ばす。
 * 選択したFREE nodeについては、offsetがbase alignment境界にあることを検証する。
 *
 * range listの末尾まで走査しても適合するFREE nodeが存在しない場合は、RANGE_FREE_LIST_NO_MEMORYを返す。
 *
 * max node countまで走査してもlistの末尾へ到達しない場合は、range listの循環またはnode数の不整合として扱う。
 *
 * 本関数はrange listおよびnodeの状態を変更しない。
 *
 * @param[in] range_free_list_ 検索対象のRange Free List。
 * @param[in] allocation_size_ 必要なallocation size。0ではなく、base alignmentの倍数でなければならない。
 * @param[out] out_node_ 検索結果のFREE nodeを受け取る。呼び出し時の*out_node_はNULLでなければならない。
 *
 * @pre
 * range_free_list_に対するshallow validationまたはdeep validationが
 * 呼び出し元によって完了していなければならない。
 *
 * @pre
 * range list上のnode stateはFREEまたはALLOCATEDでなければならない。
 *
 * @retval RANGE_FREE_LIST_SUCCESS
 * allocation_size_以上のblock sizeを持つ最初のFREE nodeが見つかった。
 *
 * @retval RANGE_FREE_LIST_INVALID_ARGUMENT
 * range_free_list_、out_node_がNULL、allocation_size_が0、または呼び出し時の*out_node_がNULLではない。
 *
 * @retval RANGE_FREE_LIST_BAD_OPERATION
 * allocation_size_がbase alignmentの倍数ではない。
 *
 * @retval RANGE_FREE_LIST_NO_MEMORY
 * range listの末尾までに、要求を満たすFREE nodeが存在しない。
 *
 * @retval RANGE_FREE_LIST_DATA_CORRUPTED
 * list headがNULL、選択したFREE nodeのoffsetがbase alignment境界にない、
 * またはmax node count以内にrange listの走査が終了しない。
 *
 * @post
 * 成功時、*out_node_はrange list上のFREE nodeを指し、
 * そのblock sizeはallocation_size_以上である。
 *
 * @post
 * 失敗時、*out_node_は変更されない。
 *
 * @note
 * 計算量はrange list上のnode数に対してO(n)である。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_free_list_result_t find_first_fit_node(const range_free_list_t* range_free_list_, size_t allocation_size_, node_t** out_node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    size_t index = 0;
    bool found = false;
    node_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "range_free_list_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != allocation_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "allocation_size_")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_first_fit_node", "*out_node_")
    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_->free_block_list_head, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "find_first_fit_node", "range_free_list_->free_block_list_head")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == (allocation_size_ % range_free_list_->base_align), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "find_first_fit_node", "allocation_size_")

    node = range_free_list_->free_block_list_head;
    while(NULL != node && index < range_free_list_->max_node_count) {
        if(NODE_STATE_FREE == node->node_state && node->block_size >= allocation_size_) {
            if(0 != (node->offset % range_free_list_->base_align)) {
                ret = RANGE_FREE_LIST_DATA_CORRUPTED;
                ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first-fit free block. reason=free block offset is not aligned to base_align, list_position=%zu, offset=%zu, block_size=%zu, base_align=%zu.", rslt_to_str(ret), index, node->offset, node->block_size, range_free_list_->base_align);
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
            ret = RANGE_FREE_LIST_NO_MEMORY;
            ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first-fit FREE range. reason=no FREE range is large enough, allocation_size=%zu.", rslt_to_str(ret), allocation_size_);
            goto cleanup;
        } else {            // max node countまでにlist走査が終了しなかった
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("find_first_fit_node(%s) - Failed to find first-fit FREE range. reason=range list traversal did not terminate within max node count, allocation_size=%zu, max_node_count=%zu.", rslt_to_str(ret), allocation_size_, range_free_list_->max_node_count);
            goto cleanup;
        }
    }
    *out_node_ = node;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief FREE nodeの先頭から指定サイズをallocationする
 *
 * @details
 * node_が表すFREE rangeの先頭からallocation_size_を確保し、
 * node_を対応するALLOCATED nodeへ遷移させる。
 *
 * allocationは次の2パターンに対応する。
 *
 * @par Exact fit
 * node_のblock sizeがallocation_size_と等しい場合、
 * offset、block size、prev、nextを変更せず、node_を
 * FREEからALLOCATEDへ遷移させる。
 *
 * 新しいnodeは取得せず、unused node countも変更しない。
 *
 * @par ALLOCATED＋後方FREE
 * node_のblock sizeがallocation_size_より大きい場合、
 * node_の先頭部分をALLOCATED rangeとして使用し、残りを新しい後方FREE nodeとして表現する。
 *
 * - node_のoffsetは変更しない
 * - node_のblock sizeをallocation_size_へ変更する
 * - 後方FREE nodeのoffsetをnode_の元offset＋allocation_size_とする
 * - 後方FREE nodeのblock sizeをnode_の元block size－allocation_size_とする
 * - 後方FREE nodeをnode_の直後へ接続する
 * - node_をFREEからALLOCATEDへ遷移させる
 *
 * 後方FREE nodeはnode poolからacquireし、TRANSITIONINGを経てrange listへFREEとして接続する。
 *
 * allocation前の不変条件によりnode_の隣にFREE nodeは存在しないため、本関数では後方FREE nodeのmergeを行わない。
 *
 * node acquire後にinsertが失敗した場合は、node_のblock sizeを復元し、
 * acquireしたTRANSITIONING nodeをnode poolへreleaseする。
 *
 * 本関数はallocation countの更新、およびallocation descriptorの設定を行わない。
 * これらは呼び出し元の責務とする。
 *
 * @param[in,out] range_free_list_ node_を所有するRange Free List。
 * @param[in,out] node_ allocation対象のFREE node。
 * @param[in] allocation_size_ node_の先頭からallocationするサイズ。
 *
 * @pre
 * range_free_list_に対するshallow validationまたはdeep validationが、
 * 呼び出し元によって完了していなければならない。
 *
 * @pre
 * node_はrange_free_list_のnode poolに所属し、range listへ
 * FREEとして接続されていなければならない。
 *
 * @pre
 * allocation_size_は0ではなく、base alignmentの倍数であり、
 * node_のblock size以下でなければならない。
 *
 * @pre
 * node_のoffsetはbase alignment境界にあり、range list上に
 * 隣接するFREE nodeが存在してはならない。
 *
 * @retval RANGE_FREE_LIST_SUCCESS
 * exact fitまたは後方FREE分割によるallocationに成功した。
 *
 * @retval RANGE_FREE_LIST_INVALID_ARGUMENT
 * range_free_list_またはnode_がNULL、もしくはallocation_size_が0である。
 *
 * @retval RANGE_FREE_LIST_BAD_OPERATION
 * node_がFREEではない、node_のblock sizeがallocation_size_より小さい、
 * またはnode acquire／insertの操作条件を満たしていない。
 *
 * @retval RANGE_FREE_LIST_LIMIT_EXCEEDED
 * 後方FREE nodeに使用できるNOT_USED nodeが存在しない。
 *
 * @retval RANGE_FREE_LIST_OVERFLOW
 * 後方FREE nodeのoffset計算でsize_tの範囲を超える。
 *
 * @retval RANGE_FREE_LIST_DATA_CORRUPTED
 * node_、node pool、range list、またはnode接続に整合性異常がある。
 * または、insert失敗後のrollbackでnew nodeをreleaseできなかった。
 *
 * @post
 * 成功時、node_はallocation_size_のALLOCATED rangeを表す。
 *
 * @post
 * partial allocation成功時、node_の直後には残りrangeを表す
 * FREE nodeが接続され、unused node countは1減少する。
 *
 * @post
 * exact fit成功時、range listの接続関係、node_のrange情報、
 * およびunused node countは変更されない。
 *
 * @post
 * 失敗時、rollbackに成功した場合はrange list、node_、
 * node pool、およびunused node countが呼び出し前の状態に保たれる。
 *
 * @warning
 * rollback中のnode releaseに失敗した場合、TRANSITIONING nodeが
 * node pool内に残る可能性がある。この場合はDATA_CORRUPTEDを返す。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_free_list_result_t allocate_from_node(range_free_list_t* range_free_list_, node_t* node_, size_t allocation_size_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;
    range_free_list_result_t ret_cleanup = RANGE_FREE_LIST_INVALID_ARGUMENT;

    node_t* new_node = NULL;
    size_t new_node_block_size = 0;
    size_t new_node_offset = 0;
    size_t block_size_escape = 0;

    bool acquired = false;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "allocate_from_node", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "allocate_from_node", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_FREE == node_->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "allocate_from_node", "node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != allocation_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "allocate_from_node", "allocation_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_->block_size >= allocation_size_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "allocate_from_node", "allocation_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "allocate_from_node", "node_")

    if(node_->block_size == allocation_size_) {
        set_node_to_allocated(node_, node_->prev, node_->next);
    } else {
        if((SIZE_MAX - allocation_size_) < node_->offset) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("allocate_from_node(%s) - allocate_from_node failed.", rslt_to_str(ret));
            goto cleanup;
        }
        new_node_offset = node_->offset + allocation_size_;
        new_node_block_size = node_->block_size - allocation_size_;

        ret = node_acquire(range_free_list_, new_node_offset, new_node_block_size, &new_node);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("allocate_from_node(%s) - allocate_from_node failed.", rslt_to_str(ret));
            goto cleanup;
        }
        acquired = true;
        block_size_escape = node_->block_size;
        node_->block_size = allocation_size_;

        ret = node_insert_between(range_free_list_, new_node, NODE_STATE_FREE, node_, node_->next);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("allocate_from_node(%s) - allocate_from_node failed.", rslt_to_str(ret));
            goto cleanup;
        }

        set_node_to_allocated(node_, node_->prev, node_->next);
    }

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        if(acquired) {
            node_->block_size = block_size_escape;
            ret_cleanup = node_release(range_free_list_, new_node);
            if(RANGE_FREE_LIST_SUCCESS != ret_cleanup) {
                ret = RANGE_FREE_LIST_DATA_CORRUPTED;
                ERROR_MESSAGE("allocate_from_node(%s) - allocate_from_node failed.", rslt_to_str(ret));
                return ret;
            }
        }
    }

    return ret;
}

/**
 * @brief node poolから未使用nodeを取得する
 *
 * @details
 * node poolから最初に見つかったNOT_USED nodeを取得し、
 * 指定されたrange情報を設定してTRANSITIONING状態へ遷移させる。
 *
 * 成功時はunused node countを1減らし、取得したnodeをout_node_へ設定する。
 *
 * 取得したnodeはrange listへ接続されない。prevとnextはNULLとなる。
 * 呼び出し元は後続処理によって、取得したnodeをFREE、ALLOCATED、
 * またはNOT_USEDの安定状態へ遷移させなければならない。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @param[in,out] range_free_list_ nodeを所有するRange Free List。
 * @param[in] offset_ 取得したnodeへ設定するrange開始offset。
 * @param[in] block_size_ 取得したnodeへ設定するrangeサイズ。
 * @param[out] out_node_ 取得したTRANSITIONING nodeの出力先。呼び出し時にNULLを指していなければならない。
 *
 * @note
 * 呼び出し元は、range_free_list_に対するshallow validationまたはdeep validationを事前に完了させなければならない。
 *
 * @retval RANGE_FREE_LIST_SUCCESS
 * NOT_USED nodeの取得に成功した。
 *
 * @retval RANGE_FREE_LIST_INVALID_ARGUMENT
 * range_free_list_またはout_node_がNULL、もしくはout_node_がすでにnodeを指している。
 *
 * @retval RANGE_FREE_LIST_BAD_OPERATION
 * offset_とblock_size_が有効なrangeを表していない。
 *
 * @retval RANGE_FREE_LIST_LIMIT_EXCEEDED
 * unused node countが0であり、取得可能なnodeが存在しない。
 *
 * @retval RANGE_FREE_LIST_DATA_CORRUPTED
 * node pool、unused node count、または取得対象nodeの内部状態に整合性異常が検出された。
 *
 * @post
 * 成功時、out_node_は指定されたrange情報を保持するTRANSITIONING nodeを指し、
 * unused node countは呼び出し前から1減少する。
 *
 * @post
 * 失敗時、node pool、range list、unused node count、およびout_node_の内容は呼び出し前から変更されない。
 *
 * @warning
 * 成功時に返されるnodeはprivate操作途中の一時的な状態である。public APIから戻る前に安定状態へ遷移させなければならない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_free_list_result_t node_acquire(range_free_list_t* range_free_list_, size_t offset_, size_t block_size_, node_t** out_node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    node_t* tmp_node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_acquire", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_acquire", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_acquire", "*out_node_")
    IF_ARG_FALSE_GOTO_CLEANUP(range_is_valid(range_free_list_, offset_, block_size_), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_acquire", "range")

    if(0 == range_free_list_->unused_node_count) {
        ret = RANGE_FREE_LIST_LIMIT_EXCEEDED;
        ERROR_MESSAGE("node_acquire(%s) - Failed to acquire free block node. reason=no unused node available, unused_node_count=%zu, max_node_count=%zu, offset=%zu, block_size=%zu.", rslt_to_str(ret), range_free_list_->unused_node_count, range_free_list_->max_node_count, offset_, block_size_);
        goto cleanup;
    }

    for(size_t i = 0; i != range_free_list_->max_node_count; ++i) {
        if(NODE_STATE_NOT_USED == range_free_list_->node_pool[i].node_state) {
            tmp_node = &range_free_list_->node_pool[i];
            found = true;
            break;
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_acquire(%s) - Failed to acquire free block node. reason=unused_node_count is nonzero but no NOT_USED node was found, unused_node_count=%zu, max_node_count=%zu.", rslt_to_str(ret), range_free_list_->unused_node_count, range_free_list_->max_node_count);
        goto cleanup;
    } else {
        if(!node_is_valid(tmp_node)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_acquire(%s) - Failed to acquire free block node. reason=found NOT_USED node is corrupted, offset=%zu, block_size=%zu.", rslt_to_str(ret), tmp_node->offset, tmp_node->block_size);
            goto cleanup;
        }
    }

    set_node_to_transitioning(tmp_node, offset_, block_size_);

    range_free_list_->unused_node_count--;
    *out_node_ = tmp_node;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief 切断済みTRANSITIONING nodeをnode poolへ返却する
 *
 * @details
 * range listから切断されたTRANSITIONING nodeを正規化された
 * NOT_USED状態へ戻し、unused node countを1増加させる。
 *
 * 成功時、node_には次の状態が設定される。
 *
 * - stateはNOT_USED
 * - offsetは0
 * - block sizeは0
 * - prevはNULL
 * - nextはNULL
 *
 * 本関数はrange list、list head、隣接nodeを変更しない。
 * node_のrange listからの切断は、呼び出し前にnode_remove()で
 * 完了させなければならない。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @param[in,out] range_free_list_ node_を所有するRange Free List。
 * @param[in,out] node_ node poolへ返却するTRANSITIONING node。
 *
 * @note
 * remove／release処理を開始する前に、呼び出し元は
 * range_free_list_全体を検証しなければならない。
 *
 * node_remove()成功後はrange listが内部遷移中となるため、
 * node_release()呼び出し直前のdeep validationは要求しない。
 *
 * @pre
 * node_のstateはTRANSITIONINGでなければならない。
 *
 * @pre
 * node_はrange_free_list_のnode poolに所属していなければならない。
 *
 * @pre
 * node_はrange listから切断済みであり、prevとnextはNULLで
 * なければならない。
 *
 * @retval RANGE_FREE_LIST_SUCCESS
 * node_のnode poolへの返却に成功した。
 *
 * @retval RANGE_FREE_LIST_INVALID_ARGUMENT
 * range_free_list_またはnode_がNULLである。
 *
 * @retval RANGE_FREE_LIST_BAD_OPERATION
 * node_のstateがTRANSITIONINGではない。
 *
 * @retval RANGE_FREE_LIST_DATA_CORRUPTED
 * node_、node pool、unused node count、またはnodeの所属関係に整合性異常が検出された。
 *
 * @post
 * 成功時、node_は正規化されたNOT_USED状態となり、
 * unused node countは呼び出し前から1増加する。
 *
 * @post
 * 失敗時、node_、node pool、range list、およびunused node countは
 * 呼び出し前から変更されない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_free_list_result_t node_release(range_free_list_t* range_free_list_, node_t* node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    size_t index = 0;   // エラーメッセージ用

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_release", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_release", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(range_free_list_->unused_node_count < range_free_list_->max_node_count, ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "node_release", "unused_node_count")

    for(size_t i = 0; i != range_free_list_->max_node_count; ++i) {
        if(&range_free_list_->node_pool[i] == node_) {
            found = true;
            index = i;
            break;
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_release(%s) - Failed to release node to pool. reason=requested node was not found in node_pool, offset=%zu, block_size=%zu, node_state=%d, max_node_count=%zu, unused_node_count=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, node_->node_state, range_free_list_->max_node_count, range_free_list_->unused_node_count);
        goto cleanup;
    }
    if(!node_is_valid(node_)) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_release(%s) - Failed to release node to pool.", rslt_to_str(ret));
        goto cleanup;
    }
    if(NODE_STATE_TRANSITIONING != node_->node_state) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_release(%s) - Failed to release node to pool.", rslt_to_str(ret));
        goto cleanup;
    }

    set_node_to_not_used(node_);

    range_free_list_->unused_node_count++;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief FREEまたはALLOCATED nodeをrange listから切断する
 *
 * @details
 * range listへ接続されているnode_を切断し、stateをFREEまたはALLOCATEDからTRANSITIONINGへ遷移させる。
 *
 * 次の切断位置に対応する。
 *
 * - node_がrange list上の唯一のnode
 * - node_がrange listの先頭
 * - node_が二つのnodeの間
 * - node_がrange listの末尾
 *
 * 切断時はlist headと隣接nodeの接続情報を更新する。
 * node_のoffsetとblock sizeは保持し、prevとnextはNULLへ設定する。
 *
 * 本関数はnode_をnode poolへ返却せず、unused node countも変更しない。
 * node poolへの返却は、切断成功後にnode_release()を使用して行う。
 *
 * @param[in,out] range_free_list_ node_を切断するRange Free List。
 * @param[in,out] node_ range listから切断するFREEまたはALLOCATED node。
 *
 * @note
 * 呼び出し元は、range_free_list_に対するshallow validationまたは
 * deep validationを事前に完了させなければならない。
 *
 * @pre
 * node_のstateはFREEまたはALLOCATEDでなければならない。
 *
 * @pre
 * node_はrange_free_list_のnode poolに所属し、range listへ
 * 正しく接続されていなければならない。
 *
 * @retval RANGE_FREE_LIST_SUCCESS
 * node_の切断とTRANSITIONINGへの遷移に成功した。
 *
 * @retval RANGE_FREE_LIST_INVALID_ARGUMENT
 * range_free_list_またはnode_がNULLである。
 *
 * @retval RANGE_FREE_LIST_BAD_OPERATION
 * node_のstateがFREEでもALLOCATEDでもない。
 *
 * @retval RANGE_FREE_LIST_DATA_CORRUPTED
 * node_、node pool、list head、またはprev／nextの接続関係に整合性異常が検出された。
 *
 * @post
 * 成功時、node_はrange listから切断されたTRANSITIONING nodeとなる。
 * offsetとblock sizeは保持され、prevとnextはNULLとなる。
 *
 * @post
 * 成功時、list headと隣接nodeの接続情報は切断後の状態と整合する。
 * unused node countは変更されない。
 *
 * @post
 * 失敗時、range list、node_、list head、およびunused node countは
 * 呼び出し前から変更されない。
 *
 * @warning
 * 成功時のnode_はprivate操作途中の状態である。
 * public APIから戻る前にnode_release()でNOT_USEDへ戻すか、
 * range listへ再挿入して安定状態へ遷移させなければならない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_free_list_result_t node_remove(range_free_list_t* range_free_list_, node_t* node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    size_t index = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_remove", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_remove", "node_")

    for(size_t i = 0; i != range_free_list_->max_node_count; ++i) {
        if(&range_free_list_->node_pool[i] == node_) {
            found = true;
            index = i;
            break;
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_remove(%s) - Failed to remove node from free list. reason=requested node was not found in node_pool, offset=%zu, block_size=%zu, node_state=%d, max_node_count=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, node_->node_state, range_free_list_->max_node_count);
        goto cleanup;
    }
    if(!node_is_valid(node_)) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_remove(%s) - Failed to remove node to pool.", rslt_to_str(ret));
        goto cleanup;
    }
    if(NODE_STATE_FREE != node_->node_state && NODE_STATE_ALLOCATED != node_->node_state) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_remove(%s) - Failed to remove node to pool.", rslt_to_str(ret));
        goto cleanup;
    }

    if(NULL == node_->prev && NULL == node_->next) {  // node_が唯一のノード
        if(range_free_list_->free_block_list_head != node_) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_remove(%s) - Failed to remove node from free list. reason=node has no prev/next but is not free_block_list_head, offset=%zu, block_size=%zu, node_index=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, index);
            goto cleanup;
        }
        range_free_list_->free_block_list_head = NULL;
    } else if(NULL == node_->prev && NULL != node_->next) {   // node_が先頭で、node_の次に別のノードがある
        if(range_free_list_->free_block_list_head != node_) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_remove(%s) - Failed to remove node from free list. reason=node has no prev but is not free_block_list_head, offset=%zu, block_size=%zu, node_index=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, index);
            goto cleanup;
        }
        range_free_list_->free_block_list_head = node_->next;

        node_->next->prev = NULL;
    } else if(NULL != node_->prev && NULL != node_->next) {   // node_の前後に別のノードがある
        node_->prev->next = node_->next;
        node_->next->prev = node_->prev;
    } else if(NULL != node_->prev && NULL == node_->next) {   // node_の前にノードが存在し、かつ、node_が末尾ノード
        node_->prev->next = NULL;
    }

    // block_size, offsetは保持する
    set_node_to_transitioning(node_, node_->offset, node_->block_size);

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t find_free_block_insert_position(const range_free_list_t* range_free_list_, size_t offset_, size_t free_size_, node_t** out_prev_node_, node_t** out_next_node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    node_t* tmp_node = NULL;
    node_t* tmp_prev_node = NULL;
    node_t* tmp_next_node = NULL;
    bool found = false;
    size_t index = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(out_prev_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "out_prev_node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_next_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "out_next_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_prev_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "*out_prev_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_next_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "find_free_block_insert_position", "*out_next_node_")

    IF_ARG_FALSE_GOTO_CLEANUP(range_is_valid(range_free_list_, offset_, free_size_), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "find_free_block_insert_position", "range")
    IF_ARG_FALSE_GOTO_CLEANUP(range_align_is_valid(range_free_list_->base_align, offset_, free_size_), ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "find_free_block_insert_position", "align")

    tmp_node = range_free_list_->free_block_list_head;
    if(NULL == tmp_node) {  // free listが空
        tmp_prev_node = NULL;
        tmp_next_node = NULL;
        found = true;
    } else {
        if(tmp_node->offset > offset_) {    // 先頭に挿入
            if(!is_non_overlap(offset_, free_size_, tmp_node->offset)) {
                ret = RANGE_FREE_LIST_BAD_OPERATION;
                ERROR_MESSAGE("find_free_block_insert_position(%s) - Failed to find free block insert position. reason=new free range overlaps head free block, offset=%zu, free_size=%zu, head_offset=%zu, head_block_size=%zu.", rslt_to_str(ret), offset_, free_size_, tmp_node->offset, tmp_node->block_size);
                goto cleanup;
            }
            tmp_prev_node = NULL;
            tmp_next_node = tmp_node;
            found = true;
        } else {
            while(NULL != tmp_node && index < range_free_list_->max_node_count) {
                if(tmp_node->offset == offset_) {
                    ret = RANGE_FREE_LIST_BAD_OPERATION;
                    ERROR_MESSAGE("find_free_block_insert_position(%s) - Failed to find free block insert position. reason=free block with same offset already exists, offset=%zu, free_size=%zu, node_index=%zu, existing_block_size=%zu.", rslt_to_str(ret), offset_, free_size_, index, tmp_node->block_size);
                    goto cleanup;
                }
                if(NULL != tmp_node->next && tmp_node->offset >= tmp_node->next->offset) {
                    ret = RANGE_FREE_LIST_DATA_CORRUPTED;
                    ERROR_MESSAGE("find_free_block_insert_position(%s) - Failed to find free block insert position. reason=free block list order is corrupted, node_index=%zu, offset=%zu, next_offset=%zu.", rslt_to_str(ret), index, tmp_node->offset, tmp_node->next->offset);
                    goto cleanup;
                }
                // TODO: ここから下はもっとスッキリできる
                if(tmp_node->offset < offset_ && NULL != tmp_node->next) {
                    if(tmp_node->next->offset > offset_) {  // 途中に挿入
                        if(!is_non_overlap(tmp_node->offset, tmp_node->block_size, offset_)) {
                            ret = RANGE_FREE_LIST_BAD_OPERATION;
                            ERROR_MESSAGE("find_free_block_insert_position(%s) - Failed to find free block insert position. reason=new free range overlaps previous free block, offset=%zu, free_size=%zu, prev_index=%zu, prev_offset=%zu, prev_block_size=%zu.", rslt_to_str(ret), offset_, free_size_, index, tmp_node->offset, tmp_node->block_size);
                            goto cleanup;
                        }
                        if(!is_non_overlap(offset_, free_size_, tmp_node->next->offset)) {
                            ret = RANGE_FREE_LIST_BAD_OPERATION;
                            ERROR_MESSAGE("find_free_block_insert_position(%s) - Failed to find free block insert position. reason=new free range overlaps next free block, offset=%zu, free_size=%zu, next_index=%zu, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), offset_, free_size_, index + 1, tmp_node->next->offset, tmp_node->next->block_size);
                            goto cleanup;
                        }
                        tmp_prev_node = tmp_node;
                        tmp_next_node = tmp_node->next;
                        found = true;
                        break;
                    }
                } else if(tmp_node->offset < offset_ && NULL == tmp_node->next) {   // 末尾に挿入
                    if(!is_non_overlap(tmp_node->offset, tmp_node->block_size, offset_)) {
                        ret = RANGE_FREE_LIST_BAD_OPERATION;
                        ERROR_MESSAGE("find_free_block_insert_position(%s) - Failed to find free block insert position. reason=new free range overlaps tail free block, offset=%zu, free_size=%zu, tail_index=%zu, tail_offset=%zu, tail_block_size=%zu.", rslt_to_str(ret), offset_, free_size_, index, tmp_node->offset, tmp_node->block_size);
                        goto cleanup;
                    }
                    tmp_prev_node = tmp_node;
                    tmp_next_node = NULL;
                    found = true;
                    break;
                }
                tmp_node = tmp_node->next;
                index++;
            }
        }
    }

    if(!found) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("find_free_block_insert_position(%s) - Failed to find free block insert position. reason=no valid insert position found, offset=%zu, free_size=%zu, max_node_count=%zu.", rslt_to_str(ret), offset_, free_size_, range_free_list_->max_node_count);
        goto cleanup;
    }

    *out_prev_node_ = tmp_prev_node;
    *out_next_node_ = tmp_next_node;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief TRANSITIONING nodeをrange listへ接続して指定stateへ遷移させる
 *
 * @details
 * insert_node_をprev_とnext_の間へ接続し、next_state_で指定されたFREEまたはALLOCATEDの安定状態へ遷移させる。
 *
 * 次の挿入位置に対応する。
 *
 * - prev_とnext_がともにNULL: 空のrange listへ挿入する。
 * - prev_がNULL: range listの先頭へ挿入する。
 * - prev_とnext_がともに非NULL: 二つの隣接nodeの間へ挿入する。
 * - next_がNULL: range listの末尾へ挿入する。
 *
 * 必要な隣接nodeとlist headを更新した後、next_state_に応じてinsert_node_をFREEまたはALLOCATEDへ遷移させる。
 *
 * 本関数はunused node countを変更しない。
 * また、挿入rangeのaddress orderと隣接rangeとの非重複性は検証しない。
 * これらは呼び出し元が挿入位置を決定する際に検証する。
 *
 * @param[in,out] range_free_list_ insert_node_を接続するRange Free List。
 * @param[in,out] insert_node_ range listへ接続するTRANSITIONING node。
 * @param[in] next_state_ 挿入後のnode state。FREEまたはALLOCATEDでなければならない。
 * @param[in,out] prev_ insert_node_の直前へ接続するnode。先頭へ挿入する場合はNULL。
 * @param[in,out] next_ insert_node_の直後へ接続するnode。末尾へ挿入する場合はNULL。
 *
 * @note
 * 呼び出し元は、range_free_list_に対するshallow validationまたは
 * deep validationを事前に完了させなければならない。
 *
 * @pre
 * insert_node_のstateはTRANSITIONINGであり、next_state_に対応する
 * 有効なrange情報を保持していなければならない。
 *
 * @pre
 * prev_とnext_はinsert_node_とは異なるnodeでなければならない。
 * 両方が非NULLの場合、prev_とnext_はrange list上で直接接続されていなければならない。
 *
 * @pre
 * insert_node_のrangeはaddress order上でprev_とnext_の間に位置し、
 * 隣接rangeと重複してはならない。
 *
 * @retval RANGE_FREE_LIST_SUCCESS
 * insert_node_の接続と指定stateへの遷移に成功した。
 *
 * @retval RANGE_FREE_LIST_INVALID_ARGUMENT
 * range_free_list_またはinsert_node_がNULLである。
 *
 * @retval RANGE_FREE_LIST_BAD_OPERATION
 * insert_node_がTRANSITIONINGではない、next_state_がFREE／ALLOCATED
 * ではない、またはprev_とnext_が同じnodeを指している。
 *
 * @retval RANGE_FREE_LIST_DATA_CORRUPTED
 * insert_node_、range list、list head、またはprev／nextの
 * 接続関係に整合性異常が検出された。
 *
 * @post
 * 成功時、insert_node_はprev_とnext_の間へ接続され、stateはnext_state_と一致する。
 * list headと隣接nodeの接続情報も挿入後の状態と整合する。
 *
 * @post
 * 失敗時、range list、insert_node_、list head、および
 * unused node countは呼び出し前から変更されない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_free_list_result_t node_insert_between(range_free_list_t* range_free_list_, node_t* insert_node_, node_state_t next_state_, node_t* prev_, node_t* next_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_insert_between", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(insert_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_insert_between", "insert_node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_TRANSITIONING == insert_node_->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert_between", "insert_node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_ALLOCATED == next_state_ || NODE_STATE_FREE == next_state_, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_insert_between", "next_state_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(insert_node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "node_insert_between", "insert_node_")

    if(NULL != prev_ && NULL != next_ && prev_ == next_) {
        ret = RANGE_FREE_LIST_BAD_OPERATION;
        ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into free list. reason=prev_ and next_ point to the same node, insert_offset=%zu, insert_block_size=%zu.", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size);
        goto cleanup;
    }

    if(NULL == prev_ && NULL == next_) {
        if(NULL != range_free_list_->free_block_list_head) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into free list. reason=insert position indicates empty list but free_block_list_head is not NULL, insert_offset=%zu, insert_block_size=%zu, head_offset=%zu, head_block_size=%zu.", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size, range_free_list_->free_block_list_head->offset, range_free_list_->free_block_list_head->block_size);
            goto cleanup;
        }
        range_free_list_->free_block_list_head = insert_node_;
    } else if(NULL == prev_ && NULL != next_) {
        if(next_ != range_free_list_->free_block_list_head || NULL != range_free_list_->free_block_list_head->prev) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into free list. reason=invalid head insert position, insert_offset=%zu, insert_block_size=%zu, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size, next_->offset, next_->block_size);
            goto cleanup;
        }
        next_->prev = insert_node_;
        range_free_list_->free_block_list_head = insert_node_;
    } else if(NULL != prev_ && NULL != next_) {
        if(prev_->next != next_ || prev_ != next_->prev) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into free list. reason=prev_ and next_ are not linked, insert_offset=%zu, insert_block_size=%zu, prev_offset=%zu, prev_block_size=%zu, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size, prev_->offset, prev_->block_size, next_->offset, next_->block_size);
            goto cleanup;
        }
        prev_->next = insert_node_;
        next_->prev = insert_node_;
    } else if(NULL != prev_ && NULL == next_) {
        if(NULL != prev_->next) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_insert_between(%s) - Failed to insert node into free list. reason=prev_ already has next for tail insertion, insert_offset=%zu, insert_block_size=%zu, prev_offset=%zu, prev_block_size=%zu.", rslt_to_str(ret), insert_node_->offset, insert_node_->block_size, prev_->offset, prev_->block_size);
            goto cleanup;
        }
        prev_->next = insert_node_;
    }

    if(NODE_STATE_FREE == next_state_) {
        set_node_to_free(insert_node_, prev_, next_);
    } else if(NODE_STATE_ALLOCATED == next_state_) {
        set_node_to_allocated(insert_node_, prev_, next_);
    }

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t node_adjacent_check_prev(const node_t* node_, bool* out_is_adjacent_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool is_adjacent = false;
    size_t prev_end = 0;

    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_adjacent_check_prev", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_is_adjacent_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_adjacent_check_prev", "out_is_adjacent_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_FREE == node_->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_adjacent_check_prev", "node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "node_adjacent_check_prev", "node_")

    if(NULL == node_->prev) {
        is_adjacent = false;
    } else {
        if(NODE_STATE_NOT_USED == node_->prev->node_state || NODE_STATE_TRANSITIONING == node_->prev->node_state) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_prev(%s) - Failed to check previous free block adjacency. reason=previous node is not CONNECTED, node_offset=%zu, node_block_size=%zu, prev_offset=%zu, prev_block_size=%zu, prev_state=%d.", rslt_to_str(ret), node_->offset, node_->block_size, node_->prev->offset, node_->prev->block_size, node_->prev->node_state);
            goto cleanup;
        }
        if(!node_is_valid(node_->prev)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_prev(%s) - Failed to check previous free block adjacency. reason=previous node is corrupted, node_offset=%zu, node_block_size=%zu, prev_offset=%zu, prev_block_size=%zu, prev_state=%d.", rslt_to_str(ret), node_->offset, node_->block_size, node_->prev->offset, node_->prev->block_size, node_->prev->node_state);
            goto cleanup;
        }
        if(node_->prev->next != node_) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_prev(%s) - Failed to check previous free block adjacency. reason=previous node is not linked to current node, node_offset=%zu, node_block_size=%zu, prev_offset=%zu, prev_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, node_->prev->offset, node_->prev->block_size);
            goto cleanup;
        }

        if(SIZE_MAX - node_->prev->block_size < node_->prev->offset) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("node_adjacent_check_prev(%s) - Failed to check previous free block adjacency. reason=previous free block end offset overflow, prev_offset=%zu, prev_block_size=%zu.", rslt_to_str(ret), node_->prev->offset, node_->prev->block_size);
            goto cleanup;
        }
        prev_end = node_->prev->block_size + node_->prev->offset;
        if(prev_end > node_->offset) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_prev(%s) - Failed to check previous free block adjacency. reason=previous free block overlaps current node, prev_offset=%zu, prev_block_size=%zu, prev_end=%zu, node_offset=%zu, node_block_size=%zu.", rslt_to_str(ret), node_->prev->offset, node_->prev->block_size, prev_end, node_->offset, node_->block_size);
            goto cleanup;
        }

        if(prev_end == node_->offset) {
            is_adjacent = true;
        } else {
            is_adjacent = false;
        }
    }

    *out_is_adjacent_ = is_adjacent;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t node_adjacent_check_next(const node_t* node_, bool* out_is_adjacent_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool is_adjacent = false;
    size_t node_end = 0;

    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_adjacent_check_next", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_is_adjacent_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_adjacent_check_next", "out_is_adjacent_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_FREE == node_->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "node_adjacent_check_next", "node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "node_adjacent_check_next", "node_")

    if(NULL == node_->next) {
        is_adjacent = false;
    } else {
        if(NODE_STATE_NOT_USED == node_->next->node_state || NODE_STATE_TRANSITIONING == node_->next->node_state) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_next(%s) - Failed to check next free block adjacency. reason=next node is not CONNECTED, node_offset=%zu, node_block_size=%zu, next_offset=%zu, next_block_size=%zu, next_state=%d.", rslt_to_str(ret), node_->offset, node_->block_size, node_->next->offset, node_->next->block_size, node_->next->node_state);
            goto cleanup;
        }
        if(!node_is_valid(node_->next)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_next(%s) - Failed to check next free block adjacency. reason=next node is corrupted, node_offset=%zu, node_block_size=%zu, next_offset=%zu, next_block_size=%zu, next_state=%d.", rslt_to_str(ret), node_->offset, node_->block_size, node_->next->offset, node_->next->block_size, node_->next->node_state);
            goto cleanup;
        }
        if(node_->next->prev != node_) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_next(%s) - Failed to check next free block adjacency. reason=next node is not linked to current node, node_offset=%zu, node_block_size=%zu, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, node_->next->offset, node_->next->block_size);
            goto cleanup;
        }

        if(SIZE_MAX - node_->block_size < node_->offset) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("node_adjacent_check_next(%s) - Failed to check next free block adjacency. reason=current free block end offset overflow, node_offset=%zu, node_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size);
            goto cleanup;
        }
        node_end = node_->block_size + node_->offset;
        if(node_end > node_->next->offset) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("node_adjacent_check_next(%s) - Failed to check next free block adjacency. reason=current free block overlaps next node, node_offset=%zu, node_block_size=%zu, node_end=%zu, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, node_end, node_->next->offset, node_->next->block_size);
            goto cleanup;
        }

        if(node_end == node_->next->offset) {
            is_adjacent = true;
        } else {
            is_adjacent = false;
        }
    }

    *out_is_adjacent_ = is_adjacent;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t merge_free_block(range_free_list_t* range_free_list_, node_t* node_, bool should_merge_prev_, bool should_merge_next_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    node_t* next = NULL;
    node_t* prev = NULL;
    size_t new_block_size = 0;
    size_t new_offset = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "merge_free_block", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "merge_free_block", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_FREE == node_->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "merge_free_block", "node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "merge_free_block", "node_")

    if(should_merge_prev_) {
        if(!node_is_valid(node_->prev)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=previous merge target is corrupted, node_offset=%zu, node_block_size=%zu, should_merge_prev=%d, should_merge_next=%d.", rslt_to_str(ret), node_->offset, node_->block_size, should_merge_prev_, should_merge_next_);
            goto cleanup;
        }
    }
    if(should_merge_next_) {
        if(!node_is_valid(node_->next)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=next merge target is corrupted, node_offset=%zu, node_block_size=%zu, should_merge_prev=%d, should_merge_next=%d.", rslt_to_str(ret), node_->offset, node_->block_size, should_merge_prev_, should_merge_next_);
            goto cleanup;
        }
    }

    if(should_merge_prev_ && should_merge_next_) {
        // 前後ノードマージ
        // node_->prevを残し、node_とnode_->nextを削除
        if(NODE_STATE_FREE != node_->prev->node_state || NODE_STATE_FREE != node_->next->node_state) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=merge targets are not CONNECTED, node_offset=%zu, node_block_size=%zu, prev_offset=%zu, prev_block_size=%zu, prev_state=%d, next_offset=%zu, next_block_size=%zu, next_state=%d.", rslt_to_str(ret), node_->offset, node_->block_size, node_->prev->offset, node_->prev->block_size, node_->prev->node_state, node_->next->offset, node_->next->block_size, node_->next->node_state);
            goto cleanup;
        }
        if(!node_is_valid(node_->prev) || !node_is_valid(node_->next)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=merge target node is corrupted, node_offset=%zu, node_block_size=%zu, prev_offset=%zu, prev_block_size=%zu, prev_state=%d, next_offset=%zu, next_block_size=%zu, next_state=%d.", rslt_to_str(ret), node_->offset, node_->block_size, node_->prev->offset, node_->prev->block_size, node_->prev->node_state, node_->next->offset, node_->next->block_size, node_->next->node_state);
            goto cleanup;
        }
        prev = node_->prev;
        next = node_->next;

        if((SIZE_MAX - node_->block_size) < prev->block_size) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=block size overflow while merging previous and current blocks, prev_offset=%zu, prev_block_size=%zu, node_offset=%zu, node_block_size=%zu.", rslt_to_str(ret), prev->offset, prev->block_size, node_->offset, node_->block_size);
            goto cleanup;
        }
        new_block_size = prev->block_size + node_->block_size;

        if((SIZE_MAX - next->block_size) < new_block_size) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=block size overflow while merging next block, merged_block_size=%zu, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), new_block_size, next->offset, next->block_size);
            goto cleanup;
        }
        new_block_size += next->block_size;

        ret = node_remove(range_free_list_, next);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=failed to remove next node, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), next->offset, next->block_size);
            goto cleanup;
        }
        ret = node_release(range_free_list_, next);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=failed to release next node, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), next->offset, next->block_size);
            goto cleanup;
        }

        ret = node_remove(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=failed to remove current node, node_offset=%zu, node_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size);
            goto cleanup;
        }
        ret = node_release(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=failed to release current node, node_offset=%zu, node_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size);
            goto cleanup;
        }

        prev->block_size = new_block_size;
    } else if(should_merge_prev_ && !should_merge_next_) {
        // 前のノードのみマージ
        prev = node_->prev;
        next = node_->next;
        if((SIZE_MAX - prev->block_size) < node_->block_size) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=block size overflow while merging previous block, prev_offset=%zu, prev_block_size=%zu, node_offset=%zu, node_block_size=%zu.", rslt_to_str(ret), prev->offset, prev->block_size, node_->offset, node_->block_size);
            goto cleanup;
        }
        new_block_size = node_->block_size + prev->block_size;

        ret = node_remove(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=failed to remove current node for previous merge, node_offset=%zu, node_block_size=%zu, prev_offset=%zu, prev_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, prev->offset, prev->block_size);
            goto cleanup;
        }
        ret = node_release(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=failed to release current node for previous merge, node_offset=%zu, node_block_size=%zu, prev_offset=%zu, prev_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, prev->offset, prev->block_size);
            goto cleanup;
        }

        prev->block_size = new_block_size;
    } else if(!should_merge_prev_ && should_merge_next_) {
        prev = node_->prev;
        next = node_->next;

        if((SIZE_MAX - next->block_size) < node_->block_size) {
            ret = RANGE_FREE_LIST_OVERFLOW;
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=block size overflow while merging next block, node_offset=%zu, node_block_size=%zu, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, next->offset, next->block_size);
            goto cleanup;
        }
        new_block_size = node_->block_size + next->block_size;
        new_offset = node_->offset;

        ret = node_remove(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=failed to remove current node for next merge, node_offset=%zu, node_block_size=%zu, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, next->offset, next->block_size);
            goto cleanup;
        }
        ret = node_release(range_free_list_, node_);
        if(RANGE_FREE_LIST_SUCCESS != ret) {
            ERROR_MESSAGE("merge_free_block(%s) - Failed to merge adjacent free blocks. reason=failed to release current node for next merge, node_offset=%zu, node_block_size=%zu, next_offset=%zu, next_block_size=%zu.", rslt_to_str(ret), node_->offset, node_->block_size, next->offset, next->block_size);
            goto cleanup;
        }

        next->offset = new_offset;
        next->block_size = new_block_size;
    }

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief nodeが所属するnode pool indexを取得する
 *
 * @details
 * range_free_list_が所有するnode poolを先頭から線形探索し、
 * node_と同じポインタを保持する要素のindexを取得する。
 *
 * node_がnode poolに所属することをポインタidentityによって
 * 確認した後、node_is_valid()によってnode単体の局所的不変条件を検証する。
 *
 * 本関数はRange Free List、node pool、およびnodeの状態を変更しない。
 *
 * @param[in] range_free_list_ node poolを所有するRange Free List。
 * @param[in] node_ indexを取得するnode。
 * @param[out] out_index_ node_が格納されているnode pool indexの出力先。
 *
 * @pre
 * range_free_list_に対するshallow validationまたはdeep validationが、
 * 呼び出し元によって完了していなければならない。
 *
 * @retval RANGE_FREE_LIST_SUCCESS
 * node_の所属確認とindexの取得に成功した。
 *
 * @retval RANGE_FREE_LIST_INVALID_ARGUMENT
 * range_free_list_、node_、またはout_index_がNULLである。
 *
 * @retval RANGE_FREE_LIST_DATA_CORRUPTED
 * node_がnode poolに所属していない、または所属確認後のnode_が局所的不変条件を満たしていない。
 *
 * @post
 * 成功時、*out_index_はnode_が格納されているnode pool indexと一致する。
 *
 * @post
 * 失敗時、*out_index_は変更されない。
 *
 * @note
 * 計算量はmax node countに対してO(n)である。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static range_free_list_result_t node_pool_find_index(const range_free_list_t* range_free_list_, const node_t* node_, size_t* out_index_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool found = false;
    size_t tmp_index = 0;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_pool_find_index", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_pool_find_index", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_index_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "node_pool_find_index", "out_index_")

    for(size_t i = 0; i != range_free_list_->max_node_count; ++i) {
        if(&range_free_list_->node_pool[i] == node_) {
            tmp_index = i;
            found = true;
            break;
        }
    }
    if(!found) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_pool_find_index(%s) - Failed to find node index. reason=target node does not belong to node pool, max_node_count=%zu.", rslt_to_str(ret), range_free_list_->max_node_count);
        goto cleanup;
    }
    if(!node_is_valid(node_)) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("node_pool_find_index(%s) - Failed to find node index. reason=target node is locally invalid, node_index=%zu, node_state=%d, offset=%zu, block_size=%zu.", rslt_to_str(ret), tmp_index, node_->node_state, node_->offset, node_->block_size);
        goto cleanup;
    }
    *out_index_ = tmp_index;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t allocation_resolve_node(range_free_list_t* range_free_list_, const range_allocation_t* allocation_info_, node_t** out_node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    node_t* tmp_node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(allocation_info_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_", "allocation_info_")
    IF_ARG_NULL_GOTO_CLEANUP(out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_", "out_node_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "range_free_list_", "*out_node_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != range_free_list_->allocation_count, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "range_free_list_", "range_free_list_->allocation_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != allocation_info_->allocated_size, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "range_free_list_", "allocation_info_->allocated_size")

    if(range_free_list_ != allocation_info_->owner) {
        ret = RANGE_FREE_LIST_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - allocation_resolve_node failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(range_free_list_->max_node_count <= allocation_info_->node_index) {
        ret = RANGE_FREE_LIST_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - allocation_resolve_node failed.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp_node = &range_free_list_->node_pool[allocation_info_->node_index];

    if(!node_is_valid(tmp_node)) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("allocation_resolve_node(%s) - allocation_resolve_node failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(NODE_STATE_ALLOCATED != tmp_node->node_state) {
        ret = RANGE_FREE_LIST_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - allocation_resolve_node failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(tmp_node->block_size != allocation_info_->allocated_size) {
        ret = RANGE_FREE_LIST_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - allocation_resolve_node failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(tmp_node->offset != allocation_info_->offset) {
        ret = RANGE_FREE_LIST_BAD_OPERATION;
        ERROR_MESSAGE("allocation_resolve_node(%s) - allocation_resolve_node failed.", rslt_to_str(ret));
        goto cleanup;
    }

    *out_node_ = tmp_node;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// エラー処理についてはスタイルガイド確定後、充実させる
static range_free_list_result_t free_merge_plan_get(const range_free_list_t* range_free_list_, const node_t* node_, bool* out_should_merge_prev_, bool* out_should_merge_next_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    bool tmp_should_merge_prev = false;
    bool tmp_should_merge_next = false;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_merge_plan_get", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_merge_plan_get", "node_")
    IF_ARG_NULL_GOTO_CLEANUP(out_should_merge_prev_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_merge_plan_get", "out_should_merge_prev_")
    IF_ARG_NULL_GOTO_CLEANUP(out_should_merge_next_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_merge_plan_get", "out_should_merge_next_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "free_merge_plan_get", "node_")

    // 前方ノード検証
    if(NULL != node_->prev && NODE_STATE_FREE == node_->prev->node_state) {
        if(!node_is_valid(node_->prev)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("free_merge_plan_get(%s) - free_merge_plan_get failed.", rslt_to_str(ret));
            goto cleanup;
        }
        tmp_should_merge_prev = true;
    }

    // 後方ノード検証
    if(NULL != node_->next && NODE_STATE_FREE == node_->next->node_state) {
        if(!node_is_valid(node_->next)) {
            ret = RANGE_FREE_LIST_DATA_CORRUPTED;
            ERROR_MESSAGE("free_merge_plan_get(%s) - free_merge_plan_get failed.", rslt_to_str(ret));
            goto cleanup;
        }
        tmp_should_merge_next = true;
    }

    *out_should_merge_prev_ = tmp_should_merge_prev;
    *out_should_merge_next_ = tmp_should_merge_next;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// エラー処理についてはスタイルガイド確定後、充実させる
static range_free_list_result_t free_from_node(range_free_list_t* range_free_list_, node_t* node_, bool should_merge_prev_, bool should_merge_next_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_from_node", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_from_node", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_from_node", "node_")

    if(!should_merge_prev_ && !should_merge_next_) {
        ret = free_node_without_merge(node_);
    } else if(!should_merge_prev_ && should_merge_next_) {
        ret = free_node_merge_next(range_free_list_, node_);
    } else if(should_merge_prev_ && !should_merge_next_) {
        ret = free_node_merge_prev(range_free_list_, node_);
    } else {
        ret = free_node_merge_prev_next(range_free_list_, node_);
    }

cleanup:
    return ret;
}

static range_free_list_result_t free_node_without_merge(node_t* node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_node_without_merge", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_node_without_merge", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_ALLOCATED == node_->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "free_node_without_merge", "node_->node_state")

    node_->node_state = NODE_STATE_FREE;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

static range_free_list_result_t free_node_merge_prev(range_free_list_t* range_free_list_, node_t* node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    size_t tmp_offset = 0;
    size_t new_block_size = 0;
    node_t* tmp_node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_node_merge_prev", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_node_merge_prev", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "free_node_merge_prev", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_ALLOCATED == node_->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "free_node_merge_prev", "NODE_STATE_ALLOCATED == node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_->prev), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "free_node_merge_prev", "node_->prev")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_FREE == node_->prev->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "free_node_merge_prev", "node_->prev->node_state")

    if(node_ != node_->prev->next) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX - node_->prev->offset) < node_->prev->block_size) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    tmp_offset = node_->prev->offset + node_->prev->block_size;
    if(tmp_offset != node_->offset) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX - node_->prev->block_size) < node_->block_size) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    new_block_size = node_->prev->block_size + node_->block_size;
    tmp_node = node_->prev;

    ret = node_remove(range_free_list_, node_);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = node_release(range_free_list_, node_);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp_node->block_size = new_block_size;
    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    // NOTE: この関数が失敗するのはrange_free_listのデータが破損している場合なのでロールバック不可
    return ret;
}

static range_free_list_result_t free_node_merge_next(range_free_list_t* range_free_list_, node_t* node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    size_t tmp_offset = 0;
    size_t new_block_size = 0;
    node_t* tmp_next = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_node_merge_next", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_node_merge_next", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "free_node_merge_next", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_ALLOCATED == node_->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "free_node_merge_next", "NODE_STATE_ALLOCATED == node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_->next), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "free_node_merge_next", "node_->next")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_FREE == node_->next->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "free_node_merge_next", "node_->next->node_state")

    if(node_ != node_->next->prev) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX - node_->offset) < node_->block_size) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    tmp_offset = node_->offset + node_->block_size;
    if(tmp_offset != node_->next->offset) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX - node_->next->block_size) < node_->block_size) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    new_block_size = node_->next->block_size + node_->block_size;

    tmp_next = node_->next;
    ret = node_remove(range_free_list_, tmp_next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = node_release(range_free_list_, tmp_next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }

    node_->block_size = new_block_size;
    node_->node_state = NODE_STATE_FREE;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    // NOTE: この関数が失敗するのはrange_free_listのデータが破損している場合なのでロールバック不可
    return ret;
}

static range_free_list_result_t free_node_merge_prev_next(range_free_list_t* range_free_list_, node_t* node_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    size_t tmp_offset = 0;
    size_t new_block_size = 0;

    node_t* tmp_node_prev = NULL;
    node_t* tmp_node_next = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(range_free_list_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_node_merge_prev_next", "range_free_list_")
    IF_ARG_NULL_GOTO_CLEANUP(node_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "free_node_merge_prev_next", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "free_node_merge_prev_next", "node_")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_ALLOCATED == node_->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "free_node_merge_prev_next", "NODE_STATE_ALLOCATED == node_->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_->prev), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "free_node_merge_prev_next", "node_->prev")
    IF_ARG_FALSE_GOTO_CLEANUP(node_is_valid(node_->next), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "free_node_merge_prev_next", "node_->next")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_FREE == node_->prev->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "free_node_merge_prev_next", "node_->prev->node_state")
    IF_ARG_FALSE_GOTO_CLEANUP(NODE_STATE_FREE == node_->next->node_state, ret, RANGE_FREE_LIST_BAD_OPERATION, rslt_to_str(RANGE_FREE_LIST_BAD_OPERATION), "free_node_merge_prev_next", "node_->next->node_state")

    if(node_ != node_->next->prev) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev_next failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(node_ != node_->prev->next) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev_next failed.", rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX - node_->prev->block_size) < node_->prev->offset) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    tmp_offset = node_->prev->offset + node_->prev->block_size;
    if(tmp_offset != node_->offset) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev_next failed.", rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX - node_->block_size) < node_->offset) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    tmp_offset = node_->offset + node_->block_size;
    if(tmp_offset != node_->next->offset) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev_next failed.", rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX - node_->prev->block_size) < node_->block_size) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    new_block_size = node_->prev->block_size + node_->block_size;

    if((SIZE_MAX - node_->next->block_size) < new_block_size) {
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev failed.", rslt_to_str(ret));
        goto cleanup;
    }
    new_block_size += node_->next->block_size;

    tmp_node_prev = node_->prev;
    tmp_node_next = node_->next;

    ret = node_remove(range_free_list_, node_);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev_next failed.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = node_release(range_free_list_, node_);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev_next failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = node_remove(range_free_list_, tmp_node_next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev_next failed.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = node_release(range_free_list_, tmp_node_next);
    if(RANGE_FREE_LIST_SUCCESS != ret) {
        ret = RANGE_FREE_LIST_DATA_CORRUPTED;
        ERROR_MESSAGE("free_node_merge_prev_next(%s) - free_node_merge_prev_next failed.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp_node_prev->block_size = new_block_size;

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief nodeを正規化されたNOT_USED状態へ設定する
 *
 * @details
 * 対象nodeが保持するrange情報と接続情報を消去し、
 * node pool内の未使用nodeとして再利用可能な状態へ設定する。
 *
 * NULL以外のtarget_に対して、次の状態を設定する。
 *
 * - stateはNOT_USED
 * - offsetは0
 * - block sizeは0
 * - prevはNULL
 * - nextはNULL
 *
 * 本関数は遷移元stateを検証しない。
 * また、range listの隣接node、list head、unused node countは更新しない。
 * nodeの切断とカウンタ更新は呼び出し元の責務とする。
 *
 * @param[in,out] target_ NOT_USED状態へ設定するnode。NULLの場合は何も行わない。
 *
 * @warning
 * range listへ接続中のnodeに直接使用してはならない。
 * 呼び出し元は、必要なlist接続の更新を事前に完了させなければならない。
 *
 * @post
 * target_がNULLでない場合、target_は正規化されたNOT_USED状態になる。
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
 * @brief nodeをTRANSITIONING状態へ設定する
 *
 * @details
 * 対象nodeへ指定されたrange情報を設定し、接続情報を消去したうえで、
 * private操作中であることを示すTRANSITIONING状態へ遷移させる。
 *
 * NULL以外のtarget_に対して、次の状態を設定する。
 *
 * - stateはTRANSITIONING
 * - offsetはoffset_
 * - block sizeはblock_size_
 * - prevはNULL
 * - nextはNULL
 *
 * 本関数は遷移元state、offset_、block_size_を検証しない。
 * また、range listの隣接node、list head、unused node countは更新しない。
 * これらの検証と更新は呼び出し元の責務とする。
 *
 * @param[in,out] target_ TRANSITIONING状態へ設定するnode。NULLの場合は何も行わない。
 * @param[in] offset_ target_へ設定するrange開始offset。
 * @param[in] block_size_ target_へ設定するrangeサイズ。
 *
 * @pre
 * target_がNULLでない場合、呼び出し元は遷移元stateと
 * offset_およびblock_size_の妥当性を検証済みでなければならない。
 *
 * @pre
 * target_がrange listへ接続されている場合、呼び出し元は
 * 隣接nodeとlist headの更新を完了していなければならない。
 *
 * @post
 * target_がNULLでない場合、target_は指定されたrange情報を保持する
 * TRANSITIONING状態となり、prevとnextはNULLになる。
 *
 * @warning
 * TRANSITIONINGはprivate操作途中の一時的な状態である。
 * 呼び出し元はpublic APIから戻る前に、target_をNOT_USED、FREE、
 * ALLOCATEDのいずれかの安定状態へ遷移させなければならない。
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
 * 対象nodeへ指定された接続情報を設定し、range list上の
 * allocation可能なFREE rangeを表す安定状態へ遷移させる。
 *
 * NULL以外のtarget_に対して、次の状態を設定する。
 *
 * - stateはFREE
 * - prevはprev_
 * - nextはnext_
 *
 * offsetとblock sizeは変更しない。
 *
 * 本関数は遷移元state、target_が保持するrange情報、prev_、next_の接続関係を検証しない。
 * また、隣接node、list head、unused node countは更新しない。
 * これらの検証と更新は呼び出し元の責務とする。
 *
 * @param[in,out] target_ FREE状態へ設定するnode。NULLの場合は何も行わない。
 * @param[in] prev_ target_の直前へ接続するnode。先頭の場合はNULL。
 * @param[in] next_ target_の直後へ接続するnode。末尾の場合はNULL。
 *
 * @pre
 * target_がNULLでない場合、呼び出し元は遷移元stateが、
 * createによる初期化時のNOT_USED、またはTRANSITIONING、ALLOCATEDのいずれかであることを検証済みでなければならない。
 *
 * @pre
 * target_のoffsetとblock sizeは、有効なFREE rangeを表していなければならない。
 *
 * @pre
 * 呼び出し元は、prev_、next_、list headを含むrange list全体が、
 * target_の接続後に整合することを保証しなければならない。
 *
 * @post
 * target_がNULLでない場合、target_はoffsetとblock sizeを保持したまま
 * FREE状態となり、prevとnextは指定されたnodeと一致する。
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
 * 対象nodeへ指定された接続情報を設定し、range list上の
 * live allocationが所有するALLOCATED rangeを表す安定状態へ遷移させる。
 *
 * NULL以外のtarget_に対して、次の状態を設定する。
 *
 * - stateはALLOCATED
 * - prevはprev_
 * - nextはnext_
 *
 * offsetとblock sizeは変更しない。
 *
 * 本関数は遷移元state、target_が保持するrange情報、prev_、next_の接続関係を検証しない。
 * また、隣接node、list head、unused node countは更新しない。
 * これらの検証と更新は呼び出し元の責務とする。
 *
 * @param[in,out] target_ ALLOCATED状態へ設定するnode。NULLの場合は何も行わない。
 * @param[in] prev_ target_の直前へ接続するnode。先頭の場合はNULL。
 * @param[in] next_ target_の直後へ接続するnode。末尾の場合はNULL。
 *
 * @pre
 * target_がNULLでない場合、呼び出し元は遷移元stateが
 * TRANSITIONINGまたはFREEであることを検証済みでなければならない。
 *
 * @pre
 * target_のoffsetとblock sizeは、有効なALLOCATED rangeを表していなければならない。
 *
 * @pre
 * 呼び出し元は、prev_、next_、list headを含むrange list全体が、
 * target_の接続後に整合することを保証しなければならない。
 *
 * @post
 * target_がNULLでない場合、target_はoffsetとblock sizeを保持したまま
 * ALLOCATED状態となり、prevとnextは指定されたnodeと一致する。
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
 * @brief Range Free Listの内部状態、node pool、およびrange listの整合性を検証する
 *
 * @details
 * range_free_list_のshallowな内部フィールドを検証した後、
 * FREE／ALLOCATED nodeで構成されるrange listを先頭から走査する。
 *
 * range listについて、次の条件を検証する。
 *
 * - list headが存在し、先頭nodeのoffsetが0
 * - list上の各nodeのblock sizeが0ではなく、prevおよびnextが自分自身を参照していない
 * - list上の各node stateがFREEまたはALLOCATED
 * - 先頭nodeのprevがNULLであり、2番目以降の各nodeのprevが、nextをたどる走査で直前に処理したnodeを参照している
 * - 各nodeのoffsetが直前nodeの終端と一致する
 * - 各rangeがmemory poolの範囲内にあり、終端計算がoverflowしない
 * - 各nodeのoffsetがbase alignment境界にある
 * - ALLOCATED nodeのblock sizeがbase alignmentの倍数
 * - 隣接する2つのnodeが両方FREEではない
 * - listの走査回数がmax node countを超えない
 * - range listの末尾がmemory poolの末尾と一致する
 * - list上のnode数とunused node countの合計がmax node countと一致する
 * - list上の各nodeが、このRange Free Listのnode poolに所属している
 *
 * expected offsetを先頭の0から各nodeの終端へ更新することで、
 * range間のgap、overlap、およびmemory pool内の未管理rangeを検出する。
 *
 * node poolについて、次の条件を検証する。
 *
 * - 全nodeがstateに対応する局所的不変条件を満たしている
 * - public API境界で許可されないTRANSITIONING nodeが存在しない
 * - NOT_USED node数がunused node countと一致する
 * - FREE／ALLOCATED node数がrange listの走査node数と一致する
 * - FREE、ALLOCATED、NOT_USEDの各node数の合計がmax node countと一致する
 *
 * range list上の全nodeがnode poolに所属することと、
 * node pool内のFREE／ALLOCATED node数がrange listの走査node数と
 * 一致することを照合し、listから切り離された安定状態のnodeが
 * 存在しないことを検証する。
 *
 * @param[in] range_free_list_ 検証するRange Free List。NULLの場合は不正と判定する。
 *
 * @retval true shallowな内部状態、node pool、およびrange listが整合している。
 * @retval false 内部フィールド、node pool、node state、list接続、range、alignment、またはnode数が不正である。
 *
 * @note 本関数はrange_free_list_およびnodeを変更しない。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が実装との整合性を確認・修正した。
 */
static bool range_free_list_is_valid(const range_free_list_t* range_free_list_) {
    node_t* node = NULL;
    node_t* prev = NULL;
    size_t loop_count = 0;
    size_t expected_offset = 0;
    size_t expected_total_allocated_size = 0;
    bool found = false;

    if(NULL == range_free_list_) {
        return false;
    }
    if(!range_free_list_is_valid_shallow(range_free_list_)) {
        return false;
    }

    if(NULL == range_free_list_->free_block_list_head) {
        return false;
    }
    node = range_free_list_->free_block_list_head;
    // NOTE: 以下はfree_listの動作実績が増えたらDEBUG_BUILD, TEST_BUILDのみで動かす
    while(NULL != node) {
        if(loop_count >= range_free_list_->max_node_count) {
            return false;
        }
        found = false;
        for(size_t i = 0; i != range_free_list_->max_node_count; ++i) {
            if(&range_free_list_->node_pool[i] == node) {
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
        if(!range_is_valid(range_free_list_, node->offset, node->block_size)) {
            return false;
        }
        if(0 != (node->offset % range_free_list_->base_align)) {
            return false;
        }
        if(NODE_STATE_ALLOCATED == node->node_state && 0 != (node->block_size % range_free_list_->base_align)) {
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
        expected_offset = node->offset + node->block_size;  // range_is_validでOVERFLOWチェック済み
        prev = node;
        node = node->next;
        loop_count++;
        found = false;
    }
    if(expected_offset != range_free_list_->memory_pool_size) { // memory_pool全体がALLOCATED / FREEノードで隙間がないため、末尾は必ずmemory_pool_sizeに等しい
        return false;
    }
    if(expected_total_allocated_size != range_free_list_->total_allocated_size) {
        return false;
    }

    size_t allocated_count = 0;
    size_t free_count = 0;
    size_t unused_count = 0;
    for(size_t i = 0; i != range_free_list_->max_node_count; ++i) {
        if(!node_is_valid(&range_free_list_->node_pool[i])) {
            return false;
        }
        if(NODE_STATE_TRANSITIONING == range_free_list_->node_pool[i].node_state) {
            return false;
        }
        if(NODE_STATE_ALLOCATED == range_free_list_->node_pool[i].node_state) {
            allocated_count++;
        }
        if(NODE_STATE_FREE == range_free_list_->node_pool[i].node_state) {
            free_count++;
        }
        if(NODE_STATE_NOT_USED == range_free_list_->node_pool[i].node_state) {
            unused_count++;
        }
    }
    if(allocated_count != range_free_list_->allocation_count) {
        return false;
    }
    if(range_free_list_->unused_node_count != unused_count) {
        return false;
    }
    if((free_count + allocated_count) != loop_count) {
        return false;
    }
    if((free_count + allocated_count + unused_count) != range_free_list_->max_node_count) {
        return false;
    }
    return true;
}

static bool range_free_list_is_valid_shallow(const range_free_list_t* range_free_list_) {
    if(NULL == range_free_list_) {
        return false;
    }
    if(0 == range_free_list_->base_align || !IS_POWER_OF_TWO(range_free_list_->base_align)) {
        return false;
    }
    if(0 == range_free_list_->max_allocation_count) {
        return false;
    }
    if(range_free_list_->total_allocated_size > range_free_list_->memory_pool_size) {
        return false;
    }
    if(((SIZE_MAX - 1) / 2) < range_free_list_->max_allocation_count) {
        return false;
    }
    if((range_free_list_->max_allocation_count * 2 + 1) != range_free_list_->max_node_count) {
        return false;
    }
    if(range_free_list_->allocation_count > range_free_list_->max_allocation_count) {
        return false;
    }
    if(range_free_list_->unused_node_count > range_free_list_->max_node_count) {
        return false;
    }
    if(0 == range_free_list_->memory_pool_size) {
        return false;
    }
    if(NULL == range_free_list_->node_pool) {
        return false;
    }
    return true;
}

/**
 * @brief node単体のstateと局所フィールドの整合性を検証する
 *
 * @details
 * node_のnode_stateに応じて、node単体で判断可能な次の不変条件を検証する。
 *
 * - FREEまたはALLOCATED
 *   - block sizeが0ではない
 *   - prevおよびnextが自分自身を参照していない
 * - TRANSITIONING
 *   - block sizeが0ではない
 *   - prevおよびnextがNULLであり、range listから切り離されている
 * - NOT_USED
 *   - offsetおよびblock sizeが0
 *   - prevおよびnextがNULL
 *
 * node_stateが定義済みのいずれの状態にも該当しない場合は、不正なnodeとして扱う。
 *
 * 本関数はnode単体の局所的な整合性のみを検証する。
 * node poolへの所属、range listの双方向接続、rangeの順序や重複、
 * memory poolの範囲、alignment、およびnode数の整合性は検証しない。
 *
 * TRANSITIONINGは本関数では有効な局所状態として扱われるが、
 * public APIの入口および出口で許可される安定状態ではない。
 *
 * @param[in] node_ 検証するnode。NULLの場合は不正と判定する。
 *
 * @retval true node単体のstateと局所フィールドが整合している。
 * @retval false node_がNULL、stateが未定義、またはstateに対応する局所フィールドの条件を満たしていない。
 *
 * @note 本関数はnode_を変更しない。
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

static range_free_list_result_t align_up(size_t base_align_, size_t required_size_, size_t* out_allocation_size_) {
    range_free_list_result_t ret = RANGE_FREE_LIST_INVALID_ARGUMENT;

    size_t padding = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_allocation_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "align_up", "out_allocation_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != base_align_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "align_up", "base_align_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != required_size_, ret, RANGE_FREE_LIST_INVALID_ARGUMENT, rslt_to_str(RANGE_FREE_LIST_INVALID_ARGUMENT), "align_up", "required_size_")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(base_align_), ret, RANGE_FREE_LIST_DATA_CORRUPTED, rslt_to_str(RANGE_FREE_LIST_DATA_CORRUPTED), "align_up", "base_align_")

    // このrange_free_listはcreate時に指定されたbase_align固定で範囲を管理する。
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
        ret = RANGE_FREE_LIST_OVERFLOW;
        ERROR_MESSAGE("align_up(%s) - Failed to align allocation size. reason=required_size plus padding overflows, base_align=%zu, required_size=%zu, padding=%zu.", rslt_to_str(ret), base_align_, required_size_, padding);
        goto cleanup;
    }
    *out_allocation_size_ = required_size_ + padding;    // 割り当て領域の後ろにpaddingを追加し、offsetは常にbase_alignに整列されるようにする

    ret = RANGE_FREE_LIST_SUCCESS;

cleanup:
    return ret;
}

// left_node(prev)の領域と、right_node(next)の領域が干渉していないかをチェックする
// (left_node_offset_ + left_node_block_size_) <= right_node_offset_であることをチェックする
static bool is_non_overlap(size_t left_node_offset_, size_t left_node_block_size_, size_t right_node_offset_) {
    if(left_node_offset_ > right_node_offset_) {
        return false;
    } else if((right_node_offset_ - left_node_offset_) < left_node_block_size_) {
        return false;
    } else {
        return true;
    }
}

static bool range_is_valid(const range_free_list_t* range_free_list_, size_t offset_, size_t block_size_) {
    if(NULL == range_free_list_) {
        return false;
    }
    if(0 == block_size_) {
        return false;
    }
    if((SIZE_MAX - block_size_) < offset_) {
        return false;
    }
    if(range_free_list_->memory_pool_size < (offset_ + block_size_)) {
        return false;
    }
    return true;
}

static bool range_align_is_valid(size_t base_align_, size_t offset_, size_t block_size_) {
    if(0 == base_align_ || !IS_POWER_OF_TWO(base_align_)) {
        return false;
    }
    if(0 != (offset_ % base_align_)) {
        return false;
    }
    if(0 == block_size_ || 0 != (block_size_ % base_align_)) {
        return false;
    }
    return true;
}

static const char* rslt_to_str(range_free_list_result_t rslt_) {
    switch(rslt_) {
    case RANGE_FREE_LIST_SUCCESS:
        return s_rslt_str_success;
    case RANGE_FREE_LIST_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case RANGE_FREE_LIST_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case RANGE_FREE_LIST_NO_MEMORY:
        return s_rslt_str_no_memory;
    case RANGE_FREE_LIST_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case RANGE_FREE_LIST_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case RANGE_FREE_LIST_OVERFLOW:
        return s_rslt_str_overflow;
    case RANGE_FREE_LIST_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

static range_free_list_result_t rslt_convert_choco_memory(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return RANGE_FREE_LIST_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return RANGE_FREE_LIST_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return RANGE_FREE_LIST_LIMIT_EXCEEDED;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return RANGE_FREE_LIST_BAD_OPERATION;
    case MEMORY_SYSTEM_NO_MEMORY:
        return RANGE_FREE_LIST_NO_MEMORY;
    }
}
