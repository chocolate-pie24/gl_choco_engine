/** @ingroup application
 *
 * @file application.c
 * @author chocolate-pie24
 * @brief プロジェクトの最上位レイヤーで全サブシステムのオーケストレーションを行うAPIの実装
 *
 * @todo application.cのエラー文字列周りを別ファイルに移す
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdalign.h>
#include <stddef.h> // for NULL
#include <string.h> // for memset
#include <stdbool.h>

#include <time.h>   // for nanosleep TODO: remove this!!

#include <GL/glew.h>    // TODO: remove this!! glfwSwapBuffersをrendererに移したら削除
#include <GLFW/glfw3.h> // TODO: remove this!! glfwSwapBuffersをrendererに移したら削除

#include "application/application.h"

#include "application/application_core/application_types.h"
#include "application/application_core/application_err_utils.h"

#include "application/command_interpreter/flight_camera.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/containers/ring_queue.h"
#include "engine/containers/choco_string.h"

#include "engine/systems/platform/platform_core/platform_types.h"
#include "engine/systems/platform/platform_context.h"

#include "engine/systems/renderer/renderer_resources/shaders/ui_mesh_shader.h"
#include "engine/systems/renderer/renderer_resources/shaders/line_mesh_shader.h"
#include "engine/systems/renderer/renderer_resources/shaders/point_mesh_shader.h"
#include "engine/systems/renderer/renderer_resources/shaders/lit_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/lit_mesh_geometry_registry.h"
#include "engine/systems/renderer/resource_registries/geometries/point_mesh_geometry_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/geometries/lit_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resource_pipelines/geometries/point_mesh_geometry_pipeline.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vbo.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_texture.h"

#include "engine/systems/camera_system/camera_manager/camera_manager.h"
#include "engine/systems/camera_system/camera_core/camera_types.h"
#include "engine/systems/camera_system/camera/camera.h"

#include "engine/systems/texture_system/texture_manager.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/resource/texture/texture.h"
#include "engine/resource/geometry/lit_mesh_geometry.h"
#include "engine/resource/geometry/line_mesh_geometry.h"
#include "engine/resource/geometry/point_mesh_geometry.h"
#include "engine/resource/geometry/ui_mesh_geometry.h"

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

    // core/memory/linear_allocator
    size_t linear_alloc_mem_req;    /**< リニアアロケータ構造体インスタンスに必要なメモリ量 */
    size_t linear_alloc_align_req;  /**< リニアアロケータ構造体インスタンスが要求するメモリアライメント */
    size_t linear_alloc_pool_size;  /**< リニアアロケータ構造体インスタンスが使用するメモリプールのサイズ */
    void* linear_alloc_pool;        /**< リニアアロケータ構造体インスタンスが使用するメモリプールのアドレス */
    linear_alloc_t* linear_alloc;   /**< リニアアロケータ構造体インスタンス */

    // event message queues
    ring_queue_t* window_event_queue;   /**< ウィンドウイベント格納用リングキュー */
    ring_queue_t* keyboard_event_queue; /**< キーボードイベント格納用リングキュー */
    ring_queue_t* mouse_event_queue;    /**< マウスイベント格納用リングキュー */

    // platform/platform_context
    platform_context_t* platform_context; /**< プラットフォームStrategyパターンへの窓口としてのコンテキスト構造体インスタンス */

    // begin temporary TODO: remove this!!
    renderer_backend_context_t* renderer_backend_context;

    ui_mesh_shader_t* ui_mesh_shader;
    line_mesh_shader_t* line_mesh_shader;
    point_mesh_shader_t* point_mesh_shader;
    lit_mesh_shader_t* lit_mesh_shader;

    camera_manager_t* camera_manager;
    camera_t* active_camera;
    int16_t active_camera_id;
    command_status_flight_camera_t flight_camera_commands[FLIGHT_CAMERA_COMMAND_MAX];

    texture_manager_t* texture_manager;
    // end

    // begin temporary TODO: remove this!!
    line_mesh_geometry_t* test_line_geometry;
    size_t test_line_geometry_vertex_count;
    size_t test_line_geometry_vertex_count_offset;
    vec4u8_t test_line_color;

    // line_mesh_geometry_t* aabb_geometry;
    // size_t aabb_geometry_vertex_count;
    // size_t aabb_geometry_vertex_count_offset;
    // vec4u8_t aabb_color;

    point_mesh_geometry_registry_t* point_mesh_geometry_registry;
    int16_t geometry_id_test_points;

    lit_mesh_geometry_registry_t* lit_mesh_geometry_registry;
    int16_t geometry_id_penguin;

    ui_mesh_geometry_t* ui_geometry;
    size_t ui_geometry_vertex_count;
    size_t ui_geometry_vertex_count_offset;

    mat4x4f_t rabbit_mesh_model_mat;
    mat4x4f_t frog_mesh_model_mat;
    mat4x4f_t green_mesh_model_mat;
    //end

    bool view_dirty;
    mat4x4f_t projection_matrix;
    mat4x4f_t view_matrix;
    mat4x4f_t model_matrix;
    // end temporary
} app_state_t;

static app_state_t* s_app_state = NULL; /**< アプリケーション内部状態およびエンジン各サブシステム内部状態 */

static void on_window(const window_event_t* event_);
static void on_key(const keyboard_event_t* event_);
static void on_mouse(const mouse_event_t* event_);

static void app_state_update(void);
static void app_state_dispatch(void);
static void app_state_clean(void);

static application_result_t test_line_geometry_create(app_state_t* app_state_);    // TODO: remove this!!
// static application_result_t aabb_geometry_create(app_state_t* app_state_);         // TODO: remove this!!
static application_result_t point_geometry_create(app_state_t* app_state_);        // TODO: remove this!!
static application_result_t ui_geometry_create(app_state_t* app_state_);    // TODO: remove this!!

static void test_line_geometry_destroy(app_state_t* app_state_);    // TODO: remove this!!
// static void aabb_geometry_destroy(app_state_t* app_state_);         // TODO: remove this!!
static void ui_geometry_destroy(app_state_t* app_state_);    // TODO: remove this!!

