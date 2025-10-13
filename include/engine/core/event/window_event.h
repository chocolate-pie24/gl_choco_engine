#ifndef GLCE_ENGINE_CORE_EVENT_WINDOW_EVENT_H
#define GLCE_ENGINE_CORE_EVENT_WINDOW_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WINDOW_EVENT_RESIZE,
} window_event_code_t;

typedef struct window_event {
    window_event_code_t event_code;
    int window_width;
    int window_height;
} window_event_t;

#ifdef __cplusplus
}
#endif
#endif
