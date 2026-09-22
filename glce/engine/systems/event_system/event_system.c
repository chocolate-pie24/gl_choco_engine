// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/event_system/event_system.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/memory/low_level_allocators/linear_allocator/linear_allocator.h"

#include "engine/systems/platform_system/core/platform_system_types.h"
#include "engine/systems/platform_system/core/platform_event_view.h"
#include "engine/systems/platform_system/platform_system.h"

#include "engine/systems/event_system/core/event_system_types.h"
#include "engine/systems/event_system/core/event_system_err_utils.h"
#include "engine/systems/event_system/core/engine_event_view.h"

#include "engine/systems/event_system/config/event_system_config.h"

typedef struct window_event_storage {
    size_t max_event_count;
    size_t current_event_count;
    window_event_t* event_storage;
} window_event_storage_t;

typedef struct keyboard_event_storage {
    size_t max_event_count;
    size_t current_event_count;
    keyboard_event_t* event_storage;
} keyboard_event_storage_t;

typedef struct mouse_event_storage {
    size_t max_event_count;
    size_t current_event_count;
    mouse_event_t* event_storage;
} mouse_event_storage_t;

// 当面はeventを取得するのがplatformのみなので、単純にplatform_event_viewの中身をengine_event_viewに移すだけ
// 将来的に通信経由のコマンド等が出てきた際に、各システムのevent_viewをengine_event_view_tにマージする方式にする
struct event_system {
    platform_system_t* platform_system;   // mutable borrow

    window_event_storage_t window_event_storage;
    keyboard_event_storage_t keyboard_event_storage;
    mouse_event_storage_t mouse_event_storage;

    engine_event_view_t event_view;
};

// Initialization / lifecycle helpers
static event_system_result_t event_storage_initialize(const event_system_config_t* config_, linear_alloc_t* linear_alloc_, window_event_storage_t* window_event_storage_, keyboard_event_storage_t* keyboard_event_storage_, mouse_event_storage_t* mouse_event_storage_);

// Per-frame update helpers
static void event_storage_counts_reset(window_event_storage_t* window_event_storage_, keyboard_event_storage_t* keyboard_event_storage_, mouse_event_storage_t* mouse_event_storage_);
static event_system_result_t event_view_refresh(bool window_close_requested_, const window_event_storage_t* window_event_storage_, const keyboard_event_storage_t* keyboard_event_storage_, const mouse_event_storage_t* mouse_event_storage_, engine_event_view_t* event_view_);

// Event storage primitive operations
static event_system_result_t window_event_storage_push(window_event_storage_t* event_storage_, const window_event_t* event_);
static event_system_result_t keyboard_event_storage_push(keyboard_event_storage_t* event_storage_, const keyboard_event_t* event_);
static event_system_result_t mouse_event_storage_push(mouse_event_storage_t* event_storage_, const mouse_event_t* event_);

// Validators
static bool window_event_storage_is_valid(const window_event_storage_t* event_storage_);
static bool keyboard_event_storage_is_valid(const keyboard_event_storage_t* event_storage_);
static bool mouse_event_storage_is_valid(const mouse_event_storage_t* event_storage_);
static bool is_valid_shallow(const event_system_t* event_system_);