application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    application_result_t ret = APPLICATION_RUNTIME_ERROR;
    memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    platform_result_t ret_platform = PLATFORM_INVALID_ARGUMENT;
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;
    texture_system_result_t ret_tex_sys = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

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
    INFO_MESSAGE("platform_backend initialized successfully.");

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Simulation -> launch all systems -> create event message queue(window event).(Don't use s_app_state here.)
    INFO_MESSAGE("Starting window event queue initialize...");
    ret_ring_queue = ring_queue_create(8, sizeof(window_event_t), alignof(window_event_t), &tmp->window_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = app_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize window event queue.", app_rslt_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("window event queue initialized successfully.");

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Simulation -> launch all systems -> create event message queue(keyboard event).(Don't use s_app_state here.)
    INFO_MESSAGE("Starting keyboard event queue initialize...");
    ret_ring_queue = ring_queue_create(KEY_CODE_MAX, sizeof(keyboard_event_t), alignof(keyboard_event_t), &tmp->keyboard_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = app_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize keyboard event queue.", app_rslt_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("keyboard event queue initialized successfully.");

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Simulation -> launch all systems -> create event message queue(mouse event).(Don't use s_app_state here.)
    INFO_MESSAGE("Starting mouse event queue initialize...");
    ret_ring_queue = ring_queue_create(128, sizeof(mouse_event_t), alignof(mouse_event_t), &tmp->mouse_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = app_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize mouse event queue.", app_rslt_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("mouse event queue initialized successfully.");

    ret_camera = camera_manager_initialize(8, tmp->linear_alloc, &tmp->camera_manager);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_create(%s) - Failed to create camera manager.", app_rslt_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("camera manager initialized successfully.");

    // texture system.
    ret_tex_sys = texture_manager_initialize(128, tmp->linear_alloc, &tmp->texture_manager);
    if(TEXTURE_SYSTEM_SUCCESS != ret_tex_sys) {
        ret = app_rslt_convert_texture_system(ret_tex_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to create texture system.", app_rslt_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("texture manager initialized successfully.");
    // end Simulation -> launch all systems.

    // end Simulation
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

    ret_renderer = renderer_backend_initialize(tmp->linear_alloc, GRAPHICS_API_GL33, &tmp->renderer_backend_context);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize renderer backend.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // UI Shader
    ret_renderer = ui_mesh_shader_create("assets/shaders/test_shader/", "ui_mesh_shader", tmp->renderer_backend_context, &tmp->ui_mesh_shader);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create ui shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_renderer = ui_mesh_shader_vertex_buffer_create(tmp->renderer_backend_context, tmp->ui_mesh_shader, BUFFER_USAGE_STATIC, 1024);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create ui vertex buffer.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // Line Shader
    ret_renderer = line_mesh_shader_create("assets/shaders/test_shader/", "line_mesh_shader", tmp->renderer_backend_context, &tmp->line_mesh_shader);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create line shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_renderer = line_mesh_shader_vertex_buffer_create(tmp->renderer_backend_context, tmp->line_mesh_shader, BUFFER_USAGE_STATIC, 1024);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create line vertex buffer.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // Point Shader
    ret_renderer = point_mesh_shader_create("assets/shaders/test_shader/", "point_mesh_shader", tmp->renderer_backend_context, &tmp->point_mesh_shader);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create point shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_renderer = point_mesh_shader_vertex_buffer_create(tmp->renderer_backend_context, tmp->point_mesh_shader, BUFFER_USAGE_DYNAMIC, BUFFER_USAGE_DYNAMIC, 1024, 1024);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create point vertex buffer.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // Lit Mesh Shader
    ret_renderer = lit_mesh_shader_create("assets/shaders/test_shader/", "lit_mesh_shader", tmp->renderer_backend_context, &tmp->lit_mesh_shader);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create lit mesh shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_renderer = lit_mesh_shader_vertex_buffer_create(tmp->renderer_backend_context, tmp->lit_mesh_shader, BUFFER_USAGE_STATIC, 1 * GIB);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create lit mesh vertex buffer.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp->build_config.selected_graphics_api = GRAPHICS_API_GL33;

    // camera create.
    ret = flight_camera_command_initialize(FLIGHT_CAMERA_COMMAND_MAX, tmp->flight_camera_commands);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - Failed to initialize flight camera commands.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp->active_camera_id = INVALID_CAMERA_ID;
    ret_camera = camera_manager_register("flight camera", tmp->camera_manager, &tmp->active_camera_id);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_create(%s) - Failed to register camera.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_camera = camera_manager_camera_get(tmp->active_camera_id, tmp->camera_manager, &tmp->active_camera);
    // ret_camera = camera_manager_camera_get_by_name("flight camera", tmp->camera_manager, &tmp->active_camera);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_create(%s) - Failed to get camera.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // geometry registries
    tmp->point_mesh_geometry_registry = NULL;
    ret_registry = point_mesh_geometry_registry_initialize(128, tmp->linear_alloc, &tmp->point_mesh_geometry_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = APPLICATION_RUNTIME_ERROR;
        // TODO: エラーコード変換
        ERROR_MESSAGE("application_create(%s) - Failed to create point mesh geometry registry.", app_rslt_to_str(APPLICATION_RUNTIME_ERROR));
        goto cleanup;
    }

    tmp->lit_mesh_geometry_registry = NULL;
    ret_registry = lit_mesh_geometry_registry_initialize(256, tmp->linear_alloc, &tmp->lit_mesh_geometry_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = APPLICATION_RUNTIME_ERROR;
        // TODO: エラーコード変換
        ERROR_MESSAGE("application_create(%s) - Failed to create lit mesh geometry registry.", app_rslt_to_str(APPLICATION_RUNTIME_ERROR));
        goto cleanup;
    }

    // geometry
    ret = test_line_geometry_create(tmp);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - Failed to create test line geometry.", app_rslt_to_str(ret));
        goto cleanup;
    }
    // ret = aabb_geometry_create(tmp);
    // if(APPLICATION_SUCCESS != ret) {
    //     ERROR_MESSAGE("application_create(%s) - Failed to create aabb geometry.", app_rslt_to_str(ret));
    //     goto cleanup;
    // }
    ret = point_geometry_create(tmp);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - Failed to create point geometry.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret = ui_geometry_create(tmp);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - Failed to create ui geometry.", app_rslt_to_str(ret));
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
            if(NULL != tmp->lit_mesh_geometry_registry) {
                lit_mesh_geometry_registry_deinitialize(tmp->lit_mesh_geometry_registry);
            }
            if(NULL != tmp->point_mesh_geometry_registry) {
                point_mesh_geometry_registry_deinitialize(tmp->point_mesh_geometry_registry);
            }
            if(NULL != tmp->ui_geometry) {
                ui_mesh_geometry_destroy(&tmp->ui_geometry);
            }
            // if(NULL != tmp->aabb_geometry) {
            //     line_mesh_geometry_destroy(&tmp->aabb_geometry);
            // }
            if(NULL != tmp->test_line_geometry) {
                line_mesh_geometry_destroy(&tmp->test_line_geometry);
            }
            if(NULL != tmp->camera_manager) {
                camera_manager_deinitialize(tmp->camera_manager);
            }
            if(NULL != tmp->renderer_backend_context) {
                if(NULL != tmp->lit_mesh_shader) {
                    lit_mesh_shader_destroy(tmp->renderer_backend_context, &tmp->lit_mesh_shader);
                }
                if(NULL != tmp->point_mesh_shader) {
                    point_mesh_shader_destroy(tmp->renderer_backend_context, &tmp->point_mesh_shader);
                }
                if(NULL != tmp->line_mesh_shader) {
                    line_mesh_shader_destroy(tmp->renderer_backend_context, &tmp->line_mesh_shader);
                }
                if(NULL != tmp->ui_mesh_shader) {
                    ui_mesh_shader_destroy(tmp->renderer_backend_context, &tmp->ui_mesh_shader);
                }
            }

            if(NULL != tmp->mouse_event_queue) {
                ring_queue_destroy(&tmp->mouse_event_queue);
                tmp->mouse_event_queue = NULL;
            }
            if(NULL != tmp->keyboard_event_queue) {
                ring_queue_destroy(&tmp->keyboard_event_queue);
                tmp->keyboard_event_queue = NULL;
            }
            if(NULL != tmp->window_event_queue) {
                ring_queue_destroy(&tmp->window_event_queue);
                tmp->window_event_queue = NULL;
            }
            if(NULL != tmp->platform_context) {
                platform_destroy(tmp->platform_context);
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
    ui_geometry_destroy(s_app_state);
    // aabb_geometry_destroy(s_app_state);
    test_line_geometry_destroy(s_app_state);
    if(NULL != s_app_state->lit_mesh_geometry_registry) {
        lit_mesh_geometry_registry_deinitialize(s_app_state->lit_mesh_geometry_registry);
    }
    if(NULL != s_app_state->point_mesh_geometry_registry) {
        point_mesh_geometry_registry_deinitialize(s_app_state->point_mesh_geometry_registry);
    }
    if(NULL != s_app_state->texture_manager) {
        texture_manager_deinitialize(s_app_state->renderer_backend_context, s_app_state->texture_manager);
    }
    if(NULL != s_app_state->camera_manager) {
        camera_manager_deinitialize(s_app_state->camera_manager);
    }
    if(NULL != s_app_state->renderer_backend_context) {
        if(NULL != s_app_state->lit_mesh_shader) {
            lit_mesh_shader_destroy(s_app_state->renderer_backend_context, &s_app_state->lit_mesh_shader);
        }
        if(NULL != s_app_state->point_mesh_shader) {
            point_mesh_shader_destroy(s_app_state->renderer_backend_context, &s_app_state->point_mesh_shader);
        }
        if(NULL != s_app_state->line_mesh_shader) {
            line_mesh_shader_destroy(s_app_state->renderer_backend_context, &s_app_state->line_mesh_shader);
        }
        if(NULL != s_app_state->ui_mesh_shader) {
            ui_mesh_shader_destroy(s_app_state->renderer_backend_context, &s_app_state->ui_mesh_shader);
        }
    }
    renderer_backend_destroy(s_app_state->renderer_backend_context);
    if(NULL != s_app_state->mouse_event_queue) {
        ring_queue_destroy(&s_app_state->mouse_event_queue);
        s_app_state->mouse_event_queue = NULL;
    }
    if(NULL != s_app_state->keyboard_event_queue) {
        ring_queue_destroy(&s_app_state->keyboard_event_queue);
        s_app_state->keyboard_event_queue = NULL;
    }
    if(NULL != s_app_state->window_event_queue) {
        ring_queue_destroy(&s_app_state->window_event_queue);
        s_app_state->window_event_queue = NULL;
    }
    if(NULL != s_app_state->platform_context) {
        platform_destroy(s_app_state->platform_context);
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
    texture_system_result_t ret_tex_sys = TEXTURE_SYSTEM_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    geometry_primitive_result_t ret_geometry = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registy = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    int16_t tex_id_rabbit = 0;
    int16_t tex_id_frog = 0;
    int16_t tex_id_green = 0;
    renderer_backend_texture_t* tex_gpu_resource = NULL;

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

    camera_viewing_frustum_update(45.0f, (float)s_app_state->framebuffer_width / (float)s_app_state->framebuffer_height, 0.1f, 50.0f, s_app_state->active_camera); // TODO: エラー処理
    camera_perspective_matrix_get(s_app_state->active_camera, &s_app_state->projection_matrix); // TODO: エラー処理
    camera_view_matrix_get(s_app_state->active_camera, &s_app_state->view_matrix);   // TODO: エラー処理

    ui_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader);
    ui_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->view_matrix, true);
    ui_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->projection_matrix, true);

    line_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);
    line_mesh_shader_model_matrix_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, &s_app_state->model_matrix, true);
    line_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, &s_app_state->view_matrix, true);
    line_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, &s_app_state->projection_matrix, true);

    point_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader);
    point_mesh_shader_model_matrix_set(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader, &s_app_state->model_matrix, true);
    point_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader, &s_app_state->view_matrix, true);
    point_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader, &s_app_state->projection_matrix, true);

    lit_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader);
    lit_mesh_shader_model_matrix_set(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader, &s_app_state->model_matrix, true);
    lit_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader, &s_app_state->view_matrix, true);
    lit_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader, &s_app_state->projection_matrix, true);

    ret_tex_sys = texture_manager_register(s_app_state->renderer_backend_context, 0, "rabbit_512", s_app_state->texture_manager, &tex_id_rabbit);
    ret_tex_sys = texture_manager_register(s_app_state->renderer_backend_context, 0, "frog_512", s_app_state->texture_manager, &tex_id_frog);
    ret_tex_sys = texture_manager_register(s_app_state->renderer_backend_context, 0, "test_texture_green", s_app_state->texture_manager, &tex_id_green);

    ret_resource_pipeline = lit_mesh_geometry_pipeline_import_from_file(
        s_app_state->renderer_backend_context,
        s_app_state->lit_mesh_shader,
        s_app_state->lit_mesh_geometry_registry,
        "./assets/stl/glce_lowpoly_animal_stl_ascii/", "glce_lowpoly_penguin_ascii", ".stl",
        &s_app_state->geometry_id_penguin);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ERROR_MESSAGE("application_run - Failed to import lit mesh geometry.");
        goto cleanup;
    }

    // TODO: window NULLチェック

    INFO_MESSAGE("current camera: %s.", camera_name_get(s_app_state->active_camera));
    // end temporary

    while(!s_app_state->window_should_close) {
        platform_result_t ret_event = platform_pump_messages(s_app_state->platform_context, on_window, on_key, on_mouse);
        if(PLATFORM_WINDOW_CLOSE == ret_event) {
            s_app_state->window_should_close = true;
            continue;
        } else if(PLATFORM_SUCCESS != ret_event) {
            ret = app_rslt_convert_platform(ret_event);
            WARN_MESSAGE("application_run(%s) - Failed to pump events.", app_rslt_to_str(ret));
            continue;
        }
        app_state_update();
        app_state_dispatch();
        app_state_clean();

        // begin temporary TODO: remove this!!
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, s_app_state->framebuffer_width, s_app_state->framebuffer_height);

        size_t vertex_count = 0;
        size_t vertex_offset = 0;
        // UI描画
        ui_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader);

        ui_mesh_shader_vertex_array_bind(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader);

        ui_mesh_shader_model_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->rabbit_mesh_model_mat, true);
        texture_manager_gpu_resource_get(tex_id_rabbit, s_app_state->texture_manager, &tex_gpu_resource);
        renderer_backend_texture_bind(s_app_state->renderer_backend_context, tex_gpu_resource);
        glDrawArrays(GL_TRIANGLES, s_app_state->ui_geometry_vertex_count_offset, s_app_state->ui_geometry_vertex_count);
        renderer_backend_texture_unbind(s_app_state->renderer_backend_context, tex_gpu_resource);

        ui_mesh_shader_model_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->green_mesh_model_mat, true);
        texture_manager_gpu_resource_get(tex_id_green, s_app_state->texture_manager, &tex_gpu_resource);
        renderer_backend_texture_bind(s_app_state->renderer_backend_context, tex_gpu_resource);
        glDrawArrays(GL_TRIANGLES, s_app_state->ui_geometry_vertex_count_offset, s_app_state->ui_geometry_vertex_count);
        renderer_backend_texture_unbind(s_app_state->renderer_backend_context, tex_gpu_resource);

        ui_mesh_shader_model_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->frog_mesh_model_mat, true);
        texture_manager_gpu_resource_get(tex_id_frog, s_app_state->texture_manager, &tex_gpu_resource);
        renderer_backend_texture_bind(s_app_state->renderer_backend_context, tex_gpu_resource);
        glDrawArrays(GL_TRIANGLES, s_app_state->ui_geometry_vertex_count_offset, s_app_state->ui_geometry_vertex_count);
        renderer_backend_texture_unbind(s_app_state->renderer_backend_context, tex_gpu_resource);

        renderer_backend_vertex_array_unbind(s_app_state->renderer_backend_context);

        // 線分描画
        line_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);
        line_mesh_shader_color_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, s_app_state->test_line_color.elem);
        line_mesh_shader_vertex_array_bind(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);

        glDrawArrays(GL_LINES, s_app_state->test_line_geometry_vertex_count_offset, s_app_state->test_line_geometry_vertex_count);
        renderer_backend_vertex_array_unbind(s_app_state->renderer_backend_context);

        // ポイント描画
        ret_resource_registy = point_mesh_geometry_registry_draw_range_get(s_app_state->geometry_id_test_points, s_app_state->point_mesh_geometry_registry, &vertex_offset, &vertex_count);
        if(RESOURCE_REGISTRY_SUCCESS == ret_resource_registy) {
            point_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader);
            point_mesh_shader_vertex_array_bind(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader);

            glDrawArrays(GL_POINTS, vertex_offset, vertex_count);
            renderer_backend_vertex_array_unbind(s_app_state->renderer_backend_context);
        }

        // STL描画
        ret_resource_registy = lit_mesh_geometry_registry_draw_range_get(s_app_state->geometry_id_penguin, s_app_state->lit_mesh_geometry_registry, &vertex_offset, &vertex_count);
        if(RESOURCE_REGISTRY_SUCCESS == ret_resource_registy) {
            lit_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader);
            lit_mesh_shader_vertex_array_bind(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader);

            glDrawArrays(GL_TRIANGLES, vertex_offset, vertex_count);
            renderer_backend_vertex_array_unbind(s_app_state->renderer_backend_context);
        }

        // Debug用STL AABB
        // line_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);
        // line_mesh_shader_color_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, s_app_state->aabb_color.elem);
        // line_mesh_shader_vertex_array_bind(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);

        // glDrawArrays(GL_LINES, s_app_state->aabb_geometry_vertex_count_offset, s_app_state->aabb_geometry_vertex_count);
        // renderer_backend_vertex_array_unbind(s_app_state->renderer_backend_context);

        platform_swap_buffers(s_app_state->platform_context);
        // end temporary

        nanosleep(&req, NULL);
    }
