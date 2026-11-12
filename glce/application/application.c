// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup application
 *
 * @file application.c
 * @author chocolate-pie24
 * @brief プロジェクトの最上位レイヤーで全サブシステムのオーケストレーションを行うAPIの実装
 *
 * @date 2025-09-20
 *
 */
#include "application/application.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>

#include <time.h>   // for nanosleep TODO: remove this!!

#include <GL/glew.h>    // TODO: remove this!! glfwSwapBuffersをrendererに移したら削除

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/core/event/keyboard_event.h"

#include "engine/io_utils/fs_path.h"

#include "engine/light/core/light_types.h"
#include "engine/light/directional_light.h"
#include "engine/light/ambient_light.h"

// Platform System
#include "engine/systems/platform_system/core/platform_system_types.h"
#include "engine/systems/platform_system/config/platform_system_config.h"
#include "engine/systems/platform_system/platform_system.h"

// Event System
#include "engine/systems/event_system/core/event_system_types.h"
#include "engine/systems/event_system/core/engine_event_view.h"
#include "engine/systems/event_system/config/event_system_config.h"
#include "engine/systems/event_system/event_system.h"

// Renderer System
#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "engine/systems/renderer/render_packet/core/render_packet_types.h"
#include "engine/systems/renderer/render_packet/render_packet.h"

// Application Layer
#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

#include "application/event/application_frame_state.h"
#include "application/cameras/application_flight_camera.h"
#include "application/diagnostics/application_diagnostics.h"


// Demos
#include "application/demos/sample_robot/core/sample_robot_types.h"
#include "application/demos/sample_robot/sample_robot_demo.h"

#include "application/demos/stl_viewer/core/stl_viewer_types.h"
#include "application/demos/stl_viewer/stl_viewer.h"

typedef enum {
    APPLICATION_DEMO_TYPE_SAMPLE_ROBOT,
    APPLICATION_DEMO_TYPE_STL_VIEWER,
} application_demo_type_t;

/**
 * @brief アプリケーション内部状態とエンジン各サブシステム状態管理構造体インスタンスを保持する
 *
 */
typedef struct application_state {
    // SubSystem Configuration
    platform_system_config_t platform_system_config;
    event_system_config_t event_system_config;
    application_diagnostics_config_t diag_config;

    // application status
    bool window_should_close;   /**< ウィンドウクローズ指示フラグ */

    // ファイルパス
    fs_path_t* executable_directory;
    fs_path_t* asset_root_path;

    // Subsystem Allocator
    subsystem_allocator_t* subsystem_allocator;

    // Platform System
    platform_system_t* platform_system;

    // Event System
    event_system_t* event_system;
    const engine_event_view_t* engine_event_view;

    // Camera System
    application_flight_camera_t* flight_camera;

    // Light
    directional_light_t directional_light;
    ambient_light_t ambient_light;

    // Renderer System
    renderer_frontend_config_t renderer_frontend_config;
    renderer_frontend_t* renderer_frontend;

    // Frame State
    application_frame_state_t frame_state;
    mat4x4f_t projection_matrix;
    mat4x4f_t view_matrix;

    // Render Packet
    render_packet_t* render_packet;

    // Demo scenes
    sample_robot_demo_t* sample_robot_demo;
    stl_viewer_t* stl_viewer_demo;
} application_state_t;

const application_demo_type_t s_demo_type = APPLICATION_DEMO_TYPE_STL_VIEWER;
static application_state_t* s_application_state = NULL; /**< アプリケーション内部状態およびエンジン各サブシステム内部状態 */

// Frame Orchestration.
static application_result_t app_state_update(void);
static application_result_t scene_update(float delta_time_, sample_robot_demo_t *demo_);
static application_result_t scene_render_packet_build(const sample_robot_demo_t *demo_, render_packet_t *packet_);

// Subsystem Configuration.
static void platform_system_config_initialize(platform_system_config_t* config_);
static void event_system_config_initialize(event_system_config_t* config_);
static void renderer_frontend_config_initialize(renderer_frontend_config_t* config_);

// Application Resource Setup.
static application_result_t directory_path_initialize(fs_path_t** out_executable_directory_, fs_path_t** out_asset_root_path_);

