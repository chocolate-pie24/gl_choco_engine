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

#include <stdalign.h>
#include <stddef.h> // for NULL
#include <string.h> // for memset
#include <stdbool.h>
#include <stdint.h>

#include <time.h>   // for nanosleep TODO: remove this!!

#include <GL/glew.h>    // TODO: remove this!! glfwSwapBuffersをrendererに移したら削除

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/core/geometry_primitive/geometry_primitive_types.h"
#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/io_utils/fs_path.h"

#include "engine/systems/platform/core/platform_types.h"
#include "engine/systems/platform/platform_context.h"

#include "engine/systems/renderer/core/renderer_types.h"
#include "engine/systems/renderer/config/renderer_config.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

#include "application/event/application_event.h"
#include "application/cameras/application_flight_camera.h"
#include "application/renderer/application_renderer.h"

/**
 * @brief アプリケーション内部状態とエンジン各サブシステム状態管理構造体インスタンスを保持する
 *
 */
typedef struct app_state {
    app_build_config_t build_config;

    // application status
    bool window_should_close;   /**< ウィンドウクローズ指示フラグ */
    bool window_resized;        /**< ウィンドウサイズ変更イベント発生フラグ */
    int window_width;           /**< ウィンドウ幅 */
    int window_height;          /**< ウィンドウ高さ */
    int framebuffer_width;      /**< フレームバッファサイズ(幅) */
    int framebuffer_height;     /**< フレームバッファサイズ(高さ) */

    // 実行ファイルパス
    fs_path_t* executable_directory;

    // core/memory/linear_allocator
    size_t linear_alloc_mem_req;    /**< リニアアロケータ構造体インスタンスに必要なメモリ量 */
    size_t linear_alloc_align_req;  /**< リニアアロケータ構造体インスタンスが要求するメモリアライメント */
    size_t linear_alloc_pool_size;  /**< リニアアロケータ構造体インスタンスが使用するメモリプールのサイズ */
    void* linear_alloc_pool;        /**< リニアアロケータ構造体インスタンスが使用するメモリプールのアドレス */
    linear_alloc_t* linear_alloc;   /**< リニアアロケータ構造体インスタンス */

    platform_context_t* platform_context; /**< プラットフォームStrategyパターンへの窓口としてのコンテキスト構造体インスタンス */

    renderer_config_t renderer_config;

    // Frame State
    bool should_draw_penguin_aabb;
    mat4x4f_t projection_matrix;
    mat4x4f_t view_matrix;
    mat4x4f_t model_matrix;

    // Texture ID
    uint16_t tex_id_rabbit;
    uint16_t tex_id_frog;
    uint16_t tex_id_green;

    // Geometry ID
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

    const application_event_view_t* event_view;
    application_flight_camera_t* flight_camera;
    application_renderer_t* renderer;
} app_state_t;

static app_state_t* s_app_state = NULL; /**< アプリケーション内部状態およびエンジン各サブシステム内部状態 */

static application_result_t app_state_update(void);
static void app_state_clean(void);

static application_result_t point_geometry_create(app_state_t* app_state_);        // TODO: remove this!!
static application_result_t ui_mesh_geometry_import(app_state_t* app_state_);
static application_result_t lit_mesh_geometry_import(app_state_t* app_state_);

static application_result_t texture_initialize(app_state_t* app_state_);

static application_result_t executable_directory_get(app_state_t* app_state_);