cleanup:
    return ret;
}

/**
 * @brief event_をウィンドウイベント用リングキューに格納する
 * @note ウィンドウイベントコールバック
 *
 * @param[in] event_ イベントキューに格納するイベント構造体インスタンスへのポインタ
 */
static void on_window(const window_event_t* event_) {
    ring_queue_result_t ret_push = RING_QUEUE_INVALID_ARGUMENT;

    if(NULL == event_) {
        WARN_MESSAGE("on_window - Argument 'event_' must not be NULL.");
        goto cleanup;
    }
    if(NULL == s_app_state) {
        WARN_MESSAGE("on_window - Application state is not initialized.");
        goto cleanup;
    }

    ret_push = ring_queue_push(event_, sizeof(window_event_t), alignof(window_event_t), s_app_state->window_event_queue);
    if(RING_QUEUE_SUCCESS != ret_push) {
        application_result_t ret = app_rslt_convert_ring_queue(ret_push);
        WARN_MESSAGE("on_window(%s) - Failed to push window event.", app_rslt_to_str(ret));
        goto cleanup;
    }
cleanup:
    return;
}

/**
 * @brief event_をキーボードイベント用リングキューに格納する
 * @note キーボードイベントコールバック
 *
 * @param[in] event_ イベントキューに格納するイベント構造体インスタンスへのポインタ
 */
