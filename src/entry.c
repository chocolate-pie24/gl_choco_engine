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

// test: base
#include "engine/base/choco_math/test_choco_math.h"

// test: core
#include "engine/core/memory/test_linear_allocator.h"
#include "engine/core/memory/test_choco_memory.h"
#include "engine/core/filesystem/test_filesystem.h"

// test: containers
#include "engine/containers/test_choco_string.h"
#include "engine/containers/test_ring_queue.h"

// test: io_utils
#include "engine/io_utils/fs_utils/test_fs_utils.h"

// test: platform
#include "engine/platform/platform_core/test_platform_err_utils.h"
#include "engine/platform/platform_concretes/test_platform_glfw.h"
#include "engine/platform/test_platform_context.h"

// test: camera_system
#include "engine/camera_system/camera_core/test_camera_err_utils.h"
#include "engine/camera_system/camera_core/test_camera_memory.h"
#include "engine/camera_system/camera/test_camera.h"
#include "engine/camera_system/camera_controller/test_flight_camera_controller.h"
#include "engine/camera_system/camera_manager/test_camera_manager.h"

// test: renderer
#include "engine/renderer/renderer_core/test_renderer_err_utils.h"
#include "engine/renderer/renderer_core/test_renderer_memory.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/test_concrete_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/test_concrete_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/test_concrete_vbo.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_context_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_context_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_context_vbo.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/test_renderer_backend_context.h"

#endif

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
    for(uint8_t i = 0; i != 200; ++i) {
        message_output(100, NULL);

        // engine/base
        test_choco_math();

        // engine/core
        test_linear_allocator();
        test_choco_memory();
        test_filesystem();

        // engine/containers
        test_choco_string();
        test_ring_queue();

        // engine/io_utils
        test_fs_utils();

        // engine/camera
        test_camera_err_utils();
        test_camera_memory();
        test_camera();
        test_flight_camera_controller();
        test_camera_manager();

        // engine/platform
        test_platform_err_utils();
        test_platform_glfw();
        test_platform_context();

        // engine/renderer
        test_renderer_err_utils();
        test_renderer_memory();
        test_concrete_shader();
        test_concrete_vao();
        test_concrete_vbo();
        test_renderer_backend_context();
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
