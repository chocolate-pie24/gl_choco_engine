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
#include <string.h> // for memset

#include "application/application.h"

#include "engine/base/choco_message.h"
#include "engine/base/choco_macros.h"

#include "engine/core/memory/linear_allocator.h"
#include "engine/core/memory/choco_memory.h"

// begin temporary
#include "application/platform_registry.h"
#include "engine/core/platform/platform_utils.h"
#include "engine/interfaces/platform_interface.h"
// end temporary

/**
 * @brief アプリケーション内部状態とエンジン各サブシステム状態管理オブジェクトを保持するオブジェクト
 *
 */
typedef struct app_state {
    // core/memory/linear_allocator
    size_t linear_alloc_mem_req;
    size_t linear_alloc_align_req;
    size_t linear_alloc_pool_size;
    void* linear_alloc_pool;
    linear_alloc_t* linear_alloc;   /**< リニアアロケータオブジェクト */

    // interfaces/platform_interface
    size_t platform_state_memory_requirement;
    size_t platform_state_alignment_requirement;
    platform_state_t* platform_state;
    const platform_vtable_t* platform_vtable;
} app_state_t;

static app_state_t* s_app_state = NULL; /**< アプリケーション内部状態およびエンジン各サブシステム内部状態 */

// TODO: oc_choco_malloc + テスト
app_err_t application_create(void) {
    app_state_t* tmp = NULL;
    void* tmp_platform_state_ptr = NULL;

    app_err_t ret = APPLICATION_RUNTIME_ERROR;
    memory_sys_err_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_alloc_err_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    platform_error_t ret_platform_state_init = PLATFORM_INVALID_ARGUMENT;

    // Preconditions
    if(NULL != s_app_state) {
        ERROR_MESSAGE("application_create(RUNTIME_ERROR) - Application state is already initialized.");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    ret_mem_sys = memory_system_create();
    if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_mem_sys) {
        ERROR_MESSAGE("application_create(INVALID_ARGUMENT) - Failed to create memory system.");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    } else if(MEMORY_SYSTEM_RUNTIME_ERROR == ret_mem_sys) {
        ERROR_MESSAGE("application_create(RUNTIME_ERROR) - Failed to create memory system.");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    } else if(MEMORY_SYSTEM_NO_MEMORY == ret_mem_sys) {
        ERROR_MESSAGE("application_create(NO_MEMORY) - Failed to create memory system.");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    } else if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ERROR_MESSAGE("application_create(UNDEFINED_ERROR) - Failed to create memory system.");
        ret = APPLICATION_UNDEFINED_ERROR;
        goto cleanup;
    }

    // begin Simulation
    ret_mem_sys = memory_system_allocate(sizeof(*tmp), MEMORY_TAG_SYSTEM, (void**)&tmp);
    if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_mem_sys) {
        ERROR_MESSAGE("application_create(INVALID_ARGUMENT) - Failed to allocate app_state memory.");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    } else if(MEMORY_SYSTEM_NO_MEMORY == ret_mem_sys) {
        ERROR_MESSAGE("application_create(NO_MEMORY) - Failed to allocate app_state memory.");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    } else if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ERROR_MESSAGE("application_create(UNDEFINED_ERROR) - Failed to allocate app_state memory.");
        ret = APPLICATION_UNDEFINED_ERROR;
        goto cleanup;
    }
    memset(tmp, 0, sizeof(*tmp));

    // begin Simulation -> launch all systems.(Don't use s_app_state here.)

    // [NOTE] linear_allocatorのプールサイズについて
    //   全サブシステムのpreinitを先に実行し、リニアアロケータで必要な容量を計算可能だが、
    //   各サブシステムのアライメント要件を考慮すると単純に総和を取れば良いと言うものではなく、ちょっと複雑
    //   当面は実施せず、多めにメモリを確保する方針にする

    // Simulation -> launch all systems -> create linear allocator.(Don't use s_app_state here.)
    INFO_MESSAGE("Starting linear_allocator initialize...");
    tmp->linear_alloc = NULL;
    linear_allocator_preinit(&tmp->linear_alloc_mem_req, &tmp->linear_alloc_align_req);
    // TODO: memory_system_allocate_aligned作成後に入れ替え
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&tmp->linear_alloc);
    if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_mem_sys) {
        ERROR_MESSAGE("application_create(INVALID_ARGUMENT) - Failed to allocate linear allocator memory.");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    } else if(MEMORY_SYSTEM_NO_MEMORY == ret_mem_sys) {
        ERROR_MESSAGE("application_create(NO_MEMORY) - Failed to allocate linear allocator memory.");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    } else if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ERROR_MESSAGE("application_create(UNDEFINED_ERROR) - Failed to allocate linear allocator memory.");
        ret = APPLICATION_UNDEFINED_ERROR;
        goto cleanup;
    }

    tmp->linear_alloc_pool_size = 1 * KIB;
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &tmp->linear_alloc_pool);
    if(MEMORY_SYSTEM_INVALID_ARGUMENT == ret_mem_sys) {
        ERROR_MESSAGE("application_create(INVALID_ARGUMENT) - Failed to allocate linear allocator pool memory.");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    } else if(MEMORY_SYSTEM_NO_MEMORY == ret_mem_sys) {
        ERROR_MESSAGE("application_create(NO_MEMORY) - Failed to allocate linear allocator pool memory.");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    } else if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ERROR_MESSAGE("application_create(UNDEFINED_ERROR) - Failed to allocate linear allocator pool memory.");
        ret = APPLICATION_UNDEFINED_ERROR;
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_init(tmp->linear_alloc, tmp->linear_alloc_pool_size, tmp->linear_alloc_pool);
    if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_linear_alloc) {
        ERROR_MESSAGE("application_create(INVALID_ARGUMENT) - Failed to initialize linear allocator.");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    } else if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ERROR_MESSAGE("application_create(UNDEFINED_ERROR) - Failed to initialize linear allocator.");
        ret = APPLICATION_UNDEFINED_ERROR;
        goto cleanup;
    }
    INFO_MESSAGE("linear_allocator initialized successfully.");

    // Simulation -> launch all systems -> create platform state.(Don't use s_app_state here.)
    INFO_MESSAGE("Starting platform_state initialize...");
    tmp->platform_vtable = NULL;
    tmp->platform_vtable = platform_registry_vtable_get(PLATFORM_USE_GLFW); // TODO: #ifdefで切り分け
    if(NULL == tmp->platform_vtable) {
        ERROR_MESSAGE("Failed to get platform vtable.");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }
    tmp->platform_state = NULL;
    tmp->platform_vtable->platform_state_preinit(&tmp->platform_state_memory_requirement, &tmp->platform_state_alignment_requirement);
    ret_linear_alloc = linear_allocator_allocate(tmp->linear_alloc, tmp->platform_state_memory_requirement, tmp->platform_state_alignment_requirement, &tmp_platform_state_ptr);
    if(LINEAR_ALLOC_NO_MEMORY == ret_linear_alloc) {
        ERROR_MESSAGE("Failed to allocate platform state memory. NO_MEMORY");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    } else if(LINEAR_ALLOC_INVALID_ARGUMENT == ret_linear_alloc) {
        ERROR_MESSAGE("Failed to allocate platform state memory. INVALID_ARGUMENT");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    } else if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ERROR_MESSAGE("Failed to allocate platform state memory. UNDEFINED_ERROR");
        ret = APPLICATION_UNDEFINED_ERROR;
        goto cleanup;
    }
    ret_platform_state_init = tmp->platform_vtable->platform_state_init((platform_state_t*)tmp_platform_state_ptr);
    if(PLATFORM_RUNTIME_ERROR == ret_platform_state_init) {
        ERROR_MESSAGE("Failed to initialize platform state. RUNTIME_ERROR");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    } else if(PLATFORM_INVALID_ARGUMENT == ret_platform_state_init) {
        ERROR_MESSAGE("Failed to initialize platform state. INVALID_ARGUMENT");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    } else if(PLATFORM_SUCCESS != ret_platform_state_init) {
        ERROR_MESSAGE("Failed to initialize platform state. UNDEFINED_ERROR");
        ret = APPLICATION_UNDEFINED_ERROR;
        goto cleanup;
    }
    tmp->platform_state = (platform_state_t*)tmp_platform_state_ptr;
    INFO_MESSAGE("platform_state initialized successfully.");

    // end Simulation -> launch all systems.
    // end Simulation

    // begin temporary
    // TODO: ウィンドウ生成はレンダラー作成時にそっちに移す
    ret_platform_state_init = tmp->platform_vtable->platform_window_create(tmp->platform_state, "test_window", 1024, 768);
    if(PLATFORM_INVALID_ARGUMENT == ret_platform_state_init) {
        ERROR_MESSAGE("Failed to create window. INVALID_ARGUMENT");
        ret = APPLICATION_INVALID_ARGUMENT;
        goto cleanup;
    } else if(PLATFORM_RUNTIME_ERROR == ret_platform_state_init) {
        ERROR_MESSAGE("Failed to create window. RUNTIME_ERROR");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    } else if(PLATFORM_NO_MEMORY == ret_platform_state_init) {
        ERROR_MESSAGE("Failed to create window. NO_MEMORY");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    } else if(PLATFORM_SUCCESS != ret_platform_state_init) {
        ERROR_MESSAGE("Failed to create window. UNDEFINED_ERROR");
        ret = APPLICATION_UNDEFINED_ERROR;
        goto cleanup;
    }
    // end temporary

    // commit
    s_app_state = tmp;
    INFO_MESSAGE("Application created successfully.");
    memory_system_report();
    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_SUCCESS != ret) {
        if(NULL != tmp && NULL != tmp->platform_state) {
            if(NULL != tmp->platform_vtable) {
                tmp->platform_vtable->platform_state_destroy(tmp->platform_state);
            }
        }
        if(NULL != tmp) {
            if(NULL != tmp->linear_alloc_pool) {
                memory_system_free(tmp->linear_alloc_pool, tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
            }
            if(NULL != tmp->linear_alloc) {
                memory_system_free(tmp->linear_alloc, tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
            }
            memory_system_free(tmp, sizeof(*tmp), MEMORY_TAG_SYSTEM);
            tmp = NULL;
        }
        memory_system_destroy();
    }

    return ret;
}

// TODO: test
void application_destroy(void) {
    INFO_MESSAGE("starting application_destroy...");
    if(NULL == s_app_state) {
        goto cleanup;
    }

    // begin cleanup all systems.
    if(NULL != s_app_state->platform_vtable) {
        s_app_state->platform_vtable->platform_state_destroy(s_app_state->platform_state);
    }
    if(NULL != s_app_state->linear_alloc_pool) {
        memory_system_free(s_app_state->linear_alloc_pool, s_app_state->linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
        s_app_state->linear_alloc_pool = NULL;
    }
    if(NULL != s_app_state->linear_alloc) {
        memory_system_free(s_app_state->linear_alloc, s_app_state->linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
        s_app_state->linear_alloc = NULL;
    }

    memory_system_free(s_app_state, sizeof(*s_app_state), MEMORY_TAG_SYSTEM);
    s_app_state = NULL;
    INFO_MESSAGE("All memory freed.");
    memory_system_report();
    memory_system_destroy();
    // end cleanup all systems.

    INFO_MESSAGE("Application destroyed successfully.");
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