application_result_t application_create(void) {
    application_result_t ret = APPLICATION_RUNTIME_ERROR;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    platform_system_result_t ret_platform_system = PLATFORM_SYSTEM_INVALID_ARGUMENT;
    event_system_result_t ret_event_system = EVENT_SYSTEM_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;
    light_result_t ret_light_result = LIGHT_INVALID_ARGUMENT;
    render_packet_result_t ret_render_packet = RENDER_PACKET_INVALID_ARGUMENT;
    sample_robot_result_t ret_sample_robot = SAMPLE_ROBOT_INVALID_ARGUMENT;
    stl_viewer_result_t ret_stl_viewer = STL_VIEWER_INVALID_ARGUMENT;

    application_state_t* tmp_state = NULL;
    subsystem_allocator_t* tmp_subsystem_allocator = NULL;
    platform_system_t* tmp_platform_system = NULL;
    render_packet_t* tmp_render_packet = NULL;
    renderer_frontend_t* tmp_renderer_frontend = NULL;
    application_flight_camera_t* tmp_flight_camera = NULL;
    sample_robot_demo_t* tmp_sample_robot_demo = NULL;
    event_system_t* tmp_event_system = NULL;
    fs_path_t* tmp_executable_directory = NULL;
    fs_path_t* tmp_asset_root_path = NULL;
    stl_viewer_t* tmp_stl_viewer_demo = NULL;

    size_t subsystem_memory_pool_size = 0;

    // Preconditions
    if(NULL != s_application_state) {
        ERROR_MESSAGE("application_create(%s) - Application state is already initialized.", application_result_to_str(APPLICATION_RUNTIME_ERROR));
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    // Prepare.
    // Application Infrastructure.
    //   Application State
    //   Subsystem Allocator
    //   Directory Paths
    INFO_MESSAGE("Creating application infrastructure...");
    ret_general_allocator = general_allocator_allocate(sizeof(*tmp_state), GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM, (void**)&tmp_state);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate memory for application state.", application_result_to_str(ret));
        goto cleanup;
    }

    subsystem_memory_pool_size = 128 * KIB;
    ret_subsystem_allocator = subsystem_allocator_create(subsystem_memory_pool_size, &tmp_subsystem_allocator);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = application_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate memory for subsystem allocator.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = directory_path_initialize(&tmp_executable_directory, &tmp_asset_root_path);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - directory_path_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("Application infrastructure created successfully.");

    // Platform / Event Systems.
    INFO_MESSAGE("Creating platform system...");
    platform_system_config_initialize(&tmp_state->platform_system_config);

    ret_platform_system = platform_system_create(&tmp_state->platform_system_config, tmp_subsystem_allocator, &tmp_state->frame_state.framebuffer_width, &tmp_state->frame_state.framebuffer_height, &tmp_platform_system);
    if(PLATFORM_SYSTEM_SUCCESS != ret_platform_system) {
        ret = application_result_convert_platform_system(ret_platform_system);
        ERROR_MESSAGE("application_create(%s) - platform_system_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("Platform system created successfully.");

    INFO_MESSAGE("Creating event system...");
    event_system_config_initialize(&tmp_state->event_system_config);
    ret_event_system = event_system_create(&tmp_state->event_system_config, tmp_subsystem_allocator, tmp_platform_system, &tmp_event_system);
    if(EVENT_SYSTEM_SUCCESS != ret_event_system) {
        ret = application_result_convert_event_system(ret_event_system);
        ERROR_MESSAGE("application_create(%s) - event_system_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("Event system created successfully.");

    // Camera / Lighting.
    INFO_MESSAGE("Creating flight camera system...");
    ret = application_flight_camera_create(8, tmp_subsystem_allocator, tmp_state->frame_state.framebuffer_width, tmp_state->frame_state.framebuffer_height, &tmp_flight_camera);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - application_flight_camera_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("Flight camera system created successfully.");

    INFO_MESSAGE("Creating directional light...");
    // NOTE: 設定値の意味
    // direction: 正面から少し斜め上の光で、面の違いが見やすい
    // color: 白色光。色による影響を除外できる
    // intensity: 明るさの計算と結果を比較しやすい
    ret_light_result = directional_light_initialize(vec3f_initialize(0.4f, 0.8f, 1.0f), vec3f_initialize(1.0f, 1.0f, 1.0f), 1.0f, &tmp_state->directional_light);
    if(LIGHT_SUCCESS != ret_light_result) {
        ret = APPLICATION_RUNTIME_ERROR;    // TODO: エラーコード変換
        ERROR_MESSAGE("application_create(%s) - directional_light_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }
    ret_light_result = ambient_light_initialize(vec3f_initialize(1.0f, 1.0f, 1.0f), 1.0f, &tmp_state->ambient_light);
    if(LIGHT_SUCCESS != ret_light_result) {
        ret = APPLICATION_RUNTIME_ERROR;    // TODO: エラーコード変換
        ERROR_MESSAGE("application_create(%s) - ambient_light_initialize failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("Directional light created successfully.");

    // Renderer System.
    INFO_MESSAGE("Creating renderer frontend...");
    renderer_frontend_config_initialize(&tmp_state->renderer_frontend_config);
    ret_renderer_frontend = renderer_frontend_create(&tmp_state->renderer_frontend_config, tmp_subsystem_allocator, fs_path_fullpath_get(tmp_executable_directory), "../../assets/shaders/test_shader/", &tmp_renderer_frontend);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = application_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("application_create(%s) - renderer_frontend_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("Renderer frontend created successfully.");

    // Application Services.
    //   Diagnostics
    tmp_state->diag_config.runtime_status_report = KEY_1;
    tmp_state->diag_config.validation_report = KEY_2;

    // Demo.
    INFO_MESSAGE("Creating sample robot demo...");
    ret_sample_robot = sample_robot_demo_create(tmp_renderer_frontend, fs_path_fullpath_get(tmp_asset_root_path), &tmp_sample_robot_demo);
    if(SAMPLE_ROBOT_SUCCESS != ret_sample_robot) {
        ret = application_result_convert_sample_robot_demo(ret_sample_robot);
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret_sample_robot) {
            ret = APPLICATION_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("application_create(%s) - sample_robot_demo_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("Sample robot demo created successfully.");

    INFO_MESSAGE("Creating STL viewer demo...");
    ret_stl_viewer = stl_viewer_create(
        fs_path_fullpath_get(tmp_asset_root_path),
        "stl/demo_stl_viewer",
        "utah_teapot_ascii",
        "stl",
        "materials/demo_stl_viewer",
        "utah_teapot",
        "untextured_mat",
        tmp_renderer_frontend,
        &tmp_stl_viewer_demo);
    if(STL_VIEWER_SUCCESS != ret_stl_viewer) {
        ret = application_result_convert_stl_viewer(ret_stl_viewer);
        if(STL_VIEWER_INVALID_ARGUMENT == ret_stl_viewer) {
            ret = APPLICATION_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("application_create(%s) - stl_viewer_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("STL viewer demo created successfully.");

    // Render packet
    INFO_MESSAGE("Creating render packet...");
    ret_render_packet = render_packet_create(16 * MIB, &tmp_render_packet);
    if(RENDER_PACKET_SUCCESS != ret_render_packet) {
        ret = application_result_convert_render_packet(ret_render_packet);
        if(RENDER_PACKET_INVALID_ARGUMENT == ret_render_packet) {
            ret = APPLICATION_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("application_create(%s) - render_packet_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("Render packet created successfully.");

    ret = application_diagnostics_status_report(tmp_subsystem_allocator);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - application_diagnostics_status_report failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    tmp_state->platform_system = tmp_platform_system;
    tmp_state->subsystem_allocator = tmp_subsystem_allocator;
    tmp_state->render_packet = tmp_render_packet;
    tmp_state->renderer_frontend = tmp_renderer_frontend;
    tmp_state->flight_camera = tmp_flight_camera;
    tmp_state->sample_robot_demo = tmp_sample_robot_demo;
    tmp_state->asset_root_path = tmp_asset_root_path;
    tmp_state->executable_directory = tmp_executable_directory;
    tmp_state->event_system = tmp_event_system;
    tmp_state->stl_viewer_demo = tmp_stl_viewer_demo;

    s_application_state = tmp_state;
    tmp_state = NULL;
    tmp_subsystem_allocator = NULL;
    tmp_platform_system = NULL;
    tmp_render_packet = NULL;
    tmp_renderer_frontend = NULL;
    tmp_flight_camera = NULL;
    tmp_sample_robot_demo = NULL;
    tmp_event_system = NULL;
    tmp_executable_directory = NULL;
    tmp_asset_root_path = NULL;
    tmp_stl_viewer_demo = NULL;
    INFO_MESSAGE("Application created successfully.");

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED == ret || APPLICATION_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_render_packet) {
        render_packet_destroy(&tmp_render_packet);
    }
    if(NULL != tmp_stl_viewer_demo && NULL != tmp_renderer_frontend) {
        stl_viewer_destroy(tmp_renderer_frontend, &tmp_stl_viewer_demo);
    }
    if(NULL != tmp_sample_robot_demo && NULL != tmp_renderer_frontend) {
        sample_robot_demo_destroy(tmp_renderer_frontend, &tmp_sample_robot_demo);
    }
    if(NULL != tmp_renderer_frontend) {
        renderer_frontend_deinitialize(tmp_renderer_frontend);
    }
    if(NULL != tmp_flight_camera) {
        application_flight_camera_deinitialize(tmp_flight_camera);
    }
    if(NULL != tmp_event_system) {
        event_system_deinitialize(tmp_event_system);
    }
    if(NULL != tmp_platform_system) {
        platform_system_deinitialize(tmp_platform_system);
    }
    if(NULL != tmp_executable_directory) {
        fs_path_destroy(&tmp_executable_directory);
    }
    if(NULL != tmp_asset_root_path) {
        fs_path_destroy(&tmp_asset_root_path);
    }
    if(NULL != tmp_subsystem_allocator) {
        subsystem_allocator_destroy(&tmp_subsystem_allocator);
    }
    if(NULL != tmp_state) {
        general_allocator_free((void**)&tmp_state, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    }

    return ret;
}

// TODO: test
void application_destroy(void) {
    INFO_MESSAGE("Starting application shutdown...");
    if(NULL == s_application_state) {
        goto cleanup;
    }

    if(NULL != s_application_state->render_packet) {
        render_packet_destroy(&s_application_state->render_packet);
    }
    if(NULL != s_application_state->stl_viewer_demo) {
        stl_viewer_destroy(s_application_state->renderer_frontend, &s_application_state->stl_viewer_demo);
    }
    if(NULL != s_application_state->sample_robot_demo) {
        sample_robot_demo_destroy(s_application_state->renderer_frontend, &s_application_state->sample_robot_demo);
    }
    if(NULL != s_application_state->renderer_frontend) {
        renderer_frontend_deinitialize(s_application_state->renderer_frontend);
    }
    if(NULL != s_application_state->flight_camera) {
        application_flight_camera_deinitialize(s_application_state->flight_camera);
    }
    if(NULL != s_application_state->event_system) {
        event_system_deinitialize(s_application_state->event_system);
    }
    if(NULL != s_application_state->platform_system) {
        platform_system_deinitialize(s_application_state->platform_system);
    }
    if(NULL != s_application_state->executable_directory) {
        fs_path_destroy(&s_application_state->executable_directory);
    }
    if(NULL != s_application_state->asset_root_path) {
        fs_path_destroy(&s_application_state->asset_root_path);
    }

    application_diagnostics_status_report(s_application_state->subsystem_allocator);

    subsystem_allocator_destroy(&s_application_state->subsystem_allocator);
    general_allocator_free((void**)&s_application_state, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    INFO_MESSAGE("Freed all memory.");

    INFO_MESSAGE("Application destroyed successfully.");
cleanup:
    return;
}

application_result_t application_run(void) {
    application_result_t ret = APPLICATION_SUCCESS;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;
    stl_viewer_result_t ret_stl_viewer = STL_VIEWER_INVALID_ARGUMENT;

    struct timespec  req = {0, 1000000};
    const float demo_delta_time = 1.0f / 60.0f; // 暫定固定値

    if(NULL == s_application_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(%s) - Application is not initialized.", application_result_to_str(ret));
        goto cleanup;
    }

    glEnable(GL_PROGRAM_POINT_SIZE);    // 将来的にはrenderer_backend内にrenderer_state.hを追加してそこにOpenGL設定を行う場所を作る

    // MVP Matrix
    mat4f_identity(&s_application_state->projection_matrix);
    mat4f_identity(&s_application_state->view_matrix);

    ret = application_flight_camera_perspective_matrix_get(s_application_state->flight_camera, &s_application_state->projection_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_flight_camera_perspective_matrix_get failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_flight_camera_view_matrix_get(s_application_state->flight_camera, &s_application_state->view_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_flight_camera_view_matrix_get failed.", application_result_to_str(ret));
        goto cleanup;
    }

    application_frame_state_begin_frame(&s_application_state->frame_state);
    s_application_state->frame_state.projection_dirty = true;
    s_application_state->frame_state.view_dirty = true;
    s_application_state->frame_state.window_resized = true;
    ret_renderer_frontend = renderer_frontend_untextured_mesh_render_update(s_application_state->renderer_frontend, &s_application_state->view_matrix, &s_application_state->projection_matrix, &s_application_state->directional_light, &s_application_state->ambient_light);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = application_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("application_run(%s) - renderer_frontend_untextured_mesh_render_update failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // TODO: window NULLチェック
    while(!s_application_state->window_should_close) {
        application_frame_state_begin_frame(&s_application_state->frame_state);

        // Application State Update.
        ret = app_state_update();
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - app_state_update failed.", application_result_to_str(ret));
            goto cleanup;
        }

        // Diagnostics.
        if(s_application_state->frame_state.runtime_status_report_requested) {
            ret = application_diagnostics_status_report(s_application_state->subsystem_allocator);
            if(APPLICATION_SUCCESS != ret) {
                ERROR_MESSAGE("application_run(%s) - application_diagnostics_status_report failed.", application_result_to_str(ret));
                goto cleanup;
            }
        } else if(s_application_state->frame_state.validation_report_requested) {
            WARN_MESSAGE("application_run(%s) - not impremented yet...");
        }

        // Demo Update.
        // TODO: scene_update整理
        if(APPLICATION_DEMO_TYPE_SAMPLE_ROBOT == s_demo_type) {
            ret = scene_update(demo_delta_time, s_application_state->sample_robot_demo);
            if(APPLICATION_SUCCESS != ret) {
                ERROR_MESSAGE("application_run(%s) - scene_update failed.", application_result_to_str(ret));
                goto cleanup;
            }
        }

        // Render Frame.
        //   Framebuffer setup
        //   Render Packet build
        //   Render Packet execution
        //   Buffer swap
        // TODO: Renderer Backendへ移管する。
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, s_application_state->frame_state.framebuffer_width, s_application_state->frame_state.framebuffer_height);

        // TODO: scene_render_packet_build整理
        if(APPLICATION_DEMO_TYPE_SAMPLE_ROBOT == s_demo_type) {
            ret = scene_render_packet_build(s_application_state->sample_robot_demo, s_application_state->render_packet);
            if(APPLICATION_SUCCESS != ret) {
                ERROR_MESSAGE("application_run(%s) - scene_render_packet_build failed.", application_result_to_str(ret));
                goto cleanup;
            }
        } else if(APPLICATION_DEMO_TYPE_STL_VIEWER == s_demo_type) {
            ret_stl_viewer = stl_viewer_render_packet_build(s_application_state->stl_viewer_demo, s_application_state->render_packet);
            if(STL_VIEWER_SUCCESS != ret_stl_viewer) {
                ret = application_result_convert_stl_viewer(ret_stl_viewer);
                if(STL_VIEWER_INVALID_ARGUMENT == ret_stl_viewer) {
                    ret = APPLICATION_DATA_CORRUPTED;
                }
                ERROR_MESSAGE("application_run(%s) - stl_viewer_render_packet_build failed.", application_result_to_str(ret));
                goto cleanup;
            }
        }

        ret_renderer_frontend = renderer_frontend_draw_packet(s_application_state->render_packet, s_application_state->renderer_frontend);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = application_result_convert_renderer_frontend(ret_renderer_frontend);
            if(RENDERER_FRONTEND_INVALID_ARGUMENT == ret_renderer_frontend) {
                ret = APPLICATION_DATA_CORRUPTED;
                ERROR_MESSAGE("application_run(%s) - renderer_frontend_draw_packet failed.", application_result_to_str(ret));
                goto cleanup;
            }
            ERROR_MESSAGE("application_run(%s) - renderer_frontend_draw_packet failed.", application_result_to_str(ret));
            continue;
        }

        platform_system_swap_buffers(s_application_state->platform_system);

        nanosleep(&req, NULL);
    }
cleanup:
    return ret;
}

static application_result_t app_state_update(void) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    event_system_result_t ret_event_system = EVENT_SYSTEM_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    // Preconditions.
    if(NULL == s_application_state) {
        ret = APPLICATION_BAD_OPERATION;
        ERROR_MESSAGE("app_state_update(%s) - Application state is not initialized.", application_result_to_str(ret));
        goto cleanup;
    }

    // Event System.
    //   OS / Engine events
    //   Window close request
    ret_event_system = event_system_update(s_application_state->event_system, &s_application_state->engine_event_view);
    if(EVENT_SYSTEM_SUCCESS != ret_event_system) {
        ret = application_result_convert_event_system(ret_event_system);
        ERROR_MESSAGE("app_state_update(%s) - event_system_update failed.", application_result_to_str(ret));
        goto cleanup;
    }
    s_application_state->window_should_close = s_application_state->engine_event_view->window_close_requested;

    // Camera System.
    //   Camera command execution
    //   View / Projection matrices
    ret = application_flight_camera_update(s_application_state->flight_camera, 0.1f, 1.0f, s_application_state->engine_event_view, &s_application_state->frame_state);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_flight_camera_update failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_flight_camera_view_matrix_get(s_application_state->flight_camera, &s_application_state->view_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_flight_camera_view_matrix_get failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_flight_camera_perspective_matrix_get(s_application_state->flight_camera, &s_application_state->projection_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_flight_camera_perspective_matrix_get failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Renderer State.
    //   Untextured Mesh
    //   Line Mesh
    ret_renderer_frontend = renderer_frontend_untextured_mesh_render_update(s_application_state->renderer_frontend, &s_application_state->view_matrix, &s_application_state->projection_matrix, &s_application_state->directional_light, &s_application_state->ambient_light);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = application_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("app_state_update(%s) - renderer_frontend_untextured_mesh_render_update failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_frontend = renderer_frontend_line_mesh_render_update(s_application_state->renderer_frontend, &s_application_state->view_matrix, &s_application_state->projection_matrix);
    if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
        ret = application_result_convert_renderer_frontend(ret_renderer_frontend);
        ERROR_MESSAGE("app_state_update(%s) - renderer_frontend_line_mesh_render_update failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Diagnostics.
    ret = application_diagnostics_update(&s_application_state->diag_config, s_application_state->engine_event_view, &s_application_state->frame_state);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_diagnostics_update failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Postconditions.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_frame_state_is_valid(&s_application_state->frame_state)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("app_state_update(%s) - Postcondition validation failed for 's_application_state->frame_state'.", application_result_to_str(ret));
        goto cleanup;
    }
#endif

cleanup:
    return ret;
}

static application_result_t scene_update(float delta_time_, sample_robot_demo_t *demo_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    sample_robot_result_t ret_sample_robot = SAMPLE_ROBOT_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(demo_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "scene_update", "demo_")

    ret_sample_robot = sample_robot_demo_posture_update(delta_time_, demo_);   // 仮で60FPSを前提
    if(SAMPLE_ROBOT_SUCCESS != ret_sample_robot) {
        ret = application_result_convert_sample_robot_demo(ret_sample_robot);
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret_sample_robot) {
            ret = APPLICATION_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("scene_update(%s) - sample_robot_demo_posture_update failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

static application_result_t scene_render_packet_build(const sample_robot_demo_t *demo_, render_packet_t *packet_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    sample_robot_result_t ret_sample_robot = SAMPLE_ROBOT_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(demo_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "scene_render_packet_build", "demo_")
    IF_ARG_NULL_GOTO_CLEANUP(packet_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "scene_render_packet_build", "packet_")

    ret_sample_robot = sample_robot_demo_render_packet_build(demo_, packet_);
    if(SAMPLE_ROBOT_SUCCESS != ret_sample_robot) {
        ret = application_result_convert_sample_robot_demo(ret_sample_robot);
        if(SAMPLE_ROBOT_INVALID_ARGUMENT == ret_sample_robot) {
            ret = APPLICATION_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("scene_render_packet_build(%s) - sample_robot_demo_render_packet_build failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

static void platform_system_config_initialize(platform_system_config_t* config_) {
    config_->max_keyboard_event_count = KEY_CODE_MAX;
    config_->max_mouse_event_count = 8;
    config_->max_window_event_count = 8;
    config_->window_height = 768;
    config_->window_width = 1024;
    config_->window_label = "test_window";
}

static void event_system_config_initialize(event_system_config_t* config_) {
    config_->max_keyboard_event_count = KEY_CODE_MAX;
    config_->max_mouse_event_count = 8;
    config_->max_window_event_count = 8;
}

static void renderer_frontend_config_initialize(renderer_frontend_config_t* config_) {
    config_->line_mesh_resource_config.max_geometry_count = 128;
    config_->line_mesh_resource_config.vbo_config.base_align = alignof(float);
    config_->line_mesh_resource_config.vbo_config.buffer_usage = BUFFER_USAGE_STATIC;
    config_->line_mesh_resource_config.vbo_config.max_allocation_count = 512;
    config_->line_mesh_resource_config.vbo_config.vbo_size = 1 * MIB;

    config_->untextured_mesh_resource_config.max_geometry_count = 128;
    config_->untextured_mesh_resource_config.max_material_count = 128;
    config_->untextured_mesh_resource_config.vbo_config.base_align = alignof(max_align_t);
    config_->untextured_mesh_resource_config.vbo_config.buffer_usage = BUFFER_USAGE_STATIC;
    config_->untextured_mesh_resource_config.vbo_config.max_allocation_count = 512;
    config_->untextured_mesh_resource_config.vbo_config.vbo_size = 1 * GIB;
}

static application_result_t directory_path_initialize(fs_path_t** out_executable_directory_, fs_path_t** out_asset_root_path_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* tmp_executable_directory_path = NULL;
    fs_path_t* tmp_asset_root_path = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_executable_directory_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "directory_path_initialize", "out_executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(out_asset_root_path_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "directory_path_initialize", "out_asset_root_path_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_executable_directory_, ret, APPLICATION_BAD_OPERATION, application_result_to_str(APPLICATION_BAD_OPERATION), "directory_path_initialize", "*out_executable_directory_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_asset_root_path_, ret, APPLICATION_BAD_OPERATION, application_result_to_str(APPLICATION_BAD_OPERATION), "directory_path_initialize", "*out_asset_root_path_")

    ret_fs_path = fs_path_create_from_executable_directory(&tmp_executable_directory_path);
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = application_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("directory_path_initialize(%s) - fs_path_create_from_executable_directory failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&tmp_asset_root_path, fs_path_fullpath_get(tmp_executable_directory_path), "../..", "assets", NULL);
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = application_result_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("directory_path_initialize(%s) - fs_path_create failed.", application_result_to_str(ret));
        goto cleanup;
    }

    *out_executable_directory_ = tmp_executable_directory_path;
    *out_asset_root_path_ = tmp_asset_root_path;
    tmp_executable_directory_path = NULL;
    tmp_asset_root_path = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED == ret || APPLICATION_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_executable_directory_path) {
        fs_path_destroy(&tmp_executable_directory_path);
    }
    if(NULL != tmp_asset_root_path) {
        fs_path_destroy(&tmp_asset_root_path);
    }
    return ret;
}
