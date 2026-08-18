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

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

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

#include "engine/resource/core/resource_types.h"

#include "engine/systems/platform/core/platform_types.h"
#include "engine/systems/platform/platform_context.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/shaders/ui_mesh_shader.h"
#include "engine/systems/renderer/resources/shaders/line_mesh_shader.h"
#include "engine/systems/renderer/resources/shaders/point_mesh_shader.h"
#include "engine/systems/renderer/resources/shaders/lit_mesh_shader.h"
#include "engine/systems/renderer/resources/texture/core/texture_gpu_resource_types.h"
#include "engine/systems/renderer/resources/texture/texture_gpu_resource.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/lit_mesh_geometry_registry.h"
#include "engine/systems/renderer/resource_registries/geometries/point_mesh_geometry_registry.h"
#include "engine/systems/renderer/resource_registries/geometries/ui_mesh_geometry_registry.h"
#include "engine/systems/renderer/resource_registries/geometries/line_mesh_geometry_registry.h"
#include "engine/systems/renderer/resource_registries/texture/texture_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/geometries/lit_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resource_pipelines/geometries/point_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resource_pipelines/geometries/ui_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resource_pipelines/geometries/line_mesh_geometry_pipeline.h"
#include "engine/systems/renderer/resource_pipelines/texture/texture_pipeline.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_vbo.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_texture.h"

#include "engine/systems/camera_system/camera_manager/camera_manager.h"
#include "engine/systems/camera_system/camera_core/camera_types.h"
#include "engine/systems/camera_system/camera/camera.h"

#include "engine/resource/geometry/lit_mesh_geometry.h"

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

    vbo_manager_config_t line_mesh_shader_vbo_config;
    vbo_manager_config_t lit_mesh_shader_vbo_config;
    vbo_manager_config_t point_mesh_shader_vbo_config;
    vbo_manager_config_t ui_mesh_shader_vbo_config;

    ui_mesh_shader_t* ui_mesh_shader;
    line_mesh_shader_t* line_mesh_shader;
    point_mesh_shader_t* point_mesh_shader;
    lit_mesh_shader_t* lit_mesh_shader;

    camera_manager_t* camera_manager;
    camera_t* active_camera;
    int16_t active_camera_id;
    command_status_flight_camera_t flight_camera_commands[FLIGHT_CAMERA_COMMAND_MAX];

    texture_registry_t* texture_registry;
    // end

    // begin temporary TODO: remove this!!
    point_mesh_geometry_registry_t* point_mesh_geometry_registry;
    int16_t geometry_id_test_points;
    vbo_range_t test_points_buffer_range;

    lit_mesh_geometry_registry_t* lit_mesh_geometry_registry;
    int16_t geometry_id_penguin;
    vbo_range_t penguin_buffer_range;
    bool should_draw_penguin_aabb;

    ui_mesh_geometry_registry_t* ui_mesh_geometry_registry;
    int16_t geometry_id_small_icon;
    int16_t geometry_id_large_icon;
    vbo_range_t small_icon_buffer_range;
    vbo_range_t large_icon_buffer_range;

    line_mesh_geometry_registry_t* line_mesh_geometry_registry;
    int16_t geometry_id_penguin_aabb;
    vbo_range_t penguin_aabb_buffer_range;
    vec4u8_t penguin_aabb_color;
    int16_t geometry_id_test_line;
    vbo_range_t test_line_buffer_range;
    vec4u8_t test_line_color;

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

static application_result_t point_geometry_create(app_state_t* app_state_);        // TODO: remove this!!

