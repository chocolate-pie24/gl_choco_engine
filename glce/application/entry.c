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

// test: application/core
#include "application/core/test_application_err_utils.h"

// test: application/command_interpreter

// test: engine/base
#include "engine/base/choco_math/test_choco_math.h"

// test: engine/core
#include "engine/core/memory/test_linear_allocator.h"
#include "engine/core/memory/test_choco_memory.h"
#include "engine/core/buffer_utils/test_buffer_utils.h"
#include "engine/core/geometry_primitive/test_aabb_3d.h"
#include "engine/core/geometry_primitive/test_geometry_primitive_err_utils.h"

// test: engine/containers

// test: engine/io_utils

// test: engine/resource
#include "engine/resource/core/test_resource_err_utils.h"
#include "engine/resource/loaders/test_bmp_loader.h"
#include "engine/resource/loaders/test_stl_loader.h"
#include "engine/resource/geometry/test_lit_mesh_geometry.h"
#include "engine/resource/geometry/test_line_mesh_geometry.h"
#include "engine/resource/geometry/test_point_mesh_geometry.h"
#include "engine/resource/geometry/test_ui_mesh_geometry.h"

// test: engine/systems/camera_system
#include "engine/systems/camera_system/camera_core/test_camera_err_utils.h"
#include "engine/systems/camera_system/camera_core/test_camera_memory.h"

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

        // application/core
        // test_application_err_utils();

        // application/command_interpreter

        // engine/base
        test_choco_math();

        // engine/core
        test_linear_allocator();
        test_choco_memory();
        test_buffer_utils();
        test_aabb_3d();
        test_geometry_primitive_err_utils();

        // engine/containers

        // engine/io_utils

        // engine/resource
        // test_resource_err_utils();
        // test_bmp_loader();
        // test_stl_loader();
        // test_lit_mesh_geometry();
        // test_line_mesh_geometry();
        // test_point_mesh_geometry();
        // test_ui_mesh_geometry();

        // engine/camera
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
