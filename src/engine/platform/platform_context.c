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

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>
#include <stdlib.h> // for malloc
#include "engine/core/memory/choco_memory.h"

typedef struct platform_test {
    // platform_type_t type;   // TODO: 何に使われるのかが分からない
    // bool test_enable;       // TODO: 廃止

    bool enable_use_test_vtable;                    // trueでテスト専用vtableを使用、falseで通常vtable
    bool enable_platform_vtable_get_return_null;    // trueでplatform_vtable_getがNULLを返す(falseは正常処理を走らせる)

    platform_result_t test_vtable_init_result_code;             // test_vtable_init関数に出力させる実行結果コード
    platform_result_t test_vtable_window_create_result_code;    // test_vtable_window_create関数に出力させる実行結果コード
    platform_result_t test_vtable_pump_messages_result_code;    // test_vtable_pump_messages関数に出力させる実行結果コード
    platform_result_t test_vtable_swap_buffers_result_code;     // test_vtable_swap_buffers関数に出力させる実行結果コード
} platform_test_t;

static platform_test_t s_test_param;
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

static const char* const s_rslt_str_success = "SUCCESS";                    /**< プラットフォームコンテキスト実行結果コード(処理成功)に対応する文字列 */
static const char* const s_rslt_str_no_memory = "NO_MEMORY";                /**< プラットフォームコンテキスト実行結果コード(メモリ不足)に対応する文字列 */
static const char* const s_rslt_str_runtime_error = "RUNTIME_ERROR";        /**< プラットフォームコンテキスト実行結果コード(ランタイムエラー)に対応する文字列 */
static const char* const s_rslt_str_invalid_argument = "INVALID_ARGUMENT";  /**< プラットフォームコンテキスト実行結果コード(無効な引数)に対応する文字列 */
static const char* const s_rslt_str_data_corrupted = "DATA_CORRUPTED";      /**< プラットフォームコンテキスト実行結果コード(メモリ破損,未初期化)に対応する文字列 */
static const char* const s_rslt_str_bad_operation = "BAD_OPERATION";        /**< プラットフォームコンテキスト実行結果コード(API誤用)に対応する文字列 */
static const char* const s_rslt_str_overflow = "OVERFLOW";                  /**< プラットフォームコンテキスト実行結果コード(計算過程オーバーフロー)に対応する文字列 */
static const char* const s_rslt_str_limit_exceeded = "LIMIT_EXCEEDED";      /**< プラットフォームコンテキスト実行結果コード(システム使用可能範囲上限超過)に対応する文字列 */
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< プラットフォームコンテキスト実行結果コード(未定義エラー)に対応する文字列 */
static const char* const s_rslt_str_window_close = "WINDOW_CLOSE";          /**< プラットフォームコンテキスト実行結果コード(ウィンドウクローズ)に対応する文字列 */

const platform_vtable_t* platform_vtable_get(platform_type_t platform_type_);

static bool platform_type_valid_check(platform_type_t platform_type_);
static platform_result_t rslt_convert_linear_alloc(linear_allocator_result_t rslt_);
static const char* rslt_to_str(platform_result_t rslt_);

#ifdef TEST_BUILD

#define TEST_PLATFORM_TYPE 128  // テスト時専用プラットフォームタイプ

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

// platform_context保有関数のテスト関数
static void test_platform_initialize(void);
static void test_platform_destroy(void);
static void test_platform_window_create(void);
static void test_platform_pump_messages(void);
static void test_platform_swap_buffers(void);
static void test_platform_vtable_get(void);
static void test_platform_type_valid_check(void);
static void test_rslt_convert_linear_alloc(void);
static void test_rslt_to_str(void);

static const platform_vtable_t s_test_vtable = {
    .platform_backend_preinit = test_vtable_preinit,
    .platform_backend_init = test_vtable_init,
    .platform_backend_destroy = test_vtable_destroy,
    .platform_backend_window_create = test_vtable_window_create,
    .platform_backend_pump_messages = test_vtable_pump_messages,
    .platform_backend_swap_buffers = test_vtable_swap_buffers,
};
#endif

