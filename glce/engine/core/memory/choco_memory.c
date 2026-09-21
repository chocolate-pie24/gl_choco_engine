// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup core
 *
 * @file choco_memory.c
 * @author chocolate-pie24
 * @brief 不定期に発生するメモリ確保、解放に対応するメモリアロケータモジュールの実装
 *
 * @date 2025-09-20
 *
 */
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>  // for fprintf
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

/**
 * @brief メモリシステム内部状態管理構造体
 *
 */
typedef struct memory_system {
    size_t total_allocated;                     /**< メモリ総割り当て量 */
    size_t mem_tag_allocated[MEMORY_TAG_MAX];   /**< 各メモリタグごとのメモリ割り当て量 */
    const char* mem_tag_str[MEMORY_TAG_MAX];    /**< 各メモリタグ文字列 */
} memory_system_t;

static memory_system_t* s_mem_sys_ptr = NULL;   /**< メモリシステム内部状態管理構造体インスタンス */

static const char* const s_rslt_str_success = "SUCCESS";                    /**< メモリシステムAPI実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< メモリシステムAPI実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< メモリシステムAPI実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< メモリシステムAPI実行結果コード(システム使用上限超過)に対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< メモリシステムAPI実行結果コード(API誤用)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< メモリシステムAPI実行結果コード(不明なエラー)に対応する文字列 */

static const char* rslt_to_str(memory_system_result_t rslt_);
static void* test_malloc(size_t size_); // TODO: 現状はlinear_allocatorと同じだが、将来的にFreeListになった際に挙動が変わるので、とりあえずコピーを置く