static void on_key(const keyboard_event_t* event_) {
    ring_queue_result_t ret_push = RING_QUEUE_INVALID_ARGUMENT;

    if(NULL == event_) {
        WARN_MESSAGE("on_key - Argument event_ requires a valid pointer.");
        goto cleanup;
    }
    if(NULL == s_app_state) {
        WARN_MESSAGE("on_key - Application state is uninitialized.");
        goto cleanup;
    }

    ret_push = ring_queue_push(event_, sizeof(keyboard_event_t), alignof(keyboard_event_t), s_app_state->keyboard_event_queue);
    if(RING_QUEUE_SUCCESS != ret_push) {
        application_result_t ret = app_rslt_convert_ring_queue(ret_push);
        WARN_MESSAGE("on_key(%s) - Failed to push keyboard event.", app_rslt_to_str(ret));
        goto cleanup;
    }
cleanup:
    return;
}

/**
 * @brief event_をマウスイベント用リングキューに格納する
 * @note マウスイベントコールバック
 *
 * @param[in] event_ イベントキューに格納するイベント構造体インスタンスへのポインタ
 */
static void on_mouse(const mouse_event_t* event_) {
    ring_queue_result_t ret_push = RING_QUEUE_INVALID_ARGUMENT;

    if(NULL == event_) {
        WARN_MESSAGE("on_mouse - Argument event_ requires a valid pointer.");
        goto cleanup;
    }
    if(NULL == s_app_state) {
        WARN_MESSAGE("on_mouse - Application state is not initialized.");
        goto cleanup;
    }

    ret_push = ring_queue_push(event_, sizeof(mouse_event_t), alignof(mouse_event_t), s_app_state->mouse_event_queue);
    if(RING_QUEUE_SUCCESS != ret_push) {
        application_result_t ret = app_rslt_convert_ring_queue(ret_push);
        WARN_MESSAGE("on_mouse(%s) - Failed to push mouse event.", app_rslt_to_str(ret));
        goto cleanup;
    }
cleanup:
    return;
}

