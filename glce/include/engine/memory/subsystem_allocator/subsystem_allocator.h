// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#ifndef GLCE_ENGINE_MEMORY_SUBSYSTEM_ALLOCATOR_SUBSYSTEM_ALLOCATOR_H
#define GLCE_ENGINE_MEMORY_SUBSYSTEM_ALLOCATOR_SUBSYSTEM_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct subsystem_allocator subsystem_allocator_t;

typedef enum {
    SUBSYSTEM_ALLOCATOR_SUCCESS = 0,
    SUBSYSTEM_ALLOCATOR_BAD_OPERATION,
    SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED,
    SUBSYSTEM_ALLOCATOR_NO_MEMORY,
    SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT,
    SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR,
} subsystem_allocator_result_t;

typedef enum {
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_PLATFORM = 0,
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER,
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_EVENT,
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_CAMERA,
    SUBSYSTEM_ALLOCATOR_MEMORY_TAG_MAX,
} subsystem_allocator_memory_tag_t;

subsystem_allocator_result_t subsystem_allocator_create(size_t memory_pool_size_, subsystem_allocator_t** out_allocator_);

void subsystem_allocator_destroy(subsystem_allocator_t** allocator_);

subsystem_allocator_result_t subsystem_allocator_allocate(subsystem_allocator_t* allocator_, size_t allocation_size_, subsystem_allocator_memory_tag_t memory_tag_, void** out_ptr_);

subsystem_allocator_result_t subsystem_allocator_reset(subsystem_allocator_t* allocator_);

bool subsystem_allocator_is_valid(const subsystem_allocator_t* allocator_);

#ifdef __cplusplus
}
#endif
#endif
