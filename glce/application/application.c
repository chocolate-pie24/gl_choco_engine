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

#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

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
#include "application/demos/core/scene_types.h"
#include "application/demos/sample_robot/sample_robot.h"
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

    // Renderer System
    renderer_frontend_config_t renderer_frontend_config;
    renderer_frontend_t* renderer_frontend;

    // Frame State
    application_frame_state_t frame_state;

    // Demo scenes
    sample_robot_t* sample_robot_demo;
    stl_viewer_t* stl_viewer_demo;
} application_state_t;

static const application_demo_type_t s_demo_type = APPLICATION_DEMO_TYPE_SAMPLE_ROBOT;
static application_state_t* s_application_state = NULL; /**< アプリケーション内部状態およびエンジン各サブシステム内部状態 */

// Frame Orchestration.
static application_result_t application_events_update(void);

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
    scene_result_t ret_scene = SCENE_INVALID_ARGUMENT;

    application_state_t* tmp_state = NULL;
    subsystem_allocator_t* tmp_subsystem_allocator = NULL;
    platform_system_t* tmp_platform_system = NULL;
    renderer_frontend_t* tmp_renderer_frontend = NULL;
    event_system_t* tmp_event_system = NULL;
    fs_path_t* tmp_executable_directory = NULL;
    fs_path_t* tmp_asset_root_path = NULL;

    sample_robot_t* tmp_sample_robot_demo = NULL;
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
    switch(s_demo_type) {
    case APPLICATION_DEMO_TYPE_SAMPLE_ROBOT:
        INFO_MESSAGE("Creating sample robot demo...");
        ret_scene = sample_robot_create(tmp_renderer_frontend, fs_path_fullpath_get(tmp_asset_root_path), tmp_state->frame_state.framebuffer_width, tmp_state->frame_state.framebuffer_height, &tmp_sample_robot_demo);
        if(SCENE_SUCCESS != ret_scene) {
            ret = application_result_convert_scene(ret_scene);
            if(SCENE_INVALID_ARGUMENT == ret_scene) {
                ret = APPLICATION_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("application_create(%s) - sample_robot_create failed.", application_result_to_str(ret));
            goto cleanup;
        }
        INFO_MESSAGE("Sample robot demo created successfully.");
        break;
    case APPLICATION_DEMO_TYPE_STL_VIEWER:
        INFO_MESSAGE("Creating STL viewer demo...");
        ret_scene = stl_viewer_create(tmp_renderer_frontend, fs_path_fullpath_get(tmp_asset_root_path), tmp_state->frame_state.framebuffer_width, tmp_state->frame_state.framebuffer_height, &tmp_stl_viewer_demo);
        if(SCENE_SUCCESS != ret_scene) {
            ret = application_result_convert_scene(ret_scene);
            if(SCENE_INVALID_ARGUMENT == ret_scene) {
                ret = APPLICATION_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("application_create(%s) - stl_viewer_create failed.", application_result_to_str(ret));
            goto cleanup;
        }
        INFO_MESSAGE("STL viewer demo created successfully.");
        break;
    default:
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_create(%s) - Invalid demo type.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_diagnostics_status_report(tmp_subsystem_allocator);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - application_diagnostics_status_report failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    tmp_state->platform_system = tmp_platform_system;
    tmp_state->subsystem_allocator = tmp_subsystem_allocator;
    tmp_state->renderer_frontend = tmp_renderer_frontend;
    tmp_state->sample_robot_demo = tmp_sample_robot_demo;
    tmp_state->asset_root_path = tmp_asset_root_path;
    tmp_state->executable_directory = tmp_executable_directory;
    tmp_state->event_system = tmp_event_system;
    tmp_state->stl_viewer_demo = tmp_stl_viewer_demo;

    s_application_state = tmp_state;
    tmp_state = NULL;
    tmp_subsystem_allocator = NULL;
    tmp_platform_system = NULL;
    tmp_renderer_frontend = NULL;
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
    if(NULL != tmp_stl_viewer_demo && NULL != tmp_renderer_frontend) {
        stl_viewer_destroy(tmp_renderer_frontend, &tmp_stl_viewer_demo);
    }
    if(NULL != tmp_sample_robot_demo && NULL != tmp_renderer_frontend) {
        sample_robot_destroy(tmp_renderer_frontend, &tmp_sample_robot_demo);
    }
    if(NULL != tmp_renderer_frontend) {
        renderer_frontend_deinitialize(tmp_renderer_frontend);
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

    if(NULL != s_application_state->stl_viewer_demo) {
        stl_viewer_destroy(s_application_state->renderer_frontend, &s_application_state->stl_viewer_demo);
    }
    if(NULL != s_application_state->sample_robot_demo) {
        sample_robot_destroy(s_application_state->renderer_frontend, &s_application_state->sample_robot_demo);
    }
    if(NULL != s_application_state->renderer_frontend) {
        renderer_frontend_deinitialize(s_application_state->renderer_frontend);
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

    scene_result_t ret_scene = SCENE_INVALID_ARGUMENT;

    struct timespec  req = {0, 1000000};
    const float demo_delta_time = 1.0f / 60.0f; // 暫定固定値

    if(NULL == s_application_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(%s) - Application is not initialized.", application_result_to_str(ret));
        goto cleanup;
    }

    glEnable(GL_PROGRAM_POINT_SIZE);    // 将来的にはrenderer_backend内にrenderer_state.hを追加してそこにOpenGL設定を行う場所を作る

    // TODO: window NULLチェック
    while(!s_application_state->window_should_close) {
        application_frame_state_begin_frame(&s_application_state->frame_state);

        // Application State Update.
        ret = application_events_update();
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_events_update failed.", application_result_to_str(ret));
            goto cleanup;
        }

        // Diagnostics Update.
        ret = application_diagnostics_update(&s_application_state->diag_config, s_application_state->engine_event_view, &s_application_state->frame_state);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_diagnostics_update failed.", application_result_to_str(ret));
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

        // Scene Update.
        ret_scene = sample_robot_update(s_application_state->sample_robot_demo, s_application_state->engine_event_view, demo_delta_time, s_application_state->frame_state.window_resized, s_application_state->frame_state.framebuffer_width, s_application_state->frame_state.framebuffer_height);
        if(SCENE_SUCCESS != ret_scene) {
            ret = application_result_convert_scene(ret_scene);
            if(SCENE_INVALID_ARGUMENT == ret_scene) {
                ret = APPLICATION_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("application_run(%s) - sample_robot_update failed.", application_result_to_str(ret));
            goto cleanup;
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

        ret_scene = sample_robot_draw(s_application_state->sample_robot_demo, s_application_state->renderer_frontend);
        if(SCENE_SUCCESS != ret_scene) {
            ret = application_result_convert_scene(ret_scene);
            if(SCENE_INVALID_ARGUMENT == ret_scene) {
                ret = APPLICATION_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("application_run(%s) - sample_robot_draw failed.", application_result_to_str(ret));
            goto cleanup;
        }

        platform_system_swap_buffers(s_application_state->platform_system);

        nanosleep(&req, NULL);
    }
cleanup:
    return ret;
}

static application_result_t application_events_update(void) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    event_system_result_t ret_event_system = EVENT_SYSTEM_INVALID_ARGUMENT;

    if(NULL == s_application_state) {
        ret = APPLICATION_BAD_OPERATION;
        ERROR_MESSAGE("application_events_update(%s) - Application state is not initialized.", application_result_to_str(ret));
        goto cleanup;
    }

    // Event System.
    //   OS / Engine events
    //   Window close request
    ret_event_system = event_system_update(s_application_state->event_system, &s_application_state->engine_event_view);
    if(EVENT_SYSTEM_SUCCESS != ret_event_system) {
        ret = application_result_convert_event_system(ret_event_system);
        ERROR_MESSAGE("application_events_update(%s) - event_system_update failed.", application_result_to_str(ret));
        goto cleanup;
    }
    s_application_state->window_should_close = s_application_state->engine_event_view->window_close_requested;

    for(size_t i = 0; i != s_application_state->engine_event_view->window_event_count; ++i) {
        if(WINDOW_EVENT_RESIZE == s_application_state->engine_event_view->window_events[i].event_code) {
            s_application_state->frame_state.framebuffer_width = s_application_state->engine_event_view->window_events[i].event_args.framebuffer_width;
            s_application_state->frame_state.framebuffer_height = s_application_state->engine_event_view->window_events[i].event_args.framebuffer_height;
            s_application_state->frame_state.window_resized = true;
        }
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
