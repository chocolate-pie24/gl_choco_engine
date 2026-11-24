// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/render_packet/core/render_packet_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/frame_allocator/frame_allocator.h"

#include "engine/systems/renderer/render_packet/core/render_packet_types.h"

const char* render_packet_result_to_str(render_packet_result_t result_) {
    switch(result_) {
    case RENDER_PACKET_SUCCESS:
        return STRINGIFY(RENDER_PACKET_SUCCESS);
    case RENDER_PACKET_INVALID_ARGUMENT:
        return STRINGIFY(RENDER_PACKET_INVALID_ARGUMENT);
    case RENDER_PACKET_RUNTIME_ERROR:
        return STRINGIFY(RENDER_PACKET_RUNTIME_ERROR);
    case RENDER_PACKET_OVERFLOW:
        return STRINGIFY(RENDER_PACKET_OVERFLOW);
    case RENDER_PACKET_BAD_OPERATION:
        return STRINGIFY(RENDER_PACKET_BAD_OPERATION);
    case RENDER_PACKET_DATA_CORRUPTED:
        return STRINGIFY(RENDER_PACKET_DATA_CORRUPTED);
    case RENDER_PACKET_NO_MEMORY:
        return STRINGIFY(RENDER_PACKET_NO_MEMORY);
    case RENDER_PACKET_LIMIT_EXCEEDED:
        return STRINGIFY(RENDER_PACKET_LIMIT_EXCEEDED);
    case RENDER_PACKET_UNDEFINED_ERROR:
        return STRINGIFY(RENDER_PACKET_UNDEFINED_ERROR);
    default:
        return STRINGIFY(RENDER_PACKET_UNDEFINED_ERROR);
    }
}

render_packet_result_t render_packet_result_convert_frame_allocator(frame_allocator_result_t result_) {
    switch(result_) {
    case FRAME_ALLOCATOR_SUCCESS:
        return RENDER_PACKET_SUCCESS;
    case FRAME_ALLOCATOR_BAD_OPERATION:
        return RENDER_PACKET_BAD_OPERATION;
    case FRAME_ALLOCATOR_DATA_CORRUPTED:
        return RENDER_PACKET_DATA_CORRUPTED;
    case FRAME_ALLOCATOR_NO_MEMORY:
        return RENDER_PACKET_NO_MEMORY;
    case FRAME_ALLOCATOR_INVALID_ARGUMENT:
        return RENDER_PACKET_INVALID_ARGUMENT;
    case FRAME_ALLOCATOR_OVERFLOW:
        return RENDER_PACKET_OVERFLOW;
    case FRAME_ALLOCATOR_LIMIT_EXCEEDED:
        return RENDER_PACKET_LIMIT_EXCEEDED;
    case FRAME_ALLOCATOR_UNDEFINED_ERROR:
        return RENDER_PACKET_UNDEFINED_ERROR;
    default:
        return RENDER_PACKET_UNDEFINED_ERROR;
    }
}

render_packet_result_t render_packet_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return RENDER_PACKET_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return RENDER_PACKET_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return RENDER_PACKET_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return RENDER_PACKET_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return RENDER_PACKET_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return RENDER_PACKET_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return RENDER_PACKET_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return RENDER_PACKET_UNDEFINED_ERROR;
    default:
        return RENDER_PACKET_UNDEFINED_ERROR;
    }
}
