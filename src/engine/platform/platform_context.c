/** @ingroup platform_context
 *
 * @file platform_context.c
 * @author chocolate-pie24
 * @brief プラットフォームシステムのStrategy Contextモジュールの実装
 *
 * @version 0.1
 * @date 2025-10-14
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdbool.h>
#include <stdalign.h>
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/platform/platform_context.h"
#include "engine/platform/platform_interface.h"
#include "engine/platform/platform_concretes/platform_glfw.h"
#include "engine/platform/platform_core/platform_types.h"
#include "engine/platform/platform_core/platform_err_utils.h"

// #define TEST_BUILD

#ifdef TEST_BUILD
// テスト時のみ使用するヘッダのinclude
#include <assert.h>
#include <stdlib.h> // for malloc

#include "test_controller.h"

#include "engine/base/choco_macros.h"

#include "engine/platform/test_platform_context.h"

// platform_context用モジュール専用テスト制御構造体定義

// テスト用リニアアロケータ初期化処理/終了処理
static void test_linear_allocator_create(linear_alloc_t** allocator_, void** out_memory_pool_, size_t pool_size_);
static void test_linear_allocator_destroy(linear_alloc_t** allocator_, void** memory_pool_);

// テスト時に使用するvtable関数(GLFW等、プラットフォーム依存コードは禁止し、platform_context単体でテスト可能にする)
static void test_vtable_preinit(size_t* memory_requirement_, size_t* alignment_requirement_);
static platform_result_t test_vtable_init(platform_backend_t* platform_backend_);
static void test_vtable_destroy(platform_backend_t* platform_backend_);
static platform_result_t test_vtable_window_create(platform_backend_t* platform_backend_, const char* window_label_, int window_width_, int window_height_, int* framebuffer_width_, int* framebuffer_height_);
static platform_result_t test_vtable_pump_messages(platform_backend_t* platform_backend_, void (*window_event_callback)(const window_event_t* event_), void (*keyboard_event_callback)(const keyboard_event_t* event_), void (*mouse_event_callback)(const mouse_event_t* event_));
static platform_result_t test_vtable_swap_buffers(platform_backend_t* platform_backend_);

// イベント処理関数用ダミーイベント処理関数
static void dummy_window_event_callback(const window_event_t* event_);
static void dummy_keyboard_event_callback(const keyboard_event_t* event_);
static void dummy_mouse_event_callback(const mouse_event_t* event_);

static const platform_vtable_t s_test_vtable = {
    .platform_backend_preinit = test_vtable_preinit,
    .platform_backend_init = test_vtable_init,
    .platform_backend_destroy = test_vtable_destroy,
    .platform_backend_window_create = test_vtable_window_create,
    .platform_backend_pump_messages = test_vtable_pump_messages,
    .platform_backend_swap_buffers = test_vtable_swap_buffers,
};  /**< テスト専用vtable */

/**
 * @brief platform_vtable_get()に強制的に返させるvtable選択リスト
 *
 */
typedef enum test_vtable_list {
    TEST_PLATFORM_VTABLE_NULL,  /**< platform_vtable_get()の返り値: NULL */
    TEST_PLATFORM_VTABLE_TEST,  /**< platform_vtable_get()の返り値: s_test_vtable */
    TEST_PLATFORM_VTABLE_GLFW   /**< platform_vtable_get()の返り値: platform_glfw_vtable_get()の返り値 */
} test_vtable_list_t;

/**
 * @brief platform_vtable_get()の返り値制御用コンフィグレーション構造体
 *
 */
typedef struct test_vtable_config {
    bool enable_test_vtable;            /**< テスト用vtable差し替え有効/無効フラグ(true: 有効, false: 無効) */
    test_vtable_list_t vtable_select;   /**< テスト用に差し替えるvtable選択値 */
} test_vtable_config_t;

// 外部公開APIテスト設定
static test_call_control_t s_test_config_platform_initialize;       /**< platform_initialize()テスト設定 */
static test_call_control_t s_test_config_platform_window_create;    /**< platform_window_create()テスト設定 */
static test_call_control_t s_test_config_platform_pump_messages;    /**< platform_pump_messages()テスト設定 */
static test_call_control_t s_test_config_platform_swap_buffers;     /**< platform_swap_buffers()テスト設定 */

