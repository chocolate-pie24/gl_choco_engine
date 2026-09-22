// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/systems/platform_system/platform_system.h"

#include <stdbool.h>
#include <stdalign.h>
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include "engine/systems/platform_system/core/platform_system_types.h"
#include "engine/systems/platform_system/core/platform_system_err_utils.h"
#include "engine/systems/platform_system/config/platform_system_config.h"
#include "engine/systems/platform_system/vtables/platform_backend_vtable.h"
#include "engine/systems/platform_system/platform_concretes/glfw/platform_glfw.h"

#include "config/build_config.h"

struct platform_system {
    platform_backend_t* backend;        /**< 各プラットフォーム固有実装バックエンドデータ */
    const platform_backend_vtable_t* vtable;    /**< 各プラットフォーム仮想関数テーブル */
};

static const platform_backend_vtable_t* backend_vtable_get(void);

static bool is_valid_shallow(const platform_system_t* platform_system_);

platform_system_result_t platform_system_create(const platform_system_config_t* config_, linear_allocator_t* allocator_, int* out_framebuffer_width_, int* out_framebuffer_height_, platform_system_t** out_platform_system_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_allocator = LINEAR_ALLOCATOR_INVALID_ARGUMENT;

    platform_backend_t* tmp_backend = NULL;
    platform_system_t* tmp_system = NULL;
    int tmp_framebuffer_width = 0;
    int tmp_framebuffer_height = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_width_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_create", "out_framebuffer_width_")
    IF_ARG_NULL_GOTO_CLEANUP(out_framebuffer_height_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_create", "out_framebuffer_height_")
    IF_ARG_NULL_GOTO_CLEANUP(out_platform_system_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_create", "out_platform_system_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_platform_system_, ret, PLATFORM_SYSTEM_BAD_OPERATION, platform_system_result_to_str(PLATFORM_SYSTEM_BAD_OPERATION), "platform_system_create", "*out_platform_system_")    if(!platform_system_config_is_valid(config_)) {
        ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("platform_system_create(%s) - Provided config_ is not valid.", platform_system_result_to_str(ret));
        goto cleanup;
    }

    // Simulation.
    ret_linear_allocator = linear_allocator_allocate(allocator_, sizeof(platform_system_t), alignof(platform_system_t), (void**)&tmp_system);
    if(LINEAR_ALLOCATOR_SUCCESS != ret_linear_allocator) {
        ret = platform_system_result_convert_linear_allocator(ret_linear_allocator);
        ERROR_MESSAGE("platform_system_create(%s) - Failed to allocate memory for platform system.", platform_system_result_to_str(ret));
        goto cleanup;
    }
    memset(tmp_system, 0, sizeof(platform_system_t));

    tmp_system->vtable = backend_vtable_get();
    if(NULL == tmp_system->vtable) {
        ret = PLATFORM_SYSTEM_UNDEFINED_ERROR;
        ERROR_MESSAGE("platform_system_create(%s) - Failed to get platform vtable.", platform_system_result_to_str(ret));
        goto cleanup;
    }

    ret = tmp_system->vtable->platform_backend_create(config_, allocator_, &tmp_framebuffer_width, &tmp_framebuffer_height, &tmp_backend);
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_system_create(%s) - platform_backend_create failed.", platform_system_result_to_str(ret));
        goto cleanup;
    }

    tmp_system->backend = tmp_backend;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!platform_system_is_valid(tmp_system)) {
        ret = PLATFORM_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("platform_system_create(%s) - Postcondition validation failed for 'tmp_system'.", platform_system_result_to_str(ret));
        goto cleanup;
    }
#endif

    // commit.
    *out_platform_system_ = tmp_system;
    *out_framebuffer_height_ = tmp_framebuffer_height;
    *out_framebuffer_width_ = tmp_framebuffer_width;
    tmp_system = NULL;
    tmp_backend = NULL;

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void platform_system_deinitialize(platform_system_t* platform_system_) {
    if(NULL == platform_system_) {
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!platform_system_is_valid(platform_system_)) {
        ERROR_MESSAGE("platform_system_deinitialize(%s) - Precondition validation failed for 'platform_system_'.", platform_system_result_to_str(PLATFORM_SYSTEM_DATA_CORRUPTED));
        return;
    }
#endif

    platform_system_->vtable->platform_backend_deinitialize(platform_system_->backend);
    platform_system_->backend = NULL;
    platform_system_->vtable = NULL;

cleanup:
    return;
}

platform_system_result_t platform_system_update(platform_system_t* platform_system_, const platform_event_view_t** out_event_view_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    // 毎フレーム呼ばれるAPIであるため、*out_event_view_ != NULLは許容する
    IF_ARG_NULL_GOTO_CLEANUP(platform_system_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_update", "platform_system_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_system_->vtable, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_update", "platform_system_->vtable")
    IF_ARG_NULL_GOTO_CLEANUP(platform_system_->backend, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_update", "platform_system_->backend")
    IF_ARG_NULL_GOTO_CLEANUP(out_event_view_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_update", "out_event_view_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(platform_system_)) {
        ret = PLATFORM_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("platform_system_update(%s) - Precondition validation failed for 'platform_system_'.", platform_system_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = platform_system_->vtable->platform_backend_update(platform_system_->backend, out_event_view_);
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_system_update(%s) - platform_backend_update failed.", platform_system_result_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!platform_system_is_valid(platform_system_)) {
        ret = PLATFORM_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("platform_system_update(%s) - Postcondition validation failed for 'platform_system_'.", platform_system_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = PLATFORM_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

platform_system_result_t platform_system_swap_buffers(platform_system_t* platform_system_) {
    platform_system_result_t ret = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(platform_system_, ret, PLATFORM_SYSTEM_INVALID_ARGUMENT, platform_system_result_to_str(PLATFORM_SYSTEM_INVALID_ARGUMENT), "platform_system_swap_buffers", "platform_system_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_system_->vtable, ret, PLATFORM_SYSTEM_BAD_OPERATION, platform_system_result_to_str(PLATFORM_SYSTEM_BAD_OPERATION), "platform_system_swap_buffers", "platform_system_->vtable")
    IF_ARG_NULL_GOTO_CLEANUP(platform_system_->backend, ret, PLATFORM_SYSTEM_BAD_OPERATION, platform_system_result_to_str(PLATFORM_SYSTEM_BAD_OPERATION), "platform_system_swap_buffers", "platform_system_->backend")

    ret = platform_system_->vtable->platform_backend_swap_buffers(platform_system_->backend);
    if(PLATFORM_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_system_swap_buffers(%s) - Failed to swap buffers.", platform_system_result_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

bool platform_system_is_valid(const platform_system_t* platform_system_) {
    if(NULL == platform_system_) {
        return false;
    }
    if(!is_valid_shallow(platform_system_)) {
        return false;
    }
    if(!platform_system_->vtable->platform_backend_is_valid(platform_system_->backend)) {
        return false;
    }
    return true;
}

static const platform_backend_vtable_t* backend_vtable_get(void) {
#if defined(GLCE_BUILD_PLATFORM_GLFW)
    return platform_glfw_vtable_get();
#else
    return NULL;
#endif
}

static bool is_valid_shallow(const platform_system_t* platform_system_) {
    if(NULL == platform_system_) {
        return false;
    }
    if(NULL == platform_system_->backend) {
        return false;
    }
    if(NULL == platform_system_->vtable) {
        return false;
    }
    const platform_backend_vtable_t* tmp_vtable = backend_vtable_get();
    if(platform_system_->vtable != tmp_vtable) {
        return false;
    }
    return true;
}
