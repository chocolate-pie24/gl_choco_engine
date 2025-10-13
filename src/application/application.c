/**
 * @file application.c
 * @author chocolate-pie24
 * @brief 最上位のオーケストレーション。サブシステム初期化、メインループ駆動、終了処理の実装
 *
 * @version 0.1
 * @date 2025-09-20
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 * @license MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdalign.h>
#include <stddef.h> // for NULL
#include <string.h> // for memset

#include "application/application.h"
#include "application/platform_registry.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/core/platform/platform_utils.h"

#include "engine/containers/ring_queue.h"

#include "engine/interfaces/platform_interface.h"

/**
 * @brief アプリケーション内部状態とエンジン各サブシステム状態管理オブジェクトを保持するオブジェクト
 *
 */
typedef struct app_state {
    // application status
    bool window_should_close;
    bool window_resized;
    int window_width;
    int window_height;

    // core/memory/linear_allocator
    size_t linear_alloc_mem_req;
    size_t linear_alloc_align_req;
    size_t linear_alloc_pool_size;
    void* linear_alloc_pool;
    linear_alloc_t* linear_alloc;   /**< リニアアロケータオブジェクト */

    // core/event
    ring_queue_t* window_event_queue;
    ring_queue_t* keyboard_event_queue;
    ring_queue_t* mouse_event_queue;

    // interfaces/platform_interface
    size_t platform_state_memory_requirement;
    size_t platform_state_alignment_requirement;
    platform_state_t* platform_state;
    const platform_vtable_t* platform_vtable;
} app_state_t;

static app_state_t* s_app_state = NULL; /**< アプリケーション内部状態およびエンジン各サブシステム内部状態 */

static const char* const s_err_str_success = "SUCCESS";
static const char* const s_err_str_no_memory = "NO_MEMORY";
static const char* const s_err_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_err_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_err_str_undefined_error = "UNDEFINED_ERROR";

static void on_window(const window_event_t* event_);
static void on_key(const keyboard_event_t* event_);
static void on_mouse(const mouse_event_t* event_);

static void app_state_update(void);
static void app_state_dispatch(void);
static void app_state_clean(void);
static const char* keycode_str(keycode_t keycode_);

static const char* app_err_to_str(app_err_t err_);
static app_err_t mem_err_to_app_err(memory_sys_err_t err_);
static app_err_t linear_alloc_err_to_app_err(linear_alloc_err_t err_);
static app_err_t platform_err_to_app_err(platform_error_t err_);
static app_err_t ring_queue_err_to_app_err(ring_queue_error_t err_);

