/**
 * @file entry.c
 * @author chocolate-pie24
 * @brief ゲームアプリケーションエントリーポイント
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdint.h>
#include <stdio.h>

#include "application/application.h"

#include "engine/base/choco_message.h"

#ifdef TEST_BUILD   // TODO: test用のmainを用意して別に移す
#include "engine/core/memory/choco_memory.h"

#include "test_linear_allocator.h"
#include "test_memory_system.h"
#include "test_choco_string.h"
#include "test_ring_queue.h"
#include "test_platform_context.h"
#include "test_platform_glfw.h"
#endif

/**
 * @brief ゲームアプリケーションメイン
 *
 * @param argc_ 引数の個数
 * @param argv_ 引数
 * @return int
 */
int main(int argc_, char** argv_) {
    (void)argc_;
    (void)argv_;
#ifdef RELEASE_BUILD
    INFO_MESSAGE("Build mode: RELEASE.");
#endif
#ifdef DEBUG_BUILD
    INFO_MESSAGE("Build mode: DEBUG.");
#endif
#ifdef TEST_BUILD
    INFO_MESSAGE("Build mode: TEST.");
    for(uint8_t i = 0; i != 200; ++i) {
        test_linear_allocator();
        test_memory_system();
        test_choco_string();

        memory_system_create();
        test_ring_queue();
        memory_system_destroy();

        memory_system_create();
        test_platform_context();
        memory_system_destroy();

        test_platform_glfw();
    }
#endif
    application_result_t app_run_result = APPLICATION_INVALID_ARGUMENT;
    application_result_t app_create_result = APPLICATION_INVALID_ARGUMENT;

    app_create_result = application_create();
    if(APPLICATION_SUCCESS != app_create_result) {
        ERROR_MESSAGE("Failed to initialize application.");
        goto cleanup;
    }

    app_run_result = application_run();
    if(APPLICATION_SUCCESS != app_run_result) {
        ERROR_MESSAGE("Failed to execute application.");
        goto cleanup;
    }

cleanup:
    application_destroy();
    return 0;
}
