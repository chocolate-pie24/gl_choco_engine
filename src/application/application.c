/**
 * @file application.c
 * @author chocolate-pie24
 * @brief 最上位のオーケストレーション。サブシステム初期化、メインループ駆動、終了処理の実装
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stddef.h> // for NULL
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset
#include <stdalign.h>

#include "application/application.h"

#include "engine/base/choco_message.h"
#include "engine/base/choco_macros.h"

#include "engine/core/memory/linear_allocator.h"
#include "engine/core/memory/choco_memory.h"

/**
 * @brief アプリケーション内部状態とエンジン各サブシステム状態管理オブジェクトを保持するオブジェクト
 *
 */
typedef struct app_state {
    // core/memory/linear_allocator
    linear_alloc_t* linear_allocator;   /**< リニアアロケータオブジェクト */

    // core/memory/memory_system
    size_t memory_system_memory_requirement;        /**< メモリーシステム要求メモリ量 */
    size_t memory_system_alignment_requirement;     /**< メモリーシステムメモリアライメント要件 */
    memory_system_t* memory_system;                 /**< メモリーシステム内部状態管理オブジェクトへのポインタ */
} app_state_t;

static app_state_t* s_app_state = NULL; /**< アプリケーション内部状態およびエンジン各サブシステム内部状態 */

// TODO: oc_choco_malloc + テスト
app_err_t application_create(void) {
    app_err_t ret = APPLICATION_RUNTIME_ERROR;
    linear_alloc_err_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    memory_sys_err_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;

    app_state_t* tmp = NULL;

    // Preconditions
    if(NULL != s_app_state) {   // TODO: CHECK_NOT_VALID_GOTO_CLEANUP()
        ERROR_MESSAGE("application_create(RUNTIME_ERROR) - Application state is already initialized.\n");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    // begin Simulation
    tmp = (app_state_t*)malloc(sizeof(*tmp)); // TODO: choco_malloc
    if(NULL == tmp) {   // TODO: CHECK_ALLOC_ERR_GOTO_CLEANUP()
        ERROR_MESSAGE("application_create(NO_MEMORY) - Failed to allocate app_state memory.\n");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    }
    memset(tmp, 0, sizeof(*tmp));

    // begin Simulation -> launch all systems.(Don't use s_app_state here.)

    // Simulation -> launch all systems -> create linear allocator.(Don't use s_app_state here.)
    tmp->linear_allocator = NULL;
    ret_linear_alloc = linear_allocator_create(&tmp->linear_allocator, 1 * KIB);
    if(LINEAR_ALLOC_NO_MEMORY == ret_linear_alloc) {
        ERROR_MESSAGE("Failed to create linear allocator.");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_linear_alloc) {
        ERROR_MESSAGE("Failed to create linear allocator.");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    } else if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ERROR_MESSAGE("Failed to create linear allocator.");
        ret = APPLICATION_UNDEFINED_ERROR;
        goto cleanup;
    }

    // Simulation -> launch all systems -> create memory system.(Don't use s_app_state here.)
    tmp->memory_system = NULL;
    memory_system_preinit(&tmp->memory_system_memory_requirement, &tmp->memory_system_alignment_requirement);
    void* tmp_memory_system_ptr = NULL;
    linear_alloc_err_t ret_memory_system_allocate = linear_allocator_allocate(tmp->linear_allocator, tmp->memory_system_memory_requirement, tmp->memory_system_alignment_requirement, &tmp_memory_system_ptr);
    if(LINEAR_ALLOC_NO_MEMORY == ret_memory_system_allocate) {
        ERROR_MESSAGE("Failed to allocate memory system memory.");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_memory_system_allocate) {
        ERROR_MESSAGE("Failed to allocate memory system memory.");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    }
    memory_sys_err_t ret_memory_system_init = memory_system_init(tmp_memory_system_ptr);
    if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_memory_system_init) {
        ERROR_MESSAGE("Failed to initialize memory system.");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    }
    tmp->memory_system = tmp_memory_system_ptr;

    // end Simulation -> launch all systems.
    // end Simulation

    // commit
    s_app_state = tmp;
    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_SUCCESS != ret) {
        if(NULL != tmp && NULL != tmp->memory_system) {
            memory_system_destroy(tmp->memory_system);
        }
        if(NULL != tmp) {
            linear_allocator_destroy(&tmp->linear_allocator);
            free(tmp);
            tmp = NULL;
        }
    }

    return ret;
}

// TODO: test
void application_destroy(void) {
    if(NULL == s_app_state) {
        goto cleanup;
    }

    // begin cleanup all systems.
    memory_system_destroy(s_app_state->memory_system);
    linear_allocator_destroy(&s_app_state->linear_allocator);
    // end cleanup all systems.

    free(s_app_state);  // TODO: choco_free
    s_app_state = NULL;
cleanup:
    return;
}

app_err_t application_run(void) {
    app_err_t ret = APPLICATION_SUCCESS;
    if(NULL == s_app_state) {
        ERROR_MESSAGE("application_run(APPLICATION_RUNTIME_ERROR) - Application is not initialized.\n");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }
    // while(APPLICATION_SUCCESS == ret) {
    // }

cleanup:
    return ret;
}
