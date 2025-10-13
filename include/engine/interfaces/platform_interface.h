#ifndef GLCE_ENGINE_INTERFACES_PLATFORM_INTERFACE_H
#define GLCE_ENGINE_INTERFACES_PLATFORM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/core/platform/platform_utils.h"

#include "engine/core/event/window_event.h"
#include "engine/core/event/keyboard_event.h"
#include "engine/core/event/mouse_event.h"

typedef struct platform_state platform_state_t;

typedef struct platform_vtable {
    void (*platform_state_preinit)(size_t* memory_requirement_, size_t* alignment_requirement_);
    platform_error_t (*platform_state_init)(platform_state_t* platform_state_);
    void (*platform_state_destroy)(platform_state_t* platform_state_);
    platform_error_t (*platform_window_create)(platform_state_t* platform_state_, const char* window_label_, int window_width_, int window_height_);
    platform_error_t (*platform_pump_messages)(
        platform_state_t* platform_state_,
        void (*window_event_callback)(const window_event_t* event_),
        void (*keyboard_event_callback)(const keyboard_event_t* event_),
        void (*mouse_event_callback)(const mouse_event_t* event_));
} platform_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