/**
 * @brief イベント格納用リングキューに格納されているイベントを処理し、アプリケーション状態を更新する
 *
 */
static void app_state_update(void) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    if(NULL == s_app_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_state_update(%s) - Application state is not initialized.", app_rslt_to_str(ret));
        goto cleanup;
    }
    if(NULL == s_app_state->window_event_queue) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_state_update(%s) - window event queue is not initialized.", app_rslt_to_str(ret));
        goto cleanup;
    }
    if(NULL == s_app_state->keyboard_event_queue) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_state_update(%s) - keyboard event queue is not initialized.", app_rslt_to_str(ret));
        goto cleanup;
    }
    if(NULL == s_app_state->mouse_event_queue) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_state_update(%s) - mouse event queue is not initialized.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // window events.
    while(!ring_queue_empty(s_app_state->window_event_queue)) {
        window_event_t event;
        ring_queue_result_t ret_ring = ring_queue_pop(sizeof(window_event_t), alignof(window_event_t), s_app_state->window_event_queue, &event);
        if(RING_QUEUE_SUCCESS != ret_ring) {
            ret = app_rslt_convert_ring_queue(ret_ring);
            WARN_MESSAGE("app_state_update(%s) - Failed to pop window event.", app_rslt_to_str(ret));
            goto cleanup;
        } else {
            if(WINDOW_EVENT_RESIZE == event.event_code) {
                INFO_MESSAGE("Window resized: window([%dx%d] -> [%dx%d]), framebuffer([%dx%d] -> [%dx%d])",
                    s_app_state->window_width, s_app_state->window_height, event.event_args.window_width, event.event_args.window_height,
                    s_app_state->framebuffer_width, s_app_state->framebuffer_height, event.event_args.framebuffer_width, event.event_args.framebuffer_height);

                s_app_state->window_resized = true;
                s_app_state->window_height = event.event_args.window_height;
                s_app_state->window_width = event.event_args.window_width;
                s_app_state->framebuffer_height = event.event_args.framebuffer_height;
                s_app_state->framebuffer_width = event.event_args.framebuffer_width;
            }
        }
    }

    // keyboard events.
    while(!ring_queue_empty(s_app_state->keyboard_event_queue)) {
        keyboard_event_t event;
        ring_queue_result_t ret_ring = ring_queue_pop(sizeof(keyboard_event_t), alignof(keyboard_event_t), s_app_state->keyboard_event_queue, &event);
        if(RING_QUEUE_SUCCESS != ret_ring) {
            ret = app_rslt_convert_ring_queue(ret_ring);
            WARN_MESSAGE("app_state_update(%s) - Failed to pop keyboard event.", app_rslt_to_str(ret));
            goto cleanup;
        } else {
            if(KEY_M == event.key && !event.event_args.pressed) {
                memory_system_report();
            } else {
                ret = flight_camera_command_update(&event, s_app_state->flight_camera_commands);
                if(APPLICATION_SUCCESS != ret) {
                    WARN_MESSAGE("app_state_update(%s) - Failed to update flight camera command.", app_rslt_to_str(ret));
                    goto cleanup;
                }
            }
        }
    }

    // mouse events.
    while(!ring_queue_empty(s_app_state->mouse_event_queue)) {
        mouse_event_t event;
        ring_queue_result_t ret_ring = ring_queue_pop(sizeof(mouse_event_t), alignof(mouse_event_t), s_app_state->mouse_event_queue, &event);
        if(RING_QUEUE_SUCCESS != ret_ring) {
            ret = app_rslt_convert_ring_queue(ret_ring);
            WARN_MESSAGE("app_state_update(%s) - Failed to pop mouse event.", app_rslt_to_str(ret));
            goto cleanup;
        } else {
            if(MOUSE_BUTTON_LEFT == event.button) {
                INFO_MESSAGE("Mouse left %s at (%d, %d)", (event.event_args.pressed) ? "pressed" : "released", event.event_args.x, event.event_args.y);
            } else if(MOUSE_BUTTON_RIGHT == event.button) {
                INFO_MESSAGE("Mouse right %s at (%d, %d)", (event.event_args.pressed) ? "pressed" : "released", event.event_args.x, event.event_args.y);
            }
        }
    }

cleanup:
    return;
}

/**
 * @brief 更新されたアプリケーション状態によって、各サブシステムにイベントを通知する
 *
 * @todo ウィンドウサイズ変化時の視錐台更新、プロジェクション行列更新に失敗した場合に、app_state_cleanでwindow_resizedフラグをfalseにしないようにする
 */