memory_system_result_t choco_memory_create(void) {
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    memory_system_t* tmp = NULL;

    // Preconditions.
    if(NULL != s_mem_sys_ptr) {
        ret = MEMORY_SYSTEM_BAD_OPERATION;
        ERROR_MESSAGE("choco_memory_create(%s) - Memory system is already initialized.", rslt_to_str(ret));
        goto cleanup;
    }

    // Simulation.
    tmp = (memory_system_t*)test_malloc(sizeof(memory_system_t));
    IF_ALLOC_FAIL_GOTO_CLEANUP(tmp, ret, MEMORY_SYSTEM_NO_MEMORY, "choco_memory_create", "tmp")
    memset(tmp, 0, sizeof(memory_system_t));

    tmp->total_allocated = 0;
    for(size_t i = 0; i != MEMORY_TAG_MAX; ++i) {
        tmp->mem_tag_allocated[i] = 0;
    }
    tmp->mem_tag_str[MEMORY_TAG_SYSTEM] = "system";
    tmp->mem_tag_str[MEMORY_TAG_STRING] = "string";
    tmp->mem_tag_str[MEMORY_TAG_RING_QUEUE] = "ring_queue";
    tmp->mem_tag_str[MEMORY_TAG_RENDERER] = "renderer";
    tmp->mem_tag_str[MEMORY_TAG_FILE_IO] = "file_io";
    tmp->mem_tag_str[MEMORY_TAG_CAMERA] = "camera";
    tmp->mem_tag_str[MEMORY_TAG_TEXTURE] = "texture";
    tmp->mem_tag_str[MEMORY_TAG_GEOMETRY] = "geometry";

    // commit
    s_mem_sys_ptr = tmp;

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

void choco_memory_destroy(void) {
    if(NULL == s_mem_sys_ptr) {
        goto cleanup;
    }
    if(0 != s_mem_sys_ptr->total_allocated) {
        WARN_MESSAGE("choco_memory_destroy - total_allocated != 0. Check memory leaks.");
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

memory_system_result_t choco_memory_allocate(size_t size_, memory_tag_t mem_tag_, void** out_ptr_) {
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    void* tmp = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(s_mem_sys_ptr, ret, MEMORY_SYSTEM_BAD_OPERATION, rslt_to_str(MEMORY_SYSTEM_BAD_OPERATION), "choco_memory_allocate", "s_mem_sys_ptr")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "choco_memory_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "choco_memory_allocate", "*out_ptr_")
    IF_ARG_FALSE_GOTO_CLEANUP(mem_tag_ < MEMORY_TAG_MAX, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "choco_memory_allocate", "mem_tag_")

    if(0 == size_) {
        WARN_MESSAGE("choco_memory_allocate - No-op: size_ is 0.");
        ret = MEMORY_SYSTEM_SUCCESS;
        goto cleanup;
    }
    if(s_mem_sys_ptr->mem_tag_allocated[mem_tag_] > (SIZE_MAX - size_)) {
        ret = MEMORY_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("choco_memory_allocate(%s) - size_t overflow: tag=%s used=%zu, requested=%zu, sum would exceed SIZE_MAX.", rslt_to_str(ret), s_mem_sys_ptr->mem_tag_str[mem_tag_], s_mem_sys_ptr->mem_tag_allocated[mem_tag_], size_);
        goto cleanup;
    }
    if(s_mem_sys_ptr->total_allocated > (SIZE_MAX - size_)) {
        ret = MEMORY_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("choco_memory_allocate(%s) - size_t overflow: total_allocated=%zu, requested=%zu, sum would exceed SIZE_MAX.", rslt_to_str(ret), s_mem_sys_ptr->total_allocated, size_);
        goto cleanup;
    }

    // Simulation.
    tmp = test_malloc(size_);    // TODO: FreeList
    IF_ALLOC_FAIL_GOTO_CLEANUP(tmp, ret, MEMORY_SYSTEM_NO_MEMORY, "choco_memory_allocate", "tmp")
    memset(tmp, 0, size_);

    // commit.
    *out_ptr_ = tmp;
    s_mem_sys_ptr->total_allocated += size_;
    s_mem_sys_ptr->mem_tag_allocated[mem_tag_] += size_;

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

void choco_memory_free(void* ptr_, size_t size_, memory_tag_t mem_tag_) {
    if(NULL == s_mem_sys_ptr) {
        WARN_MESSAGE("choco_memory_free - No-op: memory system is uninitialized.");
        goto cleanup;
    }
    if(NULL == ptr_) {
        WARN_MESSAGE("choco_memory_free - No-op: 'ptr_' must not be NULL.");
        goto cleanup;
    }
    if(mem_tag_ >= MEMORY_TAG_MAX) {
        WARN_MESSAGE("choco_memory_free - No-op: 'mem_tag_' is invalid.");
        goto cleanup;
    }
    if(s_mem_sys_ptr->mem_tag_allocated[mem_tag_] < size_) {
        WARN_MESSAGE("choco_memory_free - No-op: 'mem_tag_allocated' would underflow.");
        goto cleanup;
    }
    if(s_mem_sys_ptr->total_allocated < size_) {
        WARN_MESSAGE("choco_memory_free: No-op: 'total_allocated' would underflow.");
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

/**
 * @brief 実行結果コードを文字列に変換する
 *
 * @param[in] rslt_ 文字列に変換する実行結果コード
 * @return const char* 変換された文字列の先頭アドレス
 */
static const char* rslt_to_str(memory_system_result_t rslt_) {
    switch(rslt_) {
    case MEMORY_SYSTEM_SUCCESS:
        return s_rslt_str_success;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case MEMORY_SYSTEM_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case MEMORY_SYSTEM_NO_MEMORY:
        return s_rslt_str_no_memory;
    default:
        return s_rslt_str_undefined_error;
    }
}

/**
 * @brief mallocのラッパ関数で、size_のメモリを確保する
 *
 * @note choco_memory保有APIの単体テストのため、test_config_test_mallocの設定により、強制的にNULLを返させる、以下の条件でNULLになる
 * - s_test_config_test_malloc.fail_on_call > 0 && s_test_config_test_malloc.call_count == s_test_config_test_malloc.fail_on_call
 *
 * @param[in] size_ 確保するメモリ容量
 *
 * @return void* 確保されたメモリの先頭アドレス
 */
static void* test_malloc(size_t size_) {
    void* ret = NULL;

    ret = malloc(size_);

    return ret;
}
