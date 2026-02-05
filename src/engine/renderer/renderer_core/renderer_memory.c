/** @ingroup renderer_core
 *
 * @file renderer_memory.c
 * @author chocolate-pie24
 * @brief レンダラーレイヤーでのメモリ確保、メモリ解放処理のテストのため、memory_system内のallocate, free関数のラッパーAPIの実装
 *
 * @version 0.1
 * @date 2025-12.19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stddef.h>

#include "engine/renderer/renderer_core/renderer_memory.h"
#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/core/memory/choco_memory.h"

#ifdef TEST_BUILD

#include <assert.h>
#include <stdbool.h>
#include "engine/base/choco_macros.h"   // for NO_COVERAGE
typedef struct renderer_mem_test_param {
    bool                test_enable;
    renderer_result_t   err_code;
} renderer_mem_test_param_t;

static renderer_mem_test_param_t s_test_param;

static void test_render_mem_allocate(void);
#endif

renderer_result_t render_mem_allocate(size_t size_, void** out_ptr_) {
#ifdef TEST_BUILD
    if(s_test_param.test_enable) {
        return s_test_param.err_code;
    }
#endif

    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    memory_system_result_t ret_msys = memory_system_allocate(size_, MEMORY_TAG_RENDERER, out_ptr_);
    switch(ret_msys) {
    case MEMORY_SYSTEM_SUCCESS:
        ret = RENDERER_SUCCESS;
        break;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        ret = RENDERER_INVALID_ARGUMENT;
        break;
    case MEMORY_SYSTEM_LIMIT_EXCEEDED:
        ret = RENDERER_LIMIT_EXCEEDED;
        break;
    case MEMORY_SYSTEM_NO_MEMORY:
        ret = RENDERER_NO_MEMORY;
        break;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        ret = RENDERER_RUNTIME_ERROR;
        break;
    default:
        ret = RENDERER_UNDEFINED_ERROR;
    }

    return ret;
}

void render_mem_free(void* ptr_, size_t size_) {
    memory_system_free(ptr_, size_, MEMORY_TAG_RENDERER);
}

#ifdef TEST_BUILD
void render_mem_test_param_set(renderer_result_t err_code_) {
    s_test_param.test_enable = true;
    s_test_param.err_code = err_code_;
}

void render_mem_test_param_reset(void) {
    s_test_param.test_enable = false;
}

void test_renderer_memory(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    test_render_mem_allocate();

    memory_system_destroy();
}

static void NO_COVERAGE test_render_mem_allocate(void) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    {
        char* ptr = NULL;
        render_mem_test_param_set(RENDERER_RUNTIME_ERROR);
        ret = render_mem_allocate(64, (void**)&ptr);
        assert(ret == RENDERER_RUNTIME_ERROR);
        assert(NULL == ptr);
        render_mem_test_param_reset();
    }
    {
        // malloc fail
        char* ptr = NULL;
        memory_system_test_param_set(0);
        ret = render_mem_allocate(64, (void**)&ptr);
        assert(ret == RENDERER_NO_MEMORY);
        assert(NULL == ptr);
        memory_system_test_param_reset();
    }
    {
        // invalid argument
        char* ptr = NULL;
        ret = render_mem_allocate(64, NULL);
        assert(ret == RENDERER_INVALID_ARGUMENT);
        assert(NULL == ptr);
    }
    {
        char* ptr = NULL;
        ret = render_mem_allocate(64, (void**)&ptr);
        assert(ret == RENDERER_SUCCESS);
        assert(NULL != ptr);
        render_mem_free(ptr, 64);
    }
    {
        // memory_system_allocateに強制的にMEMORY_SYSTEM_INVALID_ARGUMENTを出力させて結果をテスト
        memory_system_err_code_set(MEMORY_SYSTEM_INVALID_ARGUMENT);

        char* ptr = NULL;
        ret = render_mem_allocate(64, NULL);
        assert(ret == RENDERER_INVALID_ARGUMENT);
        assert(NULL == ptr);

        memory_system_test_param_reset();
    }
    {
        // memory_system_allocateに強制的にMEMORY_SYSTEM_LIMIT_EXCEEDEDを出力させて結果をテスト
        memory_system_err_code_set(MEMORY_SYSTEM_LIMIT_EXCEEDED);

        char* ptr = NULL;
        ret = render_mem_allocate(64, NULL);
        assert(ret == RENDERER_LIMIT_EXCEEDED);
        assert(NULL == ptr);

        memory_system_test_param_reset();
    }
    {
        // memory_system_allocateに強制的にMEMORY_SYSTEM_NO_MEMORYを出力させて結果をテスト
        memory_system_err_code_set(MEMORY_SYSTEM_NO_MEMORY);

        char* ptr = NULL;
        ret = render_mem_allocate(64, NULL);
        assert(ret == RENDERER_NO_MEMORY);
        assert(NULL == ptr);

        memory_system_test_param_reset();
    }
    {
        // memory_system_allocateに強制的に既定値以外を出力させて結果をテスト
        memory_system_err_code_set(100);

        char* ptr = NULL;
        ret = render_mem_allocate(64, NULL);
        assert(ret == RENDERER_UNDEFINED_ERROR);
        assert(NULL == ptr);

        memory_system_test_param_reset();
    }
}
#endif