static void app_state_dispatch(void) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    if(s_app_state->window_resized) {
        if(0 < s_app_state->framebuffer_height && 0 < s_app_state->framebuffer_width) {
            camera_result_t ret_camera = camera_viewing_frustum_update(45.0f, (float)s_app_state->framebuffer_width / (float)s_app_state->framebuffer_height, 0.1f, 50.0f, s_app_state->active_camera); // TODO: エラー処理
            if(CAMERA_SUCCESS != ret_camera) {
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to update world camera frustum.", app_rslt_to_str(app_rslt_convert_camera(ret_camera)));
                goto cleanup;
            }

            mat4x4f_t tmp_projection = { 0 };
            ret_camera = camera_perspective_matrix_get(s_app_state->active_camera, &tmp_projection);
            if(CAMERA_SUCCESS != ret_camera) {
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to get perspective matrix.", app_rslt_to_str(app_rslt_convert_camera(ret_camera)));
                goto cleanup;
            }

            ui_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader);
            renderer_result_t ret_renderer = ui_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &tmp_projection, true);
            if(RENDERER_SUCCESS != ret_renderer) {
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to set projection matrix.", app_rslt_to_str(app_rslt_convert_renderer(ret_renderer)));
                goto cleanup;
            }

            line_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);
            ret_renderer = line_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, &tmp_projection, true);
            if(RENDERER_SUCCESS != ret_renderer) {
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to set projection matrix.", app_rslt_to_str(app_rslt_convert_renderer(ret_renderer)));
                goto cleanup;
            }

            point_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader);
            ret_renderer = point_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader, &tmp_projection, true);
            if(RENDERER_SUCCESS != ret_renderer) {
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to set projection matrix.", app_rslt_to_str(app_rslt_convert_renderer(ret_renderer)));
                goto cleanup;
            }

            lit_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader);
            ret_renderer = lit_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader, &tmp_projection, true);
            if(RENDERER_SUCCESS != ret_renderer) {
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to set projection matrix.", app_rslt_to_str(app_rslt_convert_renderer(ret_renderer)));
                goto cleanup;
            }

            mat4f_copy(&tmp_projection, &s_app_state->projection_matrix);
        }
    }
    ret =  flight_camera_command_execute(0.1f, 1.0f, s_app_state->flight_camera_commands, s_app_state->active_camera, &s_app_state->view_dirty);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_dispatch(%s) - Failed to execute flight camera command.", app_rslt_to_str(ret));
        goto cleanup;
    }

    if(s_app_state->view_dirty) {
        camera_view_matrix_get(s_app_state->active_camera, &s_app_state->view_matrix);   // TODO: エラー処理

        ui_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader);
        ui_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->view_matrix, true);  // TODO: エラー処理

        line_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);
        line_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, &s_app_state->view_matrix, true);  // TODO: エラー処理

        point_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader);
        point_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader, &s_app_state->view_matrix, true);    // TODO: エラー処理

        lit_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader);
        lit_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader, &s_app_state->view_matrix, true);  // TODO: エラー処理
        s_app_state->view_dirty = false;
    }
cleanup:
    return;
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
static application_result_t test_line_geometry_create(app_state_t* app_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;

    line_mesh_geometry_t* tmp_geometry = NULL;
    line_vertex_t tmp_vertices[2] = { 0 };
    const line_vertex_t* vertices = NULL;
    size_t vertex_count = 0;
    vec4u8_t line_color = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(app_state_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "test_line_geometry_create", "app_state_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(app_state_->test_line_geometry, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "test_line_geometry_create", "app_state_->test_line_geometry")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == app_state_->test_line_geometry_vertex_count, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "test_line_geometry_create", "app_state_->test_line_geometry_vertex_count")
    IF_ARG_NULL_GOTO_CLEANUP(app_state_->renderer_backend_context, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "test_line_geometry_create", "app_state_->renderer_backend_context")
    IF_ARG_NULL_GOTO_CLEANUP(app_state_->line_mesh_shader, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "test_line_geometry_create", "app_state_->line_mesh_shader")

    tmp_vertices[0].position = vec3f_initialize(1.0f, 2.0f, -3.0f);
    tmp_vertices[1].position = vec3f_initialize(4.0f, 5.0f, -6.0f);
    ret_resource = line_mesh_geometry_create_from_vertices("test_line_geometry", 2, tmp_vertices, &tmp_geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = app_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("test_line_geometry_create(%s) - Failed to create test line geometry.", app_rslt_to_str(ret));
        goto cleanup;
    }
    // NOTE:
    // verticesはtmp_verticesと中身は全く同じなので、line_mesh_geometry_vertices_getを実行する必要はないが、
    // line_mesh_geometry_vertices_getの使用例サンプルとして実行する
    ret_resource = line_mesh_geometry_vertices_get(tmp_geometry, &vertices);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = app_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("test_line_geometry_create(%s) - Failed to get test line geometry vertices.", app_rslt_to_str(ret));
        goto cleanup;
    }
    // NOTE:
    // vertex_countは2であることは自明であり、line_mesh_geometry_vertex_count_getを実行する必要はないが、
    // line_mesh_geometry_vertex_count_getの使用例サンプルとして実行する
    ret_resource = line_mesh_geometry_vertex_count_get(tmp_geometry, &vertex_count);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = app_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("test_line_geometry_create(%s) - Failed to get test line geometry vertex count.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer = line_mesh_shader_vertex_buffer_append(app_state_->renderer_backend_context, app_state_->line_mesh_shader, sizeof(line_vertex_t) * vertex_count, vertices, &app_state_->test_line_geometry_vertex_count_offset);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("test_line_geometry_create(%s) - Failed to append vertices to line shader VBO.", app_rslt_to_str(ret));
        goto cleanup;
    }

    line_color = vec4u8_initialize(255, 0, 0, 255);

    app_state_->test_line_geometry = tmp_geometry;
    app_state_->test_line_color = line_color;
    app_state_->test_line_geometry_vertex_count = vertex_count;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_SUCCESS != ret) {
        line_mesh_geometry_destroy(&tmp_geometry);
    }
    return ret;
}

// TODO: remove this!!
// static application_result_t aabb_geometry_create(app_state_t* app_state_) {
//     application_result_t ret = APPLICATION_INVALID_ARGUMENT;
//     resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
//     renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
//     geometry_primitive_result_t ret_geometry = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
//     resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

//     aabb_3d_t aabb = { 0 };
//     line_mesh_geometry_t* tmp_geometry = NULL;
//     const line_vertex_t* vertices = NULL;
//     size_t vertex_count = 0;
//     vec4u8_t color = { 0 };

