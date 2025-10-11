#ifndef GLCE_ENGINE_CORE_EVENT_H
#define GLCE_ENGINE_CORE_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct event_system_state event_system_state_t;

typedef enum {
    EVENT_SYSTEM_SUCCESS,
    EVENT_SYSTEM_INVALID_ARGUMENT,
    EVENT_SYSTEM_NO_MEMORY,
} event_system_error_t;

typedef enum {
    EVENT_CODE_KEY_PRESSED,
    EVENT_CODE_KEY_RELEASED,
    EVENT_CODE_WINDOW_RESIZED,
    EVENT_CODE_MOUSE_PRESSED,
    EVENT_CODE_MOUSE_RELEASED,
    EVENT_CODE_EVENT_TEST,
    EVENT_CODE_MAX,
} event_code_t;

typedef union event_arg {
    int64_t i64[2];
    uint64_t u64[2];
    double f64[2];
    int32_t i32[4];
    uint32_t u32[4];
    float f32[4];
    int16_t i16[8];
    uint16_t u16[8];
    char c[16];
} event_arg_t;

void event_system_preinit(size_t* memory_requirement_, size_t* align_requirement_);

event_system_error_t event_system_init(event_system_state_t* event_system_state_);

void event_system_destroy(event_system_state_t* event_system_state_);

event_system_error_t event_system_event_register(event_system_state_t* event_system_state_, event_code_t event_, event_system_error_t (*event_call_back)(event_arg_t arg_));

void event_system_event_unregister(event_system_state_t* event_system_state_, event_code_t event_);

event_system_error_t event_system_event_fire(event_system_state_t* event_system_state_, event_code_t event_, event_arg_t arg_);

#ifdef __cplusplus
}
#endif
#endif