// プライベート関数テスト設定
static test_vtable_config_t s_test_config_platform_vtable_config;             /**< テスト用vtable選択設定 */
static test_call_control_bool_t s_test_config_platform_type_valid_check;      /**< platform_type_valid_check()テスト設定 */
static test_call_control_t s_test_config_test_vtable_init;                    /**< test_vtable_init()テスト設定 */
static test_call_control_t s_test_config_test_vtable_window_create;           /**< test_vtable_window_create()テスト設定 */
static test_call_control_t s_test_config_test_vtable_pump_messages;           /**< test_vtable_pump_messages()テスト設定 */
static test_call_control_t s_test_config_test_vtable_swap_buffers;            /**< test_vtable_swap_buffers()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_platform_initialize(void);
static void test_platform_destroy(void);
static void test_platform_window_create(void);
static void test_platform_pump_messages(void);
static void test_platform_swap_buffers(void);
static void test_platform_vtable_get(void);
static void test_platform_type_valid_check(void);
#endif

/**
 * @brief プラットフォームコンテキスト構造体
 *
 */
struct platform_context {
    platform_type_t type;               /**< プラットフォームタイプ */
    platform_backend_t* backend;        /**< 各プラットフォーム固有実装バックエンドデータ */
    const platform_vtable_t* vtable;    /**< 各プラットフォーム仮想関数テーブル */
};

static const platform_vtable_t* platform_vtable_get(platform_type_t platform_type_);

static bool platform_type_valid_check(platform_type_t platform_type_);

platform_result_t platform_initialize(linear_alloc_t* allocator_, platform_type_t platform_type_, platform_context_t** out_platform_context_) {
#ifdef TEST_BUILD
    s_test_config_platform_initialize.call_count++;
    if(s_test_config_platform_initialize.fail_on_call != 0) {
        if(s_test_config_platform_initialize.call_count == s_test_config_platform_initialize.fail_on_call) {
            return (platform_result_t)s_test_config_platform_initialize.forced_result;
        }
    }
#endif
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    void* backend_ptr = NULL;
    size_t backend_memory_req = 0;
    size_t backend_align_req = 0;
    platform_context_t* tmp_context = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_platform_context_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "out_platform_context_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_platform_context_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "*out_platform_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(platform_type_valid_check(platform_type_), ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "platform_type_")

    // Simulation.
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(platform_context_t), alignof(platform_context_t), (void**)&tmp_context);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = platform_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("platform_initialize(%s) - Failed to allocate memory for platform context.", platform_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_context, 0, sizeof(platform_context_t));

    tmp_context->vtable = platform_vtable_get(platform_type_);
    if(NULL == tmp_context->vtable) {
        ret = PLATFORM_RUNTIME_ERROR;
        ERROR_MESSAGE("platform_initialize(%s) - Failed to get platform vtable.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_context->vtable->platform_backend_preinit(&backend_memory_req, &backend_align_req);
    ret_linear_alloc = linear_allocator_allocate(allocator_, backend_memory_req, backend_align_req, &backend_ptr);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = platform_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("platform_initialize(%s) - Failed to allocate memory for platform backend.", platform_rslt_to_str(ret));
        goto cleanup;
    }
    memset(backend_ptr, 0, backend_memory_req);

    ret = tmp_context->vtable->platform_backend_init((platform_backend_t*)backend_ptr);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_initialize(%s) - Failed to initialize platform backend.", platform_rslt_to_str(ret));
        goto cleanup;
    }
    tmp_context->backend = (platform_backend_t*)backend_ptr;
    tmp_context->type = platform_type_;

    // commit.
    *out_platform_context_ = tmp_context;

    ret = PLATFORM_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void platform_destroy(platform_context_t* platform_context_) {
    if(NULL == platform_context_) {
        goto cleanup;
    }
    if(NULL == platform_context_->vtable) {
        goto cleanup;
    }
    if(NULL == platform_context_->backend) {
        goto cleanup;
    }
    platform_context_->vtable->platform_backend_destroy(platform_context_->backend);
cleanup:
    return;
}