platform_result_t platform_initialize(linear_alloc_t* allocator_, platform_type_t platform_type_, platform_context_t** out_platform_context_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    void* backend_ptr = NULL;
    size_t backend_memory_req = 0;
    size_t backend_align_req = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, PLATFORM_INVALID_ARGUMENT, "platform_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_initialize", "out_platform_context_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_initialize", "out_platform_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(platform_type_valid_check(platform_type_), PLATFORM_INVALID_ARGUMENT, "platform_initialize", "platform_type_")

    // Simulation.
    platform_context_t* tmp_context = NULL;
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(platform_context_t), alignof(platform_context_t), (void**)&tmp_context);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("platform_initialize(%s) - Failed to allocate memory for platform context.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_context, 0, sizeof(platform_context_t));

    tmp_context->vtable = platform_vtable_get(platform_type_);
    if(NULL == tmp_context->vtable) {
        ret = PLATFORM_RUNTIME_ERROR;
        ERROR_MESSAGE("platform_initialize(%s) - Failed to get platform vtable.", rslt_to_str(ret));
        goto cleanup;
    }

    tmp_context->vtable->platform_backend_preinit(&backend_memory_req, &backend_align_req);
    ret_linear_alloc = linear_allocator_allocate(allocator_, backend_memory_req, backend_align_req, &backend_ptr);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("platform_initialize(%s) - Failed to allocate memory for platform backend.", rslt_to_str(ret));
        goto cleanup;
    }
    memset(backend_ptr, 0, backend_memory_req);

    ret = tmp_context->vtable->platform_backend_init((platform_backend_t*)backend_ptr);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_initialize(%s) - Failed to initialize platform backend.", rslt_to_str(ret));
        goto cleanup;
    }
    tmp_context->backend = backend_ptr;
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
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "platform_context_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "platform_context_->vtable")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "platform_context_->backend")
    IF_ARG_NULL_GOTO_CLEANUP(window_label_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "window_label_")
    IF_ARG_NULL_GOTO_CLEANUP(framebuffer_width_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "framebuffer_width_")
    IF_ARG_NULL_GOTO_CLEANUP(framebuffer_height_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "framebuffer_height_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != window_width_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "window_width_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != window_height_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "window_height_")

    ret = platform_context_->vtable->platform_backend_window_create(platform_context_->backend, window_label_, window_width_, window_height_, framebuffer_width_, framebuffer_height_);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_window_create(%s) - Failed to create window.", rslt_to_str(ret));
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

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "platform_context_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "platform_context_->vtable")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "platform_context_->backend")
    IF_ARG_NULL_GOTO_CLEANUP(window_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "window_event_callback")
    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "keyboard_event_callback")
    IF_ARG_NULL_GOTO_CLEANUP(mouse_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "mouse_event_callback")

    ret = platform_context_->vtable->platform_backend_pump_messages(platform_context_->backend, window_event_callback, keyboard_event_callback, mouse_event_callback);
    // PLATFORM_WINDOW_CLOSEはPLATFORM_SUCCESS以外でも正常なので無視
    if(PLATFORM_SUCCESS != ret && PLATFORM_WINDOW_CLOSE != ret) {
        ERROR_MESSAGE("platform_pump_messages(%s) - Failed to pump messages.", rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

platform_result_t platform_swap_buffers(platform_context_t* platform_context_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_swap_buffers", "platform_context_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, PLATFORM_BAD_OPERATION, "platform_swap_buffers", "platform_context_->vtable")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, PLATFORM_BAD_OPERATION, "platform_swap_buffers", "platform_context_->backend")

    ret = platform_context_->vtable->platform_backend_swap_buffers(platform_context_->backend);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_swap_buffers(%s) - Failed to swap buffers.", rslt_to_str(ret));
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
const platform_vtable_t* platform_vtable_get(platform_type_t platform_type_) {
#ifdef TEST_BUILD
    if(s_test_param.enable_platform_vtable_get_return_null) {
        return NULL;
    } else if(s_test_param.enable_use_test_vtable) {
        return &s_test_vtable;
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
    switch(platform_type_) {
    case PLATFORM_USE_GLFW:
        return true;
    default:
        return false;
    }
}

static platform_result_t rslt_convert_linear_alloc(linear_allocator_result_t rslt_) {
    switch(rslt_) {
    case LINEAR_ALLOC_SUCCESS:
        return PLATFORM_SUCCESS;
    case LINEAR_ALLOC_NO_MEMORY:
        return PLATFORM_NO_MEMORY;
    case LINEAR_ALLOC_INVALID_ARGUMENT:
        return PLATFORM_INVALID_ARGUMENT;
    default:
        return PLATFORM_UNDEFINED_ERROR;
    }
}

static const char* rslt_to_str(platform_result_t rslt_) {
    switch(rslt_) {
    case PLATFORM_SUCCESS:
        return s_rslt_str_success;
    case PLATFORM_INVALID_ARGUMENT:
        return s_rslt_str_invalid_argument;
    case PLATFORM_RUNTIME_ERROR:
        return s_rslt_str_runtime_error;
    case PLATFORM_NO_MEMORY:
        return s_rslt_str_no_memory;
    case PLATFORM_DATA_CORRUPTED:
        return s_rslt_str_data_corrupted;
    case PLATFORM_BAD_OPERATION:
        return s_rslt_str_bad_operation;
    case PLATFORM_OVERFLOW:
        return s_rslt_str_overflow;
    case PLATFORM_LIMIT_EXCEEDED:
        return s_rslt_str_limit_exceeded;
    case PLATFORM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case PLATFORM_WINDOW_CLOSE:
        return s_rslt_str_window_close;
    default:
        return s_rslt_str_undefined_error;
    }
}

#ifdef TEST_BUILD
static void NO_COVERAGE test_linear_allocator_create(linear_alloc_t** allocator_, void** out_memory_pool_, size_t pool_size_) {
    assert(NULL == *allocator_);
    assert(NULL == *out_memory_pool_);
    assert(0 != pool_size_);

    size_t mem_req = 0;
    size_t align_req = 0;
    linear_allocator_preinit(&mem_req, &align_req);

    *allocator_ = malloc(mem_req);
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
    return s_test_param.test_vtable_init_result_code;
}

static void NO_COVERAGE test_vtable_destroy(platform_backend_t* platform_backend_) {
    // 現状では何もしない
    return;
}

static platform_result_t NO_COVERAGE test_vtable_window_create(platform_backend_t* platform_backend_, const char* window_label_, int window_width_, int window_height_, int* framebuffer_width_, int* framebuffer_height_) {
    return s_test_param.test_vtable_window_create_result_code;
}

static platform_result_t NO_COVERAGE test_vtable_pump_messages(platform_backend_t* platform_backend_, void (*window_event_callback)(const window_event_t* event_), void (*keyboard_event_callback)(const keyboard_event_t* event_), void (*mouse_event_callback)(const mouse_event_t* event_)) {
    return s_test_param.test_vtable_pump_messages_result_code;
}

static platform_result_t NO_COVERAGE test_vtable_swap_buffers(platform_backend_t* platform_backend_) {
    return s_test_param.test_vtable_swap_buffers_result_code;
}

static void NO_COVERAGE dummy_window_event_callback(const window_event_t* event_) {
    return;
}

static void NO_COVERAGE dummy_keyboard_event_callback(const keyboard_event_t* event_) {
    return;
}

static void NO_COVERAGE dummy_mouse_event_callback(const mouse_event_t* event_) {
    return;
}

void NO_COVERAGE test_platform_context(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    test_platform_vtable_get();
    test_platform_type_valid_check();
    test_rslt_convert_linear_alloc();
    test_rslt_to_str();
    test_platform_initialize();
    test_platform_destroy();
    test_platform_window_create();
    test_platform_pump_messages();
    test_platform_swap_buffers();

    memory_system_destroy();
}

static void NO_COVERAGE test_platform_initialize(void) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    s_test_param.enable_platform_vtable_get_return_null = false;
    s_test_param.enable_use_test_vtable = false;
    s_test_param.test_vtable_init_result_code = PLATFORM_SUCCESS;
    s_test_param.test_vtable_pump_messages_result_code = PLATFORM_SUCCESS;
    s_test_param.test_vtable_swap_buffers_result_code = PLATFORM_SUCCESS;
    s_test_param.test_vtable_window_create_result_code = PLATFORM_SUCCESS;
    {
        // allocator_ == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        ret = platform_initialize(NULL, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
        assert(NULL == platform_context);
    }
    {
        // out_platform_context_ == NULLでPLATFORM_INVALID_ARGUMENT
        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&allocator, &memory_pool, 128); // 仮で128byteのリニアアロケータ生成

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, NULL);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }
    {
        // *out_platform_context_ != NULLでPLATFORM_INVALID_ARGUMENT
        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&allocator, &memory_pool, 128); // 仮で128byteのリニアアロケータ生成

        platform_context_t* platform_context = NULL;
        platform_context = malloc(sizeof(platform_context_t));
        assert(NULL != platform_context);

        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        free(platform_context);
        platform_context = NULL;
        test_linear_allocator_destroy(&allocator, &memory_pool);
    }
    {
        // platform_type_が既定値外でPLATFORM_INVALID_ARGUMENT
        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&allocator, &memory_pool, 128); // 仮で128byteのリニアアロケータ生成

        platform_context_t* platform_context = NULL;

        ret = platform_initialize(allocator, 128, &platform_context);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
        assert(NULL == platform_context);

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }
    {
        // リニアアロケータによるtmp_contextメモリ確保失敗(NO_MEMORY)でPLATFORM_NO_MEMORY
        linear_allocator_malloc_fail_set(0);

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&allocator, &memory_pool, 128); // 仮で128byteのリニアアロケータ生成

        platform_context_t* platform_context = NULL;
        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_NO_MEMORY == ret);
        assert(NULL == platform_context);

        test_linear_allocator_destroy(&allocator, &memory_pool);

        linear_allocator_malloc_fail_reset();
    }
    {
        // vtable_getで取得したvtableがNULLでPLATFORM_RUNTIME_ERROR
        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&allocator, &memory_pool, 128); // 仮で128byteのリニアアロケータ生成

        s_test_param.enable_platform_vtable_get_return_null = true;

        platform_context_t* platform_context = NULL;
        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_RUNTIME_ERROR == ret);
        assert(NULL == platform_context);

        s_test_param.enable_platform_vtable_get_return_null = false;

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }
    {
        // 2回目のlinear_allocator_allocateで失敗し、NO_MEMORY
        linear_allocator_malloc_fail_set(1);

        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&allocator, &memory_pool, 128); // 仮で128byteのリニアアロケータ生成

        s_test_param.enable_use_test_vtable = true;

        platform_context_t* platform_context = NULL;
        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_NO_MEMORY == ret);
        assert(NULL == platform_context);

        test_linear_allocator_destroy(&allocator, &memory_pool);

        s_test_param.enable_use_test_vtable = false;
        linear_allocator_malloc_fail_reset();
    }
    {
        // platform_backend_init()にPLATFORM_BAD_OPERATIONを出力させ、PLATFORM_BAD_OPERATION
        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&allocator, &memory_pool, 128); // 仮で128byteのリニアアロケータ生成

        s_test_param.test_vtable_init_result_code = PLATFORM_BAD_OPERATION;
        s_test_param.enable_use_test_vtable = true;

        platform_context_t* platform_context = NULL;
        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_BAD_OPERATION == ret);
        assert(NULL == platform_context);

        s_test_param.enable_use_test_vtable = false;
        s_test_param.test_vtable_init_result_code = PLATFORM_SUCCESS;

        test_linear_allocator_destroy(&allocator, &memory_pool);
    }
    {
        // 正常系
        linear_alloc_t* allocator = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&allocator, &memory_pool, 128); // 仮で128byteのリニアアロケータ生成

        s_test_param.enable_use_test_vtable = true;

        platform_context_t* platform_context = NULL;
        ret = platform_initialize(allocator, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);
        assert(NULL != platform_context);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&allocator, &memory_pool);
    }
    s_test_param.enable_platform_vtable_get_return_null = false;
    s_test_param.enable_use_test_vtable = false;
    s_test_param.test_vtable_init_result_code = PLATFORM_SUCCESS;
    s_test_param.test_vtable_pump_messages_result_code = PLATFORM_SUCCESS;
    s_test_param.test_vtable_swap_buffers_result_code = PLATFORM_SUCCESS;
    s_test_param.test_vtable_window_create_result_code = PLATFORM_SUCCESS;
}

