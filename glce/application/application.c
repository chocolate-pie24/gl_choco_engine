// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup application
 *
 * @file application.c
 * @author chocolate-pie24
 * @brief プロジェクトの最上位レイヤーで全サブシステムのオーケストレーションを行うAPIの実装
 *
 * @todo application.cのエラー文字列周りを別ファイルに移す
 *
 * @date 2025-09-20
 *
 */
#include "application/application.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <time.h>   // for nanosleep TODO: remove this!!

#include <GL/glew.h>    // TODO: remove this!! glfwSwapBuffersをrendererに移したら削除

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/aabb_3d.h"
#include "engine/core/event/keyboard_event.h"

#include "engine/io_utils/fs_path.h"

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
#include "engine/systems/renderer/config/renderer_config.h"

// Application Layer
#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

#include "application/event/application_frame_state.h"
#include "application/cameras/application_flight_camera.h"
#include "application/renderer/application_renderer.h"
#include "application/diagnostics/application_diagnostics.h"

/**
 * @brief アプリケーション内部状態とエンジン各サブシステム状態管理構造体インスタンスを保持する
 *
 */
typedef struct application_state {
    // SubSystem Configuration
    renderer_config_t renderer_config;
    platform_system_config_t platform_system_config;
    event_system_config_t event_system_config;
    application_diagnostics_config_t diag_config;

    // application status
    bool window_should_close;   /**< ウィンドウクローズ指示フラグ */
    int window_width;           /**< ウィンドウ幅 */
    int window_height;          /**< ウィンドウ高さ */

    // 実行ファイルパス
    fs_path_t* executable_directory;

    // Subsystem Allocator
    size_t subsystem_memory_pool_size;
    subsystem_allocator_t* subsystem_allocator;

    // Platform System
    platform_system_t* platform_system;

    // Event System
    event_system_t* event_system;
    const engine_event_view_t* engine_event_view;

    // Camera System
    application_flight_camera_t* flight_camera;

    // Renderer System
    application_renderer_t* renderer;

    // Frame State
    application_frame_state_t frame_state;
    mat4x4f_t projection_matrix;
    mat4x4f_t view_matrix;
    mat4x4f_t model_matrix;

    // Texture ID
    uint16_t tex_id_rabbit;
    uint16_t tex_id_frog;
    uint16_t tex_id_green;

    // Geometry ID
    bool should_draw_penguin_aabb;
    uint16_t geometry_id_test_points;
    uint16_t geometry_id_penguin;
    uint16_t geometry_id_small_icon;
    uint16_t geometry_id_large_icon;
    uint16_t geometry_id_penguin_aabb;
    uint16_t geometry_id_test_line;

    // Line Material
    vec4u8_t penguin_aabb_color;
    vec4u8_t test_line_color;

    mat4x4f_t rabbit_mesh_model_mat;
    mat4x4f_t frog_mesh_model_mat;
    mat4x4f_t green_mesh_model_mat;
} application_state_t;

static application_state_t* s_application_state = NULL; /**< アプリケーション内部状態およびエンジン各サブシステム内部状態 */

static application_result_t app_state_update(void);
static void app_state_begin_frame(void);

// subsystem configuration
static void platform_system_config_initialize(platform_system_config_t* config_);
static void event_system_config_initialize(event_system_config_t* config_);
static void renderer_config_initialize(renderer_config_t* config_);

static application_result_t point_mesh_geometry_import(application_state_t* state_);        // TODO: remove this!!
static application_result_t ui_mesh_geometry_import(application_state_t* state_);
static application_result_t lit_mesh_geometry_import(application_state_t* state_);
static application_result_t ui_mesh_textures_import(application_state_t* state_);

static application_result_t executable_directory_get(application_state_t* state_);

