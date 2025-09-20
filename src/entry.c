/**
 * @file entry.c
 * @author chocolate-pie24
 * @brief ゲームアプリケーションエントリーポイント
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdio.h>
#include <stdint.h>

#include "application/application.h"

#include "engine/base/choco_message.h"

#ifdef TEST_BUILD   // TODO: test用のmainを用意して別に移す
#include "test_linear_allocator.h"
#include "test_memory_system.h"
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
    }
#endif

    const app_err_t app_create_result = application_create();
    if(APPLICATION_SUCCESS != app_create_result) {
        ERROR_MESSAGE("Failed to initialize application.");
        goto cleanup;
    } else {
        INFO_MESSAGE("Application created successfully.");
    }

    const app_err_t app_run_result = application_run();
    if(APPLICATION_SUCCESS != app_run_result) {
        ERROR_MESSAGE("Failed to execute application.");
        goto cleanup;
    } else {
        INFO_MESSAGE("Application executed successfully.");
    }

cleanup:
    application_destroy();
    INFO_MESSAGE("Application destroyed successfully.");
    return 0;
}
