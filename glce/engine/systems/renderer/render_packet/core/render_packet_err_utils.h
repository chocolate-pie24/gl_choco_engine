// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_PACKET_CORE_RENDER_PACKET_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_PACKET_CORE_RENDER_PACKET_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/frame_allocator/frame_allocator.h"
#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/render_packet/core/render_packet_types.h"

const char* render_packet_result_to_str(render_packet_result_t result_);

render_packet_result_t render_packet_result_convert_frame_allocator(frame_allocator_result_t result_);

render_packet_result_t render_packet_result_convert_general_allocator(general_allocator_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
