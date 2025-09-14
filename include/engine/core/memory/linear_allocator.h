#ifndef LINEAR_ALLOCATOR_H
#define LINEAR_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct linear_alloc linear_alloc_t;

typedef enum {
    LINEAR_ALLOC_SUCCESS,
    LINEAR_ALLOC_NO_MEMORY,
    LINEAR_ALLOC_INVALID_ARGUMENT,
} linear_alloc_err_t;

linear_alloc_err_t linear_allocator_create(linear_alloc_t** allocator_, size_t capacity_);

void linear_allocator_destroy(linear_alloc_t** allocator_);

linear_alloc_err_t linear_allocator_allocate(linear_alloc_t* allocator_, size_t req_size_, size_t req_align_, void** out_ptr_);

#ifdef __cplusplus
}
#endif
#endif