static void NO_COVERAGE test_platform_destroy(void) {
    {
        // platform_context_ == NULL
        platform_destroy(NULL);
    }
    {
        // platform_context_->vtable == NULL
        platform_context_t* platform_context = NULL;
        platform_context = malloc(sizeof(platform_context_t));
        assert(NULL != platform_context);
        memset(platform_context, 0, sizeof(platform_context_t));

        platform_destroy(platform_context);

        free(platform_context);
        platform_context = NULL;
    }
    {
        // platform_context_->backend == NULL
        platform_context_t* platform_context = NULL;
        platform_context = malloc(sizeof(platform_context_t));
        assert(NULL != platform_context);
        memset(platform_context, 0, sizeof(platform_context_t));

        platform_context->vtable = &s_test_vtable;
        platform_destroy(platform_context);
        platform_context->vtable = NULL;

        free(platform_context);
        platform_context = NULL;
    }
    {
        // 正常系
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;

        platform_result_t ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        platform_destroy(platform_context);
        platform_destroy(platform_context); // 2重呼び出し

        test_linear_allocator_destroy(&linear_alloc, &memory_pool);

        s_test_param.enable_use_test_vtable = false;
    }
}

static void NO_COVERAGE test_platform_window_create(void) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    {
        // platform_context_ == NULLでPLATFORM_INVALID_ARGUMENT
        int framebuffer_width = 1024;
        int framebuffer_height = 768;
        ret = platform_window_create(NULL, "test_window", 1024, 768, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }
    {
        // platform_context_->vtable == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        platform_context = malloc(sizeof(platform_context_t));
        assert(NULL != platform_context);
        memset(platform_context, 0, sizeof(platform_context_t));

        int framebuffer_width = 1024;
        int framebuffer_height = 768;
        ret = platform_window_create(platform_context, "test_window", 1024, 768, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        free(platform_context);
        platform_context = NULL;
    }
    {
        // platform_context_->backend == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        platform_context = malloc(sizeof(platform_context_t));
        assert(NULL != platform_context);
        memset(platform_context, 0, sizeof(platform_context_t));

        int framebuffer_width = 1024;
        int framebuffer_height = 768;
        platform_context->vtable = &s_test_vtable;
        ret = platform_window_create(platform_context, "test_window", 1024, 768, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        free(platform_context);
        platform_context = NULL;
    }
    {
        // window_label_ == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        int framebuffer_width = 1024;
        int framebuffer_height = 768;
        ret = platform_window_create(platform_context, NULL, 1024, 768, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // framebuffer_width_ == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        int framebuffer_height = 768;
        ret = platform_window_create(platform_context, "test_window", 1024, 768, NULL, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // framebuffer_height == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        int framebuffer_width = 1024;
        ret = platform_window_create(platform_context, "test_window", 1024, 768, &framebuffer_width, NULL);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // window_width == 0でPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        int framebuffer_width = 1024;
        int framebuffer_height = 768;
        ret = platform_window_create(platform_context, "test_window", 0, 768, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // window_height_ == 0でPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        int framebuffer_width = 1024;
        int framebuffer_height = 768;
        ret = platform_window_create(platform_context, "test_window", 1024, 0, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // platform_backend_window_createがBAD_OPERATIONを返すようにし、BAD_OPERATION
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        int framebuffer_width = 1024;
        int framebuffer_height = 768;
        s_test_param.test_vtable_window_create_result_code = PLATFORM_BAD_OPERATION;
        ret = platform_window_create(platform_context, "test_window", 1024, 768, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_BAD_OPERATION == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);

        s_test_param.test_vtable_window_create_result_code = PLATFORM_SUCCESS;
    }
    {
        // 正常系
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        int framebuffer_width = 1024;
        int framebuffer_height = 768;
        ret = platform_window_create(platform_context, "test_window", 1024, 768, &framebuffer_width, &framebuffer_height);
        assert(PLATFORM_SUCCESS == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
}

static void NO_COVERAGE test_platform_pump_messages(void) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    {
        // platform_context_ == NULLでPLATFORM_INVALID_ARGUMENT
        ret = platform_pump_messages(NULL, &dummy_window_event_callback, &dummy_keyboard_event_callback, &dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }
    {
        // platform_context_->vtable == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        platform_context = malloc(sizeof(platform_context_t));
        assert(NULL != platform_context);
        memset(platform_context, 0, sizeof(platform_context_t));

        ret = platform_pump_messages(platform_context, &dummy_window_event_callback, &dummy_keyboard_event_callback, &dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        free(platform_context);
        platform_context = NULL;
    }
    {
        // platform_context_->backend == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        platform_context = malloc(sizeof(platform_context_t));
        assert(NULL != platform_context);
        memset(platform_context, 0, sizeof(platform_context_t));

        platform_context->vtable = &s_test_vtable;
        ret = platform_pump_messages(platform_context, &dummy_window_event_callback, &dummy_keyboard_event_callback, &dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        free(platform_context);
        platform_context = NULL;
    }
    {
        // window_event_callback == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        ret = platform_pump_messages(platform_context, NULL, &dummy_keyboard_event_callback, &dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // keyboard_event_callback == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        ret = platform_pump_messages(platform_context, &dummy_window_event_callback, NULL, &dummy_mouse_event_callback);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // mouse_event_callback == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        ret = platform_pump_messages(platform_context, &dummy_window_event_callback, &dummy_keyboard_event_callback, NULL);
        assert(PLATFORM_INVALID_ARGUMENT == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // platform_backend_pump_messagesにWINDOW_CLOSEを出力させ、WINDOW_CLOSE
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        s_test_param.test_vtable_pump_messages_result_code = PLATFORM_WINDOW_CLOSE;
        ret = platform_pump_messages(platform_context, &dummy_window_event_callback, &dummy_keyboard_event_callback, &dummy_mouse_event_callback);
        assert(PLATFORM_WINDOW_CLOSE == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
        s_test_param.test_vtable_pump_messages_result_code = PLATFORM_SUCCESS;
    }
    {
        // platform_backend_pump_messagesにBAD_OPERATIONを出力させ、BAD_OPERATION
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        s_test_param.test_vtable_pump_messages_result_code = PLATFORM_BAD_OPERATION;
        ret = platform_pump_messages(platform_context, &dummy_window_event_callback, &dummy_keyboard_event_callback, &dummy_mouse_event_callback);
        assert(PLATFORM_BAD_OPERATION == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
        s_test_param.test_vtable_pump_messages_result_code = PLATFORM_SUCCESS;
    }
    {
        // 正常系
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        ret = platform_pump_messages(platform_context, &dummy_window_event_callback, &dummy_keyboard_event_callback, &dummy_mouse_event_callback);
        assert(PLATFORM_SUCCESS == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
}

static void NO_COVERAGE test_platform_swap_buffers(void) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    {
        // platform_context_ == NULLでPLATFORM_INVALID_ARGUMENT
        platform_swap_buffers(NULL);
        assert(PLATFORM_INVALID_ARGUMENT == ret);
    }
    {
        // platform_context_->vtable == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        platform_context = malloc(sizeof(platform_context_t));
        assert(NULL != platform_context);
        memset(platform_context, 0, sizeof(platform_context_t));

        ret = platform_swap_buffers(platform_context);
        assert(PLATFORM_BAD_OPERATION == ret);

        free(platform_context);
        platform_context = NULL;
    }
    {
        // platform_context_->backend == NULLでPLATFORM_INVALID_ARGUMENT
        platform_context_t* platform_context = NULL;
        platform_context = malloc(sizeof(platform_context_t));
        assert(NULL != platform_context);
        memset(platform_context, 0, sizeof(platform_context_t));

        platform_context->vtable = &s_test_vtable;
        ret = platform_swap_buffers(platform_context);
        assert(PLATFORM_BAD_OPERATION == ret);

        free(platform_context);
        platform_context = NULL;
    }
    {
        // swap_buffersにRUNTIME_ERRORを出力させてRUNTIME_ERROR
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        s_test_param.test_vtable_swap_buffers_result_code = PLATFORM_RUNTIME_ERROR;

        ret = platform_swap_buffers(platform_context);
        assert(PLATFORM_RUNTIME_ERROR == ret);

        s_test_param.enable_use_test_vtable = false;
        s_test_param.test_vtable_swap_buffers_result_code = PLATFORM_SUCCESS;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
    {
        // 正常系
        platform_context_t* platform_context = NULL;
        linear_alloc_t* linear_alloc = NULL;
        void* memory_pool = NULL;
        test_linear_allocator_create(&linear_alloc, &memory_pool, 128);

        s_test_param.enable_use_test_vtable = true;
        ret = platform_initialize(linear_alloc, PLATFORM_USE_GLFW, &platform_context);
        assert(PLATFORM_SUCCESS == ret);

        ret = platform_swap_buffers(platform_context);
        assert(PLATFORM_SUCCESS == ret);

        s_test_param.enable_use_test_vtable = false;
        test_linear_allocator_destroy(&linear_alloc, &memory_pool);
    }
}

static void NO_COVERAGE test_platform_vtable_get(void) {
    {
        // return NULL
        s_test_param.enable_platform_vtable_get_return_null = true;

        const platform_vtable_t* vtable = platform_vtable_get(PLATFORM_USE_GLFW);

        s_test_param.enable_platform_vtable_get_return_null = false;
    }
    {
        // return test_vtable
        s_test_param.enable_use_test_vtable = true;

        const platform_vtable_t* vtable = platform_vtable_get(PLATFORM_USE_GLFW);

        s_test_param.enable_use_test_vtable = false;
    }
    {
        // vtable_typeが既定値外
        s_test_param.enable_platform_vtable_get_return_null = false;
        s_test_param.enable_use_test_vtable = false;

        const platform_vtable_t* vtable = platform_vtable_get(100);
        assert(NULL == vtable);
    }
    {
        // 通常vtable
        s_test_param.enable_platform_vtable_get_return_null = false;
        s_test_param.enable_use_test_vtable = false;

        const platform_vtable_t* vtable = platform_vtable_get(PLATFORM_USE_GLFW);
    }
}

static void NO_COVERAGE test_platform_type_valid_check(void) {
    bool ret = platform_type_valid_check(PLATFORM_USE_GLFW);
    assert(ret);

    ret = platform_type_valid_check(20);
    assert(!ret);
}

static void NO_COVERAGE test_rslt_convert_linear_alloc(void) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    ret = rslt_convert_linear_alloc(LINEAR_ALLOC_SUCCESS);
    assert(PLATFORM_SUCCESS == ret);

    ret = rslt_convert_linear_alloc(LINEAR_ALLOC_NO_MEMORY);
    assert(PLATFORM_NO_MEMORY == ret);

    ret = rslt_convert_linear_alloc(LINEAR_ALLOC_INVALID_ARGUMENT);
    assert(PLATFORM_INVALID_ARGUMENT == ret);

    ret = rslt_convert_linear_alloc(100);
    assert(PLATFORM_UNDEFINED_ERROR == ret);
}

static void NO_COVERAGE test_rslt_to_str(void) {
    {
        const char* test = rslt_to_str(PLATFORM_SUCCESS);
        assert(0 == strcmp(s_rslt_str_success, test));
    }
    {
        const char* test = rslt_to_str(PLATFORM_DATA_CORRUPTED);
        assert(0 == strcmp(s_rslt_str_data_corrupted, test));
    }
    {
        const char* test = rslt_to_str(PLATFORM_BAD_OPERATION);
        assert(0 == strcmp(s_rslt_str_bad_operation, test));
    }
    {
        const char* test = rslt_to_str(PLATFORM_OVERFLOW);
        assert(0 == strcmp(s_rslt_str_overflow, test));
    }
    {
        const char* test = rslt_to_str(PLATFORM_LIMIT_EXCEEDED);
        assert(0 == strcmp(s_rslt_str_limit_exceeded, test));
    }
    {
        const char* test = rslt_to_str(PLATFORM_NO_MEMORY);
        assert(0 == strcmp(s_rslt_str_no_memory, test));
    }
    {
        const char* test = rslt_to_str(PLATFORM_RUNTIME_ERROR);
        assert(0 == strcmp(s_rslt_str_runtime_error, test));
    }
    {
        const char* test = rslt_to_str(PLATFORM_INVALID_ARGUMENT);
        assert(0 == strcmp(s_rslt_str_invalid_argument, test));
    }
    {
        const char* test = rslt_to_str(PLATFORM_UNDEFINED_ERROR);
        assert(0 == strcmp(s_rslt_str_undefined_error, test));
    }
    {
        const char* test = rslt_to_str(PLATFORM_WINDOW_CLOSE);
        assert(0 == strcmp(s_rslt_str_window_close, test));
    }
    {
        const char* test = rslt_to_str(100);
        assert(0 == strcmp(s_rslt_str_undefined_error, test));
    }
}

#endif
