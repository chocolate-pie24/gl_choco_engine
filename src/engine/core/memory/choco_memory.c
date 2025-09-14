#include <stddef.h>
#include <stdalign.h>
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

struct memory_system {
    size_t total_allocated;
    size_t mem_tag_allocated[MEMORY_TAG_MAX];
    const char* mem_tag_str[MEMORY_TAG_MAX];
};

#define CHECK_ARG_NULL_GOTO_CLEANUP(ptr_, function_name_, variable_name_) \
    if(NULL == ptr_) { \
        ERROR_MESSAGE("%s(INVALID_ARGUMENT) - Argument %s requires a valid pointer.", function_name_, variable_name_); \
        ret = MEMORY_SYSTEM_INVALID_ARGUMENT; \
        goto cleanup;  \
    } \

#define CHECK_ARG_NOT_NULL_GOTO_CLEANUP(ptr_, function_name_, variable_name_) \
    if(NULL != ptr_) { \
        ERROR_MESSAGE("%s(INVALID_ARGUMENT) - Argument %s requires a null pointer.", function_name_, variable_name_); \
        ret = MEMORY_SYSTEM_INVALID_ARGUMENT; \
        goto cleanup;  \
    } \

#define CHECK_ALLOC_FAIL_GOTO_CLEANUP(ptr_, function_name_, variable_name_) \
    if(NULL == ptr_) { \
        ERROR_MESSAGE("%s(NO_MEMORY) - Failed to allocate %s memory.", function_name_, variable_name_); \
        ret = MEMORY_SYSTEM_NO_MEMORY; \
        goto cleanup;  \
    } \

#define CHECK_ARG_NOT_VALID_GOTO_CLEANUP(is_valid_, function_name_, variable_name_) \
    if(!(is_valid_)) { \
        ERROR_MESSAGE("%s(INVALID_ARGUMENT) - Argument %s is not valid.", function_name_, variable_name_); \
        ret = MEMORY_SYSTEM_INVALID_ARGUMENT; \
        goto cleanup;  \
    } \

void memory_system_preinit(size_t* memory_requirement_, size_t* alignment_requirement_) {
    if(NULL == memory_requirement_ || NULL == alignment_requirement_) {
        WARN_MESSAGE("memory_system_preinit - Provided memory_requirement_ or alignment_requirement_ is null pointer.");
        goto cleanup;
    }
    *memory_requirement_ = sizeof(memory_system_t);
    *alignment_requirement_ = alignof(memory_system_t);
    goto cleanup;

cleanup:
    return;
}

memory_sys_err_t memory_system_init(memory_system_t* const memory_system_) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(memory_system_, "memory_system_init", "memory_system_");

    memory_system_->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        memory_system_->mem_tag_allocated[i] = 0;
    }
    memory_system_->mem_tag_str[MEMORY_TAG_SYSTEM] = "system";
    memory_system_->mem_tag_str[MEMORY_TAG_STRING] = "string";

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

void memory_system_destroy(memory_system_t* const memory_system_) {
    if(NULL == memory_system_) {
        goto cleanup;
    }
    memory_system_->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        memory_system_->mem_tag_allocated[i] = 0;
    }

cleanup:
    return;
}

// memory_system_ == NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
// out_ptr == NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
// *out_ptr != NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
// mem_tag_ >= MEMORY_TAG_MAXでMEMORY_SYSTEM_INVALID_ARGUMENT
// size_ == 0でwarningメッセージを出し、MEMORY_SYSTEM_SUCCESS
// mem_tag_allocatedがSIZE_MAX超過でMEMORY_SYSTEM_INVALID_ARGUMENT
// total_allocatedがSIZE_MAX超過でMEMORY_SYSTEM_INVALID_ARGUMENT
memory_sys_err_t memory_system_allocate(memory_system_t* const memory_system_, size_t size_, memory_tag_t mem_tag_, void** out_ptr_) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

    // Preconditions
    CHECK_ARG_NULL_GOTO_CLEANUP(memory_system_, "memory_system_allocate", "memory_system_")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_ptr_, "memory_system_allocate", "out_ptr_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, "memory_system_allocate", "out_ptr_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(mem_tag_ < MEMORY_TAG_MAX, "memory_system_allocate", "mem_tag_")
    if(0 == size_) {
        WARN_MESSAGE("memory_system_allocate - Provided size_ is zero. Nothing to allocate.");
        ret = MEMORY_SYSTEM_SUCCESS;
        goto cleanup;
    }
    if(memory_system_->mem_tag_allocated[mem_tag_] > (SIZE_MAX - size_)) {
        ERROR_MESSAGE("memory_system_allocate(INVALID_ARGUMENT) - size_t overflow: tag=%s used=%zu, requested=%zu, sum would exceed SIZE_MAX.", memory_system_->mem_tag_str[mem_tag_], memory_system_->mem_tag_allocated[mem_tag_], size_);
        ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
        goto cleanup;
    }
    if(memory_system_->total_allocated > (SIZE_MAX - size_)) {
        ERROR_MESSAGE("memory_system_allocate(INVALID_ARGUMENT) - size_t overflow: total_allocated=%zu, requested=%zu, sum would exceed SIZE_MAX.", memory_system_->total_allocated, size_);
        ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Simulation.
    void* tmp = NULL;
    tmp = malloc(size_);    // TODO: FreeList
    CHECK_ALLOC_FAIL_GOTO_CLEANUP(tmp, "memory_system_allocate", "tmp");
    memset(tmp, 0, size_);

    // commit.
    *out_ptr_ = tmp;
    memory_system_->total_allocated += size_;
    memory_system_->mem_tag_allocated[mem_tag_] += size_;
    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

void memory_system_free(memory_system_t* const memory_system_, void* ptr_, size_t size_, memory_tag_t mem_tag_) {

}

void memory_system_report(const memory_system_t* const memory_system_) {

}
