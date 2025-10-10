/**
 * @file choco_memory.c
 * @author chocolate-pie24
 * @brief memory_system_tオブジェクトの内部状態定義と関連APIの実装
 *
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset strcmp(test only)
#include <stdlib.h> // for malloc TODO: remove this!!
#include <stdio.h>  // for fprintf

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#ifdef TEST_BUILD
#include <assert.h>
#include <stdbool.h>
typedef struct malloc_test {    // TODO: 現状はlinear_allocatorと同じだが、将来的にFreeListになった際に挙動が変わるので、とりあえずコピーを置く
    bool fail_enable;
    int32_t malloc_counter;
    int32_t malloc_fail_n;
} malloc_test_t;

static malloc_test_t s_malloc_test;

static void test_test_malloc(void); // TODO: 現状はlinear_allocatorと同じだが、将来的にFreeListになった際に挙動が変わるので、とりあえずコピーを置く
static void test_memory_system_create(void);
static void test_memory_system_destroy(void);
static void test_memory_system_allocate(void);
static void test_memory_system_free(void);
static void test_memory_system_report(void);
#endif

/**
 * @brief メモリーシステム内部状態管理オブジェクト
 *
 */
typedef struct memory_system {
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t mem_tag_allocated[MEMORY_TAG_MAX];   /**< 各メモリータグごとのメモリ割り当て量 */
    const char* mem_tag_str[MEMORY_TAG_MAX];    /**< 各メモリータグ文字列 */
} memory_system_t;

static memory_system_t* s_mem_sys_ptr = NULL;

static void* test_malloc(size_t size_); // TODO: 現状はlinear_allocatorと同じだが、将来的にFreeListになった際に挙動が変わるので、とりあえずコピーを置く

// NULL != s_mem_sys_ptr                 -> MEMORY_SYSTEM_RUNTIME_ERROR
// s_mem_sys_ptr malloc(1回目のmalloc)失敗 -> MEMORY_SYSTEM_NO_MEMORY
memory_sys_err_t memory_system_create(void) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    memory_system_t* tmp = NULL;

    // Preconditions.
    if(NULL != s_mem_sys_ptr) {
        ERROR_MESSAGE("memory_system_create(RUNTIME_ERROR) - Memory system is already initialized.");
        ret = MEMORY_SYSTEM_RUNTIME_ERROR;
        goto cleanup;
    }

    // Simulation.
    tmp = (memory_system_t*)test_malloc(sizeof(memory_system_t));
    CHECK_ALLOC_FAIL_GOTO_CLEANUP(tmp, MEMORY_SYSTEM_NO_MEMORY, "memory_system_create", "tmp")
    memset(tmp, 0, sizeof(memory_system_t));

    tmp->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        tmp->mem_tag_allocated[i] = 0;
    }
    tmp->mem_tag_str[MEMORY_TAG_SYSTEM] = "system";
    tmp->mem_tag_str[MEMORY_TAG_STRING] = "string";

    // commit
    s_mem_sys_ptr = tmp;
    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    if(MEMORY_SYSTEM_SUCCESS != ret) {
        if(NULL != tmp) {
            free(tmp);
            tmp = NULL;
        }
    }
    return ret;
}

// s_mem_sys_ptr == NULL -> no-op
// s_mem_sys_ptr->total_allocated != 0 -> warning + 正常処理
void memory_system_destroy(void) {
    if(NULL == s_mem_sys_ptr) {
        goto cleanup;
    }
    if(0 != s_mem_sys_ptr->total_allocated) {
        WARN_MESSAGE("memory_system_destroy - total_allocated != 0. Check memory leaks.");
    }
    s_mem_sys_ptr->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        s_mem_sys_ptr->mem_tag_allocated[i] = 0;
    }
    free(s_mem_sys_ptr);
    s_mem_sys_ptr = NULL;

cleanup:
    return;
}