//     const point_normal_vertex_t* stl_vertices = NULL;
//     size_t stl_vertex_count = 0;
//     size_t stl_vertex_offset = 0;

//     IF_ARG_NULL_GOTO_CLEANUP(app_state_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "aabb_geometry_create", "app_state_")
//     IF_ARG_NOT_NULL_GOTO_CLEANUP(app_state_->aabb_geometry, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "aabb_geometry_create", "app_state_->aabb_geometry")
//     IF_ARG_FALSE_GOTO_CLEANUP(0 == app_state_->aabb_geometry_vertex_count, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "aabb_geometry_create", "app_state_->aabb_geometry_vertex_count")
//     IF_ARG_NULL_GOTO_CLEANUP(app_state_->renderer_backend_context, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "aabb_geometry_create", "app_state_->renderer_backend_context")
//     IF_ARG_NULL_GOTO_CLEANUP(app_state_->line_mesh_shader, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "aabb_geometry_create", "app_state_->line_mesh_shader")
//     IF_ARG_NULL_GOTO_CLEANUP(app_state_->stl_geometry, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "aabb_geometry_create", "app_state_->stl_geometry")
//     IF_ARG_FALSE_GOTO_CLEANUP(0 != app_state_->stl_geometry_vertex_count, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "aabb_geometry_create", "app_state_->stl_geometry_vertex_count")

//     // AABB生成のための頂点データをSTLジオメトリデータから取得
//     ret_resource = lit_mesh_geometry_vertices_get(app_state_->stl_geometry, &stl_vertices);
//     if(RESOURCE_SUCCESS != ret_resource) {
//         ret = app_rslt_convert_resource(ret_resource);
//         ERROR_MESSAGE("aabb_geometry_create(%s) - Failed to get stl geometry vertices.", app_rslt_to_str(ret));
//         goto cleanup;
//     }

//     ret_registry = lit_mesh_geometry_registry_draw_range_get(app_state_->geometry_id_penguin, app_state_->lit_mesh_geometry_registry, &stl_vertex_offset, &stl_vertex_count);
//     if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
//         ERROR_MESSAGE("aabb_geometry_create - Failed to get draw range.");
//         goto cleanup;
//     }
//     ret_resource = lit_mesh_geometry_vertex_count_get(app_state_->stl_geometry, &stl_vertex_count);
//     if(RESOURCE_SUCCESS != ret_resource) {
//         ret = app_rslt_convert_resource(ret_resource);
//         ERROR_MESSAGE("aabb_geometry_create(%s) - Failed to get stl geometry vertex count.", app_rslt_to_str(ret));
//         goto cleanup;
//     }

//     // AABBの生成
//     ret_geometry = aabb_3d_initialize_from_point_normal_vertices(stl_vertices, stl_vertex_count, &aabb);
//     if(GEOMETRY_PRIMITIVE_SUCCESS != ret_geometry) {
//         ret = app_rslt_convert_geometry_primitive(ret_geometry);
//         ERROR_MESSAGE("aabb_geometry_create(%s) - Failed to create aabb from stl vertices.", app_rslt_to_str(ret));
//         goto cleanup;
//     }

//     // AABB描画用線分ジオメトリの生成
//     ret_resource = line_mesh_geometry_create_from_aabbs("aabb", 1, &aabb, &tmp_geometry);
//     if(RESOURCE_SUCCESS != ret_resource) {
//         ret = app_rslt_convert_resource(ret_resource);
//         ERROR_MESSAGE("aabb_geometry_create(%s) - Failed to create line mesh geometry from aabb.", app_rslt_to_str(ret));
//         goto cleanup;
//     }

//     // VBO書き込み
//     ret_resource = line_mesh_geometry_vertices_get(tmp_geometry, &vertices);
//     if(RESOURCE_SUCCESS != ret_resource) {
//         ret = app_rslt_convert_resource(ret_resource);
//         ERROR_MESSAGE("aabb_geometry_create(%s) - Failed to get line mesh geometry vertices.", app_rslt_to_str(ret));
//         goto cleanup;
//     }
//     ret_resource = line_mesh_geometry_vertex_count_get(tmp_geometry, &vertex_count);
//     if(RESOURCE_SUCCESS != ret_resource) {
//         ret = app_rslt_convert_resource(ret_resource);
//         ERROR_MESSAGE("application_run(%s) - Failed to get line mesh geometry vertex count.", app_rslt_to_str(ret));
//         goto cleanup;
//     }
//     ret_renderer = line_mesh_shader_vertex_buffer_append(app_state_->renderer_backend_context, app_state_->line_mesh_shader, sizeof(line_vertex_t) * vertex_count, vertices, &app_state_->aabb_geometry_vertex_count_offset);
//     if(RENDERER_SUCCESS != ret_renderer) {
//         ret = app_rslt_convert_renderer(ret_renderer);
//         ERROR_MESSAGE("application_run(%s) - Failed to append vertices to line shader VBO.", app_rslt_to_str(ret));
//         goto cleanup;
//     }

//     color = vec4u8_initialize(0, 0, 255, 255);

//     app_state_->aabb_geometry = tmp_geometry;
//     app_state_->aabb_color = color;
//     app_state_->aabb_geometry_vertex_count = vertex_count;

//     ret = APPLICATION_SUCCESS;

// cleanup:
//     if(APPLICATION_SUCCESS != ret) {
//         line_mesh_geometry_destroy(&tmp_geometry);
//     }
//     return ret;
// }

