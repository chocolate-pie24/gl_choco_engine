// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "engine/memory/core/memory_tag.h"

#include <stdbool.h>

static const char* const s_system = "SYSTEM";
static const char* const s_string = "STRING";
static const char* const s_ring_queue = "RING_QUEUE";
static const char* const s_renderer = "RENDERER";
static const char* const s_file_io = "FILE_IO";
static const char* const s_camera = "CAMERA";
static const char* const s_texture = "TEXTURE";
static const char* const s_geometry = "GEOMETRY";
static const char* const s_undefined = "UNDEFINED";

const char* memory_tag_c_str(memory_tag_t memory_tag_) {
    switch(memory_tag_) {
    case MEMORY_TAG_SYSTEM:
        return s_system;
    case MEMORY_TAG_STRING:
        return s_string;
    case MEMORY_TAG_RING_QUEUE:
        return s_ring_queue;
    case MEMORY_TAG_RENDERER:
        return s_renderer;
    case MEMORY_TAG_FILE_IO:
        return s_file_io;
    case MEMORY_TAG_CAMERA:
        return s_camera;
    case MEMORY_TAG_TEXTURE:
        return s_texture;
    case MEMORY_TAG_GEOMETRY:
        return s_geometry;
    case MEMORY_TAG_MAX:
        return s_undefined;
    default:
        return s_undefined;
    }
}

bool memory_tag_is_valid(memory_tag_t memory_tag_) {
    if((unsigned int)memory_tag_ >= (unsigned int)MEMORY_TAG_MAX) {
        return false;
    }
    return true;
}
