// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/event/application_event.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"
#include "engine/core/event/window_event.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/systems/platform/platform_context.h"

#include "application/core/application_types.h"
#include "application/core/application_err_utils.h"

typedef struct application_event {
    platform_context_t* platform_context;

    application_event_view_t event_view;

    ring_queue_t* window_event_queue;   /**< ウィンドウイベント格納用リングキュー */
    window_event_t* window_events;
    size_t max_window_event_count;

    ring_queue_t* keyboard_event_queue; /**< キーボードイベント格納用リングキュー */
    keyboard_event_t* keyboard_events;
    size_t max_keyboard_event_count;

    ring_queue_t* mouse_event_queue;    /**< マウスイベント格納用リングキュー */
    mouse_event_t* mouse_events;
    size_t max_mouse_event_count;
} application_event_t;

static application_event_t* s_application_event = NULL;
static application_result_t s_callback_result = APPLICATION_SUCCESS;

static application_result_t window_events_create(size_t max_window_event_count_, linear_alloc_t* allocator_, window_event_t** out_window_events_);
static application_result_t keyboard_events_create(size_t max_keyboard_event_count_, linear_alloc_t* allocator_, keyboard_event_t** out_keyboard_events_);
static application_result_t mouse_events_create(size_t max_mouse_event_count_, linear_alloc_t* allocator_, mouse_event_t** out_mouse_events_);

static void on_window(const window_event_t* event_);
static void on_key(const keyboard_event_t* event_);
static void on_mouse(const mouse_event_t* event_);

static bool is_valid_deep(const application_event_t* application_event_);
static bool is_valid_shallow(const application_event_t* application_event_);