// TODO: oc_choco_malloc + テスト
app_err_t application_create(void) {
    app_state_t* tmp = NULL;
    void* tmp_platform_state_ptr = NULL;

    app_err_t ret = APPLICATION_RUNTIME_ERROR;
    memory_sys_err_t ret_mem_sys = MEMORY_SYSTEM_INVALID_ARGUMENT;
    linear_alloc_err_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    platform_error_t ret_platform_state_init = PLATFORM_INVALID_ARGUMENT;
    ring_queue_error_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    // Preconditions
    if(NULL != s_app_state) {
        ERROR_MESSAGE("application_create(%s) - Application state is already initialized.", s_err_str_runtime_error);
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    ret_mem_sys = memory_system_create();
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = mem_err_to_app_err(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to create memory system.", app_err_to_str(ret));
        goto cleanup;
    }

    // begin Simulation
    ret_mem_sys = memory_system_allocate(sizeof(*tmp), MEMORY_TAG_SYSTEM, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = mem_err_to_app_err(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate app_state memory.", app_err_to_str(ret));
        goto cleanup;
    }
    memset(tmp, 0, sizeof(*tmp));

    // begin Simulation -> launch all systems.(Don't use s_app_state here.)

    // [NOTE] linear_allocatorのプールサイズについて
    //   全サブシステムのpreinitを先に実行し、リニアアロケータで必要な容量を計算可能だが、
    //   各サブシステムのアライメント要件を考慮すると単純に総和を取れば良いと言うものではなく、ちょっと複雑
    //   当面は実施せず、多めにメモリを確保する方針にする

    // Simulation -> launch all systems -> create linear allocator.(Don't use s_app_state here.)
    INFO_MESSAGE("Starting linear_allocator initialize...");
    tmp->linear_alloc = NULL;
    linear_allocator_preinit(&tmp->linear_alloc_mem_req, &tmp->linear_alloc_align_req);
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&tmp->linear_alloc);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = mem_err_to_app_err(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate linear allocator memory.", app_err_to_str(ret));
        goto cleanup;
    }

    tmp->linear_alloc_pool_size = 1 * KIB;
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &tmp->linear_alloc_pool);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem_sys) {
        ret = mem_err_to_app_err(ret_mem_sys);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate linear allocator pool memory.", app_err_to_str(ret));
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_init(tmp->linear_alloc, tmp->linear_alloc_pool_size, tmp->linear_alloc_pool);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = linear_alloc_err_to_app_err(ret_linear_alloc);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize linear allocator.", app_err_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("linear_allocator initialized successfully.");

    // Simulation -> launch all systems -> create platform state.(Don't use s_app_state here.)
    INFO_MESSAGE("Starting platform_state initialize...");
    tmp->platform_vtable = NULL;
    tmp->platform_vtable = platform_registry_vtable_get(PLATFORM_USE_GLFW); // TODO: #ifdefで切り分け
    if(NULL == tmp->platform_vtable) {
        ERROR_MESSAGE("Failed to get platform vtable.");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }
    tmp->platform_state = NULL;
    tmp->platform_vtable->platform_state_preinit(&tmp->platform_state_memory_requirement, &tmp->platform_state_alignment_requirement);
    ret_linear_alloc = linear_allocator_allocate(tmp->linear_alloc, tmp->platform_state_memory_requirement, tmp->platform_state_alignment_requirement, &tmp_platform_state_ptr);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = linear_alloc_err_to_app_err(ret_linear_alloc);
        ERROR_MESSAGE("application_create(%s) - Failed to allocate platform state memory.", app_err_to_str(ret));
        goto cleanup;
    }
    ret_platform_state_init = tmp->platform_vtable->platform_state_init((platform_state_t*)tmp_platform_state_ptr);
    if(PLATFORM_SUCCESS != ret_platform_state_init) {
        ret = platform_err_to_app_err(ret_platform_state_init);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize platform state.", app_err_to_str(ret));
        goto cleanup;
    }
    tmp->platform_state = (platform_state_t*)tmp_platform_state_ptr;
    INFO_MESSAGE("platform_state initialized successfully.");

    // Simulation -> launch all systems -> create event message queue(window event).(Don't use s_app_state here.)
    INFO_MESSAGE("Starting window event queue initialize...");
    ret_ring_queue = ring_queue_create(8, sizeof(window_event_t), alignof(window_event_t), &tmp->window_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = ring_queue_err_to_app_err(ret_ring_queue);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize window event queue.", app_err_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("window event queue initialized successfully.");

    // Simulation -> launch all systems -> create event message queue(keyboard event).(Don't use s_app_state here.)
    INFO_MESSAGE("Starting keyboard event queue initialize...");
    ret_ring_queue = ring_queue_create(KEY_CODE_MAX, sizeof(keyboard_event_t), alignof(keyboard_event_t), &tmp->keyboard_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = ring_queue_err_to_app_err(ret_ring_queue);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize keyboard event queue.", app_err_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("keyboard event queue initialized successfully.");

    // Simulation -> launch all systems -> create event message queue(mouse event).(Don't use s_app_state here.)
    INFO_MESSAGE("Starting mouse event queue initialize...");
    ret_ring_queue = ring_queue_create(128, sizeof(mouse_event_t), alignof(mouse_event_t), &tmp->mouse_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = ring_queue_err_to_app_err(ret_ring_queue);
        ERROR_MESSAGE("application_create(%s) - Failed to initialize mouse event queue.", app_err_to_str(ret));
        goto cleanup;
    }
    INFO_MESSAGE("keyboard mouse queue initialized successfully.");

    // end Simulation -> launch all systems.

    // end Simulation

    // begin temporary
    // TODO: ウィンドウ生成はレンダラー作成時にそっちに移す
    tmp->window_width = 1024;
    tmp->window_height = 768;
    ret_platform_state_init = tmp->platform_vtable->platform_window_create(tmp->platform_state, "test_window", 1024, 768);
    if(PLATFORM_SUCCESS != ret_platform_state_init) {
        ret = platform_err_to_app_err(ret_platform_state_init);
        ERROR_MESSAGE("application_create(%s) - Failed to create window.", app_err_to_str(ret));
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
            if(NULL != tmp->platform_state) {
                if(NULL != tmp->platform_vtable) {
                    tmp->platform_vtable->platform_state_destroy(tmp->platform_state);
                }
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
    INFO_MESSAGE("starting application_destroy...");
    if(NULL == s_app_state) {
        goto cleanup;
    }

    // begin cleanup all systems.
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
    if(NULL != s_app_state->platform_vtable) {
        s_app_state->platform_vtable->platform_state_destroy(s_app_state->platform_state);
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
    INFO_MESSAGE("All memory freed.");
    memory_system_report();
    memory_system_destroy();
    // end cleanup all systems.

    INFO_MESSAGE("Application destroyed successfully.");
cleanup:
    return;
}

app_err_t application_run(void) {
    app_err_t ret = APPLICATION_SUCCESS;
    if(NULL == s_app_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(%s) - Application is not initialized.", app_err_to_str(ret));
        goto cleanup;
    }
    while(!s_app_state->window_should_close) {
        platform_error_t ret_event = s_app_state->platform_vtable->platform_pump_messages(s_app_state->platform_state, on_window, on_key, on_mouse);
        if(PLATFORM_WINDOW_CLOSE == ret_event) {
            s_app_state->window_should_close = true;
            continue;
        } else if(PLATFORM_SUCCESS != ret_event) {
            ret = platform_err_to_app_err(ret_event);
            WARN_MESSAGE("application_run(%s) - Failed to get events.", app_err_to_str(ret));
            continue;
        }
        app_state_update();
        app_state_dispatch();
        app_state_clean();
        // sleep(1);
        // TODO: platform_sleep
    }
cleanup:
    return ret;
}

static void on_window(const window_event_t* event_) {
    ring_queue_error_t ret_push = RING_QUEUE_INVALID_ARGUMENT;

    if(NULL == event_) {
        WARN_MESSAGE("on_window - Argument event_ requires a valid pointer.");
        goto cleanup;
    }
    if(NULL == s_app_state) {
        WARN_MESSAGE("on_window - Application state is not initialized.");
        goto cleanup;
    }

    ret_push = ring_queue_push(s_app_state->window_event_queue, event_, sizeof(window_event_t), alignof(window_event_t));
    if(RING_QUEUE_SUCCESS != ret_push) {
        app_err_t ret = ring_queue_err_to_app_err(ret_push);
        WARN_MESSAGE("on_window(%s) - Failed to push window event.", app_err_to_str(ret));
        goto cleanup;
    }
cleanup:
    return;
}

static void on_key(const keyboard_event_t* event_) {
    ring_queue_error_t ret_push = RING_QUEUE_INVALID_ARGUMENT;

    if(NULL == event_) {
        WARN_MESSAGE("on_key - Argument event_ requires a valid pointer.");
        goto cleanup;
    }
    if(NULL == s_app_state) {
        WARN_MESSAGE("on_key - Application state is not initialized.");
        goto cleanup;
    }

    ret_push = ring_queue_push(s_app_state->keyboard_event_queue, event_, sizeof(keyboard_event_t), alignof(keyboard_event_t));
    if(RING_QUEUE_SUCCESS != ret_push) {
        app_err_t ret = ring_queue_err_to_app_err(ret_push);
        WARN_MESSAGE("on_key(%s) - Failed to push keyboard event.", app_err_to_str(ret));
        goto cleanup;
    }
cleanup:
    return;
}

static void on_mouse(const mouse_event_t* event_) {
    ring_queue_error_t ret_push = RING_QUEUE_INVALID_ARGUMENT;

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
        app_err_t ret = ring_queue_err_to_app_err(ret_push);
        WARN_MESSAGE("on_key(%s) - Failed to push mouse event.", app_err_to_str(ret));
        goto cleanup;
    }
cleanup:
    return;
}

static void app_state_update(void) {
    app_err_t ret = APPLICATION_INVALID_ARGUMENT;
    if(NULL == s_app_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_state_update(%s) - Application state is not initialized.", app_err_to_str(ret));
        goto cleanup;
    }

    // window events.
    if(NULL == s_app_state->window_event_queue) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_state_update(%s) - window event queue is not initialized.", app_err_to_str(ret));
        goto cleanup;
    }
    while(!ring_queue_empty(s_app_state->window_event_queue)) {
        window_event_t event;
        ring_queue_error_t ret_ring = ring_queue_pop(s_app_state->window_event_queue, &event, sizeof(window_event_t), alignof(window_event_t));
        if(RING_QUEUE_SUCCESS != ret_ring) {
            ret = ring_queue_err_to_app_err(ret_ring);
            WARN_MESSAGE("app_state_update(%s) - Failed to pop window event.", app_err_to_str(ret));
            goto cleanup;
        } else {
            if(WINDOW_EVENT_RESIZE == event.event_code) {
                INFO_MESSAGE("window resized - width / height: [%d, %d] -> [%d, %d]", s_app_state->window_width, s_app_state->window_height, event.window_width, event.window_height);
                s_app_state->window_resized = true;
                s_app_state->window_height = event.window_height;
                s_app_state->window_width = event.window_width;
            }
        }
    }

    // keyboard events.
    if(NULL == s_app_state->keyboard_event_queue) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_state_update(%s) - keyboard event queue is not initialized.", app_err_to_str(ret));
        goto cleanup;
    }
    while(!ring_queue_empty(s_app_state->keyboard_event_queue)) {
        keyboard_event_t event;
        ring_queue_error_t ret_ring = ring_queue_pop(s_app_state->keyboard_event_queue, &event, sizeof(keyboard_event_t), alignof(keyboard_event_t));
        if(RING_QUEUE_SUCCESS != ret_ring) {
            ret = ring_queue_err_to_app_err(ret_ring);
            WARN_MESSAGE("app_state_update(%s) - Failed to pop keyboard event.", app_err_to_str(ret));
            goto cleanup;
        } else {
            INFO_MESSAGE("keyboard event: keycode('%s')  %s", keycode_str(event.key), (event.pressed) ? "pressed" : "released");
        }
    }
    // mouse events.
    if(NULL == s_app_state->mouse_event_queue) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("app_state_update(%s) - mouse event queue is not initialized.", app_err_to_str(ret));
        goto cleanup;
    }
    while(!ring_queue_empty(s_app_state->mouse_event_queue)) {
        mouse_event_t event;
        ring_queue_error_t ret_ring = ring_queue_pop(s_app_state->mouse_event_queue, &event, sizeof(mouse_event_t), alignof(mouse_event_t));
        if(RING_QUEUE_SUCCESS != ret_ring) {
            ret = ring_queue_err_to_app_err(ret_ring);
            WARN_MESSAGE("app_state_update(%s) - Failed to pop mouse event.", app_err_to_str(ret));
            goto cleanup;
        } else {
            if(MOUSE_BUTTON_LEFT == event.button) {
                INFO_MESSAGE("mouse event: button('left')  %s pos %d %d", (event.pressed) ? "pressed" : "released", event.x, event.y);
            } else if(MOUSE_BUTTON_RIGHT == event.button) {
                INFO_MESSAGE("mouse event: button('right')  %s pos %d %d", (event.pressed) ? "pressed" : "released", event.x, event.y);
            }
        }
    }
cleanup:
    return;
}

