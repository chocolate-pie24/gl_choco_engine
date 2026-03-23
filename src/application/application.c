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

#include "engine/containers/ring_queue.h"
#include "engine/containers/choco_string.h"

#include "engine/platform/platform_core/platform_types.h"
#include "engine/platform/platform_context.h"

#include "engine/renderer/renderer_resources/ui_shader.h"

#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/context.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/context_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/context_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/context_vbo.h"

#include "engine/camera_system/camera_manager/camera_manager.h"

#include "engine/camera_system/camera_core/camera_types.h"
#include "engine/camera_system/camera/camera.h"

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

    ui_shader_t* ui_shader;
    renderer_backend_vao_t* ui_vao;
    renderer_backend_vbo_t* ui_vbo;

    camera_manager_t* camera_manager;
    camera_t* active_camera;
    int16_t active_camera_id;
    command_status_flight_camera_t flight_camera_commands[FLIGHT_CAMERA_COMMAND_MAX];
    // end

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

application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    application_result_t ret = APPLICATION_RUNTIME_ERROR;
    memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    platform_result_t ret_platform = PLATFORM_INVALID_ARGUMENT;
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

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

    tmp->linear_alloc_pool_size = 1 * KIB;
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
    ret_renderer = ui_shader_create("assets/shaders/test_shader/", "ui_shader", tmp->renderer_backend_context, &tmp->ui_shader);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create ui shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_renderer = renderer_backend_vertex_array_create(tmp->renderer_backend_context, &tmp->ui_vao);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create ui vao.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_renderer = renderer_backend_vertex_buffer_create(tmp->renderer_backend_context, &tmp->ui_vbo);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = app_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("application_create(%s) - Failed to create ui vbo.", app_rslt_to_str(ret));
        goto cleanup;
    }
    tmp->build_config.selected_graphics_api = GRAPHICS_API_GL33;

    // camera create.
    ret = flight_camera_command_initialize(FLIGHT_CAMERA_COMMAND_MAX, tmp->flight_camera_commands);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_create(%s) - Failed to initialize flight camera commands.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_camera = camera_manager_initialize(tmp->linear_alloc, 8, &tmp->camera_manager);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_create(%s) - Failed to create camera manager.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_camera = camera_manager_regist("flight camera", tmp->camera_manager, &tmp->active_camera_id);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_create(%s) - Failed to regist camera.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_camera = camera_manager_camera_get(tmp->active_camera_id, tmp->camera_manager, &tmp->active_camera);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_create(%s) - Failed to get camera.", app_rslt_to_str(ret));
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
            if(NULL != tmp->camera_manager) {
                camera_manager_deinitialize(&tmp->camera_manager);
            }
            if(NULL != tmp->renderer_backend_context) {
                if(NULL != tmp->ui_vbo) {
                    renderer_backend_vertex_buffer_destroy(tmp->renderer_backend_context, &tmp->ui_vbo);
                    tmp->ui_vbo = NULL;
                }
                if(NULL != tmp->ui_vao) {
                    renderer_backend_vertex_array_destroy(tmp->renderer_backend_context, &tmp->ui_vao);
                    tmp->ui_vao = NULL;
                }
                if(NULL != tmp->ui_shader) {
                    ui_shader_destroy(tmp->renderer_backend_context, &tmp->ui_shader);
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
    if(NULL != s_app_state->camera_manager) {
        camera_manager_deinitialize(&s_app_state->camera_manager);
    }
    if(NULL != s_app_state->renderer_backend_context) {
        if(NULL != s_app_state->ui_vbo) {
            renderer_backend_vertex_buffer_destroy(s_app_state->renderer_backend_context, &s_app_state->ui_vbo);
            s_app_state->ui_vbo = NULL;
        }
        if(NULL != s_app_state->ui_vao) {
            renderer_backend_vertex_array_destroy(s_app_state->renderer_backend_context, &s_app_state->ui_vao);
            s_app_state->ui_vao = NULL;
        }
        if(NULL != s_app_state->ui_shader) {
            ui_shader_destroy(s_app_state->renderer_backend_context, &s_app_state->ui_shader);
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
    if(NULL == s_app_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(%s) - Application is not initialized.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // begin temporary
    renderer_backend_vertex_array_bind(s_app_state->renderer_backend_context, s_app_state->ui_vao);

    static vec3f_t vertex_buffer_data[3] = { 0 };
    vec3f_initialize(-1.0f, -1.0f, -1.0f, &vertex_buffer_data[0]);
    vec3f_initialize(1.0f, -1.0f, -1.0f, &vertex_buffer_data[1]);
    vec3f_initialize(0.0f, 1.0f, -1.0f, &vertex_buffer_data[2]);

    renderer_backend_vertex_buffer_bind(s_app_state->renderer_backend_context, s_app_state->ui_vbo);
    renderer_backend_vertex_buffer_vertex_load(s_app_state->renderer_backend_context, s_app_state->ui_vbo, sizeof(vertex_buffer_data), (void*)vertex_buffer_data, BUFFER_USAGE_STATIC);
    renderer_backend_vertex_array_attribute_set(s_app_state->renderer_backend_context, s_app_state->ui_vao, 0, 3, RENDERER_TYPE_FLOAT, false, sizeof(GLfloat) * 3, 0);

    renderer_backend_vertex_buffer_unbind(s_app_state->renderer_backend_context, s_app_state->ui_vbo);
    renderer_backend_vertex_array_unbind(s_app_state->renderer_backend_context, s_app_state->ui_vao);

    mat4f_identity(&s_app_state->model_matrix);
    mat4f_identity(&s_app_state->projection_matrix);
    mat4f_identity(&s_app_state->view_matrix);

    camera_viewing_frustum_update(45.0f, (float)s_app_state->framebuffer_width / (float)s_app_state->framebuffer_height, 0.1f, 50.0f, s_app_state->active_camera); // TODO: エラー処理
    camera_perspective_matrix_get(s_app_state->active_camera, &s_app_state->projection_matrix); // TODO: エラー処理
    camera_view_matrix_get(s_app_state->active_camera, &s_app_state->view_matrix);   // TODO: エラー処理

    ui_shader_model_matrix_set(&s_app_state->model_matrix, true, s_app_state->renderer_backend_context, s_app_state->ui_shader);
    ui_shader_view_matrix_set(&s_app_state->view_matrix, true, s_app_state->renderer_backend_context, s_app_state->ui_shader);
    ui_shader_projection_matrix_set(&s_app_state->projection_matrix, true, s_app_state->renderer_backend_context, s_app_state->ui_shader);
    // TODO: window NULLチェック

    INFO_MESSAGE("current camera: %s.", camera_name_get(s_app_state->active_camera));
    // end temporary

    struct timespec  req = {0, 1000000};
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

        ui_shader_use(s_app_state->renderer_backend_context, s_app_state->ui_shader);

        glViewport(0, 0, s_app_state->framebuffer_width, s_app_state->framebuffer_height);

        renderer_backend_vertex_array_bind(s_app_state->renderer_backend_context, s_app_state->ui_vao);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        renderer_backend_vertex_array_unbind(s_app_state->renderer_backend_context, s_app_state->ui_vao);

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
 * @param event_ イベントキューに格納するイベントオブジェクト
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

    ret_push = ring_queue_push(s_app_state->window_event_queue, event_, sizeof(window_event_t), alignof(window_event_t));
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
 * @param event_ イベントキューに格納するイベントオブジェクト
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

    ret_push = ring_queue_push(s_app_state->keyboard_event_queue, event_, sizeof(keyboard_event_t), alignof(keyboard_event_t));
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
 * @param event_ イベントキューに格納するイベントオブジェクト
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

    ret_push = ring_queue_push(s_app_state->mouse_event_queue, event_, sizeof(mouse_event_t), alignof(mouse_event_t));
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
        ring_queue_result_t ret_ring = ring_queue_pop(s_app_state->window_event_queue, &event, sizeof(window_event_t), alignof(window_event_t));
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
        ring_queue_result_t ret_ring = ring_queue_pop(s_app_state->keyboard_event_queue, &event, sizeof(keyboard_event_t), alignof(keyboard_event_t));
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
        ring_queue_result_t ret_ring = ring_queue_pop(s_app_state->mouse_event_queue, &event, sizeof(mouse_event_t), alignof(mouse_event_t));
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

            renderer_result_t ret_renderer = ui_shader_projection_matrix_set(&tmp_projection, true, s_app_state->renderer_backend_context, s_app_state->ui_shader);
            if(RENDERER_SUCCESS != ret_renderer) {
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to set projection matrix.", app_rslt_to_str(app_rslt_convert_renderer(ret_renderer)));
                goto cleanup;
            }
            mat4f_copy(&tmp_projection, &s_app_state->projection_matrix);
        }
    }
    application_result_t ret =  flight_camera_command_execute(0.1f, 1.0f, s_app_state->active_camera, s_app_state->flight_camera_commands, &s_app_state->view_dirty);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("app_state_dispatch(%s) - Failed to execute flight camera command.", app_rslt_to_str(ret));
        goto cleanup;
    }

    if(s_app_state->view_dirty) {
        camera_view_matrix_get(s_app_state->active_camera, &s_app_state->view_matrix);   // TODO: エラー処理
        ui_shader_view_matrix_set(&s_app_state->view_matrix, true, s_app_state->renderer_backend_context, s_app_state->ui_shader);  // TODO: エラー処理
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