memory_sys_err_t memory_system_allocate(size_t size_, memory_tag_t mem_tag_, void** out_ptr_) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    void* tmp = NULL;

    // Preconditions.
    CHECK_ARG_NULL_GOTO_CLEANUP(s_mem_sys_ptr, MEMORY_SYSTEM_INVALID_ARGUMENT, "memory_system_allocate", "s_mem_sys_ptr")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_ptr_, MEMORY_SYSTEM_INVALID_ARGUMENT, "memory_system_allocate", "out_ptr_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, MEMORY_SYSTEM_INVALID_ARGUMENT, "memory_system_allocate", "*out_ptr_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(mem_tag_ < MEMORY_TAG_MAX, MEMORY_SYSTEM_INVALID_ARGUMENT, "memory_system_allocate", "mem_tag_")
    if(0 == size_) {
        WARN_MESSAGE("memory_system_allocate - No-op: size_ is 0.");
        ret = MEMORY_SYSTEM_SUCCESS;
        goto cleanup;
    }
    if(s_mem_sys_ptr->mem_tag_allocated[mem_tag_] > (SIZE_MAX - size_)) {
        ERROR_MESSAGE("memory_system_allocate(INVALID_ARGUMENT) - size_t overflow: tag=%s used=%zu, requested=%zu, sum would exceed SIZE_MAX.", s_mem_sys_ptr->mem_tag_str[mem_tag_], s_mem_sys_ptr->mem_tag_allocated[mem_tag_], size_);
        ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
        goto cleanup;
    }
    if(s_mem_sys_ptr->total_allocated > (SIZE_MAX - size_)) {
        ERROR_MESSAGE("memory_system_allocate(INVALID_ARGUMENT) - size_t overflow: total_allocated=%zu, requested=%zu, sum would exceed SIZE_MAX.", s_mem_sys_ptr->total_allocated, size_);
        ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Simulation.
    tmp = test_malloc(size_);    // TODO: FreeList
    CHECK_ALLOC_FAIL_GOTO_CLEANUP(tmp, MEMORY_SYSTEM_NO_MEMORY, "memory_system_allocate", "tmp")
    memset(tmp, 0, size_);

    // commit.
    *out_ptr_ = tmp;
    s_mem_sys_ptr->total_allocated += size_;
    s_mem_sys_ptr->mem_tag_allocated[mem_tag_] += size_;
    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

// NULL == s_mem_sys_ptrでワーニング / No-op
// NULL == ptr_でワーニング / No-op
// mem_tag_ >= MEMORY_TAG_MAXでワーニング / No-op
// mem_tag_allocatedがマイナスとなる量をfreeしようとするとワーニング / No-op
// total_allocatedがマイナスとなる量をfreeしようとするとワーニング / No-op
void memory_system_free(void* ptr_, size_t size_, memory_tag_t mem_tag_) {
    if(NULL == s_mem_sys_ptr) {
        WARN_MESSAGE("memory_system_free - No-op: s_mem_sys_ptr is NULL.");
        goto cleanup;
    }
    if(NULL == ptr_) {
        WARN_MESSAGE("memory_system_free - No-op: ptr_ is NULL.");
        goto cleanup;
    }
    if(mem_tag_ >= MEMORY_TAG_MAX) {
        WARN_MESSAGE("memory_system_free - No-op: invalid mem_tag_.");
        goto cleanup;
    }
    if(s_mem_sys_ptr->mem_tag_allocated[mem_tag_] < size_) {
        WARN_MESSAGE("memory_system_free - No-op: mem_tag_allocated broken.");
        goto cleanup;
    }
    if(s_mem_sys_ptr->total_allocated < size_) {
        WARN_MESSAGE("memory_system_free: No-op: total_allocated broken.");
        goto cleanup;
    }

    free(ptr_);
    s_mem_sys_ptr->total_allocated -= size_;
    s_mem_sys_ptr->mem_tag_allocated[mem_tag_] -= size_;
cleanup:
    return;
}

void memory_system_report(void) {
    if(NULL == s_mem_sys_ptr) {
        WARN_MESSAGE("memory_system_report - No-op: s_mem_sys_ptr is NULL.");
        goto cleanup;
    }
    INFO_MESSAGE("memory_system_report");
    // TODO: [INFORMATION]を出力しないINFO_MESSAGE_RAW(...)をbase/messageに追加し、fprintfを廃止する
    fprintf(stdout, "\033[1;35m\tTotal allocated: %zu\n", s_mem_sys_ptr->total_allocated);
    fprintf(stdout, "\tMemory tag allocated:\n");
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        const char* const tag_str = s_mem_sys_ptr->mem_tag_str[i];
        fprintf(stdout, "\t\ttag(%s): %zu\n", (NULL != tag_str) ? tag_str : "unknown", s_mem_sys_ptr->mem_tag_allocated[i]);
    }
    fprintf(stdout, "\033[0m\n");
cleanup:
    return;
}

