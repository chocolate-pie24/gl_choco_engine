#include "engine/core/memory/free_list_allocator.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>
#include <stdint.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/memory_utility.h"

#include "engine/core/memory/memory_tag.h"

static const char* const s_rslt_str_success = "SUCCESS";
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_rslt_str_no_memory = "NO_MEMORY";
static const char* const s_rslt_str_overflow = "OVERFLOW";
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";

// for allocate
static free_list_allocator_result_t allocation_block_size_calc(size_t payload_offset_, size_t allocation_size_, size_t* out_required_block_size_);
static free_list_allocator_result_t free_block_find_first_fit(const free_list_allocator_t* free_list_allocator_, size_t required_block_size_, free_list_block_header_t** out_free_block_);
static free_list_allocator_result_t free_block_split(size_t minimum_block_size_, free_list_block_header_t* free_block_, size_t required_block_size_);
static free_list_allocator_result_t free_block_allocate(size_t payload_offset_, free_list_block_header_t* free_block_, size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_);

// for free
static bool allocation_ptr_is_valid(const free_list_allocator_t* free_list_allocator_, const void* ptr_);
static free_list_allocator_result_t allocated_block_free(free_list_block_header_t* allocation_block_);
static free_list_allocator_result_t free_block_merge_next(free_list_block_header_t* free_block_);
static free_list_allocator_result_t free_block_coalesce(free_list_block_header_t* free_block_);

// utility
static const char* rslt_to_str(free_list_allocator_result_t rslt_);

// validator
static bool is_valid_shallow(const free_list_allocator_t* free_list_allocator_);
static bool free_list_block_state_is_valid(free_list_block_state_t state_);

free_list_allocator_result_t free_list_allocator_initialize(size_t memory_pool_size_, void* memory_pool_, free_list_allocator_t* free_list_allocator_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t payload_offset = 0;
    size_t minimum_block_size = 0;
    bool is_aligned = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(memory_pool_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_initialize", "memory_pool_")
    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_initialize", "free_list_allocator_")
    if(0 == memory_pool_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Provided memory_pool_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!memory_utility_is_aligned((uintptr_t)(memory_pool_), alignof(max_align_t), &is_aligned)) {
        ret = FREE_LIST_ALLOCATOR_UNDEFINED_ERROR;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - memory_utility_is_aligned failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!is_aligned) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Provided memory_pool_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if((UINTPTR_MAX - (uintptr_t)memory_pool_) < memory_pool_size_) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Address range overflow.", rslt_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    if(!memory_utility_align_up(sizeof(free_list_block_header_t), alignof(max_align_t), &payload_offset)) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW; // このケースでmemory_utility_align_upが失敗しるのはOVERFLOWのみ
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - memory_utility_align_up failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - payload_offset) < alignof(max_align_t)) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW; // このケースでmemory_utility_align_upが失敗しるのはOVERFLOWのみ
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - minimum_block_size overflow.", rslt_to_str(ret));
        goto cleanup;
    }
    minimum_block_size = payload_offset + alignof(max_align_t);
    if(memory_pool_size_ < minimum_block_size) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Provided memory_pool_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    free_list_allocator_->memory_pool = memory_pool_;
    free_list_allocator_->memory_pool_size = memory_pool_size_;
    free_list_allocator_->payload_offset = payload_offset;
    free_list_allocator_->minimum_block_size = minimum_block_size;

    free_list_allocator_->head = (free_list_block_header_t*)memory_pool_;
    free_list_allocator_->head->block_size = memory_pool_size_;
    free_list_allocator_->head->allocation_size = 0;
    free_list_allocator_->head->block_state = FREE_LIST_BLOCK_STATE_FREE;
    free_list_allocator_->head->next = NULL;
    free_list_allocator_->head->prev = NULL;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Postcondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

