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
#include "engine/systems/platform/config/platform_config.h"
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
static bool is_valid_shallow(const platform_context_t* platform_context_);

platform_result_t platform_initialize(platform_type_t platform_type_, const platform_config_t* config_, linear_alloc_t* allocator_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_context_t** out_platform_context_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    platform_backend_t* tmp_backend = NULL;
    platform_context_t* tmp_context = NULL;
    int tmp_framebuffer_width = 0;
    int tmp_framebuffer_height = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_width_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "out_framebuffer_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_height_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "out_framebuffer_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_platform_context_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_initialize", "out_platform_context_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_platform_context_, ret, PLATFORM_BAD_OPERATION, platform_rslt_to_str(PLATFORM_BAD_OPERATION), "platform_initialize", "*out_platform_context_")
    if(!platform_type_is_valid(platform_type_)) {
        ret = PLATFORM_INVALID_ARGUMENT;
        ERROR_MESSAGE("platform_initialize(%s) - Provided platform_type_ is not valid.", platform_rslt_to_str(ret));
        goto cleanup;
    }
    if(!platform_config_is_valid(config_)) {
        ret = PLATFORM_INVALID_ARGUMENT;
        ERROR_MESSAGE("platform_initialize(%s) - Provided config_ is not valid.", platform_rslt_to_str(ret));
        goto cleanup;
    }

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
        ret = PLATFORM_UNDEFINED_ERROR;
        ERROR_MESSAGE("platform_initialize(%s) - Failed to get platform vtable.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    ret = tmp_context->vtable->platform_backend_initialize(config_, allocator_, &tmp_framebuffer_width, &tmp_framebuffer_height, &tmp_backend);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_initialize(%s) - platform_backend_initialize failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_context->backend = tmp_backend;
    tmp_context->type = platform_type_;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!platform_is_valid(tmp_context)) {
        ret = PLATFORM_DATA_CORRUPTED;
        ERROR_MESSAGE("platform_initialize(%s) - Postcondition validation failed for 'tmp_context'.", platform_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    // commit.
    *out_platform_context_ = tmp_context;
    *out_framebuffer_height_ = tmp_framebuffer_height;
    *out_framebuffer_width_ = tmp_framebuffer_width;
    tmp_context = NULL;
    tmp_backend = NULL;

    ret = PLATFORM_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void platform_deinitialize(platform_context_t* platform_context_) {
    if(NULL == platform_context_) {
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!platform_is_valid(platform_context_)) {
        ERROR_MESSAGE("platform_deinitialize(%s) - Precondition validation failed for 'platform_context_'.", platform_rslt_to_str(PLATFORM_DATA_CORRUPTED));
        return;
    }
#endif

    platform_context_->vtable->platform_backend_deinitialize(platform_context_->backend);
    platform_context_->backend = NULL;
    platform_context_->vtable = NULL;

cleanup:
    return;
}

platform_result_t platform_update(platform_context_t* platform_context_, const platform_event_view_t** out_event_view_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    // 毎フレーム呼ばれるAPIであるため、*out_event_view_ != NULLは許容する
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_update", "platform_context_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_update", "platform_context_->vtable")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_update", "platform_context_->backend")
    IF_ARG_NULL_GOTO_CLEANUP(out_event_view_, ret, PLATFORM_INVALID_ARGUMENT, platform_rslt_to_str(PLATFORM_INVALID_ARGUMENT), "platform_update", "out_event_view_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(platform_context_)) {
        ret = PLATFORM_DATA_CORRUPTED;
        ERROR_MESSAGE("platform_update(%s) - Precondition validation failed for 'platform_context_'.", platform_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = platform_context_->vtable->platform_backend_update(platform_context_->backend, out_event_view_);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_pump_messages(%s) - platform_backend_update failed.", platform_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(platform_context_)) {
        ret = PLATFORM_DATA_CORRUPTED;
        ERROR_MESSAGE("platform_update(%s) - Postcondition validation failed for 'platform_context_'.", platform_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = PLATFORM_SUCCESS;

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

bool platform_is_valid(const platform_context_t* platform_context_) {
    if(NULL == platform_context_) {
        return false;
    }
    if(!is_valid_shallow(platform_context_)) {
        return false;
    }
    if(!platform_context_->vtable->platform_backend_is_valid(platform_context_->backend)) {
        return false;
    }
    return true;
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

static bool is_valid_shallow(const platform_context_t* platform_context_) {
    if(NULL == platform_context_) {
        return false;
    }
    if(!platform_type_is_valid(platform_context_->type)) {
        return false;
    }
    if(NULL == platform_context_->backend) {
        return false;
    }
    if(NULL == platform_context_->vtable) {
        return false;
    }
    const platform_vtable_t* tmp_vtable = platform_vtable_get(platform_context_->type);
    if(platform_context_->vtable != tmp_vtable) {
        return false;
    }
    return true;
}
