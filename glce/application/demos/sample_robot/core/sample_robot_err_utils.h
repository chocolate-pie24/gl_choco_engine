// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_APPLICATION_DEMOS_SAMPLE_ROBOT_CORE_SAMPLE_ROBOT_ERR_UTILS_H
#define GLCE_APPLICATION_DEMOS_SAMPLE_ROBOT_CORE_SAMPLE_ROBOT_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/camera/core/camera_types.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/meshes/core/mesh_types.h"

#include "engine/systems/renderer/render_packet/core/render_packet_types.h"
#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"

#include "application/model_framework/core/model_framework_types.h"

#include "application/demos/sample_robot/core/sample_robot_types.h"

const char* sample_robot_result_to_str(sample_robot_result_t result_);

sample_robot_result_t sample_robot_result_convert_model_framework(model_framework_result_t result_);

sample_robot_result_t sample_robot_result_convert_general_allocator(general_allocator_result_t result_);

sample_robot_result_t sample_robot_result_convert_mesh(mesh_result_t result_);

sample_robot_result_t sample_robot_result_convert_geometry(geometry_result_t result_);

sample_robot_result_t sample_robot_result_convert_renderer_frontend(renderer_frontend_result_t result_);

sample_robot_result_t sample_robot_result_convert_render_packet(render_packet_result_t result_);

sample_robot_result_t sample_robot_result_convert_camera(camera_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
