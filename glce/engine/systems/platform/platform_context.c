// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup platform
 *
 * @file platform_context.c
 * @author chocolate-pie24
 * @brief プラットフォームシステムのStrategy Contextモジュールの実装
 *
 * @date 2025-10-14
 *
 */
#include "engine/systems/platform/platform_context.h"

#include <stdbool.h>
#include <stdalign.h>
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"
#include "engine/core/memory/linear_allocator.h"

#include "engine/systems/platform/core/platform_types.h"
#include "engine/systems/platform/core/platform_err_utils.h"
#include "engine/systems/platform/vtables/platform_vtable.h"
#include "engine/systems/platform/platform_concretes/glfw/platform_glfw.h"

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

static bool platform_type_is_valid(platform_type_t platform_type_);

platform_result_t platform_initialize(linear_alloc_t* allocator_, platform_type_t platform_type_, platform_context_t** out_platform_context_) {
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
    IF_ARG_FALSE_GOTO_CLEANUP(platform_type_is_valid(platform_type_), ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "platform_type_")

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
    switch (platform_type_) {
    case PLATFORM_USE_GLFW:
        return platform_glfw_vtable_get();
    default:
        return NULL;
    }
}

static bool platform_type_is_valid(platform_type_t platform_type_) {
    switch(platform_type_) {
    case PLATFORM_USE_GLFW:
        return true;
    default:
        return false;
    }
}