// TODO: remove this!!
static application_result_t point_geometry_create(app_state_t* app_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    point_vertex_t tmp_vertices[8] = { 0 };
    vec4u8_t colors[8] = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(app_state_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "point_geometry_create", "app_state_")
    IF_ARG_NULL_GOTO_CLEANUP(app_state_->renderer_backend_context, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "point_geometry_create", "app_state_->renderer_backend_context")
    IF_ARG_NULL_GOTO_CLEANUP(app_state_->point_mesh_shader, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "point_geometry_create", "app_state_->point_mesh_shader")

    tmp_vertices[0].position = vec3f_initialize(-0.5, -0.5f, -3.0f);
    tmp_vertices[1].position = vec3f_initialize(-0.4f, -0.4f, -3.0f);
    tmp_vertices[2].position = vec3f_initialize(-0.3f, -0.3f, -3.0f);
    tmp_vertices[3].position = vec3f_initialize(-0.2f, -0.2f, -3.0f);
    tmp_vertices[4].position = vec3f_initialize(-0.1f, -0.1f, -3.0f);
    tmp_vertices[5].position = vec3f_initialize(0.1f, 0.1f, -3.0f);
    tmp_vertices[6].position = vec3f_initialize(0.2f, 0.2f, -3.0f);
    tmp_vertices[7].position = vec3f_initialize(0.3f, 0.3f, -3.0f);

    colors[0] = vec4u8_initialize(255, 0, 0, 255);
    colors[1] = vec4u8_initialize(255, 255, 0, 255);
    colors[2] = vec4u8_initialize(255, 0, 255, 255);
    colors[3] = vec4u8_initialize(0, 255, 0, 255);
    colors[4] = vec4u8_initialize(255, 255, 0, 255);
    colors[5] = vec4u8_initialize(255, 255, 0, 255);
    colors[6] = vec4u8_initialize(255, 255, 0, 255);
    colors[7] = vec4u8_initialize(255, 255, 0, 255);

    ret_resource_pipeline = point_mesh_geometry_pipeline_import_from_vertices(app_state_->renderer_backend_context, app_state_->point_mesh_shader, app_state_->point_mesh_geometry_registry, "test_points", tmp_vertices, 8, &app_state_->geometry_id_test_points);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ERROR_MESSAGE("point_geometry_create - Failed to import point mesh geometry.");
        goto cleanup;
    }

    ret_renderer = point_mesh_shader_vertex_buffer_color_append(app_state_->renderer_backend_context, app_state_->point_mesh_shader, sizeof(vec4u8_t) * 8, &colors[0]);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("point_geometry_create(%s) - Failed to append colors to point shader VBO.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

// TODO: remove this!!
// TODO: 共通のgeometryでウサギとテストテクスチャuiを描画する(モデル行列は変える)
static application_result_t ui_geometry_create(app_state_t* app_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    geometry_primitive_result_t ret_geometry = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;

    ui_mesh_geometry_t* geometry = NULL;
    const ui_vertex_t* vertices = NULL;
    size_t vertex_count = 0;
    ui_vertex_t ui_vertex[6] = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(app_state_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "ui_geometry_create", "app_state_")
    IF_ARG_NULL_GOTO_CLEANUP(app_state_->renderer_backend_context, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "ui_geometry_create", "app_state_->renderer_backend_context")
    IF_ARG_NULL_GOTO_CLEANUP(app_state_->ui_mesh_shader, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "ui_geometry_create", "app_state_->ui_mesh_shader")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(app_state_->ui_geometry, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "ui_geometry_create", "app_state_->ui_geometry")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == app_state_->ui_geometry_vertex_count, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "ui_geometry_create", "app_state_->ui_geometry_vertex_count")

    ui_vertex[0].position = vec2f_initialize(-1.0f, -1.0f);
    ui_vertex[1].position = vec2f_initialize(1.0f, -1.0f);
    ui_vertex[2].position = vec2f_initialize(1.0f, 1.0f);

    ui_vertex[3].position = vec2f_initialize(-1.0f, -1.0f);
    ui_vertex[4].position = vec2f_initialize(1.0f, 1.0f);
    ui_vertex[5].position = vec2f_initialize(-1.0f, 1.0f);

    ui_vertex[0].tex_coord = vec2f_initialize(0.0f, 1.0f);
    ui_vertex[1].tex_coord = vec2f_initialize(1.0f, 1.0f);
    ui_vertex[2].tex_coord = vec2f_initialize(1.0f, 0.0f);

    ui_vertex[3].tex_coord = vec2f_initialize(0.0f, 1.0f);
    ui_vertex[4].tex_coord = vec2f_initialize(1.0f, 0.0f);
    ui_vertex[5].tex_coord = vec2f_initialize(0.0f, 0.0f);

    ret_resource = ui_mesh_geometry_create_from_vertices("ui_geometry", 6, ui_vertex, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = app_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("ui_geometry_create(%s) - Failed to create ui geometry.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_resource = ui_mesh_geometry_vertices_get(geometry, &vertices);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = app_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("ui_gemetry_create(%s) - Failed to get ui geometry vertices.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_resource = ui_mesh_geometry_vertex_count_get(geometry, &vertex_count);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = app_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("ui_gemetry_create(%s) - Failed to get ui geometry vertex count.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer = ui_mesh_shader_vertex_buffer_append(app_state_->renderer_backend_context, app_state_->ui_mesh_shader, sizeof(ui_vertex_t) * vertex_count, vertices, &app_state_->ui_geometry_vertex_count_offset);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("ui_gemetry_create(%s) - Failed to append vertex to ui shader VBO.", app_rslt_to_str(ret));
        goto cleanup;
    }

    app_state_->ui_geometry = geometry;
    app_state_->ui_geometry_vertex_count = vertex_count;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_SUCCESS != ret) {
        ui_mesh_geometry_destroy(&geometry);
    }
    return ret;
}

// TODO: remove this!!
static void test_line_geometry_destroy(app_state_t* app_state_) {
    if(NULL == app_state_) {
        ERROR_MESSAGE("test_line_geometry_destroy(%s) - Application state is not initialized.", app_rslt_to_str(APPLICATION_RUNTIME_ERROR));
        return;
    }
    line_mesh_geometry_destroy(&app_state_->test_line_geometry);
    app_state_->test_line_geometry_vertex_count = 0;
    app_state_->test_line_color = vec4u8_initialize(0, 0, 0, 0);
    app_state_->test_line_geometry_vertex_count_offset = 0;
}

// TODO: remove this!!
// static void aabb_geometry_destroy(app_state_t* app_state_) {
//     if(NULL == app_state_) {
//         ERROR_MESSAGE("aabb_geometry_destroy(%s) - Application state is not initialized.", app_rslt_to_str(APPLICATION_RUNTIME_ERROR));
//         return;
//     }
//     line_mesh_geometry_destroy(&app_state_->aabb_geometry);
//     app_state_->aabb_geometry_vertex_count = 0;
//     app_state_->aabb_color = vec4u8_initialize(0, 0, 0, 0);
//     app_state_->aabb_geometry_vertex_count_offset = 0;
// }

// TODO: remove this!!
static void ui_geometry_destroy(app_state_t* app_state_) {
    if(NULL == app_state_) {
        ERROR_MESSAGE("ui_geometry_destroy(%s) - Application state is not initialized.", app_rslt_to_str(APPLICATION_RUNTIME_ERROR));
        return;
    }
    ui_mesh_geometry_destroy(&app_state_->ui_geometry);
    app_state_->ui_geometry_vertex_count = 0;
    app_state_->ui_geometry_vertex_count_offset = 0;
}
