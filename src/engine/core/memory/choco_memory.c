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
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset strcmp(test only)
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
static void test_memory_system_preinit(void);
static void test_memory_system_init(void);
static void test_memory_system_destroy(void);
static void test_memory_system_allocate(void);
static void test_memory_system_free(void);
static void test_memory_system_report(void);
#endif

/**
 * @brief メモリーシステム内部状態管理オブジェクト
 *
 */
struct memory_system {
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t mem_tag_allocated[MEMORY_TAG_MAX];   /**< 各メモリータグごとのメモリ割り当て量 */
    const char* mem_tag_str[MEMORY_TAG_MAX];    /**< 各メモリータグ文字列 */
};

static void* test_malloc(size_t size_); // TODO: 現状はlinear_allocatorと同じだが、将来的にFreeListになった際に挙動が変わるので、とりあえずコピーを置く

void memory_system_preinit(size_t* const memory_requirement_, size_t* const alignment_requirement_) {
    if(NULL == memory_requirement_ || NULL == alignment_requirement_) {
        WARN_MESSAGE("memory_system_preinit - No-op: memory_requirement_ or alignment_requirement_ is NULL.");
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
    CHECK_ARG_NULL_GOTO_CLEANUP(memory_system_, MEMORY_SYSTEM_INVALID_ARGUMENT, "memory_system_init", "memory_system_");

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

memory_sys_err_t memory_system_allocate(memory_system_t* const memory_system_, size_t size_, memory_tag_t mem_tag_, void** out_ptr_) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;

    // Preconditions.
    CHECK_ARG_NULL_GOTO_CLEANUP(memory_system_, MEMORY_SYSTEM_INVALID_ARGUMENT, "memory_system_allocate", "memory_system_")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_ptr_, MEMORY_SYSTEM_INVALID_ARGUMENT, "memory_system_allocate", "out_ptr_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, MEMORY_SYSTEM_INVALID_ARGUMENT, "memory_system_allocate", "*out_ptr_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(mem_tag_ < MEMORY_TAG_MAX, MEMORY_SYSTEM_INVALID_ARGUMENT, "memory_system_allocate", "mem_tag_")
    if(0 == size_) {
        WARN_MESSAGE("memory_system_allocate - No-op: size_ is 0.");
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
    tmp = test_malloc(size_);    // TODO: FreeList
    CHECK_ALLOC_FAIL_GOTO_CLEANUP(tmp, MEMORY_SYSTEM_NO_MEMORY, "memory_system_allocate", "tmp");
    memset(tmp, 0, size_);

    // commit.
    *out_ptr_ = tmp;
    memory_system_->total_allocated += size_;
    memory_system_->mem_tag_allocated[mem_tag_] += size_;
    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

// NULL == memory_system_でワーニング / No-op
// NULL == ptr_でワーニング // No-op
// mem_tag_ >= MEMORY_TAG_MAXでワーニング / No-op
// mem_tag_allocatedがマイナスとなる量をfreeしようとするとワーニング / No-op
// total_allocatedがマイナスとなる量をfreeしようとするとワーニング / No-op
void memory_system_free(memory_system_t* const memory_system_, void* ptr_, size_t size_, memory_tag_t mem_tag_) {
    if(NULL == memory_system_) {
        WARN_MESSAGE("memory_system_free - No-op: memory_system_ is NULL.");
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
    if(memory_system_->mem_tag_allocated[mem_tag_] < size_) {
        WARN_MESSAGE("memory_system_free - No-op: mem_tag_allocated broken.");
        goto cleanup;
    }
    if(memory_system_->total_allocated < size_) {
        WARN_MESSAGE("memory_system_free: No-op: total_allocated broken.");
        goto cleanup;
    }

    free(ptr_);
    memory_system_->total_allocated -= size_;
    memory_system_->mem_tag_allocated[mem_tag_] -= size_;
cleanup:
    return;
}

void memory_system_report(const memory_system_t* const memory_system_) {
    if(NULL == memory_system_) {
        WARN_MESSAGE("memory_system_report - No-op: memory_system_ is NULL.");
        goto cleanup;
    }
    INFO_MESSAGE("memory_system_report");
    // TODO: [INFORMATION]を出力しないINFO_MESSAGE_RAW(...)をbase/messageに追加し、fprintfを廃止する
    fprintf(stdout, "\033[1;35m\tTotal allocated: %zu\n", memory_system_->total_allocated);
    fprintf(stdout, "\tMemory tag allocated:\n");
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        const char* tag_str = memory_system_->mem_tag_str[i];
        fprintf(stdout, "\t\ttag(%s): %zu\n", (NULL != tag_str) ? tag_str : "unknown", memory_system_->mem_tag_allocated[i]);
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
    test_memory_system_preinit();
    test_memory_system_init();
    test_memory_system_destroy();
    test_memory_system_allocate();
    test_memory_system_free();
    test_memory_system_report();
}

static void NO_COVERAGE test_memory_system_preinit(void) {
    size_t memory = 0;
    size_t align = 0;
    // memory_requirement_ == NULLでワーニング -> No-op
    memory_system_preinit(NULL, &align);
    assert(0 == memory);
    assert(0 == align);

    // alignment_requirement_ == NULLでワーニング -> No-op
    memory_system_preinit(&memory, NULL);
    assert(0 == memory);
    assert(0 == align);

    memory_system_preinit(&memory, &align);
    assert(sizeof(memory_system_t) == memory);
    assert(alignof(memory_system_t) == align);
}

static void NO_COVERAGE test_memory_system_init(void) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    // memory_system_ == NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
    ret = memory_system_init(NULL);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);

    memory_system_t* system = NULL;
    size_t memory = 0;
    size_t align = 0;
    memory_system_preinit(&memory, &align);
    system = malloc(memory);
    ret = memory_system_init(system);
    assert(0 == strcmp(system->mem_tag_str[MEMORY_TAG_SYSTEM], "system"));
    assert(0 == strcmp(system->mem_tag_str[MEMORY_TAG_STRING], "string"));
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(0 == system->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == system->mem_tag_allocated[i]);
    }

    free(system);
    system = NULL;
}

static void NO_COVERAGE test_memory_system_destroy(void) {
    // memory_system_ == NULLでNo-op
    memory_system_destroy(NULL);

    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    memory_system_t* system = NULL;
    size_t memory = 0;
    size_t align = 0;
    memory_system_preinit(&memory, &align);
    system = malloc(memory);
    ret = memory_system_init(system);
    assert(MEMORY_SYSTEM_SUCCESS == ret);

    system->total_allocated = 100;
    system->mem_tag_allocated[MEMORY_TAG_SYSTEM] = 10;
    system->mem_tag_allocated[MEMORY_TAG_STRING] = 20;
    memory_system_destroy(system);
    assert(0 == system->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == system->mem_tag_allocated[i]);
    }

    free(system);
    system = NULL;
}

