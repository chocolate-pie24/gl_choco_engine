/**
 *
 * @file platform_context.c
 * @author chocolate-pie24
 * @brief プラットフォームstrategyパターンへの窓口となる処理の実装
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

#include "engine/platform/platform_context.h"

#include "engine/interfaces/platform_interface.h"

#include "engine/platform_concretes/platform_glfw.h"

#include "engine/core/platform/platform_utils.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

/**
 * @brief プラットフォームコンテキストオブジェクト
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
static const char* const s_rslt_str_undefined_error = "UNDEFINED_ERROR";    /**< プラットフォームコンテキスト実行結果コード(未定義エラー)に対応する文字列 */
static const char* const s_rslt_str_window_close = "WINDOW_CLOSE";          /**< プラットフォームコンテキスト実行結果コード(ウィンドウクローズ)に対応する文字列 */

const platform_vtable_t* platform_vtable_get(platform_type_t platform_type_);

static bool platform_type_valid_check(platform_type_t platform_type_);
static platform_result_t rslt_convert_linear_alloc(linear_allocator_result_t rslt_);
static const char* rslt_to_str(platform_result_t rslt_);

// PLATFORM_INVALID_ARGUMENT allocator_ == NULL
// PLATFORM_INVALID_ARGUMENT out_platform_context_ == NULL
// PLATFORM_INVALID_ARGUMENT *out_platform_context_ != NULL
// PLATFORM_INVALID_ARGUMENT platform_type_が無効値
// PLATFORM_RUNTIME_ERROR vtable取得失敗
// PLATFORM_SUCCESS 初期化およびメモリ確保に成功し、正常終了
// 上記以外 リニアアロケータによるメモリ確保失敗
platform_result_t platform_initialize(linear_alloc_t* allocator_, platform_type_t platform_type_, platform_context_t** out_platform_context_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    void* backend_ptr = NULL;
    size_t backend_memory_req = 0;
    size_t backend_align_req = 0;

    // Preconditions.
    CHECK_ARG_NULL_GOTO_CLEANUP(allocator_, PLATFORM_INVALID_ARGUMENT, "platform_initialize", "allocator_")
    CHECK_ARG_NULL_GOTO_CLEANUP(out_platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_initialize", "out_platform_context_")
    CHECK_ARG_NOT_NULL_GOTO_CLEANUP(*out_platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_initialize", "out_platform_context_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(platform_type_valid_check(platform_type_), PLATFORM_INVALID_ARGUMENT, "platform_initialize", "platform_type_")

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

// PLATFORM_INVALID_ARGUMENT platform_context_ == NULL
// PLATFORM_INVALID_ARGUMENT platform_context_->vtable
// PLATFORM_INVALID_ARGUMENT platform_context_->backend
// PLATFORM_INVALID_ARGUMENT window_label_ == NULL
// PLATFORM_INVALID_ARGUMENT window_width_ == 0
// PLATFORM_INVALID_ARGUMENT window_height_ == 0
// PLATFORM_SUCCESS ウィンドウ生成に成功し、正常終了
// 上記以外 各プラットフォーム実装依存
platform_result_t platform_window_create(platform_context_t* platform_context_, const char* window_label_, int window_width_, int window_height_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "platform_context_")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "platform_context_->vtable")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "platform_context_->backend")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_label_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "window_label_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(window_width_ != 0, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "window_width_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(window_height_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "window_height_")

    ret = platform_context_->vtable->platform_window_create(platform_context_->backend, window_label_, window_width_, window_height_);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_window_create(%s) - Failed to create window.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = PLATFORM_SUCCESS;
cleanup:
    return ret;
}

// PLATFORM_INVALID_ARGUMENT platform_context_ == NULL
// PLATFORM_INVALID_ARGUMENT platform_context_->vtable == NULL
// PLATFORM_INVALID_ARGUMENT platform_context_->backend == NULL
// PLATFORM_INVALID_ARGUMENT window_event_callback == NULL
// PLATFORM_INVALID_ARGUMENT keyboard_event_callback == NULL
// PLATFORM_INVALID_ARGUMENT mouse_event_callback == NULL
// PLATFORM_WINDOW_CLOSE ウィンドウクローズイベント発生(これは絶対に補足しなくてはいけないため、コールバックとは別に処理する)
// PLATFORM_SUCCESS イベントの吸い上げに成功し、正常終了
// 上記以外 プラットフォーム実装依存
platform_result_t platform_pump_messages(
    platform_context_t* platform_context_,
    void (*window_event_callback)(const window_event_t* event_),
    void (*keyboard_event_callback)(const keyboard_event_t* event_),
    void (*mouse_event_callback)(const mouse_event_t* event_)) {

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "platform_context_")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "platform_context_->vtable")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "platform_context_->backend")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "window_event_callback")
    CHECK_ARG_NULL_GOTO_CLEANUP(keyboard_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "keyboard_event_callback")
    CHECK_ARG_NULL_GOTO_CLEANUP(mouse_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_pump_messages", "mouse_event_callback")

    ret = platform_context_->vtable->platform_pump_messages(platform_context_->backend, window_event_callback, keyboard_event_callback, mouse_event_callback);
    if(PLATFORM_SUCCESS != ret && PLATFORM_WINDOW_CLOSE != ret) {
        ERROR_MESSAGE("platform_pump_messages(%s) - Failed to pump messages.", rslt_to_str(ret));
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
    case PLATFORM_UNDEFINED_ERROR:
        return s_rslt_str_undefined_error;
    case PLATFORM_WINDOW_CLOSE:
        return s_rslt_str_window_close;
    default:
        return s_rslt_str_undefined_error;
    }
}
