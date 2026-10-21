// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_UNTEXTURED_MESH_RENDER_RESOURCE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDER_RESOURCES_UNTEXTURED_MESH_RENDER_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"

// typedef struct untextured_mesh_render_resource untextured_mesh_render_resource_t;

// typedef struct vbo_manager_config vbo_manager_config_t;
// typedef struct subsystem_allocator subsystem_allocator_t;
// typedef struct renderer_backend_context renderer_backend_context_t;
// typedef struct mat4x4f mat4x4f_t;
// typedef struct aabb_3d aabb_3d_t;

// render_resource_result_t untextured_mesh_render_resource_create(renderer_backend_context_t* backend_context_, const vbo_manager_config_t* vbo_config_, size_t max_geometry_count_, size_t max_material_count_, subsystem_allocator_t* allocator_, const char* executable_directory_, const char* shader_dir_, untextured_mesh_render_resource_t** out_render_resource_);

// void untextured_mesh_render_resource_deinitialize(untextured_mesh_render_resource_t* render_resource_);

// Geometry resource operation
// render_resource_result_t untextured_mesh_render_resource_geometry_import_from_file(untextured_mesh_render_resource_t* render_resource_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

// render_resource_result_t untextured_mesh_render_resource_geometry_release(untextured_mesh_render_resource_t* render_resource_, uint16_t geometry_id_);

// Material resource operation
// render_resource_result_t untextured_mesh_render_resource_material_import_from_file(untextured_mesh_render_resource_t* render_resource_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_);

// render_resource_result_t untextured_mesh_render_resource_material_release(untextured_mesh_render_resource_t* render_resource_, uint16_t material_id_);

// render_resource_result_t untextured_mesh_render_resource_vbo_write(untextured_mesh_render_resource_t* render_resource_, size_t vertex_count_, const point_normal_vertex_t* vertices_, vbo_range_t* out_buffer_range_);

// shader_result_t untextured_mesh_render_resource_vbo_free(untextured_mesh_render_resource_t* render_resource_, const vbo_range_t* buffer_range_);

// Utility
// render_resource_result_t untextured_mesh_render_resource_geometry_convert_to_aabb_3d(untextured_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, aabb_3d_t* out_aabb_3d_);

// Canonical validator
// bool untextured_mesh_render_resource_is_valid(const untextured_mesh_render_resource_t* render_resource_);

#ifdef __cplusplus
}
#endif
#endif