static void NO_COVERAGE test_memory_system_allocate(void) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    memory_system_t* system = NULL;
    size_t memory = 0;
    size_t align = 0;
    memory_system_preinit(&memory, &align);
    system = malloc(memory);
    ret = memory_system_init(system);
    assert(MEMORY_SYSTEM_SUCCESS == ret);

    void* ptr = NULL;
    // memory_system_ == NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
    ret = memory_system_allocate(NULL, 128, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(0 == system->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == system->mem_tag_allocated[i]);
    }
    assert(NULL == ptr);

    // out_ptr == NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
    ret = memory_system_allocate(system, 128, MEMORY_TAG_STRING, NULL);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(0 == system->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == system->mem_tag_allocated[i]);
    }

    // *out_ptr != NULLでMEMORY_SYSTEM_INVALID_ARGUMENT
    ptr = malloc(8);
    ret = memory_system_allocate(system, 128, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(0 == system->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == system->mem_tag_allocated[i]);
    }
    free(ptr);
    ptr = NULL;

    // mem_tag_ >= MEMORY_TAG_MAXでMEMORY_SYSTEM_INVALID_ARGUMENT
    ret = memory_system_allocate(system, 128, MEMORY_TAG_MAX, &ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(0 == system->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == system->mem_tag_allocated[i]);
    }
    assert(NULL == ptr);

    // size_ == 0でwarningメッセージを出し、MEMORY_SYSTEM_SUCCESS
    ret = memory_system_allocate(system, 0, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(0 == system->total_allocated);
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        assert(0 == system->mem_tag_allocated[i]);
    }
    assert(NULL == ptr);

    // mem_tag_allocatedがSIZE_MAX超過でMEMORY_SYSTEM_INVALID_ARGUMENT
    system->mem_tag_allocated[MEMORY_TAG_STRING] = SIZE_MAX - 100;
    ret = memory_system_allocate(system, 101, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert(0 == system->total_allocated);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    assert((SIZE_MAX - 100) == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(NULL == ptr);
    system->mem_tag_allocated[MEMORY_TAG_STRING] = 0;

    // total_allocatedがSIZE_MAX超過でMEMORY_SYSTEM_INVALID_ARGUMENT
    system->total_allocated = SIZE_MAX - 100;
    ret = memory_system_allocate(system, 101, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_INVALID_ARGUMENT == ret);
    assert((SIZE_MAX - 100) == system->total_allocated);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(NULL == ptr);
    system->mem_tag_allocated[MEMORY_TAG_STRING] = 0;
    assert(NULL == ptr);
    system->total_allocated = 0;

    // // 1回目のmallocで失敗させる
    s_malloc_test.fail_enable = true;
    s_malloc_test.malloc_counter = 0;
    s_malloc_test.malloc_fail_n = 0;
    ret = memory_system_allocate(system, 128, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_NO_MEMORY == ret);
    assert(0 == system->total_allocated);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    assert(NULL == ptr);
    s_malloc_test.fail_enable = false;
    s_malloc_test.malloc_counter = 0;

    // 正常系
    ret = memory_system_allocate(system, 128, MEMORY_TAG_STRING, &ptr);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(128 == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    free(ptr);
    ptr = NULL;

    free(system);
    system = NULL;
}

static void NO_COVERAGE test_memory_system_free(void) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    memory_system_t* system = NULL;
    size_t memory = 0;
    size_t align = 0;
    memory_system_preinit(&memory, &align);
    system = malloc(memory);
    ret = memory_system_init(system);
    assert(MEMORY_SYSTEM_SUCCESS == ret);

    void* ptr_string = NULL;
    void* ptr_system = NULL;

    ret = memory_system_allocate(system, 128, MEMORY_TAG_STRING, &ptr_string);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(128 == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    ret = memory_system_allocate(system, 256, MEMORY_TAG_SYSTEM, &ptr_system);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert((128 + 256) == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // NULL == memory_system_でワーニング / No-op
    memory_system_free(NULL, ptr_string, 128, MEMORY_TAG_STRING);
    assert((128 + 256) == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // NULL == ptr_でワーニング // No-op
    memory_system_free(system, NULL, 128, MEMORY_TAG_STRING);
    assert((128 + 256) == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // mem_tag_ >= MEMORY_TAG_MAXでワーニング / No-op
    memory_system_free(system, ptr_string, 128, MEMORY_TAG_MAX);
    assert((128 + 256) == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // mem_tag_allocatedがマイナスとなる量をfreeしようとするとワーニング / No-op
    memory_system_free(system, ptr_string, 1024, MEMORY_TAG_STRING);
    assert((128 + 256) == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // total_allocatedがマイナスとなる量をfreeしようとするとワーニング / No-op
    system->total_allocated = 64;   // temporary
    memory_system_free(system, ptr_string, 128, MEMORY_TAG_STRING);
    assert(64 == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);
    system->total_allocated = 128 + 256;

    // 正常系
    memory_system_free(system, ptr_string, 128, MEMORY_TAG_STRING);
    assert(256 == system->total_allocated);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    memory_system_free(system, ptr_system, 256, MEMORY_TAG_SYSTEM);
    assert(0 == system->total_allocated);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    ptr_string = NULL;
    ptr_system = NULL;

    free(system);
    system = NULL;
}

static void NO_COVERAGE test_memory_system_report(void) {
    memory_sys_err_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    memory_system_t* system = NULL;
    size_t memory = 0;
    size_t align = 0;
    memory_system_preinit(&memory, &align);
    system = malloc(memory);
    ret = memory_system_init(system);
    assert(MEMORY_SYSTEM_SUCCESS == ret);

    void* ptr_string = NULL;
    void* ptr_system = NULL;

    // memory_system_ == NULLでワーニング No-op
    memory_system_report(NULL);

    // all 0.
    memory_system_report(system);

    ret = memory_system_allocate(system, 128, MEMORY_TAG_STRING, &ptr_string);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert(128 == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(0 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // total = 128
    // string 128
    // system 0
    memory_system_report(system);

    ret = memory_system_allocate(system, 256, MEMORY_TAG_SYSTEM, &ptr_system);
    assert(MEMORY_SYSTEM_SUCCESS == ret);
    assert((128 + 256) == system->total_allocated);
    assert(128 == system->mem_tag_allocated[MEMORY_TAG_STRING]);
    assert(256 == system->mem_tag_allocated[MEMORY_TAG_SYSTEM]);

    // total = 384
    // string 128
    // system 256
    memory_system_report(system);

    system->mem_tag_str[MEMORY_TAG_STRING] = NULL;
    memory_system_report(system);

    free(system);
    system = NULL;
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
