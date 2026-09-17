// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/core/memory/memory_tag.h"

#include <stdbool.h>

bool memory_tag_is_valid(memory_tag_t memory_tag_) {
    if((unsigned int)memory_tag_ >= (unsigned int)MEMORY_TAG_MAX) {
        return false;
    }
    return true;
}