platform_result_t platform_window_create(platform_context_t* platform_context_, const char* window_label_, int window_width_, int window_height_, int* framebuffer_width_, int* framebuffer_height_) {
#ifdef TEST_BUILD
    s_test_config_platform_window_create.call_count++;
    if(s_test_config_platform_window_create.fail_on_call != 0) {
        if(s_test_config_platform_window_create.call_count == s_test_config_platform_window_create.fail_on_call) {
            return (platform_result_t)s_test_config_platform_window_create.forced_result;
        }
    }
#endif
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(platform_context_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_window_create", "platform_context_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_window_create", "platform_context_->vtable")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_window_create", "platform_context_->backend")
    IF_ARG_NULL_GOTO_CLEANUP(window_label_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_window_create", "window_label_")
    IF_ARG_NULL_GOTO_CLEANUP(framebuffer_width_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_window_create", "framebuffer_width_")
    IF_ARG_NULL_GOTO_CLEANUP(framebuffer_height_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_window_create", "framebuffer_height_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < window_width_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_window_create", "window_width_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < window_height_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_window_create", "window_height_")

    ret = platform_context_->vtable->platform_backend_window_create(platform_context_->backend, window_label_, window_width_, window_height_, framebuffer_width_, framebuffer_height_);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_window_create(%s) - Failed to create window.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}

platform_result_t platform_pump_messages(
    platform_context_t* platform_context_,
    void (*window_event_callback)(const window_event_t* event_),
    void (*keyboard_event_callback)(const keyboard_event_t* event_),
    void (*mouse_event_callback)(const mouse_event_t* event_)) {
#ifdef TEST_BUILD
    s_test_config_platform_pump_messages.call_count++;
    if(s_test_config_platform_pump_messages.fail_on_call != 0) {
        if(s_test_config_platform_pump_messages.call_count == s_test_config_platform_pump_messages.fail_on_call) {
            return (platform_result_t)s_test_config_platform_pump_messages.forced_result;
        }
    }
#endif
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(platform_context_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_pump_messages", "platform_context_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_pump_messages", "platform_context_->vtable")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_pump_messages", "platform_context_->backend")
    IF_ARG_NULL_GOTO_CLEANUP(window_event_callback, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_pump_messages", "window_event_callback")
    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_callback, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_pump_messages", "keyboard_event_callback")
    IF_ARG_NULL_GOTO_CLEANUP(mouse_event_callback, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_pump_messages", "mouse_event_callback")

    ret = platform_context_->vtable->platform_backend_pump_messages(platform_context_->backend, window_event_callback, keyboard_event_callback, mouse_event_callback);
    // PLATFORM_WINDOW_CLOSEはPLATFORM_SUCCESS以外でも正常なので無視
    if(PLATFORM_SUCCESS != ret && PLATFORM_WINDOW_CLOSE != ret) {
        ERROR_MESSAGE("platform_pump_messages(%s) - Failed to pump messages.", platform_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

platform_result_t platform_swap_buffers(platform_context_t* platform_context_) {
#ifdef TEST_BUILD
    s_test_config_platform_swap_buffers.call_count++;
    if(s_test_config_platform_swap_buffers.fail_on_call != 0) {
        if(s_test_config_platform_swap_buffers.call_count == s_test_config_platform_swap_buffers.fail_on_call) {
            return (platform_result_t)s_test_config_platform_swap_buffers.forced_result;
        }
    }
#endif
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(platform_context_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_swap_buffers", "platform_context_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, ret, PLATFORM_BAD_OPERATION, platform_rslt_to_str(PLATFORM_BAD_OPERATION), "platform_swap_buffers", "platform_context_->vtable")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, ret, PLATFORM_BAD_OPERATION, platform_rslt_to_str(PLATFORM_BAD_OPERATION), "platform_swap_buffers", "platform_context_->backend")

    ret = platform_context_->vtable->platform_backend_swap_buffers(platform_context_->backend);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_swap_buffers(%s) - Failed to swap buffers.", platform_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

/**
 * @brief プラットフォーム(x11, win32, glfw...)の差異を吸収するため、プラットフォームに応じた仮想関数テーブル取得処理
 *
 * 使用例:
 * @code{.c}
 * const platform_vtable_t vtable = platform_registry_vtable_get(PLATFORM_USE_GLFW);    // GLFWを使用したテーブルを取得
 * @endcode
 *
 * @param[in] platform_type_ 仮想関数テーブルを取得するプラットフォーム種別
 * @return const platform_vtable_t* 仮想関数テーブル(引数で指定したプラットフォームが見つからない場合はNULL)
 */
static const platform_vtable_t* platform_vtable_get(platform_type_t platform_type_) {
#ifdef TEST_BUILD
    if(s_test_config_platform_vtable_config.enable_test_vtable) {
        if(TEST_PLATFORM_VTABLE_NULL == s_test_config_platform_vtable_config.vtable_select) {
            return NULL;
        } else if(TEST_PLATFORM_VTABLE_TEST == s_test_config_platform_vtable_config.vtable_select) {
            return &s_test_vtable;
        } else if(TEST_PLATFORM_VTABLE_GLFW == s_test_config_platform_vtable_config.vtable_select) {
            return platform_glfw_vtable_get();
        }
    }
#endif

    switch (platform_type_) {
    case PLATFORM_USE_GLFW:
        return platform_glfw_vtable_get();
    default:
        return NULL;
    }
}

static bool platform_type_valid_check(platform_type_t platform_type_) {
#ifdef TEST_BUILD
    s_test_config_platform_type_valid_check.call_count++;
    if(s_test_config_platform_type_valid_check.fail_on_call != 0) {
        if(s_test_config_platform_type_valid_check.call_count == s_test_config_platform_type_valid_check.fail_on_call) {
            return s_test_config_platform_type_valid_check.forced_result;
        }
    }
#endif
    switch(platform_type_) {
    case PLATFORM_USE_GLFW:
        return true;
    default:
        return false;
    }
}

#ifdef TEST_BUILD
void NO_COVERAGE test_platform_initialize_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_platform_initialize.fail_on_call = config_->fail_on_call;
    s_test_config_platform_initialize.forced_result = config_->forced_result;
}

void NO_COVERAGE test_platform_window_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_platform_window_create.fail_on_call = config_->fail_on_call;
    s_test_config_platform_window_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_platform_pump_messages_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_platform_pump_messages.fail_on_call = config_->fail_on_call;
    s_test_config_platform_pump_messages.forced_result = config_->forced_result;
}

void NO_COVERAGE test_platform_swap_buffers_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_platform_swap_buffers.fail_on_call = config_->fail_on_call;
    s_test_config_platform_swap_buffers.forced_result = config_->forced_result;
}

void NO_COVERAGE test_platform_context_config_reset(void) {
    test_call_control_reset(&s_test_config_platform_initialize);
    test_call_control_reset(&s_test_config_platform_pump_messages);
    test_call_control_reset(&s_test_config_platform_swap_buffers);
    test_call_control_reset(&s_test_config_platform_window_create);

    test_call_control_bool_reset(&s_test_config_platform_type_valid_check);

    test_call_control_reset(&s_test_config_test_vtable_init);
    test_call_control_reset(&s_test_config_test_vtable_window_create);
    test_call_control_reset(&s_test_config_test_vtable_pump_messages);
    test_call_control_reset(&s_test_config_test_vtable_swap_buffers);

    s_test_config_platform_vtable_config.enable_test_vtable = false;
    s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_GLFW;
}

void test_platform_context(void) {
    test_platform_context_config_reset();

    test_platform_initialize();
    test_platform_destroy();
    test_platform_window_create();
    test_platform_pump_messages();
    test_platform_swap_buffers();
    test_platform_vtable_get();
    test_platform_type_valid_check();

    test_platform_context_config_reset();
}

static void NO_COVERAGE test_linear_allocator_create(linear_alloc_t** allocator_, void** out_memory_pool_, size_t pool_size_) {
    assert(NULL == *allocator_);
    assert(NULL == *out_memory_pool_);
    assert(0 != pool_size_);

    size_t mem_req = 0;
    size_t align_req = 0;
    linear_allocator_preinit(&mem_req, &align_req);

    *allocator_ = (linear_alloc_t*)malloc(mem_req);
    assert(NULL != *allocator_);

    *out_memory_pool_ = malloc(pool_size_);
    assert(NULL != *out_memory_pool_);

    assert(LINEAR_ALLOC_SUCCESS == linear_allocator_init(*allocator_, pool_size_, *out_memory_pool_));
}

static void NO_COVERAGE test_linear_allocator_destroy(linear_alloc_t** allocator_, void** memory_pool_) {
    if(NULL != *allocator_) {
        free(*allocator_);
        *allocator_ = NULL;
    }
    if(NULL != *memory_pool_) {
        free(*memory_pool_);
        *memory_pool_ = NULL;
    }
}

static void NO_COVERAGE test_vtable_preinit(size_t* memory_requirement_, size_t* alignment_requirement_) {
    // 仮
    *memory_requirement_ = 8;
    *alignment_requirement_ = 8;
}

static platform_result_t NO_COVERAGE test_vtable_init(platform_backend_t* platform_backend_) {
    (void)platform_backend_;
    return (platform_result_t)s_test_config_test_vtable_init.forced_result;
}

static void NO_COVERAGE test_vtable_destroy(platform_backend_t* platform_backend_) {
    (void)platform_backend_;
    // 現状では何もしない
    return;
}

static platform_result_t NO_COVERAGE test_vtable_window_create(platform_backend_t* platform_backend_, const char* window_label_, int window_width_, int window_height_, int* framebuffer_width_, int* framebuffer_height_) {
    (void)platform_backend_;
    (void)window_label_;
    (void)window_width_;
    (void)window_height_;
    (void)framebuffer_width_;
    (void)framebuffer_height_;
    return (platform_result_t)s_test_config_test_vtable_window_create.forced_result;
}

static platform_result_t NO_COVERAGE test_vtable_pump_messages(platform_backend_t* platform_backend_, void (*window_event_callback)(const window_event_t* event_), void (*keyboard_event_callback)(const keyboard_event_t* event_), void (*mouse_event_callback)(const mouse_event_t* event_)) {
    (void)platform_backend_;
    (void)window_event_callback;
    (void)keyboard_event_callback;
    (void)mouse_event_callback;
    return (platform_result_t)s_test_config_test_vtable_pump_messages.forced_result;
}

static platform_result_t NO_COVERAGE test_vtable_swap_buffers(platform_backend_t* platform_backend_) {
    (void)platform_backend_;
    return (platform_result_t)s_test_config_test_vtable_swap_buffers.forced_result;
}

static void NO_COVERAGE dummy_window_event_callback(const window_event_t* event_) {
    (void)event_;
    return;
}

static void NO_COVERAGE dummy_keyboard_event_callback(const keyboard_event_t* event_) {
    (void)event_;
    return;
}

static void NO_COVERAGE dummy_mouse_event_callback(const mouse_event_t* event_) {
    (void)event_;
    return;
}

// Generated by ChatGPT
static void NO_COVERAGE test_platform_initialize(void) {
    test_platform_context_config_reset();

    {
        // テスト基盤による強制失敗（前提条件チェックより前）
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        test_call_control_t config = {0};

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;
        platform_context_t* context = NULL;

        test_linear_allocator_create(&allocator, &memory_pool, 128U);

        config.fail_on_call = 1U;
        config.forced_result = (int)PLATFORM_RUNTIME_ERROR;
        test_platform_initialize_config_set(&config);

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &context);
        assert(PLATFORM_RUNTIME_ERROR == ret);
        assert(NULL == context);
        assert(1U == s_test_config_platform_initialize.call_count);

        test_platform_context_config_reset();
        test_linear_allocator_destroy(&allocator, &memory_pool);
    }

    {
        // allocator_ == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t* context = NULL;

        test_platform_context_config_reset();

        ret = platform_initialize(NULL, PLATFORM_USE_GLFW, &context);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
        assert(NULL == context);
    }

    {
        // out_platform_context_ == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;

        test_linear_allocator_create(&allocator, &memory_pool, 128U);
        test_platform_context_config_reset();

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, NULL);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }

    {
        // *out_platform_context_ != NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t dummy_context = {0};
        platform_context_t* context = &dummy_context;

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;

        test_linear_allocator_create(&allocator, &memory_pool, 128U);
        test_platform_context_config_reset();

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &context);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
        assert(&dummy_context == context);

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }

    {
        // platform_type_ が無効値
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t* context = NULL;

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;

        test_linear_allocator_create(&allocator, &memory_pool, 128U);
        test_platform_context_config_reset();

        ret = platform_initialize(allocator, (platform_type_t)100, &context);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
        assert(NULL == context);

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }

    {
        // platform context 用メモリ確保失敗
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t* context = NULL;
        const size_t pool_size = (sizeof(platform_context_t) > 1U) ? (sizeof(platform_context_t) - 1U) : 1U;

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;

        test_linear_allocator_create(&allocator, &memory_pool, pool_size);
        test_platform_context_config_reset();

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &context);
        assert(PLATFORM_NO_MEMORY == ret);
        assert(NULL == context);

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }

    {
        // vtable 取得失敗
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t* context = NULL;

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;

        test_linear_allocator_create(&allocator, &memory_pool, sizeof(platform_context_t) + 16U);
        test_platform_context_config_reset();

        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_NULL;

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &context);
        assert(PLATFORM_RUNTIME_ERROR == ret);
        assert(NULL == context);

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }

    {
        // backend 用メモリ確保失敗
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t* context = NULL;
        const size_t pool_size = sizeof(platform_context_t);

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;

        test_linear_allocator_create(&allocator, &memory_pool, pool_size);
        test_platform_context_config_reset();

        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_TEST;
        s_test_config_test_vtable_init.forced_result = PLATFORM_SUCCESS;

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &context);
        assert(PLATFORM_NO_MEMORY == ret);
        assert(NULL == context);

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }

    {
        // backend 初期化失敗
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t* context = NULL;

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;

        test_linear_allocator_create(&allocator, &memory_pool, sizeof(platform_context_t) + 64U);
        test_platform_context_config_reset();

        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_TEST;
        s_test_config_test_vtable_init.forced_result = PLATFORM_RUNTIME_ERROR;

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &context);
        assert(PLATFORM_RUNTIME_ERROR == ret);
        assert(NULL == context);

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }

    {
        // 正常系（test vtable 使用）
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t* context = NULL;

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;

        test_linear_allocator_create(&allocator, &memory_pool, sizeof(platform_context_t) + 64U);
        test_platform_context_config_reset();

        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_TEST;
        s_test_config_test_vtable_init.forced_result = PLATFORM_SUCCESS;

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &context);
        assert(PLATFORM_SUCCESS == ret);
        assert(NULL != context);
        assert(PLATFORM_USE_GLFW == context->type);
        assert(&s_test_vtable == context->vtable);
        assert(NULL != context->backend);

        platform_destroy(context);
        context = NULL;

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }

    test_platform_context_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_platform_destroy(void) {
    test_platform_context_config_reset();

    {
        // platform_context_ == NULL
        platform_destroy(NULL);
    }

    {
        // platform_context_->vtable == NULL
        platform_context_t context;
        memset(&context, 0, sizeof(platform_context_t));

        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)0x1;
        context.vtable = NULL;

        platform_destroy(&context);
    }

    {
        // platform_context_->backend == NULL
        platform_context_t context;
        memset(&context, 0, sizeof(platform_context_t));

        context.type = PLATFORM_USE_GLFW;
        context.backend = NULL;
        context.vtable = &s_test_vtable;

        platform_destroy(&context);
    }

    {
        // 正常系
        // 現状は test_vtable_destroy() に副作用や call_count がないため、
        // 自動assertではなく、ステップ実行で
        // platform_context_->vtable->platform_backend_destroy(platform_context_->backend)
        // に到達することを確認する前提
        platform_context_t context;
        uint8_t dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));

        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        platform_destroy(&context);
    }

    test_platform_context_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_platform_window_create(void) {
    test_platform_context_config_reset();

    {
        // テスト基盤による強制失敗（前提条件チェックより前）
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        test_call_control_t config = {0};
        int framebuffer_width = 111;
        int framebuffer_height = 222;

        config.fail_on_call = 1U;
        config.forced_result = (int)PLATFORM_RUNTIME_ERROR;
        test_platform_window_create_config_set(&config);

        ret = platform_window_create(NULL, "test_window", 640, 480, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_platform_window_create.call_count);
        assert(111 == framebuffer_width);
        assert(222 == framebuffer_height);

        test_platform_context_config_reset();
    }

    {
        // platform_context_ == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        int framebuffer_width = 0;
        int framebuffer_height = 0;

        test_platform_context_config_reset();

        ret = platform_window_create(NULL, "test_window", 640, 480, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // platform_context_->vtable == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_width = 0;
        int framebuffer_height = 0;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = NULL;

        test_platform_context_config_reset();

        ret = platform_window_create(&context, "test_window", 640, 480, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // platform_context_->backend == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        int framebuffer_width = 0;
        int framebuffer_height = 0;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = NULL;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_window_create(&context, "test_window", 640, 480, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // window_label_ == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_width = 0;
        int framebuffer_height = 0;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_window_create(&context, NULL, 640, 480, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // framebuffer_width_ == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_height = 0;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_window_create(&context, "test_window", 640, 480, NULL, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // framebuffer_height_ == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_width = 0;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_window_create(&context, "test_window", 640, 480, &framebuffer_width, NULL);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // window_width_ == 0 -> INVALID_ARGUMENT
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_width = 0;
        int framebuffer_height = 0;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_window_create(&context, "test_window", 0, 480, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // window_width_ < 0 -> INVALID_ARGUMENT
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_width = 0;
        int framebuffer_height = 0;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_window_create(&context, "test_window", -1, 480, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // window_height_ == 0 -> INVALID_ARGUMENT
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_width = 0;
        int framebuffer_height = 0;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_window_create(&context, "test_window", 640, 0, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // window_height_ < 0 -> INVALID_ARGUMENT
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_width = 0;
        int framebuffer_height = 0;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_window_create(&context, "test_window", 640, -1, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // vtable委譲先が失敗を返す
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_width = 333;
        int framebuffer_height = 444;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();
        s_test_config_test_vtable_window_create.forced_result = PLATFORM_RUNTIME_ERROR;

        ret = platform_window_create(&context, "test_window", 640, 480, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_RUNTIME_ERROR == ret);
        assert(333 == framebuffer_width);
        assert(444 == framebuffer_height);
    }

    {
        // 正常系
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};
        int framebuffer_width = 555;
        int framebuffer_height = 666;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();
        s_test_config_test_vtable_window_create.forced_result = PLATFORM_SUCCESS;

        ret = platform_window_create(&context, "test_window", 640, 480, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_SUCCESS == ret);
        assert(555 == framebuffer_width);
        assert(666 == framebuffer_height);
    }

    test_platform_context_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_platform_pump_messages(void) {
    test_platform_context_config_reset();

    {
        // テスト基盤による強制失敗（前提条件チェックより前）
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        test_call_control_t config = {0};

        config.fail_on_call = 1U;
        config.forced_result = (int)PLATFORM_RUNTIME_ERROR;
        test_platform_pump_messages_config_set(&config);

        ret = platform_pump_messages(NULL, dummy_window_event_callback, dummy_keyboard_event_callback, dummy_mouse_event_callback);
        assert(PLATFORM_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_platform_pump_messages.call_count);

        test_platform_context_config_reset();
    }

    {
        // platform_context_ == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

        test_platform_context_config_reset();

        ret = platform_pump_messages(NULL, dummy_window_event_callback, dummy_keyboard_event_callback, dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // platform_context_->vtable == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = NULL;

        test_platform_context_config_reset();

        ret = platform_pump_messages(&context, dummy_window_event_callback, dummy_keyboard_event_callback, dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // platform_context_->backend == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = NULL;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_pump_messages(&context, dummy_window_event_callback, dummy_keyboard_event_callback, dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // window_event_callback == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_pump_messages(&context, NULL, dummy_keyboard_event_callback, dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // keyboard_event_callback == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_pump_messages(&context, dummy_window_event_callback, NULL, dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // mouse_event_callback == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_pump_messages(&context, dummy_window_event_callback, dummy_keyboard_event_callback, NULL);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // vtable委譲先が失敗を返す
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();
        s_test_config_test_vtable_pump_messages.forced_result = PLATFORM_RUNTIME_ERROR;

        ret = platform_pump_messages(&context, dummy_window_event_callback, dummy_keyboard_event_callback, dummy_mouse_event_callback);
        assert(PLATFORM_RUNTIME_ERROR == ret);
    }

    {
        // PLATFORM_WINDOW_CLOSE は正常系としてそのまま返す
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();
        s_test_config_test_vtable_pump_messages.forced_result = PLATFORM_WINDOW_CLOSE;

        ret = platform_pump_messages(&context, dummy_window_event_callback, dummy_keyboard_event_callback, dummy_mouse_event_callback);
        assert(PLATFORM_WINDOW_CLOSE == ret);
    }

    {
        // 正常系
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();
        s_test_config_test_vtable_pump_messages.forced_result = PLATFORM_SUCCESS;

        ret = platform_pump_messages(&context, dummy_window_event_callback, dummy_keyboard_event_callback, dummy_mouse_event_callback);
        assert(PLATFORM_SUCCESS == ret);
    }

    test_platform_context_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_platform_swap_buffers(void) {
    test_platform_context_config_reset();

    {
        // テスト基盤による強制失敗（前提条件チェックより前）
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        test_call_control_t config = {0};

        config.fail_on_call = 1U;
        config.forced_result = (int)PLATFORM_RUNTIME_ERROR;
        test_platform_swap_buffers_config_set(&config);

        ret = platform_swap_buffers(NULL);
        assert(PLATFORM_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_platform_swap_buffers.call_count);

        test_platform_context_config_reset();
    }

    {
        // platform_context_ == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

        test_platform_context_config_reset();

        ret = platform_swap_buffers(NULL);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }

    {
        // platform_context_->vtable == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = NULL;

        test_platform_context_config_reset();

        ret = platform_swap_buffers(&context);
        assert(PLATFORM_BAD_OPERATION == ret);
    }

    {
        // platform_context_->backend == NULL
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = NULL;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();

        ret = platform_swap_buffers(&context);
        assert(PLATFORM_BAD_OPERATION == ret);
    }

    {
        // vtable委譲先が失敗を返す
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();
        s_test_config_test_vtable_swap_buffers.forced_result = PLATFORM_RUNTIME_ERROR;

        ret = platform_swap_buffers(&context);
        assert(PLATFORM_RUNTIME_ERROR == ret);
    }

    {
        // 正常系
        platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
        platform_context_t context;
        char dummy_backend[8] = {0};

        memset(&context, 0, sizeof(platform_context_t));
        context.type = PLATFORM_USE_GLFW;
        context.backend = (platform_backend_t*)dummy_backend;
        context.vtable = &s_test_vtable;

        test_platform_context_config_reset();
        s_test_config_test_vtable_swap_buffers.forced_result = PLATFORM_SUCCESS;

        ret = platform_swap_buffers(&context);
        assert(PLATFORM_SUCCESS == ret);
    }

    test_platform_context_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_platform_vtable_get(void) {
    test_platform_context_config_reset();

    {
        // 通常系: TEST_BUILD差し替え無効 + PLATFORM_USE_GLFW
        const platform_vtable_t* vtable = NULL;
        const platform_vtable_t* glfw_vtable = platform_glfw_vtable_get();

        test_platform_context_config_reset();

        vtable = platform_vtable_get(PLATFORM_USE_GLFW);
        assert(NULL != vtable);
        assert(glfw_vtable == vtable);
    }

    {
        // 通常系: TEST_BUILD差し替え無効 + 無効なplatform_type_
        const platform_vtable_t* vtable = NULL;

        test_platform_context_config_reset();

        vtable = platform_vtable_get((platform_type_t)100);
        assert(NULL == vtable);
    }

    {
        // 差し替え有効: NULLを返させる（通常のplatform_type_判定より優先）
        const platform_vtable_t* vtable = (const platform_vtable_t*)0x1;

        test_platform_context_config_reset();
        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_NULL;

        vtable = platform_vtable_get(PLATFORM_USE_GLFW);
        assert(NULL == vtable);
    }

    {
        // 差し替え有効: NULLを返させる（無効なplatform_type_でも差し替えが優先）
        const platform_vtable_t* vtable = (const platform_vtable_t*)0x1;

        test_platform_context_config_reset();
        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_NULL;

        vtable = platform_vtable_get((platform_type_t)100);
        assert(NULL == vtable);
    }

    {
        // 差し替え有効: test vtable を返させる
        const platform_vtable_t* vtable = NULL;

        test_platform_context_config_reset();
        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_TEST;

        vtable = platform_vtable_get(PLATFORM_USE_GLFW);
        assert(NULL != vtable);
        assert(&s_test_vtable == vtable);
    }

    {
        // 差し替え有効: test vtable を返させる（無効なplatform_type_でも差し替えが優先）
        const platform_vtable_t* vtable = NULL;

        test_platform_context_config_reset();
        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_TEST;

        vtable = platform_vtable_get((platform_type_t)100);
        assert(NULL != vtable);
        assert(&s_test_vtable == vtable);
    }

    {
        // 差し替え有効: GLFW vtable を返させる
        const platform_vtable_t* vtable = NULL;
        const platform_vtable_t* glfw_vtable = platform_glfw_vtable_get();

        test_platform_context_config_reset();
        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_GLFW;

        vtable = platform_vtable_get(PLATFORM_USE_GLFW);
        assert(NULL != vtable);
        assert(glfw_vtable == vtable);
    }

    {
        // 差し替え有効: GLFW vtable を返させる（無効なplatform_type_でも差し替えが優先）
        const platform_vtable_t* vtable = NULL;
        const platform_vtable_t* glfw_vtable = platform_glfw_vtable_get();

        test_platform_context_config_reset();
        s_test_config_platform_vtable_config.enable_test_vtable = true;
        s_test_config_platform_vtable_config.vtable_select = TEST_PLATFORM_VTABLE_GLFW;

        vtable = platform_vtable_get((platform_type_t)100);
        assert(NULL != vtable);
        assert(glfw_vtable == vtable);
    }

    test_platform_context_config_reset();
}

// Generated by ChatGPT
static void NO_COVERAGE test_platform_type_valid_check(void) {
    test_platform_context_config_reset();

    {
        // テスト基盤による強制失敗（通常はtrueになる入力をfalseへ上書き）
        bool ret = true;

        test_platform_context_config_reset();
        s_test_config_platform_type_valid_check.fail_on_call = 1U;
        s_test_config_platform_type_valid_check.forced_result = false;

        ret = platform_type_valid_check(PLATFORM_USE_GLFW);
        assert(false == ret);
        assert(1U == s_test_config_platform_type_valid_check.call_count);
    }

    {
        // テスト基盤による強制成功（通常はfalseになる入力をtrueへ上書き）
        bool ret = false;

        test_platform_context_config_reset();
        s_test_config_platform_type_valid_check.fail_on_call = 1U;
        s_test_config_platform_type_valid_check.forced_result = true;

        ret = platform_type_valid_check((platform_type_t)100);
        assert(true == ret);
        assert(1U == s_test_config_platform_type_valid_check.call_count);
    }

    {
        // 正常系: 有効なplatform_type_
        bool ret = false;

        test_platform_context_config_reset();

        ret = platform_type_valid_check(PLATFORM_USE_GLFW);
        assert(true == ret);
    }

    {
        // 異常系: 無効なplatform_type_
        bool ret = true;

        test_platform_context_config_reset();

        ret = platform_type_valid_check((platform_type_t)100);
        assert(false == ret);
    }

    test_platform_context_config_reset();
}
#endif