static void* test_malloc(size_t size_) {
    void* ret = NULL;
#ifdef TEST_BUILD
    if(s_malloc_test.fail_enable) {
        if(s_malloc_test.malloc_counter == s_malloc_test.malloc_fail_n) {
            ret = NULL;
        } else {
            ret = malloc(size_);
        }
        s_malloc_test.malloc_counter++;
    } else {
        ret = malloc(size_);
    }
#else
    ret = malloc(size_);
#endif
    return ret;
}

#ifdef TEST_BUILD
void test_memory_system(void) {
    test_test_malloc();
    test_memory_system_create();
    test_memory_system_destroy();
    test_memory_system_allocate();
    test_memory_system_free();
    test_memory_system_report();
}

static void NO_COVERAGE test_memory_system_create(void) {
    {
        // NULL != s_mem_sys_ptr -> MEMORY_SYSTEM_RUNTIME_ERROR
        memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

        assert(NULL == s_mem_sys_ptr);
        s_mem_sys_ptr = malloc(sizeof(memory_system_t));
        assert(NULL != s_mem_sys_ptr);

        ret = memory_system_create();
        assert(NULL != s_mem_sys_ptr);
        assert(MEMORY_SYSTEM_RUNTIME_ERROR == ret);

        free(s_mem_sys_ptr);
        s_mem_sys_ptr = NULL;
    }
    {
        // s_mem_sys_ptr malloc(1回目のmalloc)失敗 -> MEMORY_SYSTEM_NO_MEMORY
        memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

        s_malloc_test.fail_enable = true;
        s_malloc_test.malloc_counter = 0;
        s_malloc_test.malloc_fail_n = 0;

        assert(NULL == s_mem_sys_ptr);
        ret = memory_system_create();
        assert(MEMORY_SYSTEM_NO_MEMORY == ret);
        assert(NULL == s_mem_sys_ptr);

        s_malloc_test.fail_enable = false;
    }
    {
        // 正常系
        memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

        assert(NULL == s_mem_sys_ptr);
        ret = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret);
        assert(0 == s_mem_sys_ptr->total_allocated);
        assert(0 == strcmp("system", s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_SYSTEM]));
        assert(0 == strcmp("string", s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_STRING]));
        for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
            assert(0 == s_mem_sys_ptr->mem_tag_allocated[i]);
        }
        memory_system_destroy();
        assert(NULL == s_mem_sys_ptr);
    }
}

void NO_COVERAGE test_memory_system_destroy(void) {
    // s_mem_sys_ptr == NULL -> no-op
    memory_system_destroy();

    // 正常処理
    memory_sys_err_t ret = memory_system_create();
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(NULL != s_mem_sys_ptr);
    memory_system_destroy();
    assert(NULL == s_mem_sys_ptr);
    memory_system_destroy();

    // s_mem_sys_ptr->total_allocated != 0 -> warning + 正常処理
    ret = memory_system_create();
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(NULL != s_mem_sys_ptr);
    s_mem_sys_ptr->total_allocated = 128;
    s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING] = 32;
    s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM] = 96;
    memory_system_destroy();
    assert(NULL == s_mem_sys_ptr);
}

