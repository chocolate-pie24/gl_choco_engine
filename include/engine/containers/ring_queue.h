#ifndef GLCE_ENGINE_CONTAINERS_RING_QUEUE_H
#define GLCE_ENGINE_CONTAINERS_RING_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct ring_queue ring_queue_t;

typedef enum {
    RING_QUEUE_SUCCESS,
    RING_QUEUE_INVALID_ARGUMENT,
    RING_QUEUE_NO_MEMORY,
    RING_QUEUE_RUNTIME_ERROR,
    RING_QUEUE_UNDEFINED_ERROR,
    RING_QUEUE_EMPTY,
} ring_queue_error_t;

ring_queue_error_t ring_queue_create(size_t max_element_count_, size_t element_size_, size_t element_align_, ring_queue_t** ring_queue_);

void ring_queue_destroy(ring_queue_t** ring_queue_);

ring_queue_error_t ring_queue_push(ring_queue_t* ring_queue_, const void* data_, size_t element_size_, size_t element_align_);

ring_queue_error_t ring_queue_pop(ring_queue_t* ring_queue_, void* data_, size_t element_size_, size_t element_align_);

bool ring_queue_empty(const ring_queue_t* ring_queue_);

#ifdef __cplusplus
}
#endif
#endif