application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    application_result_t ret = APPLICATION_RUNTIME_ERROR;

    memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    platform_result_t ret_platform = PLATFORM_INVALID_ARGUMENT;

    // Preconditions
    if(NULL != s_app_state) {
        ERROR_MESSAGE("application_create(%s) - Application state is already initialized.", app_rslt_to_str(APPLICATION_RUNTIME_ERROR));
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    ret_mem_sys = memory_system_create();
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = app_rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to create memory system.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // begin Simulation
    ret_mem_sys = memory_system_allocate(sizeof(*tmp), MEMORY_TAG_SYSTEM, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = app_rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate memory for application state.", app_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp, 0, sizeof(*tmp));

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // begin Simulation -> launch all systems.(Don't use s_app_state here.)

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Simulation -> launch all systems -> create linear allocator.(Don't use s_app_state here.)
    // [NOTE] linear_allocatorのプールサイズについて
    //   全サブシステムのpreinitを先に実行し、リニアアロケータで必要な容量を計算可能だが、
    //   各サブシステムのアライメント要件を考慮すると単純に総和を取れば良いと言うものではなく、ちょっと複雑
    //   当面は実施せず、多めにメモリを確保する方針にする
    INFO_MESSAGE("Initializing linear allocator...");
    tmp->linear_alloc = NULL;
    linear_allocator_preinit(&tmp->linear_alloc_mem_req, &tmp->linear_alloc_align_req);
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&tmp->linear_alloc);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = app_rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate linear allocator memory.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp->linear_alloc_pool_size = 128 * KIB;
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &tmp->linear_alloc_pool);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = app_rslt_convert_mem_sys(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate memory for the linear allocator pool.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_init(tmp->linear_alloc, tmp->linear_alloc_pool_size, tmp->linear_alloc_pool);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = app_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize linear allocator.", app_rslt_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("linear_allocator initialized successfully.");

    ret = executable_directory_get(tmp);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - executable_directory_get failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Simulation -> launch all systems -> create platform.(Don't use s_app_state here.)
    INFO_MESSAGE("Initializing platform state...");
    ret_platform = platform_initialize(tmp->linear_alloc, PLATFORM_USE_GLFW, &tmp->platform_context);
    if(PLATFORM_SUCCESS != ret_platform) {
        ret = app_rslt_convert_platform(ret_platform);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize platform.", app_rslt_to_str(ret));
        goto cleanup;
    }
    tmp->build_config.selected_platform = PLATFORM_USE_GLFW;
    tmp->build_config.selected_graphics_api = GRAPHICS_API_GL33;
    INFO_MESSAGE("platform_backend initialized successfully.");

    // begin temporary
    // TODO: ウィンドウ生成はレンダラー作成時にそっちに移す
    tmp->window_width = 1024;
    tmp->window_height = 768;
    ret_platform = platform_window_create(tmp->platform_context, "test_window", 1024, 768, &tmp->framebuffer_width, &tmp->framebuffer_height);
    if(PLATFORM_SUCCESS != ret_platform) {
        ret = app_rslt_convert_platform(ret_platform);
        ERROR_MESSAGE("application_create(%s) - Failed to create window.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // application event system
    ret = application_event_initialize(tmp->platform_context, 8, KEY_CODE_MAX, 128, tmp->linear_alloc);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - application_event_initialize failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // application flight camera
    ret = application_flight_camera_initialize(8, tmp->linear_alloc, tmp->framebuffer_width, tmp->framebuffer_height, &tmp->flight_camera);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - application_flight_camera_initialize failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // application renderer
    // Shader config
    tmp->renderer_config.ui_mesh_shader_config.buffer_usage = BUFFER_USAGE_STATIC;
    tmp->renderer_config.ui_mesh_shader_config.max_allocation_count = 512;
    tmp->renderer_config.ui_mesh_shader_config.vbo_size = 1024;

    tmp->renderer_config.line_mesh_shader_config.buffer_usage = BUFFER_USAGE_STATIC;
    tmp->renderer_config.line_mesh_shader_config.max_allocation_count = 512;
    tmp->renderer_config.line_mesh_shader_config.vbo_size = 1024;

    tmp->renderer_config.point_mesh_shader_config.buffer_usage = BUFFER_USAGE_DYNAMIC;
    tmp->renderer_config.point_mesh_shader_config.max_allocation_count = 128;
    tmp->renderer_config.point_mesh_shader_config.vbo_size = 1 * KIB;

    tmp->renderer_config.lit_mesh_shader_config.buffer_usage = BUFFER_USAGE_STATIC;
    tmp->renderer_config.lit_mesh_shader_config.max_allocation_count = 512;
    tmp->renderer_config.lit_mesh_shader_config.vbo_size = 1 * GIB;
    ret = application_renderer_initialize(&tmp->renderer_config, tmp->build_config.selected_graphics_api, tmp->linear_alloc, fs_path_fullpath_get(tmp->executable_directory), "../../assets/shaders/test_shader/", &tmp->renderer);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - application_renderer_initialize failed.", app_rslt_to_str(ret));
        goto cleanup;
    }


    // geometry
    ret = point_geometry_create(tmp);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - Failed to create point geometry.", app_rslt_to_str(ret));
        goto cleanup;
    }
    // end temporary

    // commit
    s_app_state = tmp;
    INFO_MESSAGE("Application created successfully.");
    memory_system_report();

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_SUCCESS != ret) {
        if(NULL != tmp) {
            if(NULL != tmp->renderer) {
                application_renderer_deinitialize(tmp->renderer);
            }
            if(NULL != tmp->flight_camera) {
                application_flight_camera_deinitialize(tmp->flight_camera);
            }
            application_event_deinitialize();
            if(NULL != tmp->platform_context) {
                platform_destroy(tmp->platform_context);
            }
            if(NULL != tmp->executable_directory) {
                fs_path_destroy(&tmp->executable_directory);
            }
            if(NULL != tmp->linear_alloc_pool) {
                memory_system_free(tmp->linear_alloc_pool, tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
            }
            if(NULL != tmp->linear_alloc) {
                memory_system_free(tmp->linear_alloc, tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
            }
            memory_system_free(tmp, sizeof(*tmp), MEMORY_TAG_SYSTEM);
            tmp = NULL;
        }
        memory_system_destroy();
    }

    return ret;
}

// TODO: test
void application_destroy(void) {
    INFO_MESSAGE("Starting application shutdown...");
    if(NULL == s_app_state) {
        goto cleanup;
    }

    // begin cleanup all systems.
    if(NULL != s_app_state->renderer) {
        application_renderer_deinitialize(s_app_state->renderer);
    }
    if(NULL != s_app_state->flight_camera) {
        application_flight_camera_deinitialize(s_app_state->flight_camera);
    }
    application_event_deinitialize();
    if(NULL != s_app_state->platform_context) {
        platform_destroy(s_app_state->platform_context);
    }
    if(NULL != s_app_state->executable_directory) {
        fs_path_destroy(&s_app_state->executable_directory);
    }
    if(NULL != s_app_state->linear_alloc_pool) {
        memory_system_free(s_app_state->linear_alloc_pool, s_app_state->linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
        s_app_state->linear_alloc_pool = NULL;
    }
    if(NULL != s_app_state->linear_alloc) {
        memory_system_free(s_app_state->linear_alloc, s_app_state->linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
        s_app_state->linear_alloc = NULL;
    }

    memory_system_free(s_app_state, sizeof(*s_app_state), MEMORY_TAG_SYSTEM);
    s_app_state = NULL;
    INFO_MESSAGE("Freed all memory.");
    memory_system_report();
    memory_system_destroy();
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

    if(NULL == s_app_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(%s) - Application is not initialized.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // begin temporary

    glEnable(GL_PROGRAM_POINT_SIZE);    // 将来的にはrenderer_backend内にrenderer_state.hを追加してそこにOpenGL設定を行う場所を作る

    // MVP Matrix
    mat4f_identity(&s_app_state->model_matrix);
    mat4f_identity(&s_app_state->rabbit_mesh_model_mat);
    mat4f_identity(&s_app_state->frog_mesh_model_mat);
    mat4f_identity(&s_app_state->green_mesh_model_mat);
    mat4f_identity(&s_app_state->projection_matrix);
    mat4f_identity(&s_app_state->view_matrix);
    mat4f_translation(vec3f_initialize(2.5f, 0.0f, 0.0f), &s_app_state->green_mesh_model_mat);
    mat4f_translation(vec3f_initialize(0.0f, -2.5f, 0.0f), &s_app_state->frog_mesh_model_mat);

    ret = application_flight_camera_perspective_matrix_get(s_app_state->flight_camera, &s_app_state->projection_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_flight_camera_perspective_matrix_get failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_flight_camera_view_matrix_get(s_app_state->flight_camera, &s_app_state->view_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_flight_camera_view_matrix_get failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_update(s_app_state->renderer, true, true, &s_app_state->view_matrix, &s_app_state->projection_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_renderer_update failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = texture_initialize(s_app_state);
    if(APPLICATION_SUCCESS != ret) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run - texture_initialize failed.");
        goto cleanup;
    }

    ret = lit_mesh_geometry_import(s_app_state);
    if(APPLICATION_SUCCESS != ret) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run - Failed to import lit mesh geometry.");
        goto cleanup;
    }

    // ペンギンAABB pipeline import
    s_app_state->should_draw_penguin_aabb = true;
    s_app_state->penguin_aabb_color = vec4u8_initialize(255, 0, 0, 255);
    if(s_app_state->should_draw_penguin_aabb) {
        ret = application_renderer_lit_mesh_geometry_to_aabb_3d(s_app_state->renderer, s_app_state->geometry_id_penguin, &penguin_aabb);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_lit_mesh_geometry_to_aabb_3d failed.", app_rslt_to_str(ret));
            goto cleanup;
        }
        ret = application_renderer_line_mesh_geometry_import_from_aabb(s_app_state->renderer, "penguin_aabb", &penguin_aabb, &s_app_state->geometry_id_penguin_aabb);
        if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_renderer_line_mesh_geometry_import_from_aabb failed.", app_rslt_to_str(ret));
        goto cleanup;
        }
    }

    // テスト線分 pipeline import
    tmp_vertices[0].position = vec3f_initialize(1.0f, 2.0f, -3.0f);
    tmp_vertices[1].position = vec3f_initialize(4.0f, 5.0f, -6.0f);
    s_app_state->test_line_color = vec4u8_initialize(0, 255, 0, 255);
    ret = application_renderer_line_mesh_geometry_import_from_vertices(s_app_state->renderer, "test_line", tmp_vertices, 2, &s_app_state->geometry_id_test_line);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_run(%s) - application_renderer_line_mesh_geometry_import_from_vertices failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // UI pipeline import
    // アイコンサイズはUI描画用projection, viewができたら整える
    ret = ui_mesh_geometry_import(s_app_state);
    if(APPLICATION_SUCCESS != ret) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run - Failed to import ui mesh geometry.");
        goto cleanup;
    }

    // TODO: window NULLチェック
    // end temporary

    while(!s_app_state->window_should_close) {
        ret = app_state_update();
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - app_state_update failed.", app_rslt_to_str(ret));
            goto cleanup;
        }
        // app_state_dispatch();
        app_state_clean();

        // begin temporary TODO: remove this!!
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, s_app_state->framebuffer_width, s_app_state->framebuffer_height);

        // UI描画
        ret = application_renderer_ui_mesh_draw(s_app_state->renderer, s_app_state->geometry_id_small_icon, s_app_state->tex_id_rabbit, &s_app_state->rabbit_mesh_model_mat);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_ui_mesh_draw failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret = application_renderer_ui_mesh_draw(s_app_state->renderer, s_app_state->geometry_id_large_icon, s_app_state->tex_id_frog, &s_app_state->frog_mesh_model_mat);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_ui_mesh_draw failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret = application_renderer_ui_mesh_draw(s_app_state->renderer, s_app_state->geometry_id_small_icon, s_app_state->tex_id_green, &s_app_state->green_mesh_model_mat);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_ui_mesh_draw failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        // 線分描画
        ret = application_renderer_line_mesh_draw(s_app_state->renderer, s_app_state->geometry_id_penguin_aabb, &s_app_state->model_matrix, s_app_state->penguin_aabb_color.elem);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_line_mesh_draw failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret = application_renderer_line_mesh_draw(s_app_state->renderer, s_app_state->geometry_id_test_line, &s_app_state->model_matrix, s_app_state->test_line_color.elem);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_line_mesh_draw failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        // ポイント描画
        ret = application_renderer_point_mesh_draw(s_app_state->renderer, s_app_state->geometry_id_test_points, &s_app_state->model_matrix);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_point_mesh_draw failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        // STL描画
        ret = application_renderer_lit_mesh_draw(s_app_state->renderer, s_app_state->geometry_id_penguin, &s_app_state->model_matrix);
        if(APPLICATION_SUCCESS != ret) {
            ERROR_MESSAGE("application_run(%s) - application_renderer_lit_mesh_draw failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        platform_swap_buffers(s_app_state->platform_context);
        // end temporary

        nanosleep(&req, NULL);
    }
cleanup:
    return ret;
}

static application_result_t app_state_update(void) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    // camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    bool view_dirty = false;
    bool projection_dirty = false;

    if(NULL == s_app_state) {
        ret = APPLICATION_BAD_OPERATION;
        ERROR_MESSAGE("app_state_update(%s) - Application state is not initialized.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_event_update(&s_app_state->event_view);
    if(APPLICATION_WINDOW_CLOSE == ret) {
        s_app_state->window_should_close = true;
        goto cleanup;
    } else if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_event_update failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_flight_camera_update(s_app_state->flight_camera, 0.1f, 1.0f, s_app_state->event_view, &view_dirty, &projection_dirty);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_flight_camera_update failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_flight_camera_view_matrix_get(s_app_state->flight_camera, &s_app_state->view_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_flight_camera_view_matrix_get failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_flight_camera_perspective_matrix_get(s_app_state->flight_camera, &s_app_state->projection_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_flight_camera_perspective_matrix_get failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_update(s_app_state->renderer, view_dirty, projection_dirty, &s_app_state->view_matrix, &s_app_state->projection_matrix);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_update(%s) - application_renderer_update failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != s_app_state->event_view->window_event_count; ++i) {
        if(WINDOW_EVENT_RESIZE == s_app_state->event_view->window_events[i].event_code) {
            s_app_state->framebuffer_width = s_app_state->event_view->window_events[i].event_args.framebuffer_width;
            s_app_state->framebuffer_height = s_app_state->event_view->window_events[i].event_args.framebuffer_height;
            s_app_state->window_resized = true;
        }
    }

cleanup:
    return ret;
}

/**
 * @brief アプリケーション状態変化フラグの値を元に戻す
 *
 *
 */
static void app_state_clean(void) {
    if(NULL == s_app_state) {
        ERROR_MESSAGE("app_state_clean(%s) - Application state is not initialized.", app_rslt_to_str(APPLICATION_RUNTIME_ERROR));
        goto cleanup;
    }
    s_app_state->window_resized = false;
cleanup:
    return;
}

// TODO: remove this!!
static application_result_t point_geometry_create(app_state_t* app_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    point_vertex_t tmp_vertices[8] = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(app_state_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "point_geometry_create", "app_state_")
    // IF_ARG_NULL_GOTO_CLEANUP(app_state_->renderer_backend_context, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "point_geometry_create", "app_state_->renderer_backend_context")
    // IF_ARG_NULL_GOTO_CLEANUP(app_state_->point_mesh_shader, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "point_geometry_create", "app_state_->point_mesh_shader")

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

    ret = application_renderer_point_mesh_geometry_import_from_vertices(app_state_->renderer, "test_points", tmp_vertices, 8, &app_state_->geometry_id_test_points);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("point_geometry_create(%s) - application_renderer_point_mesh_geometry_import_from_vertices failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

static application_result_t ui_mesh_geometry_import(app_state_t* app_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* small_icon_path = NULL;
    fs_path_t* large_icon_path = NULL;

    static const char* const small_icon_name = "small_icon";
    static const char* const large_icon_name = "large_icon";

    IF_ARG_NULL_GOTO_CLEANUP(app_state_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "ui_mesh_geometry_import", "app_state_")

    ret_fs_path = fs_path_create(&small_icon_path, fs_path_fullpath_get(app_state_->executable_directory), "../../assets/geometries/", small_icon_name, "ui_geom");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;    // 正式な実行結果コード変換は後でやる
        ERROR_MESSAGE("ui_mesh_geometry_import(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_geometry_import_from_file(app_state_->renderer, small_icon_name, fs_path_fullpath_get(small_icon_path), &app_state_->geometry_id_small_icon);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("ui_mesh_geometry_import - application_renderer_ui_mesh_geometry_import_from_file failed.");
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&large_icon_path, fs_path_fullpath_get(app_state_->executable_directory), "../../assets/geometries/", large_icon_name, "ui_geom");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;    // 正式な実行結果コード変換は後でやる
        ERROR_MESSAGE("ui_mesh_geometry_import(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_geometry_import_from_file(app_state_->renderer, large_icon_name, fs_path_fullpath_get(large_icon_path), &app_state_->geometry_id_large_icon);
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

static application_result_t lit_mesh_geometry_import(app_state_t* app_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* penguin_path = NULL;

    static const char* const penguin_name = "glce_lowpoly_penguin_ascii";

    IF_ARG_NULL_GOTO_CLEANUP(app_state_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "lit_mesh_geometry_import", "app_state_")

    ret_fs_path = fs_path_create(&penguin_path, fs_path_fullpath_get(app_state_->executable_directory), "../../assets/stl/glce_lowpoly_animal_stl_ascii/", penguin_name, "stl");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;    // 正式な実行結果コード変換は後でやる
        ERROR_MESSAGE("lit_mesh_geometry_import(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_lit_mesh_geometry_import_from_file(app_state_->renderer, penguin_name, fs_path_fullpath_get(penguin_path), &app_state_->geometry_id_penguin);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_geometry_import(%s) - application_renderer_lit_mesh_geometry_import_from_file failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    fs_path_destroy(&penguin_path);

    return ret;
}

static application_result_t texture_initialize(app_state_t* app_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* rabbit_path = NULL;
    fs_path_t* frog_path = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(app_state_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "texture_initialize", "app_state_")

    ret_fs_path = fs_path_create(&rabbit_path, fs_path_fullpath_get(app_state_->executable_directory), "../../assets/textures/", "rabbit_512", "bmp");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("texture_initialize(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_texture_import_from_bmp(app_state_->renderer, 0, "rabbit_512", fs_path_fullpath_get(rabbit_path), &app_state_->tex_id_rabbit);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("texture_initialize(%s) - application_renderer_ui_mesh_texture_import_from_bmp failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&frog_path, fs_path_fullpath_get(app_state_->executable_directory), "../../assets/textures/", "frog_512", "bmp");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("texture_initialize(%s) - fs_path_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_texture_import_from_bmp(app_state_->renderer, 0, "frog_512", fs_path_fullpath_get(frog_path), &app_state_->tex_id_frog);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("texture_initialize(%s) - application_renderer_ui_mesh_texture_import_from_bmp failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = application_renderer_ui_mesh_texture_import_from_solid_color(app_state_->renderer, 0, "test_texture_green", 0, 255, 0, &app_state_->tex_id_green);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("texture_initialize(%s) - application_renderer_ui_mesh_texture_import_from_solid_color failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    fs_path_destroy(&frog_path);
    fs_path_destroy(&rabbit_path);

    return ret;
}

static application_result_t executable_directory_get(app_state_t* app_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(app_state_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "executable_directory_get", "app_state_")

    ret_fs_path = fs_path_create_from_executable_directory(&app_state_->executable_directory);
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = APPLICATION_RUNTIME_ERROR;    // TODO: エラーコード変換
        ERROR_MESSAGE("executable_directory_get(%s) - fs_path_create_from_executable_directory failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}
