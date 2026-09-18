// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/**
 * @file entry.c
 * @author chocolate-pie24
 * @brief ゲームアプリケーションエントリーポイント
 * @date 2025-09-20
 *
 */
#include <stdint.h>
#include <stdio.h>

#include "application/application.h"

#include "engine/base/choco_message.h"

/**
 * @brief ゲームアプリケーションメイン
 *
 * @param[in] argc_ 引数の個数
 * @param[in] argv_ 引数
 *
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
#endif
    application_result_t app_run_result = APPLICATION_INVALID_ARGUMENT;
    application_result_t app_create_result = APPLICATION_INVALID_ARGUMENT;

    app_create_result = application_create();
    if(APPLICATION_SUCCESS != app_create_result) {
        ERROR_MESSAGE("Failed to create application.");
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