application_result_t application_create(void) {
    application_result_t ret = APPLICATION_RUNTIME_ERROR;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;
    platform_system_result_t ret_platform_system = PLATFORM_SYSTEM_INVALID_ARGUMENT;
    event_system_result_t ret_event_system = EVENT_SYSTEM_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    application_state_t* tmp_state = NULL;

    // Preconditions
    if(NULL != s_application_state) {
        ERROR_MESSAGE("application_create(%s) - Application state is already initialized.", application_result_to_str(APPLICATION_RUNTIME_ERROR));
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    // begin Simulation
    // Application State
    ret_general_allocator = general_allocator_allocate(sizeof(*tmp_state), GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM, (void**)&tmp_state);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = application_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate memory for application state.", application_result_to_str(ret));
        goto cleanup;
    }

    // Subsystem Allocator
    tmp_state->subsystem_memory_pool_size = 128 * KIB;
    ret_subsystem_allocator = subsystem_allocator_create(tmp_state->subsystem_memory_pool_size, &tmp_state->subsystem_allocator);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = application_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate memory for subsystem allocator.", application_result_to_str(ret));
        goto cleanup;
    }

    // 実行ファイルパス取得
    ret = executable_directory_get(tmp_state);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - executable_directory_get failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // Platform System
    INFO_MESSAGE("Creating platform system...");
    platform_system_config_initialize(&tmp_state->platform_system_config);

    tmp_state->window_width = 1024;
    tmp_state->window_height = 768;

    ret_platform_system = platform_system_create(&tmp_state->platform_system_config, tmp_state->subsystem_allocator, &tmp_state->frame_state.framebuffer_width, &tmp_state->frame_state.framebuffer_height, &tmp_state->platform_system);
    if(PLATFORM_SYSTEM_SUCCESS != ret_platform_system) {
        ret = application_result_convert_platform_system(ret_platform_system);
        ERROR_MESSAGE("application_create(%s) - platform_system_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("platform system created successfully.");

    // Event System
    INFO_MESSAGE("Creating event system...");
    event_system_config_initialize(&tmp_state->event_system_config);
    ret_event_system = event_system_create(&tmp_state->event_system_config, tmp_state->subsystem_allocator, tmp_state->platform_system, &tmp_state->event_system);
    if(EVENT_SYSTEM_SUCCESS != ret_event_system) {
        ret = application_result_convert_event_system(ret_event_system);
        ERROR_MESSAGE("application_create(%s) - event_system_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("event system created successfully.");

    // application flight camera
    INFO_MESSAGE("Creating flight camera system...");
    ret = application_flight_camera_create(8, tmp_state->subsystem_allocator, tmp_state->frame_state.framebuffer_width, tmp_state->frame_state.framebuffer_height, &tmp_state->flight_camera);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - application_flight_camera_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("flight camera system created successfully.");

    // application renderer
    INFO_MESSAGE("Creating renderer system...");
    renderer_config_initialize(&tmp_state->renderer_config);
    ret = application_renderer_create(&tmp_state->renderer_config, tmp_state->subsystem_allocator, fs_path_fullpath_get(tmp_state->executable_directory), "../../assets/shaders/test_shader/", &tmp_state->renderer);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - application_renderer_create failed.", application_result_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("renderer system created successfully.");

    // diagnostic config
    tmp_state->diag_config.runtime_status_report = KEY_1;
    tmp_state->diag_config.validation_report = KEY_2;

    application_diagnostics_status_report(tmp_state->subsystem_allocator);

    // commit
    s_application_state = tmp_state;
    INFO_MESSAGE("Application created successfully.");

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_SUCCESS != ret) {
        if(NULL != tmp_state) {
            if(NULL != tmp_state->renderer) {
                application_renderer_deinitialize(tmp_state->renderer);
            }
            if(NULL != tmp_state->flight_camera) {
                application_flight_camera_deinitialize(tmp_state->flight_camera);
            }
            if(NULL != tmp_state->event_system) {
                event_system_deinitialize(tmp_state->event_system);
            }
            if(NULL != tmp_state->platform_system) {
                platform_system_deinitialize(tmp_state->platform_system);
            }
            if(NULL != tmp_state->executable_directory) {
                fs_path_destroy(&tmp_state->executable_directory);
            }
            subsystem_allocator_destroy(&tmp_state->subsystem_allocator);
            general_allocator_free((void**)&tmp_state, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
        }
    }

    return ret;
}

// TODO: test
void application_destroy(void) {
    INFO_MESSAGE("Starting application shutdown...");
    if(NULL == s_application_state) {
        goto cleanup;
    }

    // begin cleanup all systems.
    if(NULL != s_application_state->renderer) {
        application_renderer_deinitialize(s_application_state->renderer);
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

    subsystem_allocator_destroy(&s_application_state->subsystem_allocator);
    general_allocator_free((void**)&s_application_state, GENERAL_ALLOCATOR_MEMORY_TAG_SYSTEM);
    INFO_MESSAGE("Freed all memory.");
    // memory_system_report();
    // end cleanup all systems.

    INFO_MESSAGE("Application destroyed successfully.");
cleanup:
    return;
}

application_result_t application_run(void) {
    application_result_t ret = APPLICATION_SUCCESS;

    // penguin AABB
    aabb_3d_t penguin_aabb = { 0 };

    line_vertex_t tmp_vertices[2] = { 0 };

    struct timespec  req = {0, 1000000};

    if(NULL == s_application_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(%s) - Application is not initialized.", application_result_to_str(ret));
        goto cleanup;
    }

    // begin temporary

    glEnable(GL_PROGRAM_POINT_SIZE);    // 将来的にはrenderer_backend内にrenderer_state.hを追加してそこにOpenGL設定を行う場所を作る

    // MVP Matrix
    mat4f_identity(&s_application_state->model_matrix);
    mat4f_identity(&s_application_state->rabbit_mesh_model_mat);
    mat4f_identity(&s_application_state->frog_mesh_model_mat);
    mat4f_identity(&s_application_state->green_mesh_model_mat);
    mat4f_identity(&s_application_state->projection_matrix);
    mat4f_identity(&s_application_state->view_matrix);
    mat4f_translation(vec3f_initialize(2.5f, 0.0f, 0.0f), &s_application_state->green_mesh_model_mat);
    mat4f_translation(vec3f_initialize(0.0f, -2.5f, 0.0f), &s_application_state->frog_mesh_model_mat);

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
    ret = application_renderer_update(s_application_state->renderer, &s_application_state->view_matrix, &s_application_state->projection_matrix, &s_application_state->frame_state);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_renderer_update failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = ui_mesh_textures_import(s_application_state);
    if(APPLICATION_SUCCESS != ret) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run - ui_mesh_textures_import failed.");
        goto cleanup;
    }

    ret = lit_mesh_geometry_import(s_application_state);
    if(APPLICATION_SUCCESS != ret) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run - Failed to import lit mesh geometry.");
        goto cleanup;
    }

    ret = point_mesh_geometry_import(s_application_state);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - Failed to import point mesh geometry.", application_result_to_str(ret));
        goto cleanup;
    }

    // ペンギンAABB pipeline import
    s_application_state->should_draw_penguin_aabb = true;
    s_application_state->penguin_aabb_color = vec4u8_initialize(255, 0, 0, 255);
    if(s_application_state->should_draw_penguin_aabb) {
        ret = application_renderer_lit_mesh_geometry_convert_to_aabb_3d(s_application_state->renderer, s_application_state->geometry_id_penguin, &penguin_aabb);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_lit_mesh_geometry_convert_to_aabb_3d failed.", application_result_to_str(ret));
            goto cleanup;
        }
        ret = application_renderer_line_mesh_geometry_import_from_aabb(s_application_state->renderer, "penguin_aabb", &penguin_aabb, &s_application_state->geometry_id_penguin_aabb);
        if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_renderer_line_mesh_geometry_import_from_aabb failed.", application_result_to_str(ret));
        goto cleanup;
        }
    }

    // テスト線分 pipeline import
    tmp_vertices[0].position = vec3f_initialize(1.0f, 2.0f, -3.0f);
    tmp_vertices[1].position = vec3f_initialize(4.0f, 5.0f, -6.0f);
    s_application_state->test_line_color = vec4u8_initialize(0, 255, 0, 255);
    ret = application_renderer_line_mesh_geometry_import_from_vertices(s_application_state->renderer, "test_line", tmp_vertices, 2, &s_application_state->geometry_id_test_line);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_renderer_line_mesh_geometry_import_from_vertices failed.", application_result_to_str(ret));
        goto cleanup;
    }

    // UI pipeline import
    // アイコンサイズはUI描画用projection, viewができたら整える
    ret = ui_mesh_geometry_import(s_application_state);
    if(APPLICATION_SUCCESS != ret) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run - Failed to import ui mesh geometry.");
        goto cleanup;
    }

    // TODO: window NULLチェック
    // end temporary

    while(!s_application_state->window_should_close) {
        app_state_begin_frame();

        ret = app_state_update();
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - app_state_update failed.", application_result_to_str(ret));
            goto cleanup;
        }
        if(s_application_state->frame_state.runtime_status_report_requested) {
            ret = application_diagnostics_status_report(s_application_state->subsystem_allocator);
            if(APPLICATION_SUCCESS != ret) {
                ERROR_MESSAGE("application_run(%s) - application_diagnostics_status_report failed.", application_result_to_str(ret));
                goto cleanup;
            }
        } else if(s_application_state->frame_state.validation_report_requested) {
            WARN_MESSAGE("application_run(%s) - not impremented yet...");
        }

        // begin temporary TODO: remove this!!
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, s_application_state->frame_state.framebuffer_width, s_application_state->frame_state.framebuffer_height);

        // UI描画
        ret = application_renderer_ui_mesh_draw(s_application_state->renderer, s_application_state->geometry_id_small_icon, s_application_state->tex_id_rabbit, &s_application_state->rabbit_mesh_model_mat);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_ui_mesh_draw failed.", application_result_to_str(ret));
            goto cleanup;
        }

        ret = application_renderer_ui_mesh_draw(s_application_state->renderer, s_application_state->geometry_id_large_icon, s_application_state->tex_id_frog, &s_application_state->frog_mesh_model_mat);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_ui_mesh_draw failed.", application_result_to_str(ret));
            goto cleanup;
        }

        ret = application_renderer_ui_mesh_draw(s_application_state->renderer, s_application_state->geometry_id_small_icon, s_application_state->tex_id_green, &s_application_state->green_mesh_model_mat);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_ui_mesh_draw failed.", application_result_to_str(ret));
            goto cleanup;
        }

        // 線分描画
        ret = application_renderer_line_mesh_draw(s_application_state->renderer, s_application_state->geometry_id_penguin_aabb, &s_application_state->model_matrix, s_application_state->penguin_aabb_color.elem);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_line_mesh_draw failed.", application_result_to_str(ret));
            goto cleanup;
        }

        ret = application_renderer_line_mesh_draw(s_application_state->renderer, s_application_state->geometry_id_test_line, &s_application_state->model_matrix, s_application_state->test_line_color.elem);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_line_mesh_draw failed.", application_result_to_str(ret));
            goto cleanup;
        }

        // ポイント描画
        ret = application_renderer_point_mesh_draw(s_application_state->renderer, s_application_state->geometry_id_test_points, &s_application_state->model_matrix);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_point_mesh_draw failed.", application_result_to_str(ret));
            goto cleanup;
        }

        // STL描画
        ret = application_renderer_lit_mesh_draw(s_application_state->renderer, s_application_state->geometry_id_penguin, &s_application_state->model_matrix);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_lit_mesh_draw failed.", application_result_to_str(ret));
            goto cleanup;
        }

        platform_system_swap_buffers(s_application_state->platform_system);
        // end temporary

        nanosleep(&req, NULL);
    }