static void app_state_dispatch(void) {
    // 各サブシステムへイベントを通知 まだ処理はなし
}

static void app_state_clean(void) {
    if(NULL == s_app_state) {
        ERROR_MESSAGE("app_state_clean(%s) - Application state is not initialized.", app_err_to_str(APPLICATION_RUNTIME_ERROR));
        goto cleanup;
    }
    s_app_state->window_resized = false;
    s_app_state->window_should_close = false;
cleanup:
    return;
}

static const char* keycode_str(keycode_t keycode_) {
    static const char* s_key_1 = "key: '1'";
    static const char* s_key_2 = "key: '2'";
    static const char* s_key_3 = "key: '3'";
    static const char* s_key_4 = "key: '4'";
    static const char* s_key_5 = "key: '5'";
    static const char* s_key_6 = "key: '6'";
    static const char* s_key_7 = "key: '7'";
    static const char* s_key_8 = "key: '8'";
    static const char* s_key_9 = "key: '9'";
    static const char* s_key_0 = "key: '0'";
    static const char* s_key_a = "key: 'a'";
    static const char* s_key_b = "key: 'b'";
    static const char* s_key_c = "key: 'c'";
    static const char* s_key_d = "key: 'd'";
    static const char* s_key_e = "key: 'e'";
    static const char* s_key_f = "key: 'f'";
    static const char* s_key_g = "key: 'g'";
    static const char* s_key_h = "key: 'h'";
    static const char* s_key_i = "key: 'i'";
    static const char* s_key_j = "key: 'j'";
    static const char* s_key_k = "key: 'k'";
    static const char* s_key_l = "key: 'l'";
    static const char* s_key_m = "key: 'm'";
    static const char* s_key_n = "key: 'n'";
    static const char* s_key_o = "key: 'o'";
    static const char* s_key_p = "key: 'p'";
    static const char* s_key_q = "key: 'q'";
    static const char* s_key_r = "key: 'r'";
    static const char* s_key_s = "key: 's'";
    static const char* s_key_t = "key: 't'";
    static const char* s_key_u = "key: 'u'";
    static const char* s_key_v = "key: 'v'";
    static const char* s_key_w = "key: 'w'";
    static const char* s_key_x = "key: 'x'";
    static const char* s_key_y = "key: 'y'";
    static const char* s_key_z = "key: 'z'";
    static const char* s_key_right = "key: 'right'";
    static const char* s_key_left = "key: 'left'";
    static const char* s_key_up = "key: 'up'";
    static const char* s_key_down = "key: 'down'";
    static const char* s_key_shift = "key: 'shift'";
    static const char* s_key_space = "key: 'space'";
    static const char* s_key_semicolon = "key: 'semicolon'";
    static const char* s_key_minus = "key: 'minus'";
    static const char* s_key_f1 = "key: 'f1'";
    static const char* s_key_f2 = "key: 'f2'";
    static const char* s_key_f3 = "key: 'f3'";
    static const char* s_key_f4 = "key: 'f4'";
    static const char* s_key_f5 = "key: 'f5'";
    static const char* s_key_f6 = "key: 'f6'";
    static const char* s_key_f7 = "key: 'f7'";
    static const char* s_key_f8 = "key: 'f8'";
    static const char* s_key_f9 = "key: 'f9'";
    static const char* s_key_f10 = "key: 'f10'";
    static const char* s_key_f11 = "key: 'f11'";
    static const char* s_key_f12 = "key: 'f12'";
    static const char* s_key_undefined = "key: 'undefined'";

    switch(keycode_) {
    case KEY_1:
        return s_key_1;
    case KEY_2:
        return s_key_2;
    case KEY_3:
        return s_key_3;
    case KEY_4:
        return s_key_4;
    case KEY_5:
        return s_key_5;
    case KEY_6:
        return s_key_6;
    case KEY_7:
        return s_key_7;
    case KEY_8:
        return s_key_8;
    case KEY_9:
        return s_key_9;
    case KEY_0:
        return s_key_0;
    case KEY_A:
        return s_key_a;
    case KEY_B:
        return s_key_b;
    case KEY_C:
        return s_key_c;
    case KEY_D:
        return s_key_d;
    case KEY_E:
        return s_key_e;
    case KEY_F:
        return s_key_f;
    case KEY_G:
        return s_key_g;
    case KEY_H:
        return s_key_h;
    case KEY_I:
        return s_key_i;
    case KEY_J:
        return s_key_j;
    case KEY_K:
        return s_key_k;
    case KEY_L:
        return s_key_l;
    case KEY_M:
        return s_key_m;
    case KEY_N:
        return s_key_n;
    case KEY_O:
        return s_key_o;
    case KEY_P:
        return s_key_p;
    case KEY_Q:
        return s_key_q;
    case KEY_R:
        return s_key_r;
    case KEY_S:
        return s_key_s;
    case KEY_T:
        return s_key_t;
    case KEY_U:
        return s_key_u;
    case KEY_V:
        return s_key_v;
    case KEY_W:
        return s_key_w;
    case KEY_X:
        return s_key_x;
    case KEY_Y:
        return s_key_y;
    case KEY_Z:
        return s_key_z;
    case KEY_RIGHT:
        return s_key_right;
    case KEY_LEFT:
        return s_key_left;
    case KEY_UP:
        return s_key_up;
    case KEY_DN:
        return s_key_down;
    case KEY_SHIFT:
        return s_key_shift;
    case KEY_SPACE:
        return s_key_space;
    case KEY_SEMICOLON:
        return s_key_semicolon;
    case KEY_MINUS:
        return s_key_minus;
    case KEY_F1:
        return s_key_f1;
    case KEY_F2:
        return s_key_f2;
    case KEY_F3:
        return s_key_f3;
    case KEY_F4:
        return s_key_f4;
    case KEY_F5:
        return s_key_f5;
    case KEY_F6:
        return s_key_f6;
    case KEY_F7:
        return s_key_f7;
    case KEY_F8:
        return s_key_f8;
    case KEY_F9:
        return s_key_f9;
    case KEY_F10:
        return s_key_f10;
    case KEY_F11:
        return s_key_f11;
    case KEY_F12:
        return s_key_f12;
    default:
        return s_key_undefined;
    }
}

