#ifndef ENGINE_CORE_MOUSE_EVENT_H
#define ENGINE_CORE_MOUSE_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_RIGHT,
} mouse_button_t;

typedef struct mouse_event {
    int x;
    int y;
    mouse_button_t button;
    bool pressed;
} mouse_event_t;

#ifdef __cplusplus
}
#endif
#endif