application_result_t application_event_initialize(platform_context_t* platform_context_, size_t max_window_event_count_, size_t max_keyboard_event_count_, size_t max_mouse_event_count_, linear_alloc_t* allocator_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    application_event_t* tmp_application_event = NULL;

    window_event_t* tmp_window_events = NULL;
    keyboard_event_t* tmp_keyboard_events = NULL;
    mouse_event_t* tmp_mouse_events = NULL;

    ring_queue_t* tmp_window_event_queue = NULL;
    ring_queue_t* tmp_keyboard_event_queue = NULL;
    ring_queue_t* tmp_mouse_event_queue = NULL;

    IF_ARG_NOT_NULL_GOTO_CLEANUP(s_application_event, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "application_event_initialize", "s_application_event")
    IF_ARG_NULL_GOTO_CLEANUP(platform_context_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_event_initialize", "platform_context_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_event_initialize", "allocator_")
    if(0 == max_window_event_count_ || 0 == max_keyboard_event_count_ || 0 == max_mouse_event_count_) {
        ret = APPLICATION_INVALID_ARGUMENT;
        ERROR_MESSAGE("application_event_initialize(%s) - Provided event count is not valid.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(application_event_t), alignof(application_event_t), (void**)&tmp_application_event);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = app_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("application_event_initialize(%s) - Failed to allocate application_event_t instance.", app_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_application_event, 0, sizeof(application_event_t));

    ret = window_events_create(max_window_event_count_, allocator_, &tmp_window_events);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_event_initialize(%s) - window_events_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = keyboard_events_create(max_keyboard_event_count_, allocator_, &tmp_keyboard_events);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_event_initialize(%s) - keyboard_events_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret = mouse_events_create(max_mouse_event_count_, allocator_, &tmp_mouse_events);
    if(APPLICATION_SUCCESS != ret) {
        ERROR_MESSAGE("application_event_initialize(%s) - mouse_events_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_ring_queue = ring_queue_create(max_window_event_count_, sizeof(window_event_t), alignof(window_event_t), &tmp_window_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = app_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("application_event_initialize(%s) - ring_queue_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_ring_queue = ring_queue_create(max_keyboard_event_count_, sizeof(keyboard_event_t), alignof(keyboard_event_t), &tmp_keyboard_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = app_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("application_event_initialize(%s) - ring_queue_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    ret_ring_queue = ring_queue_create(max_mouse_event_count_, sizeof(mouse_event_t), alignof(mouse_event_t), &tmp_mouse_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = app_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("application_event_initialize(%s) - ring_queue_create failed.", app_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_application_event->platform_context = platform_context_;

    tmp_application_event->max_keyboard_event_count = max_keyboard_event_count_;
    tmp_application_event->keyboard_events = tmp_keyboard_events;
    tmp_application_event->keyboard_event_queue = tmp_keyboard_event_queue;

    tmp_application_event->max_window_event_count = max_window_event_count_;
    tmp_application_event->window_events = tmp_window_events;
    tmp_application_event->window_event_queue = tmp_window_event_queue;

    tmp_application_event->max_mouse_event_count = max_mouse_event_count_;
    tmp_application_event->mouse_events = tmp_mouse_events;
    tmp_application_event->mouse_event_queue = tmp_mouse_event_queue;

    tmp_application_event->event_view.window_close_requested = false;
    tmp_application_event->event_view.keyboard_events = tmp_application_event->keyboard_events;
    tmp_application_event->event_view.mouse_events = tmp_application_event->mouse_events;
    tmp_application_event->event_view.window_events = tmp_application_event->window_events;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_deep(tmp_application_event)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_event_initialize(%s) - application_event_t is corrupted.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    s_application_event = tmp_application_event;
    tmp_application_event = NULL;
    tmp_keyboard_event_queue = NULL;
    tmp_keyboard_events = NULL;
    tmp_mouse_event_queue = NULL;
    tmp_mouse_events = NULL;
    tmp_window_event_queue = NULL;
    tmp_window_events = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_DATA_CORRUPTED != ret) {
        if(NULL != tmp_keyboard_event_queue) {
            ring_queue_destroy(&tmp_keyboard_event_queue);
        }
        if(NULL != tmp_mouse_event_queue) {
            ring_queue_destroy(&tmp_mouse_event_queue);
        }
        if(NULL != tmp_window_event_queue) {
            ring_queue_destroy(&tmp_window_event_queue);
        }
    }
    return ret;
}

void application_event_deinitialize(void) {
    if(NULL == s_application_event) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_event_is_valid()) {
        ERROR_MESSAGE("application_event_deinitialize(%s) - application_event_t is corrupted.", app_rslt_to_str(APPLICATION_DATA_CORRUPTED));
        return;
    }
#endif

    ring_queue_destroy(&s_application_event->keyboard_event_queue);
    ring_queue_destroy(&s_application_event->mouse_event_queue);
    ring_queue_destroy(&s_application_event->window_event_queue);

    s_application_event->max_keyboard_event_count = 0;
    s_application_event->max_mouse_event_count = 0;
    s_application_event->max_window_event_count = 0;

    s_application_event->event_view.window_close_requested = false;

    s_application_event->event_view.keyboard_event_count = 0;
    s_application_event->event_view.mouse_event_count = 0;
    s_application_event->event_view.window_event_count = 0;

    s_application_event->event_view.keyboard_events = NULL;
    s_application_event->event_view.mouse_events = NULL;
    s_application_event->event_view.window_events = NULL;

    s_application_event = NULL;
}

application_result_t application_event_update(const application_event_view_t** out_event_view_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    platform_result_t ret_platform = PLATFORM_INVALID_ARGUMENT;
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    window_event_t window_event;
    keyboard_event_t keyboard_event;
    mouse_event_t mouse_event;

    size_t event_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(out_event_view_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "application_event_update", "out_event_view_")
    if(NULL == s_application_event) {
        ret = APPLICATION_BAD_OPERATION;
        ERROR_MESSAGE("application_event_update(%s) - Application Event is not initialized.", app_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(s_application_event)) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_event_update(%s) - application_event_t is corrupted.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    s_application_event->event_view.window_close_requested = false;

    s_callback_result = APPLICATION_SUCCESS;
    ret_platform = platform_pump_messages(s_application_event->platform_context, on_window, on_key, on_mouse);
    if(APPLICATION_SUCCESS != s_callback_result) {
        ret = s_callback_result;
        ERROR_MESSAGE("application_event_update(%s) - Ring Queue push failed.", app_rslt_to_str(ret));
        goto cleanup;
    }
    if(PLATFORM_WINDOW_CLOSE == ret_platform) {
        s_application_event->event_view.window_close_requested = true;
    } else if(PLATFORM_SUCCESS != ret_platform) {
        ret = app_rslt_convert_platform(ret_platform);
        ERROR_MESSAGE("application_event_update(%s) - Failed to pump events.", app_rslt_to_str(ret));
        goto cleanup;
    }

    // window events.
    event_count = 0;
    while(!ring_queue_empty(s_application_event->window_event_queue)) {
        ret_ring_queue = ring_queue_pop(sizeof(window_event_t), alignof(window_event_t), s_application_event->window_event_queue, &window_event);
        if(RING_QUEUE_SUCCESS != ret_ring_queue) {
            ret = app_rslt_convert_ring_queue(ret_ring_queue);
            ERROR_MESSAGE("application_event_update(%s) - Failed to pop window event.", app_rslt_to_str(ret));
            goto cleanup;
        }
        s_application_event->window_events[event_count] = window_event;
        event_count++;
    }
    s_application_event->event_view.window_event_count = event_count;

    // keyboard events.
    event_count = 0;
    while(!ring_queue_empty(s_application_event->keyboard_event_queue)) {
        ret_ring_queue = ring_queue_pop(sizeof(keyboard_event_t), alignof(keyboard_event_t), s_application_event->keyboard_event_queue, &keyboard_event);
        if(RING_QUEUE_SUCCESS != ret_ring_queue) {
            ret = app_rslt_convert_ring_queue(ret_ring_queue);
            ERROR_MESSAGE("application_event_update(%s) - Failed to pop keyboard event.", app_rslt_to_str(ret));
            goto cleanup;
        }
        s_application_event->keyboard_events[event_count] = keyboard_event;
        event_count++;
    }
    s_application_event->event_view.keyboard_event_count = event_count;

    // mouse events.
    event_count = 0;
    while(!ring_queue_empty(s_application_event->mouse_event_queue)) {
        ret_ring_queue = ring_queue_pop(sizeof(mouse_event_t), alignof(mouse_event_t), s_application_event->mouse_event_queue, &mouse_event);
        if(RING_QUEUE_SUCCESS != ret_ring_queue) {
            ret = app_rslt_convert_ring_queue(ret_ring_queue);
            ERROR_MESSAGE("application_event_update(%s) - Failed to pop mouse event.", app_rslt_to_str(ret));
            goto cleanup;
        }
        s_application_event->mouse_events[event_count] = mouse_event;
        event_count++;
    }
    s_application_event->event_view.mouse_event_count = event_count;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!application_event_is_valid()) {
        ret = APPLICATION_DATA_CORRUPTED;
        ERROR_MESSAGE("application_event_update(%s) - application_event_t is corrupted.", app_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_event_view_ = &s_application_event->event_view;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

bool application_event_is_valid(void) {
    if(NULL == s_application_event) {
        return false;
    }

    return is_valid_deep(s_application_event);
}

static application_result_t window_events_create(size_t max_window_event_count_, linear_alloc_t* allocator_, window_event_t** out_window_events_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    window_event_t* tmp_window_events = NULL;

    size_t allocation_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "window_events_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_window_events_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "window_events_create", "out_window_events_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_window_events_, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "window_events_create", "*out_window_events_")

    if((SIZE_MAX / max_window_event_count_) < sizeof(window_event_t)) {
        ret = APPLICATION_OVERFLOW;
        ERROR_MESSAGE("window_events_create(%s) - allocation size overflow.", app_rslt_to_str(ret));
        goto cleanup;
    }
    allocation_size = sizeof(window_event_t) * max_window_event_count_;
    ret_linear_alloc = linear_allocator_allocate(allocator_, allocation_size, alignof(window_event_t), (void**)&tmp_window_events);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = app_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("window_events_create(%s) - Failed to allocate window_event_t instance.", app_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_window_events, 0, allocation_size);

    *out_window_events_ = tmp_window_events;
    tmp_window_events = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

static application_result_t keyboard_events_create(size_t max_keyboard_event_count_, linear_alloc_t* allocator_, keyboard_event_t** out_keyboard_events_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    keyboard_event_t* tmp_keyboard_events = NULL;

    size_t allocation_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "keyboard_events_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_keyboard_events_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "keyboard_events_create", "out_keyboard_events_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_keyboard_events_, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "keyboard_events_create", "*out_keyboard_events_")

    if((SIZE_MAX / max_keyboard_event_count_) < sizeof(keyboard_event_t)) {
        ret = APPLICATION_OVERFLOW;
        ERROR_MESSAGE("keyboard_events_create(%s) - allocation size overflow.", app_rslt_to_str(ret));
        goto cleanup;
    }
    allocation_size = sizeof(keyboard_event_t) * max_keyboard_event_count_;
    ret_linear_alloc = linear_allocator_allocate(allocator_, allocation_size, alignof(keyboard_event_t), (void**)&tmp_keyboard_events);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = app_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("keyboard_events_create(%s) - Failed to allocate keyboard_event_t instance.", app_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_keyboard_events, 0, allocation_size);

    *out_keyboard_events_ = tmp_keyboard_events;
    tmp_keyboard_events = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

static application_result_t mouse_events_create(size_t max_mouse_event_count_, linear_alloc_t* allocator_, mouse_event_t** out_mouse_events_) {
    application_result_t ret = APPLICATION_INVALID_ARGUMENT;

    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    mouse_event_t* tmp_mouse_events = NULL;

    size_t allocation_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "mouse_events_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mouse_events_, ret, APPLICATION_INVALID_ARGUMENT, app_rslt_to_str(APPLICATION_INVALID_ARGUMENT), "mouse_events_create", "out_mouse_events_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_mouse_events_, ret, APPLICATION_BAD_OPERATION, app_rslt_to_str(APPLICATION_BAD_OPERATION), "mouse_events_create", "*out_mouse_events_")

    if((SIZE_MAX / max_mouse_event_count_) < sizeof(mouse_event_t)) {
        ret = APPLICATION_OVERFLOW;
        ERROR_MESSAGE("mouse_events_create(%s) - allocation size overflow.", app_rslt_to_str(ret));
        goto cleanup;
    }
    allocation_size = sizeof(mouse_event_t) * max_mouse_event_count_;
    ret_linear_alloc = linear_allocator_allocate(allocator_, allocation_size, alignof(mouse_event_t), (void**)&tmp_mouse_events);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = app_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("mouse_events_create(%s) - Failed to allocate mouse_event_t instance.", app_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_mouse_events, 0, allocation_size);

    *out_mouse_events_ = tmp_mouse_events;
    tmp_mouse_events = NULL;

    ret = APPLICATION_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

static void on_window(const window_event_t* event_) {
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    if(NULL == event_) {
        ERROR_MESSAGE("on_window - Argument 'event_' must not be NULL.");
        goto cleanup;
    }
    if(NULL == s_application_event) {
        ERROR_MESSAGE("on_window - Application Event is not initialized.");
        goto cleanup;
    }
    if(APPLICATION_SUCCESS != s_callback_result) {
        return;
    }

    ret_ring_queue = ring_queue_push(event_, sizeof(window_event_t), alignof(window_event_t), s_application_event->window_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        s_callback_result = app_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("on_window(%s) - Failed to push window event.", app_rslt_to_str(s_callback_result));
        goto cleanup;
    }
cleanup:
    return;
}

static void on_key(const keyboard_event_t* event_) {
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    if(NULL == event_) {
        ERROR_MESSAGE("on_key - Argument event_ requires a valid pointer.");
        goto cleanup;
    }
    if(NULL == s_application_event) {
        ERROR_MESSAGE("on_key - Application Event is not initialized.");
        goto cleanup;
    }
    if(APPLICATION_SUCCESS != s_callback_result) {
        return;
    }

    ret_ring_queue = ring_queue_push(event_, sizeof(keyboard_event_t), alignof(keyboard_event_t), s_application_event->keyboard_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        s_callback_result = app_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("on_key(%s) - Failed to push keyboard event.", app_rslt_to_str(s_callback_result));
        goto cleanup;
    }
cleanup:
    return;
}

static void on_mouse(const mouse_event_t* event_) {
    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    if(NULL == event_) {
        ERROR_MESSAGE("on_mouse - Argument event_ requires a valid pointer.");
        goto cleanup;
    }
    if(NULL == s_application_event) {
        ERROR_MESSAGE("on_mouse - Application Event is not initialized.");
        goto cleanup;
    }
    if(APPLICATION_SUCCESS != s_callback_result) {
        return;
    }

    ret_ring_queue = ring_queue_push(event_, sizeof(mouse_event_t), alignof(mouse_event_t), s_application_event->mouse_event_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        s_callback_result = app_rslt_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("on_mouse(%s) - Failed to push mouse event.", app_rslt_to_str(s_callback_result));
        goto cleanup;
    }

cleanup:
    return;
}

static bool is_valid_deep(const application_event_t* application_event_) {
    if(NULL == application_event_) {
        return false;
    }
    if(!is_valid_shallow(application_event_)) {
        return false;
    }

    if(!ring_queue_is_valid(application_event_->keyboard_event_queue)) {
        return false;
    }
    if(!ring_queue_is_valid(application_event_->mouse_event_queue)) {
        return false;
    }
    if(!ring_queue_is_valid(application_event_->window_event_queue)) {
        return false;
    }

    // TODO: イベント配列のdeep validationはevent_tのcanonical validatorがないため現状はなし

    return true;
}

static bool is_valid_shallow(const application_event_t* application_event_) {
    if(NULL == application_event_) {
        return false;
    }
    if(NULL == application_event_->platform_context) {
        return false;
    }
    if(NULL == application_event_->event_view.keyboard_events) {
        return false;
    }
    if(NULL == application_event_->event_view.mouse_events) {
        return false;
    }
    if(NULL == application_event_->event_view.window_events) {
        return false;
    }
    if(0 == application_event_->max_keyboard_event_count) {
        return false;
    }
    if(0 == application_event_->max_mouse_event_count) {
        return false;
    }
    if(0 == application_event_->max_window_event_count) {
        return false;
    }
    if(NULL == application_event_->window_event_queue) {
        return false;
    }
    if(NULL == application_event_->keyboard_event_queue) {
        return false;
    }
    if(NULL == application_event_->mouse_event_queue) {
        return false;
    }
    if(NULL == application_event_->window_events) {
        return false;
    }
    if(NULL == application_event_->keyboard_events) {
        return false;
    }
    if(NULL == application_event_->mouse_events) {
        return false;
    }
    if(application_event_->event_view.keyboard_events != application_event_->keyboard_events) {
        return false;
    }
    if(application_event_->event_view.mouse_events != application_event_->mouse_events) {
        return false;
    }
    if(application_event_->event_view.window_events != application_event_->window_events) {
        return false;
    }
    if(application_event_->event_view.keyboard_event_count > application_event_->max_keyboard_event_count) {
        return false;
    }
    if(application_event_->event_view.mouse_event_count > application_event_->max_mouse_event_count) {
        return false;
    }
    if(application_event_->event_view.window_event_count > application_event_->max_window_event_count) {
        return false;
    }
    return true;
}
