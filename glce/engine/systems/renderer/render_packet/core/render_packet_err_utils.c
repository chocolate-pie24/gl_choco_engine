// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/render_packet/core/render_packet_err_utils.h"
#include "engine/systems/renderer/render_packet/core/render_packet_types.h"

#include "engine/memory/frame_allocator/frame_allocator.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

static const char* const s_result_str_success = "SUCCESS";
static const char* const s_result_str_invalid_argument = "INVALID_ARGUMENT";
static const char* const s_result_str_runtime_error = "RUNTIME_ERROR";
static const char* const s_result_str_overflow = "OVERFLOW";
static const char* const s_result_str_bad_operation = "BAD_OPERATION";
static const char* const s_result_str_data_corrupted = "DATA_CORRUPTED";
static const char* const s_result_str_no_memory = "NO_MEMORY";
static const char* const s_result_str_limit_exceeded = "LIMIT_EXCEEDED";
static const char* const s_result_str_undefined_error = "UNDEFINED_ERROR";

const char* render_packet_result_to_str(render_packet_result_t result_) {
    switch(result_) {
    case RENDER_PACKET_SUCCESS:
        return s_result_str_success;
    case RENDER_PACKET_INVALID_ARGUMENT:
        return s_result_str_invalid_argument;
    case RENDER_PACKET_RUNTIME_ERROR:
        return s_result_str_runtime_error;
    case RENDER_PACKET_OVERFLOW:
        return s_result_str_overflow;
    case RENDER_PACKET_BAD_OPERATION:
        return s_result_str_bad_operation;
    case RENDER_PACKET_DATA_CORRUPTED:
        return s_result_str_data_corrupted;
    case RENDER_PACKET_NO_MEMORY:
        return s_result_str_no_memory;
    case RENDER_PACKET_LIMIT_EXCEEDED:
        return s_result_str_limit_exceeded;
    case RENDER_PACKET_UNDEFINED_ERROR:
        return s_result_str_undefined_error;
    default:
        return s_result_str_undefined_error;
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
