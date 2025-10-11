#include <stdalign.h>

#include "engine/core/event_system.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

struct event_system_state {
    event_system_error_t (*event_callback_functions[EVENT_CODE_MAX])(event_arg_t arg_);
};

void event_system_preinit(size_t* memory_requirement_, size_t* align_requirement_) {
    if(NULL == memory_requirement_ || NULL == align_requirement_) {
        WARN_MESSAGE("event_system_preinit - No-op: memory_requirement_ or align_requirement_ is NULL.");
        goto cleanup;
    }
    *memory_requirement_ = sizeof(event_system_state_t);
    *align_requirement_ = alignof(event_system_state_t);
    goto cleanup;
cleanup:
    return;
}

event_system_error_t event_system_init(event_system_state_t* event_system_state_) {
    event_system_error_t ret = EVENT_SYSTEM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(event_system_state_, EVENT_SYSTEM_INVALID_ARGUMENT, "event_system_init", "event_system_state_")

    for(size_t i = 0; i != EVENT_CODE_MAX; ++i) {
        event_system_state_->event_callback_functions[i] = NULL;
    }
    ret = EVENT_SYSTEM_SUCCESS;

cleanup:
    return ret;
}

void event_system_destroy(event_system_state_t* event_system_state_) {
    if(NULL == event_system_state_) {
        return;
    }
    for(size_t i = 0; i != EVENT_CODE_MAX; ++i) {
        event_system_state_->event_callback_functions[i] = NULL;
    }
}

event_system_error_t event_system_event_register(event_system_state_t* event_system_state_, event_code_t event_, event_system_error_t (*event_call_back)(event_arg_t arg_)) {
    event_system_error_t ret = EVENT_SYSTEM_INVALID_ARGUMENT;
    // TODO: CHECK_ARG_NULL
    if(NULL == event_system_state_) {
        // TODO: エラー処理
        goto cleanup;
    }
    if(event_ >= EVENT_CODE_MAX) {
        // TODO: エラー処理
        goto cleanup;
    }
    if(NULL == event_call_back) {
        // TODO: エラー処理
        goto cleanup;
    }
    if(NULL != event_system_state_->event_callback_functions[event_]) {
        // TODO: エラー処理
        goto cleanup;
    }
    event_system_state_->event_callback_functions[event_] = event_call_back;
    ret = EVENT_SYSTEM_SUCCESS;
cleanup:
    return ret;
}

void event_system_event_unregister(event_system_state_t* event_system_state_, event_code_t event_) {
    if(NULL == event_system_state_) {
        return;
    }
    if(event_ >= EVENT_CODE_MAX) {
        return;
    }
    event_system_state_->event_callback_functions[event_] = NULL;
}

event_system_error_t event_system_event_fire(event_system_state_t* event_system_state_, event_code_t event_, event_arg_t arg_) {
    event_system_error_t ret = EVENT_SYSTEM_INVALID_ARGUMENT;
    // CHECK_ARG_NULL
    if(NULL == event_system_state_) {
        goto cleanup;
    }
    if(NULL == event_system_state_->event_callback_functions[event_]) {
    }

    ret = event_system_state_->event_callback_functions[event_](arg_);

cleanup:
    return ret;
}
