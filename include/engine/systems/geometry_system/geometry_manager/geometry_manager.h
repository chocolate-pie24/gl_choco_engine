#ifndef GLCE_ENGINE_SYSTEMS_GEOMETRY_SYSTEM_GEOMETRY_MANAGER_GEOMETRY_MANAGER_H
#define GLCE_ENGINE_SYSTEMS_GEOMETRY_SYSTEM_GEOMETRY_MANAGER_GEOMETRY_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/systems/geometry_system/geometry_system_core/geometry_system_types.h"

#include "engine/core/memory/linear_allocator.h"

typedef struct geometry_system geometry_system_t;

geometry_system_result_t geometry_system_initialize(const geometry_system_config_t* config_, linear_alloc_t* allocator_, geometry_system_t** out_geometry_system_);

void geometry_system_deinitialize(geometry_system_t* geometry_system_);

geometry_system_result_t geometry_system_draw_range_get_by_name(geometry_type_t geometry_type_, const char* name_, const geometry_system_t* geometry_system_, size_t* out_vertex_offset_, size_t* out_vertex_count_);

geometry_system_result_t geometry_system_draw_range_get_by_id(geometry_type_t geometry_type_, int16_t geometry_id_, const geometry_system_t* geometry_system_, size_t* out_vertex_offset_, size_t* out_vertex_count_);

// geometry_をgeometry_system_へdeep copy
geometry_system_result_t geometry_system_geometry_register(geometry_type_t geometry_type_, const void* geometry_, geometry_system_t* geometry_system_, int16_t* out_geometry_id_);

geometry_system_result_t geometry_system_geometry_unregister_by_name(geometry_type_t geometry_type_, const char* name_, geometry_system_t* geometry_system_);

geometry_system_result_t geometry_system_geometry_unregister_by_id(geometry_type_t geometry_type_, int16_t geometry_id_, geometry_system_t* geometry_system_);

geometry_system_result_t geometry_system_geometry_id_get(geometry_type_t geometry_type_, const char* name_, const geometry_system_t* geometry_system_, int16_t* out_geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