application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    application_result_t ret = APPLICATION_RUNTIME_ERROR;

    memory_system_result_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    platform_result_t ret_platform = PLATFORM_INVALID_ARGUMENT;
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

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

    // texture registry.
    ret_registry = texture_registry_initialize(128, tmp->linear_alloc, &tmp->texture_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = APPLICATION_RUNTIME_ERROR;  // TODO: エラーコード返還
        ERROR_MESSAGE("application_create(%s) - Failed to create texture registry.", app_rslt_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("texture registry initialized successfully.");
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

    ret_renderer_backend = renderer_backend_initialize(tmp->linear_alloc, GRAPHICS_API_GL33, &tmp->renderer_backend_context);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = app_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize renderer backend.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // UI Shader
    ret_shader = ui_mesh_shader_create(&tmp->ui_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create ui shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_shader = ui_mesh_shader_program_initialize(tmp->renderer_backend_context, tmp->ui_mesh_shader, "../assets/shaders/test_shader/", "ui_mesh_shader");
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create ui mesh shader.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp->ui_mesh_shader_vbo_config.base_align = alignof(float);
    tmp->ui_mesh_shader_vbo_config.buffer_usage = BUFFER_USAGE_STATIC;
    tmp->ui_mesh_shader_vbo_config.max_allocation_count = 512;
    tmp->ui_mesh_shader_vbo_config.vbo_size = 1024;
    ret_shader = ui_mesh_shader_vbo_initialize(tmp->renderer_backend_context, tmp->ui_mesh_shader, &tmp->ui_mesh_shader_vbo_config);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create ui vertex buffer.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_shader = ui_mesh_shader_vao_initialize(tmp->renderer_backend_context, tmp->ui_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize ui vao.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // Line Shader
    ret_shader = line_mesh_shader_create(&tmp->line_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create line shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_shader = line_mesh_shader_program_initialize(tmp->renderer_backend_context, tmp->line_mesh_shader, "../assets/shaders/test_shader/", "line_mesh_shader");
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create line shader.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp->line_mesh_shader_vbo_config.base_align = alignof(float);
    tmp->line_mesh_shader_vbo_config.buffer_usage = BUFFER_USAGE_STATIC;
    tmp->line_mesh_shader_vbo_config.max_allocation_count = 512;
    tmp->line_mesh_shader_vbo_config.vbo_size = 1024;
    ret_shader = line_mesh_shader_vbo_initialize(tmp->renderer_backend_context, tmp->line_mesh_shader, &tmp->line_mesh_shader_vbo_config);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create line vertex buffer.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_shader = line_mesh_shader_vao_initialize(tmp->renderer_backend_context, tmp->line_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize line vao.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // Point Shader
    ret_shader = point_mesh_shader_create(&tmp->point_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create point shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_shader = point_mesh_shader_program_initialize(tmp->renderer_backend_context, tmp->point_mesh_shader, "../assets/shaders/test_shader/", "point_mesh_shader");
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create point mesh shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    tmp->point_mesh_shader_vbo_config.base_align = alignof(float);
    tmp->point_mesh_shader_vbo_config.buffer_usage = BUFFER_USAGE_DYNAMIC;
    tmp->point_mesh_shader_vbo_config.max_allocation_count = 128;
    tmp->point_mesh_shader_vbo_config.vbo_size = 1 * KIB;
    ret_shader = point_mesh_shader_vbo_initialize(tmp->renderer_backend_context, tmp->point_mesh_shader, &tmp->point_mesh_shader_vbo_config);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create point mesh vertex buffer.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_shader = point_mesh_shader_vao_initialize(tmp->renderer_backend_context, tmp->point_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize point mesh vao.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // Lit Mesh Shader
    ret_shader = lit_mesh_shader_create(&tmp->lit_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create lit mesh shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_shader = lit_mesh_shader_program_initialize(tmp->renderer_backend_context, tmp->lit_mesh_shader, "../assets/shaders/test_shader/", "lit_mesh_shader");
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create lit mesh shader.", app_rslt_to_str(ret));
        goto cleanup;
    }
    tmp->lit_mesh_shader_vbo_config.base_align = alignof(float);
    tmp->lit_mesh_shader_vbo_config.buffer_usage = BUFFER_USAGE_STATIC;
    tmp->lit_mesh_shader_vbo_config.max_allocation_count = 512;
    tmp->lit_mesh_shader_vbo_config.vbo_size = 1 * GIB;
    ret_shader = lit_mesh_shader_vbo_initialize(tmp->renderer_backend_context, tmp->lit_mesh_shader, &tmp->lit_mesh_shader_vbo_config);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to create lit vertex buffer.", app_rslt_to_str(ret));
        goto cleanup;
    }
    ret_shader = lit_mesh_shader_vao_initialize(tmp->renderer_backend_context, tmp->lit_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = app_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize lit vao.", app_rslt_to_str(ret));
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
    ret_camera = camera_manager_register(tmp->camera_manager, "flight camera", &tmp->active_camera_id);
    if(CAMERA_SUCCESS != ret_camera) {
        ret = app_rslt_convert_camera(ret_camera);
        ERROR_MESSAGE("application_create(%s) - Failed to register camera.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_camera = camera_manager_camera_get(tmp->camera_manager, tmp->active_camera_id, &tmp->active_camera);
    // ret_camera = camera_manager_camera_get_by_name(tmp->camera_manager, "flight camera", &tmp->active_camera);
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

    tmp->ui_mesh_geometry_registry = NULL;
    ret_registry = ui_mesh_geometry_registry_initialize(32, tmp->linear_alloc, &tmp->ui_mesh_geometry_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = APPLICATION_RUNTIME_ERROR;
        // TODO: エラーコード変換
        ERROR_MESSAGE("application_create(%s) - Failed to create ui mesh geometry registry.", app_rslt_to_str(APPLICATION_RUNTIME_ERROR));
        goto cleanup;
    }

    tmp->line_mesh_geometry_registry = NULL;
    ret_registry = line_mesh_geometry_registry_initialize(128, tmp->linear_alloc, &tmp->line_mesh_geometry_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = APPLICATION_RUNTIME_ERROR;
        // TODO: エラーコード変換
        ERROR_MESSAGE("application_create(%s) - Failed to create line mesh geometry registry.", app_rslt_to_str(APPLICATION_RUNTIME_ERROR));
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
            if(NULL != tmp->line_mesh_geometry_registry) {
                line_mesh_geometry_registry_deinitialize(tmp->line_mesh_geometry_registry);
            }
            if(NULL != tmp->ui_mesh_geometry_registry) {
                ui_mesh_geometry_registry_deinitialize(tmp->ui_mesh_geometry_registry);
            }
            if(NULL != tmp->lit_mesh_geometry_registry) {
                lit_mesh_geometry_registry_deinitialize(tmp->lit_mesh_geometry_registry);
            }
            if(NULL != tmp->point_mesh_geometry_registry) {
                point_mesh_geometry_registry_deinitialize(tmp->point_mesh_geometry_registry);
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
    if(NULL != s_app_state->line_mesh_geometry_registry) {
        line_mesh_geometry_registry_deinitialize(s_app_state->line_mesh_geometry_registry);
    }
    if(NULL != s_app_state->ui_mesh_geometry_registry) {
        ui_mesh_geometry_registry_deinitialize(s_app_state->ui_mesh_geometry_registry);
    }
    if(NULL != s_app_state->lit_mesh_geometry_registry) {
        lit_mesh_geometry_registry_deinitialize(s_app_state->lit_mesh_geometry_registry);
    }
    if(NULL != s_app_state->point_mesh_geometry_registry) {
        point_mesh_geometry_registry_deinitialize(s_app_state->point_mesh_geometry_registry);
    }
    if(NULL != s_app_state->texture_registry) {
        texture_registry_deinitialize(s_app_state->texture_registry, s_app_state->renderer_backend_context);
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

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    geometry_primitive_result_t ret_geometry = GEOMETRY_PRIMITIVE_INVALID_ARGUMENT;
    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registy = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    int16_t tex_id_rabbit = 0;
    int16_t tex_id_frog = 0;
    int16_t tex_id_green = 0;
    const texture_gpu_resource_t* tex_gpu_resource = NULL;

    // penguin AABB
    const lit_mesh_geometry_t* penguin_geometry = NULL;
    const point_normal_vertex_t* penguin_vertices = NULL;
    size_t penguin_vertex_count = 0;
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

    camera_viewing_frustum_update(s_app_state->active_camera, 45.0f, (float)s_app_state->framebuffer_width / (float)s_app_state->framebuffer_height, 0.1f, 50.0f); // TODO: エラー処理
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

    ret_resource_pipeline = texture_pipeline_import_from_file(s_app_state->renderer_backend_context, s_app_state->texture_registry, 0, "rabbit_512", &tex_id_rabbit);
    ret_resource_pipeline = texture_pipeline_import_from_file(s_app_state->renderer_backend_context, s_app_state->texture_registry, 0, "frog_512", &tex_id_frog);
    ret_resource_pipeline = texture_pipeline_import_from_file(s_app_state->renderer_backend_context, s_app_state->texture_registry, 0, "test_texture_green", &tex_id_green);

    // ペンギンSTL pipeline import
    ret_resource_pipeline = lit_mesh_geometry_pipeline_import_from_file(
        s_app_state->renderer_backend_context,
        s_app_state->lit_mesh_shader,
        s_app_state->lit_mesh_geometry_registry,
        "../assets/stl/glce_lowpoly_animal_stl_ascii/", "glce_lowpoly_penguin_ascii", ".stl",
        &s_app_state->geometry_id_penguin);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = APPLICATION_RUNTIME_ERROR;    // temporary
        ERROR_MESSAGE("application_run - Failed to import lit mesh geometry.");
        goto cleanup;
    }

    // ペンギンAABB pipeline import
    s_app_state->should_draw_penguin_aabb = true;
    if(s_app_state->should_draw_penguin_aabb) {
        s_app_state->penguin_aabb_color = vec4u8_initialize(255, 0, 0, 255);
        penguin_geometry = lit_mesh_geometry_registry_geometry_get(s_app_state->lit_mesh_geometry_registry, s_app_state->geometry_id_penguin);
        if(NULL == penguin_geometry) {
            ret = APPLICATION_RUNTIME_ERROR;    // temporary
            ERROR_MESSAGE("application_run - Failed to get penguin geometry.");
            goto cleanup;
        }
        ret_resource = lit_mesh_geometry_vertex_count_get(penguin_geometry, &penguin_vertex_count);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = APPLICATION_RUNTIME_ERROR;
            ERROR_MESSAGE("application_run - Failed to get penguin vertex count.");
            goto cleanup;
        }
        ret_resource = lit_mesh_geometry_vertices_get(penguin_geometry, &penguin_vertices);
        if(RESOURCE_SUCCESS != ret_resource) {
            ret = APPLICATION_RUNTIME_ERROR;
            ERROR_MESSAGE("application_run - Failed to get penguin vertices.");
            goto cleanup;
        }
        ret_geometry = aabb_3d_initialize_from_point_normal_vertices(penguin_vertices, penguin_vertex_count, &penguin_aabb);
        if(GEOMETRY_PRIMITIVE_SUCCESS != ret_geometry) {
            ret = APPLICATION_RUNTIME_ERROR;
            ERROR_MESSAGE("application_run - Failed to initialize penguin aabb.");
            goto cleanup;
        }
        ret_resource_pipeline = line_mesh_geometry_pipeline_import_from_aabb(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, s_app_state->line_mesh_geometry_registry, "penguin_aabb", &penguin_aabb, &s_app_state->geometry_id_penguin_aabb);
        if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
            ret = APPLICATION_RUNTIME_ERROR;
            ERROR_MESSAGE("application_run - Failed to import penguin aabb.");
            goto cleanup;
        }
    }

    // テスト線分 pipeline import
    tmp_vertices[0].position = vec3f_initialize(1.0f, 2.0f, -3.0f);
    tmp_vertices[1].position = vec3f_initialize(4.0f, 5.0f, -6.0f);
    s_app_state->test_line_color = vec4u8_initialize(0, 255, 0, 255);

    ret_resource_pipeline = line_mesh_geometry_pipeline_import_from_vertices(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, s_app_state->line_mesh_geometry_registry, "test_line", tmp_vertices, 2, &s_app_state->geometry_id_test_line);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run - Failed to import test line.");
        goto cleanup;
    }

    // UI pipeline import
    // アイコンサイズはUI描画用projection, viewができたら整える
    ret_resource_pipeline = ui_mesh_geometry_pipeline_import_from_file(
        s_app_state->renderer_backend_context,
        s_app_state->ui_mesh_shader,
        s_app_state->ui_mesh_geometry_registry,
        "small_icon",
        &s_app_state->geometry_id_small_icon
    );
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = APPLICATION_RUNTIME_ERROR;    // temporary
        ERROR_MESSAGE("application_run - Failed to import ui mesh geometry(small icon).");
        goto cleanup;
    }

    ret_resource_pipeline = ui_mesh_geometry_pipeline_import_from_file(
        s_app_state->renderer_backend_context,
        s_app_state->ui_mesh_shader,
        s_app_state->ui_mesh_geometry_registry,
        "large_icon",
        &s_app_state->geometry_id_large_icon
    );
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = APPLICATION_RUNTIME_ERROR;    // temporary
        ERROR_MESSAGE("application_run - Failed to import ui mesh geometry(large icon).");
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

        // UI描画
        ui_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader);
        ui_mesh_shader_vao_bind(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader);
        ret_resource_registy = ui_mesh_geometry_registry_vbo_range_get(s_app_state->ui_mesh_geometry_registry, s_app_state->geometry_id_small_icon, &s_app_state->small_icon_buffer_range);
        if(RESOURCE_REGISTRY_SUCCESS == ret_resource_registy) {
            // ウサギ
            ui_mesh_shader_model_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->rabbit_mesh_model_mat, true);
            tex_gpu_resource = texture_registry_gpu_resource_get(s_app_state->texture_registry, tex_id_rabbit);
            texture_gpu_resource_bind(s_app_state->renderer_backend_context, tex_gpu_resource);

            glDrawArrays(GL_TRIANGLES, s_app_state->small_icon_buffer_range.draw_range.first_vertex_count, s_app_state->small_icon_buffer_range.draw_range.vertex_count);

            texture_gpu_resource_unbind(s_app_state->renderer_backend_context, tex_gpu_resource);

            // テストテクスチャ
            ui_mesh_shader_model_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->green_mesh_model_mat, true);
            tex_gpu_resource = texture_registry_gpu_resource_get(s_app_state->texture_registry, tex_id_green);
            texture_gpu_resource_bind(s_app_state->renderer_backend_context, tex_gpu_resource);

            glDrawArrays(GL_TRIANGLES, s_app_state->small_icon_buffer_range.draw_range.first_vertex_count, s_app_state->small_icon_buffer_range.draw_range.vertex_count);

            texture_gpu_resource_unbind(s_app_state->renderer_backend_context, tex_gpu_resource);
        }

        ret_resource_registy = ui_mesh_geometry_registry_vbo_range_get(s_app_state->ui_mesh_geometry_registry, s_app_state->geometry_id_large_icon, &s_app_state->large_icon_buffer_range);
        if(RESOURCE_REGISTRY_SUCCESS == ret_resource_registy) {
            // カエル
            ui_mesh_shader_model_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->frog_mesh_model_mat, true);
            tex_gpu_resource = texture_registry_gpu_resource_get(s_app_state->texture_registry, tex_id_frog);
            texture_gpu_resource_bind(s_app_state->renderer_backend_context, tex_gpu_resource);

            glDrawArrays(GL_TRIANGLES, s_app_state->large_icon_buffer_range.draw_range.first_vertex_count, s_app_state->large_icon_buffer_range.draw_range.vertex_count);

            texture_gpu_resource_unbind(s_app_state->renderer_backend_context, tex_gpu_resource);
        }
        renderer_backend_vao_unbind(s_app_state->renderer_backend_context);

        // 線分描画
        line_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);
        line_mesh_shader_vao_bind(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);
        ret_resource_registy = line_mesh_geometry_registry_vbo_range_get(s_app_state->line_mesh_geometry_registry, s_app_state->geometry_id_penguin_aabb, &s_app_state->penguin_aabb_buffer_range);
        if(RESOURCE_REGISTRY_SUCCESS == ret_resource_registy) {
            line_mesh_shader_color_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, s_app_state->penguin_aabb_color.elem);
            glDrawArrays(GL_LINES, s_app_state->penguin_aabb_buffer_range.draw_range.first_vertex_count, s_app_state->penguin_aabb_buffer_range.draw_range.vertex_count);
        }
        ret_resource_registy = line_mesh_geometry_registry_vbo_range_get(s_app_state->line_mesh_geometry_registry, s_app_state->geometry_id_test_line, &s_app_state->test_line_buffer_range);
        if(RESOURCE_REGISTRY_SUCCESS == ret_resource_registy) {
            line_mesh_shader_color_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, s_app_state->test_line_color.elem);
            glDrawArrays(GL_LINES, s_app_state->test_line_buffer_range.draw_range.first_vertex_count, s_app_state->test_line_buffer_range.draw_range.vertex_count);
        }
        renderer_backend_vao_unbind(s_app_state->renderer_backend_context);

        // ポイント描画
        ret_resource_registy = point_mesh_geometry_registry_vbo_range_get(s_app_state->point_mesh_geometry_registry, s_app_state->geometry_id_test_points, &s_app_state->test_points_buffer_range);
        if(RESOURCE_REGISTRY_SUCCESS == ret_resource_registy) {
            point_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader);
            point_mesh_shader_vao_bind(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader);

            glDrawArrays(GL_POINTS, s_app_state->test_points_buffer_range.draw_range.first_vertex_count, s_app_state->test_points_buffer_range.draw_range.vertex_count);
            renderer_backend_vao_unbind(s_app_state->renderer_backend_context);
        }

        // STL描画
        ret_resource_registy = lit_mesh_geometry_registry_vbo_range_get(s_app_state->lit_mesh_geometry_registry, s_app_state->geometry_id_penguin, &s_app_state->penguin_buffer_range);
        if(RESOURCE_REGISTRY_SUCCESS == ret_resource_registy) {
            lit_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader);
            lit_mesh_shader_vao_bind(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader);

            glDrawArrays(GL_TRIANGLES, s_app_state->penguin_buffer_range.draw_range.first_vertex_count, s_app_state->penguin_buffer_range.draw_range.vertex_count);
            renderer_backend_vao_unbind(s_app_state->renderer_backend_context);
        }

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

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    camera_result_t ret_camera = CAMERA_INVALID_ARGUMENT;

    mat4x4f_t tmp_projection = { 0 };

    if(s_app_state->window_resized) {
        if(0 < s_app_state->framebuffer_height && 0 < s_app_state->framebuffer_width) {
            ret_camera = camera_viewing_frustum_update(s_app_state->active_camera, 45.0f, (float)s_app_state->framebuffer_width / (float)s_app_state->framebuffer_height, 0.1f, 50.0f); // TODO: エラー処理
            if(CAMERA_SUCCESS != ret_camera) {
                ret = app_rslt_convert_camera(ret_camera);
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to update world camera frustum.", app_rslt_to_str(ret));
                goto cleanup;
            }

            ret_camera = camera_perspective_matrix_get(s_app_state->active_camera, &tmp_projection);
            if(CAMERA_SUCCESS != ret_camera) {
                ret = app_rslt_convert_camera(ret_camera);
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to get perspective matrix.", app_rslt_to_str(ret));
                goto cleanup;
            }

            ret_shader = ui_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader);
            if(SHADER_SUCCESS != ret_shader) {
                ret = app_rslt_convert_shader(ret_shader);
                ERROR_MESSAGE("app_state_dispatch(%s) - ui_mesh_shader_use failed.", app_rslt_to_str(ret));
                goto cleanup;
            }

            ret_shader = ui_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &tmp_projection, true);
            if(SHADER_SUCCESS != ret_shader) {
                ret = app_rslt_convert_shader(ret_shader);
                ERROR_MESSAGE("app_state_dispatch(%s) - Failed to set projection matrix.", app_rslt_to_str(ret));
                goto cleanup;
            }

            ret_shader = line_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);
            if(SHADER_SUCCESS != ret_shader) {
                ret = app_rslt_convert_shader(ret_shader);
                ERROR_MESSAGE("app_state_dispatch(%s) - line_mesh_shader_use failed.", app_rslt_to_str(ret));
                goto cleanup;
            }

            ret_shader = line_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, &tmp_projection, true);
            if(SHADER_SUCCESS != ret_shader) {
                ret = app_rslt_convert_shader(ret_shader);
                ERROR_MESSAGE("app_state_dispatch(%s) - line_mesh_shader_projection_matrix_set failed.", app_rslt_to_str(ret));
                goto cleanup;
            }

            ret_shader = point_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader);
            if(SHADER_SUCCESS != ret_shader) {
                ret = app_rslt_convert_shader(ret_shader);
                ERROR_MESSAGE("app_state_dispatch(%s) - point_mesh_shader_use failed.", app_rslt_to_str(ret));
                goto cleanup;
            }

            ret_shader = point_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader, &tmp_projection, true);
            if(SHADER_SUCCESS != ret_shader) {
                ret = app_rslt_convert_shader(ret_shader);
                ERROR_MESSAGE("app_state_dispatch(%s) - point_mesh_shader_projection_matrix_set failed.", app_rslt_to_str(ret));
                goto cleanup;
            }

            ret_shader = lit_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader);
            if(SHADER_SUCCESS != ret_shader) {
                ret = app_rslt_convert_shader(ret_shader);
                ERROR_MESSAGE("app_state_dispatch(%s) - lit_mesh_shader_use failed.", app_rslt_to_str(ret));
                goto cleanup;
            }

            ret_shader = lit_mesh_shader_projection_matrix_set(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader, &tmp_projection, true);
            if(SHADER_SUCCESS != ret_shader) {
                ret = app_rslt_convert_shader(ret_shader);
                ERROR_MESSAGE("app_state_dispatch(%s) - lit_mesh_shader_projection_matrix_set failed.", app_rslt_to_str(ret));
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
        ret_camera = camera_view_matrix_get(s_app_state->active_camera, &s_app_state->view_matrix);
        if(CAMERA_SUCCESS != ret_camera) {
                ret = app_rslt_convert_camera(ret_camera);
                ERROR_MESSAGE("app_state_dispatch(%s) - camera_view_matrix_get failed.", app_rslt_to_str(ret));
                goto cleanup;
        }

        ret_shader = ui_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("app_state_dispatch(%s) - ui_mesh_shader_use failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = ui_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->ui_mesh_shader, &s_app_state->view_matrix, true);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("app_state_dispatch(%s) - ui_mesh_shader_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = line_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("app_state_dispatch(%s) - renderer_backend_context failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = line_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->line_mesh_shader, &s_app_state->view_matrix, true);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("app_state_dispatch(%s) - line_mesh_shader_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = point_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("app_state_dispatch(%s) - point_mesh_shader_use failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = point_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->point_mesh_shader, &s_app_state->view_matrix, true);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("app_state_dispatch(%s) - point_mesh_shader_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = lit_mesh_shader_use(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("app_state_dispatch(%s) - lit_mesh_shader_use failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

        ret_shader = lit_mesh_shader_view_matrix_set(s_app_state->renderer_backend_context, s_app_state->lit_mesh_shader, &s_app_state->view_matrix, true);
        if(SHADER_SUCCESS != ret_shader) {
            ret = app_rslt_convert_shader(ret_shader);
            ERROR_MESSAGE("app_state_dispatch(%s) - lit_mesh_shader_view_matrix_set failed.", app_rslt_to_str(ret));
            goto cleanup;
        }

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
static application_result_t point_geometry_create(app_state_t* app_state_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;
    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    point_vertex_t tmp_vertices[8] = { 0 };

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

    tmp_vertices[0].color = vec4u8_initialize(255, 0, 0, 255);
    tmp_vertices[1].color = vec4u8_initialize(255, 255, 0, 255);
    tmp_vertices[2].color = vec4u8_initialize(255, 0, 255, 255);
    tmp_vertices[3].color = vec4u8_initialize(0, 255, 0, 255);
    tmp_vertices[4].color = vec4u8_initialize(255, 255, 0, 255);
    tmp_vertices[5].color = vec4u8_initialize(255, 255, 0, 255);
    tmp_vertices[6].color = vec4u8_initialize(255, 255, 0, 255);
    tmp_vertices[7].color = vec4u8_initialize(255, 255, 0, 255);

    ret_resource_pipeline = point_mesh_geometry_pipeline_import_from_vertices(app_state_->renderer_backend_context, app_state_->point_mesh_shader, app_state_->point_mesh_geometry_registry, "test_points", tmp_vertices, 8, &app_state_->geometry_id_test_points);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ERROR_MESSAGE("point_geometry_create - Failed to import point mesh geometry.");
        goto cleanup;
    }

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}
