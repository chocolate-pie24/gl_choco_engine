#include "engine/systems/memory_system/memory_system.h"

#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>

#include "config/build_config.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/allocators/free_list_allocator.h"

#include "engine/systems/memory_system/core/memory_system_types.h"
#include "engine/systems/memory_system/core/memory_system_err_utils.h"

struct memory_system {
    free_list_allocator_t free_list_allocator;
    void* memory_pool;
};

static memory_system_t s_memory_system;

// 組み込み向けに静的領域でmemory poolを用意(アライメントはmax_align_tでfree list allocatorのメモリ要件を満たす)
#if defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
alignas(max_align_t)
static unsigned char s_memory_pool[GLCE_BUILD_MEMORY_POOL_SIZE];
#endif

memory_system_result_t memory_system_create(void) {
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    // memory poolの初期化
#if defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
    s_memory_system.memory_pool = (void*)s_memory_pool;
#elif defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
    s_memory_system.memory_pool = malloc(GLCE_BUILD_MEMORY_POOL_SIZE);
#else
    ret = MEMORY_SYSTEM_UNDEFINED_ERROR;
    ERROR_MESSAGE("memory_system_create(%s) - Undefined build config.", memory_system_rslt_to_str(ret));
    goto cleanup;
#endif
    if(NULL == s_memory_system.memory_pool) {
        ret = MEMORY_SYSTEM_NO_MEMORY;
        ERROR_MESSAGE("memory_system_create(%s) - Failed to allocate memory for memory pool.", memory_system_rslt_to_str(ret));
        goto cleanup;
    }

    ret_free_list_allocator = free_list_allocator_initialize(GLCE_BUILD_MEMORY_POOL_SIZE, s_memory_system.memory_pool, &s_memory_system.free_list_allocator);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ret = memory_system_result_convert_free_list_allocator(ret_free_list_allocator);
        ERROR_MESSAGE("memory_system_create(%s) - free_list_allocator_initialize failed.", memory_system_rslt_to_str(ret));
        goto cleanup;
    }

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    if(MEMORY_SYSTEM_SUCCESS != ret) {
#if defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
        free(s_memory_system.memory_pool);
        s_memory_system.memory_pool = NULL;
#endif
    }
    return ret;
}

void memory_system_destroy(void) {
    free_list_allocator_result_t ret_free_list_allocator = FREE_LIST_ALLOCATOR_INVALID_ARGUMENT;

    ret_free_list_allocator = free_list_allocator_deinitialize(&s_memory_system.free_list_allocator);
    if(FREE_LIST_ALLOCATOR_SUCCESS != ret_free_list_allocator) {
        ERROR_MESSAGE("memory_system_destroy(%s) - free_list_allocator_deinitialize failed.", memory_system_rslt_to_str(MEMORY_SYSTEM_DATA_CORRUPTED));
        return;
    }

#if defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP)
    free(s_memory_system.memory_pool);
    s_memory_system.memory_pool = NULL;
#endif

}

bool memory_system_is_valid(const memory_system_t* memory_system_) {
    if(NULL == memory_system_) {
        return false;
    }
    return true;
}