cleanup:
    return ret;
}

static application_result_t app_state_update(void) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    event_system_result_t ret_event_system = EVENT_SYSTEM_INVALID_ARGUMENT;

    if(NULL == s_application_state) {
        ret = APPLICATION_BAD_OPERATION;
        ERROR_MESSAGE("app_state_update(%s) - Application state is not initialized.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_event_system = event_system_update(s_application_state->event_system, &s_application_state->engine_event_view);
    if(EVENT_SYSTEM_SUCCESS != ret_event_system) {
        ret = application_result_convert_event_system(ret_event_system);
        ERROR_MESSAGE("app_state_update(%s) - event_system_update failed.", application_result_to_str(ret));
        goto cleanup;
    }
    s_application_state->window_should_close = s_application_state->engine_event_view->window_close_requested;

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

    ret = application_renderer_update(s_application_state->renderer, &s_application_state->view_matrix, &s_application_state->projection_matrix, &s_application_state->frame_state);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_renderer_update failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_diagnostics_update(&s_application_state->diag_config, s_application_state->engine_event_view, &s_application_state->frame_state);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_diagnostics_update failed.", application_result_to_str(ret));
        goto cleanup;
    }

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

static void app_state_begin_frame(void) {
    if(NULL == s_application_state) {
        ERROR_MESSAGE("app_state_begin_frame(%s) - Application state is not initialized.", application_result_to_str(APPLICATION_RUNTIME_ERROR));
        goto cleanup;
    }
    application_frame_state_begin_frame(&s_application_state->frame_state);
cleanup:
    return;
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

static void renderer_config_initialize(renderer_config_t* config_) {
    config_->ui_mesh_shader_config.buffer_usage = BUFFER_USAGE_STATIC;
    config_->ui_mesh_shader_config.max_allocation_count = 512;
    config_->ui_mesh_shader_config.vbo_size = 1024;

    config_->line_mesh_shader_config.buffer_usage = BUFFER_USAGE_STATIC;
    config_->line_mesh_shader_config.max_allocation_count = 512;
    config_->line_mesh_shader_config.vbo_size = 1024;

    config_->point_mesh_shader_config.buffer_usage = BUFFER_USAGE_DYNAMIC;
    config_->point_mesh_shader_config.max_allocation_count = 128;
    config_->point_mesh_shader_config.vbo_size = 1 * KIB;

    config_->lit_mesh_shader_config.buffer_usage = BUFFER_USAGE_STATIC;
    config_->lit_mesh_shader_config.max_allocation_count = 512;
    config_->lit_mesh_shader_config.vbo_size = 1 * GIB;
}

// TODO: remove this!!
static application_result_t point_mesh_geometry_import(application_state_t* state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    point_vertex_t tmp_vertices[8] = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "point_mesh_geometry_import", "state_")

    tmp_vertices[0].position = vec3f_initialize(-0.5, -0.5f, -3.0f);
    tmp_vertices[1].position = vec3f_initialize(-0.4f, -0.4f, -3.0f);
    tmp_vertices[2].position = vec3f_initialize(-0.3f, -0.3f, -3.0f);
    tmp_vertices[3].position = vec3f_initialize(-0.2f, -0.2f, -3.0f);
    tmp_vertices[4].position = vec3f_initialize(-0.1f, -0.1f, -3.0f);
    tmp_vertices[5].position = vec3f_initialize(0.1f, 0.1f, -3.0f);
    tmp_vertices[6].position = vec3f_initialize(0.2f, 0.2f, -3.0f);
    tmp_vertices[7].position = vec3f_initialize(0.3f, 0.3f, -3.0f);

    tmp_vertices[0].color = vec4u8_initialize(255, 0, 0, 255);
    tmp_vertices[1].color = vec4u8_initialize(255, 255, 0, 255);
    tmp_vertices[2].color = vec4u8_initialize(255, 0, 255, 255);
    tmp_vertices[3].color = vec4u8_initialize(0, 255, 0, 255);
    tmp_vertices[4].color = vec4u8_initialize(255, 255, 0, 255);
    tmp_vertices[5].color = vec4u8_initialize(255, 255, 0, 255);
    tmp_vertices[6].color = vec4u8_initialize(255, 255, 0, 255);
    tmp_vertices[7].color = vec4u8_initialize(255, 255, 0, 255);

    ret = application_renderer_point_mesh_geometry_import_from_vertices(state_->renderer, "test_points", tmp_vertices, 8, &state_->geometry_id_test_points);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("point_mesh_geometry_import(%s) - application_renderer_point_mesh_geometry_import_from_vertices failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

static application_result_t ui_mesh_geometry_import(application_state_t* state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* small_icon_path = NULL;
    fs_path_t* large_icon_path = NULL;

    static const char* const small_icon_name = "small_icon";
    static const char* const large_icon_name = "large_icon";

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "ui_mesh_geometry_import", "state_")

    ret_fs_path = fs_path_create(&small_icon_path, fs_path_fullpath_get(state_->executable_directory), "../../assets/geometries/", small_icon_name, "ui_geom");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;    // 正式な実行結果コード変換は後でやる
        ERROR_MESSAGE("ui_mesh_geometry_import(%s) - fs_path_create failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_geometry_import_from_file(state_->renderer, small_icon_name, fs_path_fullpath_get(small_icon_path), &state_->geometry_id_small_icon);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_geometry_import - application_renderer_ui_mesh_geometry_import_from_file failed.");
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&large_icon_path, fs_path_fullpath_get(state_->executable_directory), "../../assets/geometries/", large_icon_name, "ui_geom");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;    // 正式な実行結果コード変換は後でやる
        ERROR_MESSAGE("ui_mesh_geometry_import(%s) - fs_path_create failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_geometry_import_from_file(state_->renderer, large_icon_name, fs_path_fullpath_get(large_icon_path), &state_->geometry_id_large_icon);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_geometry_import - application_renderer_ui_mesh_geometry_import_from_file failed.");
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    fs_path_destroy(&small_icon_path);
    fs_path_destroy(&large_icon_path);

    return ret;
}

static application_result_t lit_mesh_geometry_import(application_state_t* state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* penguin_path = NULL;

    static const char* const penguin_name = "glce_lowpoly_penguin_ascii";

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "lit_mesh_geometry_import", "state_")

    ret_fs_path = fs_path_create(&penguin_path, fs_path_fullpath_get(state_->executable_directory), "../../assets/stl/glce_lowpoly_animal_stl_ascii/", penguin_name, "stl");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;    // 正式な実行結果コード変換は後でやる
        ERROR_MESSAGE("lit_mesh_geometry_import(%s) - fs_path_create failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_lit_mesh_geometry_import_from_file(state_->renderer, penguin_name, fs_path_fullpath_get(penguin_path), &state_->geometry_id_penguin);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_geometry_import(%s) - application_renderer_lit_mesh_geometry_import_from_file failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    fs_path_destroy(&penguin_path);

    return ret;
}

static application_result_t ui_mesh_textures_import(application_state_t* state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* rabbit_path = NULL;
    fs_path_t* frog_path = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "ui_mesh_textures_import", "state_")

    ret_fs_path = fs_path_create(&rabbit_path, fs_path_fullpath_get(state_->executable_directory), "../../assets/textures/", "rabbit_512", "bmp");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("ui_mesh_textures_import(%s) - fs_path_create failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_texture_import_from_bmp(state_->renderer, 0, "rabbit_512", fs_path_fullpath_get(rabbit_path), &state_->tex_id_rabbit);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_textures_import(%s) - application_renderer_ui_mesh_texture_import_from_bmp failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&frog_path, fs_path_fullpath_get(state_->executable_directory), "../../assets/textures/", "frog_512", "bmp");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("ui_mesh_textures_import(%s) - fs_path_create failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_texture_import_from_bmp(state_->renderer, 0, "frog_512", fs_path_fullpath_get(frog_path), &state_->tex_id_frog);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_textures_import(%s) - application_renderer_ui_mesh_texture_import_from_bmp failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_texture_import_from_solid_color(state_->renderer, 0, "test_texture_green", 0, 255, 0, &state_->tex_id_green);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_textures_import(%s) - application_renderer_ui_mesh_texture_import_from_solid_color failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    fs_path_destroy(&frog_path);
    fs_path_destroy(&rabbit_path);

    return ret;
}

static application_result_t executable_directory_get(application_state_t* state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(state_, ret, APPLICATION_INVALID_ARGUMENT, application_result_to_str(APPLICATION_INVALID_ARGUMENT), "executable_directory_get", "state_")

    ret_fs_path = fs_path_create_from_executable_directory(&state_->executable_directory);
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;    // TODO: エラーコード変換
        ERROR_MESSAGE("executable_directory_get(%s) - fs_path_create_from_executable_directory failed.", application_result_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}