static void NO_COVERAGE test_memory_system_allocate(void) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    void* ptr = NULL;

    // s_mem_sys_ptr == NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
    ret = memory_system_allocate(128, MEMORY_TAG_STRING, &ptr);
    assert(NULL == ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(NULL == s_mem_sys_ptr);

    ret = memory_system_create();
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(NULL != s_mem_sys_ptr);

    // out_ptr == NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
    ret = memory_system_allocate(128, MEMORY_TAG_STRING, NULL);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(0 == s_mem_sys_ptr->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == s_mem_sys_ptr->mem_tag_allocated[i]);
    }

    // *out_ptr != NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
    ptr = malloc(8);
    ret = memory_system_allocate(128, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(0 == s_mem_sys_ptr->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == s_mem_sys_ptr->mem_tag_allocated[i]);
    }
    free(ptr);
    ptr = NULL;

    // mem_tag_ >= MEMORY_TAG_MAXでMEMORY_SYSTEM_INVALID_ARGUMENT
    ret = memory_system_allocate(128, MEMORY_TAG_MAX, &ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(0 == s_mem_sys_ptr->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == s_mem_sys_ptr->mem_tag_allocated[i]);
    }
    assert(NULL == ptr);

    // size_ == 0でwarningメッセージを出し、MEMORY_SYSTEM_SUCCESS
    ret = memory_system_allocate(0, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(0 == s_mem_sys_ptr->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == s_mem_sys_ptr->mem_tag_allocated[i]);
    }
    assert(NULL == ptr);

    // mem_tag_allocatedがSIZE_MAX超過でMEMORY_SYSTEM_INVALID_ARGUMENT
    s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING] = SIZE_MAX - 100;
    ret = memory_system_allocate(101, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(0 == s_mem_sys_ptr->total_allocated);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    assert((SIZE_MAX - 100) == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(NULL == ptr);
    s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING] = 0;

    // total_allocatedがSIZE_MAX超過でMEMORY_SYSTEM_INVALID_ARGUMENT
    s_mem_sys_ptr->total_allocated = SIZE_MAX - 100;
    ret = memory_system_allocate(101, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert((SIZE_MAX - 100) == s_mem_sys_ptr->total_allocated);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(NULL == ptr);
    s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING] = 0;
    assert(NULL == ptr);
    s_mem_sys_ptr->total_allocated = 0;

    // // 1回目のmallocで失敗させる
    s_malloc_test.fail_enable = true;
    s_malloc_test.malloc_counter = 0;
    s_malloc_test.malloc_fail_n = 0;
    ret = memory_system_allocate(128, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_NO_MEMORY == ret);
    assert(0 == s_mem_sys_ptr->total_allocated);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    assert(NULL == ptr);
    s_malloc_test.fail_enable = false;
    s_malloc_test.malloc_counter = 0;

    // 正常系
    ret = memory_system_allocate(128, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(128 == s_mem_sys_ptr->total_allocated);
    assert(128 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    free(ptr);
    ptr = NULL;

    memory_system_destroy();
}

static void NO_COVERAGE test_memory_system_free(void) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

    void* ptr = malloc(8);
    assert(NULL != ptr);
    // s_mem_sys_ptr == NULLでno-op
    memory_system_free(ptr, 8, MEMORY_TAG_STRING);
    free(ptr);

    ret = memory_system_create();
    assert(NULL != s_mem_sys_ptr);
    assert(MEMORY_SYSTEM_SUCCESS == ret);

    void* ptr_string = NULL;
    void* ptr_system = NULL;

    ret = memory_system_allocate(128, MEMORY_TAG_STRING, &ptr_string);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(128 == s_mem_sys_ptr->total_allocated);
    assert(128 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    ret = memory_system_allocate(256, MEMORY_TAG_SYSTEM, &ptr_system);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert((128 + 256) == s_mem_sys_ptr->total_allocated);
    assert(128 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // NULL == ptr_でワーニング // No-op
    memory_system_free(NULL, 128, MEMORY_TAG_STRING);
    assert((128 + 256) == s_mem_sys_ptr->total_allocated);
    assert(128 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // mem_tag_ >= MEMORY_TAG_MAXでワーニング / No-op
    memory_system_free(ptr_string, 128, MEMORY_TAG_MAX);
    assert((128 + 256) == s_mem_sys_ptr->total_allocated);
    assert(128 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // mem_tag_allocatedがマイナスとなる量をfreeしようとするとワーニング / No-op
    memory_system_free(ptr_string, 1024, MEMORY_TAG_STRING);
    assert((128 + 256) == s_mem_sys_ptr->total_allocated);
    assert(128 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // total_allocatedがマイナスとなる量をfreeしようとするとワーニング / No-op
    s_mem_sys_ptr->total_allocated = 64;   // temporary
    memory_system_free(ptr_string, 128, MEMORY_TAG_STRING);
    assert(64 == s_mem_sys_ptr->total_allocated);
    assert(128 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    s_mem_sys_ptr->total_allocated = 128 + 256;

    // 正常系
    memory_system_free(ptr_string, 128, MEMORY_TAG_STRING);
    assert(256 == s_mem_sys_ptr->total_allocated);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    memory_system_free(ptr_system, 256, MEMORY_TAG_SYSTEM);
    assert(0 == s_mem_sys_ptr->total_allocated);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    ptr_string = NULL;
    ptr_system = NULL;

    memory_system_destroy();
}

static void NO_COVERAGE test_memory_system_report(void) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

    void* ptr_string = NULL;
    void* ptr_system = NULL;

    // s_mem_sys_ptr == NULLでワーニング No-op
    memory_system_report();

    ret = memory_system_create();
    assert(NULL != s_mem_sys_ptr);

    // all 0.
    memory_system_report();

    ret = memory_system_allocate(128, MEMORY_TAG_STRING, &ptr_string);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(128 == s_mem_sys_ptr->total_allocated);
    assert(128 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // total = 128
    // string 128
    // system 0
    memory_system_report();

    ret = memory_system_allocate(256, MEMORY_TAG_SYSTEM, &ptr_system);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert((128 + 256) == s_mem_sys_ptr->total_allocated);
    assert(128 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == s_mem_sys_ptr->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // total = 384
    // string 128
    // system 256
    memory_system_report();

    s_mem_sys_ptr->mem_tag_str[MEMORY_TAG_STRING] = NULL;
    memory_system_report();

    memory_system_destroy();
}

// TODO: 現状はlinear_allocatorと同じだが、将来的にFreeListになった際に挙動が変わるので、とりあえずコピーを置く
static void NO_COVERAGE test_test_malloc(void) {
    {
        DEBUG_MESSAGE("test_test_malloc test_case1");
        s_malloc_test.fail_enable = false;
        s_malloc_test.malloc_counter = 0;
        s_malloc_test.malloc_fail_n = 0;
        void* tmp = NULL;
        tmp = test_malloc(128);
        assert(NULL != tmp);
        free(tmp);
        tmp = NULL;
    }
    {
        DEBUG_MESSAGE("test_test_malloc test_case2");
        s_malloc_test.fail_enable = true;
        s_malloc_test.malloc_counter = 0;
        s_malloc_test.malloc_fail_n = 1;
        void* tmp = NULL;
        tmp = test_malloc(128); // 1回目は成功
        assert(NULL != tmp);
        free(tmp);
        tmp = NULL;

        tmp = test_malloc(128); // 2回目で失敗
        assert(NULL == tmp);
        free(tmp);
        tmp = NULL;
    }
    s_malloc_test.fail_enable = false;
    s_malloc_test.malloc_counter = 0;
    s_malloc_test.malloc_fail_n = 0;
}
#endif
