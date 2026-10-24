// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_MANAGER_RESOURCE_MANAGER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_MANAGER_RESOURCE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/systems/renderer/core/renderer_types.h"
#include "engine/systems/renderer/resource_manager/core/resource_manager_types.h"

typedef struct resource_manager resource_manager_t;

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct renderer_backend_context renderer_backend_context_t;
typedef struct untextured_mesh_material untextured_mesh_material_t;
typedef struct renderer_backend_vbo renderer_backend_vbo_t;

typedef struct {
    size_t vbo_size;
    size_t max_allocation_count;
    size_t base_align;
    buffer_usage_t buffer_usage;
} vbo_config_t;

typedef struct {
    vbo_config_t vbo_config;
    size_t max_geometry_count;
    size_t max_material_count;
} untextured_mesh_resource_config_t;

typedef struct {
    // Untextured mesh
    untextured_mesh_resource_config_t untextured_mesh_resource_config;
} resource_manager_config_t;

//////////////////////////////////////////////////////////////////////////////////////////
// Lifecycle API
//////////////////////////////////////////////////////////////////////////////////////////
resource_manager_result_t resource_manager_create(const resource_manager_config_t* config_, subsystem_allocator_t* allocator_, renderer_backend_context_t* backend_context_, resource_manager_t** out_resource_manager_);

void resource_manager_deinitialize(resource_manager_t* resource_manager_);

//////////////////////////////////////////////////////////////////////////////////////////
// For Untextured mesh
//////////////////////////////////////////////////////////////////////////////////////////
const draw_range_t* resource_manager_untextured_mesh_draw_range_get(const resource_manager_t* resource_manager_, uint16_t geometry_id_);

const untextured_mesh_material_t* resource_manager_untextured_mesh_material_get(const resource_manager_t* resource_manager_, uint16_t material_id_);

const renderer_backend_vbo_t* resource_manager_untextured_mesh_vbo_get(const resource_manager_t* resource_manager_);

resource_manager_result_t resource_manager_untextured_mesh_geometry_import_from_file(resource_manager_t* resource_manager_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

resource_manager_result_t resource_manager_untextured_mesh_geometry_release(resource_manager_t* resource_manager_, uint16_t geometry_id_);

resource_manager_result_t resource_manager_untextured_mesh_material_import_from_file(resource_manager_t* resource_manager_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_material_id_);

resource_manager_result_t resource_manager_untextured_mesh_material_release(resource_manager_t* resource_manager_, uint16_t material_id_);

#ifdef __cplusplus
}
#endif
#endif