void free_list_allocator_deinitialize(free_list_allocator_t* free_list_allocator_) {
    if(NULL == free_list_allocator_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ERROR_MESSAGE("free_list_allocator_deinitialize(%s) - Precondition validation failed for 'free_list_allocator_'.", rslt_to_str(FREE_LIST_ALLOCATOR_DATA_CORRUPTED));
        return;
    }
#endif

    free_list_allocator_->head = NULL;
    free_list_allocator_->payload_offset = 0;
    free_list_allocator_->memory_pool = NULL;
    free_list_allocator_->memory_pool_size = 0;
}

free_list_allocator_result_t free_list_allocator_allocate(free_list_allocator_t* free_list_allocator_, size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t new_block_size = 0;
    free_list_block_header_t* allocation_block = NULL;
    void* tmp_ptr = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_allocate", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, FREE_LIST_ALLOCATOR_BAD_OPERATION, rslt_to_str(FREE_LIST_ALLOCATOR_BAD_OPERATION), "free_list_allocator_allocate", "*out_ptr_")
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - Provided memory_tag_ is not valid.", rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT));
        goto cleanup;
    }
    if(0 == allocation_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - Provided allocation_size_ is not valid.", rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - Precondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = allocation_block_size_calc(free_list_allocator_->payload_offset, allocation_size_, &new_block_size);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - allocation_block_size_calc failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = free_block_find_first_fit(free_list_allocator_, new_block_size, &allocation_block);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - free_block_find_first_fit failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = free_block_split(free_list_allocator_->minimum_block_size, allocation_block, new_block_size);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - free_block_split failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret =free_block_allocate(free_list_allocator_->payload_offset, allocation_block, allocation_size_, memory_tag_, &tmp_ptr);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - free_block_allocate failed.", rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - Postcondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_ptr_ = tmp_ptr;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

// 正常にallocateされたptr_で、かつvalidなfree_list_allocator_tであれば失敗することは基本ないためvoidにしても良いが、
// engine private moduleであるためresult codeを返すことにする
free_list_allocator_result_t free_list_allocator_free(free_list_allocator_t* free_list_allocator_, void* ptr_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    free_list_block_header_t* tmp_header = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_free", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(ptr_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_list_allocator_free", "ptr_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_free(%s) - Precondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif
    if(!allocation_ptr_is_valid(free_list_allocator_, ptr_)) {  // 内部でblockを走査するため、canonical validatorの後で実行する
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_free(%s) - Provided ptr_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp_header = (free_list_block_header_t*)((uintptr_t)ptr_ - free_list_allocator_->payload_offset);

    ret = allocated_block_free(tmp_header);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_free(%s) - allocated_block_free failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = free_block_coalesce(tmp_header);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_free(%s) - free_block_coalesce failed.", rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!free_list_allocator_is_valid(free_list_allocator_)) {
        ret = FREE_LIST_ALLOCATOR_DATA_CORRUPTED;
        ERROR_MESSAGE("free_list_allocator_free(%s) - Postcondition validation failed for 'free_list_allocator_'.", rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

bool free_list_allocator_is_valid(const free_list_allocator_t* free_list_allocator_) {
    const free_list_block_header_t* node = NULL;
    const free_list_block_header_t* prev_node = NULL;

    uintptr_t pool_address = 0;
    uintptr_t pool_end_address = 0;
    uintptr_t expected_node_address = 0;
    uintptr_t node_address = 0;
    uintptr_t next_node_address = 0;

    size_t remaining_size = 0;

    bool is_aligned = false;
    bool prev_node_is_free = false;

    if(NULL == free_list_allocator_) {
        return false;
    }
    if(!is_valid_shallow(free_list_allocator_)) {
        return false;
    }

    pool_address = (uintptr_t)free_list_allocator_->memory_pool;
    if((UINTPTR_MAX - pool_address) < free_list_allocator_->memory_pool_size) {
        return false;
    }

    pool_end_address = pool_address + free_list_allocator_->memory_pool_size;
    node = free_list_allocator_->head;
    expected_node_address = pool_address;
    while(NULL != node) {
        node_address = (uintptr_t)node;

        // blockがpoolが先頭からgap/overlapなしで並んでいること
        if(node_address != expected_node_address) {
            return false;
        }
        if(node_address >= pool_end_address) {
            return false;
        }
        if(!memory_utility_is_aligned(node_address, alignof(max_align_t), &is_aligned)) {
            return false;
        }
        if(!is_aligned) {
            return false;
        }
        if(node->prev != prev_node) {
            return false;
        }

        // block header + payload開始位置を保持できるだけの領域があるか
        remaining_size = (size_t)(pool_end_address - node_address);
        if(node->block_size < free_list_allocator_->payload_offset) {
            return false;
        }
        if(node->block_size > remaining_size) {
            return false;
        }

        if(!free_list_block_state_is_valid(node->block_state)) {
            return false;
        }

        if(prev_node_is_free && FREE_LIST_BLOCK_STATE_FREE == node->block_state) {
            return false;
        }

        if(FREE_LIST_BLOCK_STATE_ALLOCATED == node->block_state) {
            if(0 == node->allocation_size) {
                return false;
            }
            if(!memory_tag_is_valid(node->memory_tag)) {
                return false;
            }
            if(node->allocation_size > (node->block_size - free_list_allocator_->payload_offset)) {
                return false;
            }
        }

        next_node_address = node_address + node->block_size;
        if(NULL == node->next) {
            // tail blockはpool末尾であること
            if(next_node_address != pool_end_address) {
                return false;
            }
        }
        else {
            if((uintptr_t)node->next != next_node_address) {
                return false;
            }

            if(next_node_address >= pool_end_address) {
                return false;
            }
        }

        prev_node_is_free = (FREE_LIST_BLOCK_STATE_FREE == node->block_state);
        expected_node_address = next_node_address;
        prev_node = node;
        node = node->next;
    }

    if(expected_node_address != pool_end_address) {
        return false;
    }

    return true;
}

static free_list_allocator_result_t allocation_block_size_calc(size_t payload_offset_, size_t allocation_size_, size_t* out_required_block_size_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t required_block_size = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_required_block_size_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "allocation_block_size_calc", "out_required_block_size_")
    if(0 == allocation_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_block_size_calc(%s) - Provided allocation_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == payload_offset_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_block_size_calc(%s) - Provided payload_offset_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - allocation_size_) < payload_offset_) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("allocation_block_size_calc(%s) - Provided allocation_size_ overflow.", rslt_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    if(!memory_utility_align_up(allocation_size_ + payload_offset_, alignof(max_align_t), &required_block_size)) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW; // このケースでmemory_utility_align_upが失敗するのはOVERFLOWのみ
        ERROR_MESSAGE("allocation_block_size_calc(%s) - memory_utility_align_up failed.", rslt_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_required_block_size_ = required_block_size;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t free_block_find_first_fit(const free_list_allocator_t* free_list_allocator_, size_t required_block_size_, free_list_block_header_t** out_free_block_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    free_list_block_header_t* block = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_find_first_fit", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_free_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_find_first_fit", "out_free_block_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_free_block_, ret, FREE_LIST_ALLOCATOR_BAD_OPERATION, rslt_to_str(FREE_LIST_ALLOCATOR_BAD_OPERATION), "free_block_find_first_fit", "*out_free_block_")
    if(0 == required_block_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_find_first_fit(%s) - Provided required_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    // Preflight.
    block = free_list_allocator_->head;
    while(NULL != block) {
        if(FREE_LIST_BLOCK_STATE_FREE == block->block_state && block->block_size >= required_block_size_) {
            break;
        }
        block = block->next;
    }
    if(NULL == block) {
        ret = FREE_LIST_ALLOCATOR_NO_MEMORY;
        ERROR_MESSAGE("free_block_find_first_fit(%s) - free block not found.", rslt_to_str(ret));
        goto cleanup;
    }

    // Output.
    *out_free_block_ = block;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t free_block_split(size_t minimum_block_size_, free_list_block_header_t* free_block_, size_t required_block_size_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    void* split_block_address = NULL;
    free_list_block_header_t* split_block = NULL;
    free_list_block_header_t* next_block = NULL;

    size_t remaining_block_size = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(free_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_split", "free_block_")
    if(0 == minimum_block_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_split(%s) - Provided minimum_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == required_block_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_split(%s) - Provided required_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(required_block_size_ > free_block_->block_size) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_split(%s) - Provided required_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(FREE_LIST_BLOCK_STATE_FREE != free_block_->block_state) {
        ret = FREE_LIST_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("free_block_split(%s) - Provided free_block_ is not freed.", rslt_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    remaining_block_size = free_block_->block_size - required_block_size_;
    if(minimum_block_size_ > remaining_block_size) {
        ret = FREE_LIST_ALLOCATOR_SUCCESS;
        goto cleanup;
    }
    split_block_address = (unsigned char*)free_block_ + required_block_size_;
    split_block = (free_list_block_header_t*)(split_block_address);
    next_block = free_block_->next;

    // Commit.
    if(NULL != next_block) { // free_block_が末尾ノードでなければ更新
        next_block->prev = split_block;
    }

    split_block->block_size = remaining_block_size;
    split_block->allocation_size = 0;
    split_block->block_state = FREE_LIST_BLOCK_STATE_FREE;
    split_block->prev = free_block_;
    split_block->next = next_block;
    free_block_->next = split_block;
    free_block_->block_size = required_block_size_;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t free_block_allocate(size_t payload_offset_, free_list_block_header_t* free_block_, size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    void* payload_address = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(free_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_allocate", "free_block_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, FREE_LIST_ALLOCATOR_BAD_OPERATION, rslt_to_str(FREE_LIST_ALLOCATOR_BAD_OPERATION), "free_block_allocate", "*out_ptr_")
    if(FREE_LIST_BLOCK_STATE_FREE != free_block_->block_state) {
        ret = FREE_LIST_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("free_block_allocate(%s) - Provided free_block_ is not freed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == payload_offset_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_allocate(%s) - Provided payload_offset_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == allocation_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_allocate(%s) - Provided allocation_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(!memory_tag_is_valid(memory_tag_)) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_allocate(%s) - Provided memory_tag_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    payload_address = (unsigned char*)free_block_ + payload_offset_;

    // Commit.
    free_block_->allocation_size = allocation_size_;
    free_block_->block_state = FREE_LIST_BLOCK_STATE_ALLOCATED;
    free_block_->memory_tag = memory_tag_;

    // Output.
    *out_ptr_ = payload_address;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static bool allocation_ptr_is_valid(const free_list_allocator_t* free_list_allocator_, const void* ptr_) {
    bool ret = false;

    const free_list_block_header_t* block = NULL;
    const void* payload_address = NULL;

    // Preconditions.
    if(NULL == free_list_allocator_ || NULL == ptr_) {
        return false;
    }

    // Preflight.
    block = free_list_allocator_->head;
    while(NULL != block) {
        if(FREE_LIST_BLOCK_STATE_ALLOCATED == block->block_state) {
            payload_address = (const unsigned char*)block + free_list_allocator_->payload_offset;
            if(ptr_ == payload_address) {
                ret = true;
                break;
            }
        }
        block = block->next;
    }

    return ret;
}

static free_list_allocator_result_t allocated_block_free(free_list_block_header_t* allocation_block_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocation_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "allocated_block_free", "allocation_block_")
    if(FREE_LIST_BLOCK_STATE_ALLOCATED != allocation_block_->block_state) {
        ret = FREE_LIST_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("allocated_block_free(%s) - Provided allocation_block_ is not allocated.", rslt_to_str(ret));
        goto cleanup;
    }

    // Commit.
    allocation_block_->allocation_size = 0;
    allocation_block_->block_state = FREE_LIST_BLOCK_STATE_FREE;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t free_block_merge_next(free_list_block_header_t* free_block_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    free_list_block_header_t* next_block = NULL;
    free_list_block_header_t* next_next_block = NULL;

    size_t merged_block_size = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(free_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_merge_next", "free_block_")
    if(FREE_LIST_BLOCK_STATE_FREE != free_block_->block_state) {
        ret = FREE_LIST_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("free_block_merge_next(%s) - Provided free_block_ is not freed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(NULL == free_block_->next) {
        ret = FREE_LIST_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("free_block_merge_next(%s) - Next block is NULL.", rslt_to_str(ret));
        goto cleanup;
    }
    if(FREE_LIST_BLOCK_STATE_FREE != free_block_->next->block_state) {
        ret = FREE_LIST_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("free_block_merge_next(%s) - Provided next block is not freed.", rslt_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    merged_block_size = free_block_->block_size + free_block_->next->block_size;    // validなfree_list_allocatorであればオーバーフローは起こらないためチェック不要
    next_block = free_block_->next;
    next_next_block = next_block->next;

    // Commit.
    if(NULL != next_next_block) {
        next_next_block->prev = free_block_;
    }
    free_block_->next = next_next_block;
    free_block_->block_size = merged_block_size;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t free_block_coalesce(free_list_block_header_t* free_block_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(free_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_coalesce", "free_block_")
    if(FREE_LIST_BLOCK_STATE_FREE != free_block_->block_state) {
        ret = FREE_LIST_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("free_block_coalesce(%s) - Provided free_block_ is not freed.", rslt_to_str(ret));
        goto cleanup;
    }

    // Commit.
    // 後方merge
    if(NULL != free_block_->next && FREE_LIST_BLOCK_STATE_FREE == free_block_->next->block_state) {
        ret = free_block_merge_next(free_block_);
        if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
            ERROR_MESSAGE("free_block_coalesce(%s) - free_block_merge_next failed.", rslt_to_str(ret));
            goto cleanup;
        }
    }
    // 前方merge
    if(NULL != free_block_->prev && FREE_LIST_BLOCK_STATE_FREE == free_block_->prev->block_state) {
        ret = free_block_merge_next(free_block_->prev);
        if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
            ERROR_MESSAGE("free_block_coalesce(%s) - free_block_merge_next failed.", rslt_to_str(ret));
            goto cleanup;
        }
    }

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static const char* rslt_to_str(free_list_allocator_result_t rslt_) {
    switch(rslt_) {
    case FREE_LIST_ALLOCATOR_SUCCESS:
        return s_rslt_str_success;
    case FREE_LIST_ALLOCATOR_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case FREE_LIST_ALLOCATOR_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case FREE_LIST_ALLOCATOR_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case FREE_LIST_ALLOCATOR_NO_MEMORY:
        return s_rslt_str_no_memory;
    case FREE_LIST_ALLOCATOR_OVERFLOW:
        return s_rslt_str_overflow;
    case FREE_LIST_ALLOCATOR_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    default:
        return s_rslt_str_undefined_error;
    }
}

static bool is_valid_shallow(const free_list_allocator_t* free_list_allocator_) {
    if(NULL == free_list_allocator_) {
        return false;
    }
    if(NULL == free_list_allocator_->memory_pool) {
        return false;
    }
    if(0 == free_list_allocator_->memory_pool_size) {
        return false;
    }
    if(NULL == free_list_allocator_->head) {
        return false;
    }
    if(free_list_allocator_->head != free_list_allocator_->memory_pool) {
        return false;
    }
    if(0 == free_list_allocator_->payload_offset) {
        return false;
    }
    if(free_list_allocator_->payload_offset > free_list_allocator_->memory_pool_size) {
        return false;
    }
    bool is_aligned = false;
    uintptr_t pool_address = (uintptr_t)free_list_allocator_->memory_pool;
    if(!memory_utility_is_aligned(pool_address, alignof(max_align_t), &is_aligned)) {
        return false;
    }
    if(!is_aligned) {
        return false;
    }
    if(!memory_utility_is_aligned(free_list_allocator_->payload_offset, alignof(max_align_t), &is_aligned)) {
        return false;
    }
    if(!is_aligned) {
        return false;
    }

    return true;
}

static bool free_list_block_state_is_valid(free_list_block_state_t state_) {
    if(FREE_LIST_BLOCK_STATE_FREE != state_ && FREE_LIST_BLOCK_STATE_ALLOCATED != state_) {
        return false;
    }
    return true;
}
