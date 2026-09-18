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

static free_list_allocator_result_t allocation_block_size_calc(const free_list_allocator_t* free_list_allocator_, size_t allocation_size_, size_t* out_block_size_);
static free_list_allocator_result_t free_block_find_first_fit(const free_list_allocator_t* free_list_allocator_, size_t required_block_size_, free_list_block_header_t** out_block_);
static free_list_allocator_result_t free_block_split(free_list_allocator_t* free_list_allocator_, free_list_block_header_t* free_block_, size_t required_block_size_);
static free_list_allocator_result_t free_block_allocate(free_list_allocator_t* free_list_allocator_, free_list_block_header_t* free_block_, size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_);

static const char* rslt_to_str(free_list_allocator_result_t rslt_);

static bool is_valid_shallow(const free_list_allocator_t* free_list_allocator_);

free_list_allocator_result_t free_list_allocator_initialize(size_t memory_pool_size_, void* memory_pool_, free_list_allocator_t* free_list_allocator_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t payload_offset = 0;
    bool is_aligned = false;

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
    if(!memory_utility_align_up(sizeof(free_list_block_header_t), alignof(max_align_t), &payload_offset)) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW; // このケースでmemory_utility_align_upが失敗しるのはOVERFLOWのみ
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - memory_utility_align_up failed.", rslt_to_str(ret));
        goto cleanup;
    }
    if(memory_pool_size_ < payload_offset) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_list_allocator_initialize(%s) - Provided memory_pool_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    free_list_allocator_->memory_pool = memory_pool_;
    free_list_allocator_->memory_pool_size = memory_pool_size_;
    free_list_allocator_->payload_offset = payload_offset;

    free_list_allocator_->head = (free_list_block_header_t*)memory_pool_;
    free_list_allocator_->head->block_size = memory_pool_size_;
    free_list_allocator_->head->allocation_size = 0;
    free_list_allocator_->head->block_state = FREE_LIST_BLOCK_STATE_FREE;
    free_list_allocator_->head->next = NULL;
    free_list_allocator_->head->prev = NULL;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

void free_list_allocator_deinitialize(free_list_allocator_t* free_list_allocator_) {
    if(NULL == free_list_allocator_) {
        return;
    }
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

    ret = allocation_block_size_calc(free_list_allocator_, allocation_size_, &new_block_size);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - allocation_block_size_calc failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = free_block_find_first_fit(free_list_allocator_, new_block_size, &allocation_block);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - free_block_find_first_fit failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret = free_block_split(free_list_allocator_, allocation_block, new_block_size);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - free_block_split failed.", rslt_to_str(ret));
        goto cleanup;
    }

    ret =free_block_allocate(free_list_allocator_, allocation_block, allocation_size_, memory_tag_, &tmp_ptr);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret) {
        ERROR_MESSAGE("free_list_allocator_allocate(%s) - free_block_allocate failed.", rslt_to_str(ret));
        goto cleanup;
    }

    *out_ptr_ = tmp_ptr;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

bool free_list_allocator_is_valid(const free_list_allocator_t* free_list_allocator_) {
    if(NULL == free_list_allocator_) {
        return false;
    }
    if(!is_valid_shallow(free_list_allocator_)) {
        return false;
    }
    return true;
}