static const char* app_err_to_str(app_err_t err_) {
    switch(err_) {
    case APPLICATION_SUCCESS:
        return s_err_str_success;
    case APPLICATION_NO_MEMORY:
        return s_err_str_no_memory;
    case APPLICATION_RUNTIME_ERROR:
        return s_err_str_runtime_error;
    case APPLICATION_INVALID_ARGUMENT:
        return s_err_str_invalid_argument;
    case APPLICATION_UNDEFINED_ERROR:
        return s_err_str_undefined_error;
    default:
        return s_err_str_undefined_error;
    }
}

static app_err_t mem_err_to_app_err(memory_sys_err_t err_) {
    switch(err_) {
    case MEMORY_SYSTEM_SUCCESS:
        return APPLICATION_SUCCESS;
    case MEMORY_SYSTEM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case MEMORY_SYSTEM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case MEMORY_SYSTEM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

static app_err_t linear_alloc_err_to_app_err(linear_alloc_err_t err_) {
    switch(err_) {
    case LINEAR_ALLOC_SUCCESS:
        return APPLICATION_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

static app_err_t platform_err_to_app_err(platform_error_t err_) {
    switch(err_) {
    case PLATFORM_SUCCESS:
        return APPLICATION_SUCCESS;
    case PLATFORM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case PLATFORM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case PLATFORM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case PLATFORM_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case PLATFORM_WINDOW_CLOSE:
        return APPLICATION_SUCCESS; // これはエラーではないので、成功扱いにする
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

static app_err_t ring_queue_err_to_app_err(ring_queue_error_t err_) {
    switch(err_) {
    case RING_QUEUE_SUCCESS:
        return APPLICATION_SUCCESS;
    case RING_QUEUE_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RING_QUEUE_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RING_QUEUE_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RING_QUEUE_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case RING_QUEUE_EMPTY:
        return APPLICATION_RUNTIME_ERROR;   // リングキュー空はRuntime errorに変換
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}
