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
#include <stdbool.h>
#include <stdio.h>

#include "application/application.h"

#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

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
    application_result_t ret_application = APPLICATION_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    bool general_allocator_created = false;
    bool application_created = false;

    ret_general_allocator = general_allocator_create();
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ERROR_MESSAGE("main - general_allocator_create failed.");
        goto cleanup;
    }
    general_allocator_created = true;

    ret_application = application_create();
    if(APPLICATION_SUCCESS != ret_application) {
        ERROR_MESSAGE("Failed to create application.");
        goto cleanup;
    }
    application_created = true;

    ret_application = application_run();
    if(APPLICATION_SUCCESS != ret_application) {
        ERROR_MESSAGE("Failed to execute application.");
        goto cleanup;
    }

cleanup:
    if(application_created) {
        application_destroy();
    }
    if(general_allocator_created) {
        general_allocator_destroy();
    }
    return 0;
}