static free_list_allocator_result_t allocation_block_size_calc(const free_list_allocator_t* free_list_allocator_, size_t allocation_size_, size_t* out_block_size_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    size_t tmp_block_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "allocation_block_size_calc", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_block_size_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "allocation_block_size_calc", "out_block_size_")
    if(0 == allocation_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("allocation_block_size_calc(%s) - Provided allocation_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if((SIZE_MAX - allocation_size_) < free_list_allocator_->payload_offset) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW;
        ERROR_MESSAGE("allocation_block_size_calc(%s) - Provided allocation_size_ overflow.", rslt_to_str(ret));
        goto cleanup;
    }

    if(!memory_utility_align_up(allocation_size_ + free_list_allocator_->payload_offset, alignof(max_align_t), &tmp_block_size)) {
        ret = FREE_LIST_ALLOCATOR_OVERFLOW; // このケースでmemory_utility_align_upが失敗しるのはOVERFLOWのみ
        ERROR_MESSAGE("allocation_block_size_calc(%s) - memory_utility_align_up failed.", rslt_to_str(ret));
        goto cleanup;
    }

    *out_block_size_ = tmp_block_size;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t free_block_find_first_fit(const free_list_allocator_t* free_list_allocator_, size_t required_block_size_, free_list_block_header_t** out_block_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    free_list_block_header_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_find_first_fit", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_find_first_fit", "out_block_")
    if(0 == required_block_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_find_first_fit(%s) - Provided required_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    node = free_list_allocator_->head;
    while(NULL != node) {
        if(node->block_size >= required_block_size_ && FREE_LIST_BLOCK_STATE_FREE == node->block_state) {
            break;
        }
        node = node->next;
    }

    if(NULL == node) {
        ret = FREE_LIST_ALLOCATOR_NO_MEMORY;
        ERROR_MESSAGE("free_block_find_first_fit(%s) - free block not found.", rslt_to_str(ret));
        goto cleanup;
    }

    *out_block_ = node;

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t free_block_split(free_list_allocator_t* free_list_allocator_, free_list_block_header_t* free_block_, size_t required_block_size_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    void* new_node_address = NULL;
    free_list_block_header_t* new_node = NULL;
    free_list_block_header_t* tmp_next_node = NULL;
    size_t new_block_size = 0;
    size_t minimum_block_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_split", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(free_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_split", "free_block_")
    if(0 == required_block_size_) {
        ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;
        ERROR_MESSAGE("free_block_split(%s) - Provided required_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }
    if(free_block_->block_size < required_block_size_) {
        ret = FREE_LIST_ALLOCATOR_BAD_OPERATION;
        ERROR_MESSAGE("free_block_split(%s) - Provided required_block_size_ is not valid.", rslt_to_str(ret));
        goto cleanup;
    }

    minimum_block_size = free_list_allocator_->payload_offset + alignof(max_align_t);
    new_block_size = free_block_->block_size - required_block_size_;
    if(minimum_block_size <= new_block_size) {
        tmp_next_node = free_block_->next;
        new_node_address = (unsigned char*)free_block_ + required_block_size_;
        new_node = (free_list_block_header_t*)(new_node_address);

        if(NULL != tmp_next_node) { // free_block_が末尾ノードでなければ更新
            tmp_next_node->prev = new_node;
        }

        new_node->block_size = new_block_size;
        new_node->allocation_size = 0;
        new_node->block_state = FREE_LIST_BLOCK_STATE_FREE;
        new_node->prev = free_block_;
        new_node->next = tmp_next_node;

        free_block_->next = new_node;

        free_block_->block_size = required_block_size_;
    }

    ret = FREE_LIST_ALLOCATOR_SUCCESS;

cleanup:
    return ret;
}

static free_list_allocator_result_t free_block_allocate(free_list_allocator_t* free_list_allocator_, free_list_block_header_t* free_block_, size_t allocation_size_, memory_tag_t memory_tag_, void** out_ptr_) {
    free_list_allocator_result_t ret = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    void* payload_address = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(free_list_allocator_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_allocate", "free_list_allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(free_block_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_allocate", "free_block_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, FREE_LIST_ALLOCATOR_INVALID_ARGUMENT, rslt_to_str(FREE_LIST_ALLOCATOR_INVALID_ARGUMENT), "free_block_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, FREE_LIST_ALLOCATOR_BAD_OPERATION, rslt_to_str(FREE_LIST_ALLOCATOR_BAD_OPERATION), "free_block_allocate", "*out_ptr_")
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

    free_block_->allocation_size = allocation_size_;
    free_block_->block_state = FREE_LIST_BLOCK_STATE_ALLOCATED;
    free_block_->memory_tag = memory_tag_;

    payload_address= (unsigned char*)free_block_ + free_list_allocator_->payload_offset;

    *out_ptr_ = payload_address;

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
    return true;
}