event_system_result_t event_system_create(const event_system_config_t* config_, linear_alloc_t* linear_alloc_, platform_system_t* platform_system_, event_system_t** out_event_system_) {
    event_system_result_t ret = EVENT_SYSTEM_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    event_system_t* tmp_event_system = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_system_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(linear_alloc_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_system_create", "linear_alloc_")
    IF_ARG_NULL_GOTO_CLEANUP(platform_system_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_system_create", "platform_system_")
    IF_ARG_NULL_GOTO_CLEANUP(out_event_system_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_system_create", "out_event_system_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_event_system_, ret, EVENT_SYSTEM_BAD_OPERATION, event_system_rslt_to_str(EVENT_SYSTEM_BAD_OPERATION), "event_system_create", "*out_event_system_")
    if(!event_system_config_is_valid(config_)) {
        ret = EVENT_SYSTEM_INVALID_ARGUMENT;
        ERROR_MESSAGE("event_system_create(%s) - Provided config_ is not valid.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    // event system
    ret_linear_alloc = linear_allocator_allocate(linear_alloc_, sizeof(event_system_t), alignof(event_system_t), (void**)&tmp_event_system);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = event_system_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("event_system_create(%s) - linear_allocator_allocate failed.", event_system_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_event_system, 0, sizeof(event_system_t));

    // event array, event count
    ret = event_storage_initialize(config_, linear_alloc_, &tmp_event_system->window_event_storage, &tmp_event_system->keyboard_event_storage, &tmp_event_system->mouse_event_storage);
    if(EVENT_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("event_system_create(%s) - event_storage_initialize failed.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    // event view
    ret = event_view_refresh(false, &tmp_event_system->window_event_storage, &tmp_event_system->keyboard_event_storage, &tmp_event_system->mouse_event_storage, &tmp_event_system->event_view);
    if(EVENT_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("event_system_create(%s) - event_view_refresh failed.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_event_system->platform_system = platform_system_;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!event_system_is_valid(tmp_event_system)) {
        ret = EVENT_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("event_system_create(%s) - Postcondition validation failed for 'tmp_event_system'.", event_system_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_event_system_ = tmp_event_system;
    tmp_event_system = NULL;

    ret = EVENT_SYSTEM_SUCCESS;

cleanup:
    if(EVENT_SYSTEM_DATA_CORRUPTED != ret) {
        if(NULL != tmp_event_system) {
            event_system_deinitialize(tmp_event_system);
        }
    }
    return ret;
}

void event_system_deinitialize(event_system_t* event_system_) {
    if(NULL == event_system_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!event_system_is_valid(event_system_)) {
        ERROR_MESSAGE("event_system_deinitialize(%s) - Precondition validation failed for 'event_system_'.", event_system_rslt_to_str(EVENT_SYSTEM_DATA_CORRUPTED));
        return;
    }
#endif

    event_storage_counts_reset(&event_system_->window_event_storage, &event_system_->keyboard_event_storage, &event_system_->mouse_event_storage);
    event_system_->platform_system = NULL;
}

event_system_result_t event_system_update(event_system_t* event_system_, const engine_event_view_t** out_event_view_) {
    event_system_result_t ret = EVENT_SYSTEM_SUCCESS;

    platform_system_result_t ret_platform_system = PLATFORM_SYSTEM_INVALID_ARGUMENT;

    const platform_event_view_t* platform_event_view;

    IF_ARG_NULL_GOTO_CLEANUP(event_system_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_system_update", "event_system_")
    IF_ARG_NULL_GOTO_CLEANUP(out_event_view_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_system_update", "out_event_view_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(event_system_)) {
        ret = EVENT_SYSTEM_DATA_CORRUPTED;
        ERROR_MESSAGE("event_system_update(%s) - Precondition validation failed for 'event_system_'.", event_system_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_platform_system = platform_system_update(event_system_->platform_system, &platform_event_view);
    if(PLATFORM_SYSTEM_SUCCESS != ret_platform_system) {
        ret = event_system_rslt_convert_platform_system(ret_platform_system);
        ERROR_MESSAGE("event_system_update(%s) - platform_system_update failed.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    if(platform_event_view->keyboard_event_count > event_system_->keyboard_event_storage.max_event_count) {
        ret = EVENT_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("event_system_update(%s) - keyboard event count limit exceeded.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    if(platform_event_view->mouse_event_count > event_system_->mouse_event_storage.max_event_count) {
        ret = EVENT_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("event_system_update(%s) - mouse event count limit exceeded.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    if(platform_event_view->window_event_count > event_system_->window_event_storage.max_event_count) {
        ret = EVENT_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("event_system_update(%s) - window event count limit exceeded.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    event_storage_counts_reset(&event_system_->window_event_storage, &event_system_->keyboard_event_storage, &event_system_->mouse_event_storage);

    for(size_t i = 0; i != platform_event_view->keyboard_event_count; ++i) {
        ret = keyboard_event_storage_push(&event_system_->keyboard_event_storage, &platform_event_view->keyboard_events[i]);
        if(EVENT_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("event_system_update(%s) - keyboard_event_storage_push failed.", event_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    for(size_t i = 0; i != platform_event_view->mouse_event_count; ++i) {
        ret = mouse_event_storage_push(&event_system_->mouse_event_storage, &platform_event_view->mouse_events[i]);
        if(EVENT_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("event_system_update(%s) - mouse_event_storage_push failed.", event_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    for(size_t i = 0; i != platform_event_view->window_event_count; ++i) {
        ret = window_event_storage_push(&event_system_->window_event_storage, &platform_event_view->window_events[i]);
        if(EVENT_SYSTEM_SUCCESS != ret) {
            ERROR_MESSAGE("event_system_update(%s) - window_event_storage_push failed.", event_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    ret = event_view_refresh(platform_event_view->window_close_requested, &event_system_->window_event_storage, &event_system_->keyboard_event_storage, &event_system_->mouse_event_storage, &event_system_->event_view);
    if(EVENT_SYSTEM_SUCCESS != ret) {
        ERROR_MESSAGE("event_system_update(%s) - event_view_refresh failed.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    *out_event_view_ = &event_system_->event_view;

    ret = EVENT_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

bool event_system_is_valid(const event_system_t* event_system_) {
    if(NULL == event_system_) {
        return false;
    }
    if(!is_valid_shallow(event_system_)) {
        return false;
    }
    if(!window_event_storage_is_valid(&event_system_->window_event_storage)) {
        return false;
    }
    if(!keyboard_event_storage_is_valid(&event_system_->keyboard_event_storage)) {
        return false;
    }
    if(!mouse_event_storage_is_valid(&event_system_->mouse_event_storage)) {
        return false;
    }
    if(!engine_event_view_is_valid(&event_system_->event_view)) {
        return false;
    }
    return true;
}

// ============================================================
// Initialization / lifecycle helpers
// ============================================================
static event_system_result_t event_storage_initialize(const event_system_config_t* config_, linear_alloc_t* linear_alloc_, window_event_storage_t* window_event_storage_, keyboard_event_storage_t* keyboard_event_storage_, mouse_event_storage_t* mouse_event_storage_) {
    event_system_result_t ret = EVENT_SYSTEM_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    size_t allocation_size = 0;

    window_event_t* tmp_window_event_storage = NULL;
    keyboard_event_t* tmp_keyboard_event_storage = NULL;
    mouse_event_t* tmp_mouse_event_storage = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(linear_alloc_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "linear_alloc_")
    IF_ARG_NULL_GOTO_CLEANUP(window_event_storage_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "window_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_storage_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "keyboard_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(mouse_event_storage_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_storage_initialize", "mouse_event_storage_")

    // window event
    if((SIZE_MAX / config_->max_window_event_count) < sizeof(window_event_t)) {
        ret = EVENT_SYSTEM_OVERFLOW;
        ERROR_MESSAGE("event_storage_initialize(%s) - window event array size overflow.", event_system_rslt_to_str(ret));
        goto cleanup;
    }
    allocation_size = sizeof(window_event_t) * config_->max_window_event_count;
    ret_linear_alloc = linear_allocator_allocate(linear_alloc_, allocation_size, alignof(window_event_t), (void**)&tmp_window_event_storage);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = event_system_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("event_storage_initialize(%s) - linear_allocator_allocate failed.", event_system_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_window_event_storage, 0, allocation_size);

    // keyboard event
    if((SIZE_MAX / config_->max_keyboard_event_count) < sizeof(keyboard_event_t)) {
        ret = EVENT_SYSTEM_OVERFLOW;
        ERROR_MESSAGE("event_storage_initialize(%s) - keyboard event array size overflow.", event_system_rslt_to_str(ret));
        goto cleanup;
    }
    allocation_size = sizeof(keyboard_event_t) * config_->max_keyboard_event_count;
    ret_linear_alloc = linear_allocator_allocate(linear_alloc_, allocation_size, alignof(keyboard_event_t), (void**)&tmp_keyboard_event_storage);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = event_system_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("event_storage_initialize(%s) - linear_allocator_allocate failed.", event_system_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_keyboard_event_storage, 0, allocation_size);

    // mouse event
    if((SIZE_MAX / config_->max_mouse_event_count) < sizeof(mouse_event_t)) {
        ret = EVENT_SYSTEM_OVERFLOW;
        ERROR_MESSAGE("event_storage_initialize(%s) - mouse event array size overflow.", event_system_rslt_to_str(ret));
        goto cleanup;
    }
    allocation_size = sizeof(mouse_event_t) * config_->max_mouse_event_count;
    ret_linear_alloc = linear_allocator_allocate(linear_alloc_, allocation_size, alignof(mouse_event_t), (void**)&tmp_mouse_event_storage);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = event_system_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("event_storage_initialize(%s) - linear_allocator_allocate failed.", event_system_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_mouse_event_storage, 0, allocation_size);

    window_event_storage_->current_event_count = 0;
    window_event_storage_->max_event_count = config_->max_window_event_count;
    window_event_storage_->event_storage = tmp_window_event_storage;
    tmp_window_event_storage = NULL;

    keyboard_event_storage_->current_event_count = 0;
    keyboard_event_storage_->max_event_count = config_->max_keyboard_event_count;
    keyboard_event_storage_->event_storage = tmp_keyboard_event_storage;
    tmp_keyboard_event_storage = NULL;

    mouse_event_storage_->current_event_count = 0;
    mouse_event_storage_->max_event_count = config_->max_mouse_event_count;
    mouse_event_storage_->event_storage = tmp_mouse_event_storage;
    tmp_mouse_event_storage = NULL;

    ret = EVENT_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Per-frame update helpers
// ============================================================
static void event_storage_counts_reset(window_event_storage_t* window_event_storage_, keyboard_event_storage_t* keyboard_event_storage_, mouse_event_storage_t* mouse_event_storage_) {
    if(NULL == window_event_storage_ || NULL == keyboard_event_storage_ || NULL == mouse_event_storage_) {
        return;
    }

    window_event_storage_->current_event_count = 0;
    keyboard_event_storage_->current_event_count = 0;
    mouse_event_storage_->current_event_count = 0;
}

static event_system_result_t event_view_refresh(bool window_close_requested_, const window_event_storage_t* window_event_storage_, const keyboard_event_storage_t* keyboard_event_storage_, const mouse_event_storage_t* mouse_event_storage_, engine_event_view_t* event_view_) {
    event_system_result_t ret = EVENT_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(window_event_storage_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_view_refresh", "window_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(keyboard_event_storage_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_view_refresh", "keyboard_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(mouse_event_storage_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_view_refresh", "mouse_event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(event_view_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "event_view_refresh", "event_view_")

    event_view_->keyboard_event_count = keyboard_event_storage_->current_event_count;
    event_view_->mouse_event_count = mouse_event_storage_->current_event_count;
    event_view_->window_event_count = window_event_storage_->current_event_count;

    event_view_->keyboard_events = keyboard_event_storage_->event_storage;
    event_view_->mouse_events = mouse_event_storage_->event_storage;
    event_view_->window_events = window_event_storage_->event_storage;

    event_view_->window_close_requested = window_close_requested_;

    ret = EVENT_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Event storage primitive operations
// ============================================================
static event_system_result_t window_event_storage_push(window_event_storage_t* event_storage_, const window_event_t* event_) {
    event_system_result_t ret = EVENT_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(event_storage_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "window_event_storage_push", "event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(event_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "window_event_storage_push", "event_")
    if(event_storage_->current_event_count >= event_storage_->max_event_count) {
        ret = EVENT_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("window_event_storage_push(%s) - window event storage limit exceeded.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    event_storage_->event_storage[event_storage_->current_event_count] = *event_;
    event_storage_->current_event_count++;

    ret = EVENT_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static event_system_result_t keyboard_event_storage_push(keyboard_event_storage_t* event_storage_, const keyboard_event_t* event_) {
    event_system_result_t ret = EVENT_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(event_storage_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "keyboard_event_storage_push", "event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(event_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "keyboard_event_storage_push", "event_")
    if(event_storage_->current_event_count >= event_storage_->max_event_count) {
        ret = EVENT_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("keyboard_event_storage_push(%s) - keyboard event storage limit exceeded.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    event_storage_->event_storage[event_storage_->current_event_count] = *event_;
    event_storage_->current_event_count++;

    ret = EVENT_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

static event_system_result_t mouse_event_storage_push(mouse_event_storage_t* event_storage_, const mouse_event_t* event_) {
    event_system_result_t ret = EVENT_SYSTEM_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(event_storage_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "mouse_event_storage_push", "event_storage_")
    IF_ARG_NULL_GOTO_CLEANUP(event_, ret, EVENT_SYSTEM_INVALID_ARGUMENT, event_system_rslt_to_str(EVENT_SYSTEM_INVALID_ARGUMENT), "mouse_event_storage_push", "event_")
    if(event_storage_->current_event_count >= event_storage_->max_event_count) {
        ret = EVENT_SYSTEM_LIMIT_EXCEEDED;
        ERROR_MESSAGE("mouse_event_storage_push(%s) - mouse event storage limit exceeded.", event_system_rslt_to_str(ret));
        goto cleanup;
    }

    event_storage_->event_storage[event_storage_->current_event_count] = *event_;
    event_storage_->current_event_count++;

    ret = EVENT_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Validators
// ============================================================
static bool window_event_storage_is_valid(const window_event_storage_t* event_storage_) {
    if(NULL == event_storage_) {
        return false;
    }
    if(0 == event_storage_->max_event_count) {
        return false;
    }
    if(event_storage_->current_event_count > event_storage_->max_event_count) {
        return false;
    }
    if(NULL == event_storage_->event_storage) {
        return false;
    }
    return true;
}

static bool keyboard_event_storage_is_valid(const keyboard_event_storage_t* event_storage_) {
    if(NULL == event_storage_) {
        return false;
    }
    if(0 == event_storage_->max_event_count) {
        return false;
    }
    if(event_storage_->current_event_count > event_storage_->max_event_count) {
        return false;
    }
    if(NULL == event_storage_->event_storage) {
        return false;
    }
    return true;
}

static bool mouse_event_storage_is_valid(const mouse_event_storage_t* event_storage_) {
    if(NULL == event_storage_) {
        return false;
    }
    if(0 == event_storage_->max_event_count) {
        return false;
    }
    if(event_storage_->current_event_count > event_storage_->max_event_count) {
        return false;
    }
    if(NULL == event_storage_->event_storage) {
        return false;
    }
    return true;
}

static bool is_valid_shallow(const event_system_t* event_system_) {
    if(NULL == event_system_) {
        return false;
    }
    if(NULL == event_system_->platform_system) {
        return false;
    }
    if(NULL == event_system_->keyboard_event_storage.event_storage) {
        return false;
    }
    if(NULL == event_system_->mouse_event_storage.event_storage) {
        return false;
    }
    if(NULL == event_system_->window_event_storage.event_storage) {
        return false;
    }
    if(0 == event_system_->keyboard_event_storage.max_event_count) {
        return false;
    }
    if(0 == event_system_->mouse_event_storage.max_event_count) {
        return false;
    }
    if(0 == event_system_->window_event_storage.max_event_count) {
        return false;
    }
    if(event_system_->keyboard_event_storage.event_storage != event_system_->event_view.keyboard_events) {
        return false;
    }
    if(event_system_->mouse_event_storage.event_storage != event_system_->event_view.mouse_events) {
        return false;
    }
    if(event_system_->window_event_storage.event_storage != event_system_->event_view.window_events) {
        return false;
    }
    if(event_system_->keyboard_event_storage.current_event_count != event_system_->event_view.keyboard_event_count) {
        return false;
    }
    if(event_system_->mouse_event_storage.current_event_count != event_system_->event_view.mouse_event_count) {
        return false;
    }
    if(event_system_->window_event_storage.current_event_count != event_system_->event_view.window_event_count) {
        return false;
    }
    return true;
}
